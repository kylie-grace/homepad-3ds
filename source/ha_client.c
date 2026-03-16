#include "app.h"
#include "jsmn.h"

#include <3ds/services/httpc.h>
#include <3ds/services/sslc.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define HA3DS_HTTP_BUFFER 262144
#define HA3DS_JSON_TOKENS 8192
#define HTTP_TIMEOUT_NS 8000000000ULL
#define HTTP_REDIRECT_LIMIT 4

static size_t json_token_span(const jsmntok_t* tokens, int index) {
    size_t span = 1;
    int children = tokens[index].size;
    int cursor = index + 1;
    while (children-- > 0) {
        size_t child_span = json_token_span(tokens, cursor);
        span += child_span;
        cursor += (int)child_span;
    }
    return span;
}

static bool json_eq(const char* json, const jsmntok_t* token, const char* text) {
    size_t len = (size_t)(token->end - token->start);
    return strlen(text) == len && strncmp(json + token->start, text, len) == 0;
}

static int json_find_key(const char* json, const jsmntok_t* tokens, int object_index, const char* key) {
    if (object_index < 0 || tokens[object_index].type != JSMN_OBJECT) {
        return -1;
    }
    int cursor = object_index + 1;
    int pair_count = tokens[object_index].size / 2;
    for (int pair = 0; pair < pair_count; pair++) {
        int key_index = cursor;
        int value_index = cursor + 1;
        if (json_eq(json, &tokens[key_index], key)) {
            return value_index;
        }
        cursor = value_index + (int)json_token_span(tokens, value_index);
    }
    return -1;
}

static void json_copy_string(const char* json, const jsmntok_t* token, char* dest, size_t dest_size) {
    if (!token || token->start < 0 || token->end < 0 || dest_size == 0) {
        if (dest_size > 0) {
            dest[0] = '\0';
        }
        return;
    }
    size_t len = (size_t)(token->end - token->start);
    if (len >= dest_size) {
        len = dest_size - 1;
    }
    memcpy(dest, json + token->start, len);
    dest[len] = '\0';
}

static float json_to_float(const char* json, const jsmntok_t* token, float fallback) {
    char buffer[32];
    json_copy_string(json, token, buffer, sizeof(buffer));
    if (buffer[0] == '\0' || strcmp(buffer, "null") == 0) {
        return fallback;
    }
    return strtof(buffer, NULL);
}

static void entity_reset(EntityState* entity) {
    memset(entity, 0, sizeof(*entity));
    entity->temperature = -9999.0f;
    entity->target_temperature = -9999.0f;
    entity->feels_like = -9999.0f;
    entity->humidity = -9999.0f;
    entity->high = -9999.0f;
    entity->low = -9999.0f;
    entity->wind_speed = -9999.0f;
    entity->precipitation_chance = -9999.0f;
    entity->min_temp = -9999.0f;
    entity->max_temp = -9999.0f;
    entity->target_temp_step = 1.0f;
}

static void parse_string_array_attr(const char* json, const jsmntok_t* tokens, int array_index, char items[][16], int max_items, int* out_count) {
    *out_count = 0;
    if (array_index < 0 || tokens[array_index].type != JSMN_ARRAY) {
        return;
    }
    int cursor = array_index + 1;
    int limit = tokens[array_index].size < max_items ? tokens[array_index].size : max_items;
    for (int i = 0; i < limit; i++) {
        json_copy_string(json, &tokens[cursor], items[i], 16);
        (*out_count)++;
        cursor += (int)json_token_span(tokens, cursor);
    }
}

const EntityState* app_find_entity(const AppState* app, const char* entity_id) {
    for (int i = 0; i < app->store.count; i++) {
        if (strcmp(app->store.entities[i].entity_id, entity_id) == 0) {
            return &app->store.entities[i];
        }
    }
    return NULL;
}

EntityState* app_find_entity_mut(AppState* app, const char* entity_id) {
    for (int i = 0; i < app->store.count; i++) {
        if (strcmp(app->store.entities[i].entity_id, entity_id) == 0) {
            return &app->store.entities[i];
        }
    }
    return NULL;
}

int app_count_domain_state(const AppState* app, const char* domain, const char* desired_state) {
    int count = 0;
    for (int i = 0; i < app->store.count; i++) {
        const EntityState* entity = &app->store.entities[i];
        if (strcmp(entity->domain, domain) == 0 && strcmp(entity->state, desired_state) == 0) {
            count++;
        }
    }
    return count;
}

int app_count_active_devices(const AppState* app) {
    int count = 0;
    for (int i = 0; i < app->store.count; i++) {
        const EntityState* entity = &app->store.entities[i];
        if ((strcmp(entity->domain, "switch") == 0 || strcmp(entity->domain, "fan") == 0 || strcmp(entity->domain, "media_player") == 0) &&
            strcmp(entity->state, "off") != 0 && strcmp(entity->state, "idle") != 0 && strcmp(entity->state, "unavailable") != 0) {
            count++;
        }
    }
    return count;
}

const char* app_greeting(void) {
    time_t raw = time(NULL);
    struct tm* current = localtime(&raw);
    if (!current) {
        return "Hello";
    }
    if (current->tm_hour < 12) {
        return "Good morning";
    }
    if (current->tm_hour < 18) {
        return "Good afternoon";
    }
    return "Good evening";
}

void app_set_status(AppState* app, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(app->status_line, sizeof(app->status_line), fmt, args);
    va_end(args);
}

static bool http_download(httpcContext* context, char* output, size_t output_size, u32* out_status) {
    Result rc = httpcBeginRequest(context);
    if (R_FAILED(rc)) {
        return false;
    }

    u32 status = 0;
    rc = httpcGetResponseStatusCodeTimeout(context, &status, HTTP_TIMEOUT_NS);
    if (R_FAILED(rc)) {
        return false;
    }
    if (out_status) {
        *out_status = status;
    }

    u32 content_size = 0;
    httpcGetDownloadSizeState(context, NULL, &content_size);

    u32 total_read = 0;
    u32 read_size = 0;
    u32 chunk_size = content_size > 0 && content_size < output_size ? content_size : 0x1000;
    if (chunk_size == 0 || chunk_size >= output_size) {
        chunk_size = output_size - 1;
    }

    do {
        if (total_read + chunk_size >= output_size) {
            return false;
        }
        read_size = 0;
        rc = httpcDownloadData(context, (u8*)output + total_read, chunk_size, &read_size);
        total_read += read_size;
    } while (rc == (Result)HTTPC_RESULTCODE_DOWNLOADPENDING);

    if (R_FAILED(rc)) {
        return false;
    }
    output[total_read] = '\0';
    return status >= 200 && status < 300;
}

static bool http_request_json(const AppState* app, HTTPC_RequestMethod method, const char* path, const char* body, char* output, size_t output_size) {
    httpcContext context;
    char url[HA3DS_STR_URL + HA3DS_STR_MEDIUM];
    char next_url[HA3DS_STR_URL + HA3DS_STR_MEDIUM];
    char auth[HA3DS_STR_TOKEN + 16];
    Result rc;
    u32 status = 0;
    int redirects = 0;

    snprintf(url, sizeof(url), "%s%s", app->config.base_url, path);

    while (redirects <= HTTP_REDIRECT_LIMIT) {
        rc = httpcOpenContext(&context, method, url, 1);
        if (R_FAILED(rc)) {
            return false;
        }

        httpcSetSSLOpt(&context, SSLCOPT_DisableVerify);
        httpcSetKeepAlive(&context, HTTPC_KEEPALIVE_ENABLED);
        snprintf(auth, sizeof(auth), "Bearer %s", app->config.access_token);
        httpcAddRequestHeaderField(&context, "User-Agent", "HomePad/0.1 (Nintendo 3DS)");
        httpcAddRequestHeaderField(&context, "Authorization", auth);
        httpcAddRequestHeaderField(&context, "Accept", "application/json");
        httpcAddRequestHeaderField(&context, "Connection", "Keep-Alive");
        if (method != HTTPC_METHOD_GET) {
            httpcAddRequestHeaderField(&context, "Content-Type", "application/json");
            if (body && body[0]) {
                httpcAddPostDataRaw(&context, (const u32*)body, (u32)strlen(body));
            }
        }

        bool ok = http_download(&context, output, output_size, &status);
        if (ok) {
            httpcCloseContext(&context);
            return true;
        }

        if (!((status >= 301 && status <= 303) || (status >= 307 && status <= 308))) {
            httpcCloseContext(&context);
            return false;
        }

        if (R_FAILED(httpcGetResponseHeader(&context, "Location", next_url, sizeof(next_url)))) {
            httpcCloseContext(&context);
            return false;
        }

        httpcCloseContext(&context);
        snprintf(url, sizeof(url), "%s", next_url);
        redirects++;
    }

    return false;
}

static void set_domain_from_entity(EntityState* entity) {
    char* dot = strchr(entity->entity_id, '.');
    if (!dot) {
        entity->domain[0] = '\0';
        return;
    }
    size_t len = (size_t)(dot - entity->entity_id);
    if (len >= sizeof(entity->domain)) {
        len = sizeof(entity->domain) - 1;
    }
    memcpy(entity->domain, entity->entity_id, len);
    entity->domain[len] = '\0';
}

static void parse_forecast(const char* json, const jsmntok_t* tokens, int forecast_index, EntityState* entity) {
    if (forecast_index < 0 || tokens[forecast_index].type != JSMN_ARRAY || tokens[forecast_index].size < 1) {
        return;
    }
    int first = forecast_index + 1;
    int high_index = json_find_key(json, tokens, first, "temperature");
    int low_index = json_find_key(json, tokens, first, "templow");
    int precip_index = json_find_key(json, tokens, first, "precipitation_probability");
    if (high_index >= 0) entity->high = json_to_float(json, &tokens[high_index], entity->high);
    if (low_index >= 0) entity->low = json_to_float(json, &tokens[low_index], entity->low);
    if (precip_index >= 0) entity->precipitation_chance = json_to_float(json, &tokens[precip_index], entity->precipitation_chance);
}

static void parse_attributes(const char* json, const jsmntok_t* tokens, int attr_index, EntityState* entity) {
    if (attr_index < 0 || tokens[attr_index].type != JSMN_OBJECT) {
        return;
    }

    int idx = json_find_key(json, tokens, attr_index, "friendly_name");
    if (idx >= 0) json_copy_string(json, &tokens[idx], entity->friendly_name, sizeof(entity->friendly_name));
    idx = json_find_key(json, tokens, attr_index, "unit_of_measurement");
    if (idx >= 0) json_copy_string(json, &tokens[idx], entity->unit, sizeof(entity->unit));
    idx = json_find_key(json, tokens, attr_index, "device_class");
    if (idx >= 0) json_copy_string(json, &tokens[idx], entity->device_class, sizeof(entity->device_class));
    idx = json_find_key(json, tokens, attr_index, "temperature");
    if (idx >= 0) {
        if (strcmp(entity->domain, "climate") == 0) {
            entity->target_temperature = json_to_float(json, &tokens[idx], entity->target_temperature);
        } else {
            entity->temperature = json_to_float(json, &tokens[idx], entity->temperature);
        }
    }
    idx = json_find_key(json, tokens, attr_index, "current_temperature");
    if (idx >= 0) entity->temperature = json_to_float(json, &tokens[idx], entity->temperature);
    idx = json_find_key(json, tokens, attr_index, "target_temp_step");
    if (idx >= 0) entity->target_temp_step = json_to_float(json, &tokens[idx], entity->target_temp_step);
    idx = json_find_key(json, tokens, attr_index, "min_temp");
    if (idx >= 0) entity->min_temp = json_to_float(json, &tokens[idx], entity->min_temp);
    idx = json_find_key(json, tokens, attr_index, "max_temp");
    if (idx >= 0) entity->max_temp = json_to_float(json, &tokens[idx], entity->max_temp);
    idx = json_find_key(json, tokens, attr_index, "feels_like_temperature");
    if (idx >= 0) entity->feels_like = json_to_float(json, &tokens[idx], entity->feels_like);
    idx = json_find_key(json, tokens, attr_index, "humidity");
    if (idx >= 0) entity->humidity = json_to_float(json, &tokens[idx], entity->humidity);
    idx = json_find_key(json, tokens, attr_index, "current_humidity");
    if (idx >= 0) entity->humidity = json_to_float(json, &tokens[idx], entity->humidity);
    idx = json_find_key(json, tokens, attr_index, "wind_speed");
    if (idx >= 0) entity->wind_speed = json_to_float(json, &tokens[idx], entity->wind_speed);
    idx = json_find_key(json, tokens, attr_index, "next_rising");
    if (idx >= 0) json_copy_string(json, &tokens[idx], entity->next_rising, sizeof(entity->next_rising));
    idx = json_find_key(json, tokens, attr_index, "next_setting");
    if (idx >= 0) json_copy_string(json, &tokens[idx], entity->next_setting, sizeof(entity->next_setting));
    idx = json_find_key(json, tokens, attr_index, "hvac_modes");
    parse_string_array_attr(json, tokens, idx, entity->hvac_modes, 6, &entity->hvac_mode_count);
    idx = json_find_key(json, tokens, attr_index, "forecast");
    parse_forecast(json, tokens, idx, entity);
}

static bool parse_states_payload(AppState* app, const char* json) {
    jsmn_parser parser;
    jsmntok_t* tokens = (jsmntok_t*)calloc(HA3DS_JSON_TOKENS, sizeof(jsmntok_t));
    if (!tokens) {
        return false;
    }

    jsmn_init(&parser);
    int token_count = jsmn_parse(&parser, json, (unsigned int)strlen(json), tokens, HA3DS_JSON_TOKENS);
    if (token_count < 1 || tokens[0].type != JSMN_ARRAY) {
        free(tokens);
        return false;
    }

    app->store.count = 0;
    int cursor = 1;
    int limit = tokens[0].size < HA3DS_MAX_ENTITIES ? tokens[0].size : HA3DS_MAX_ENTITIES;
    for (int i = 0; i < limit; i++) {
        EntityState* entity = &app->store.entities[app->store.count];
        entity_reset(entity);

        int id_index = json_find_key(json, tokens, cursor, "entity_id");
        int state_index = json_find_key(json, tokens, cursor, "state");
        int attr_index = json_find_key(json, tokens, cursor, "attributes");
        if (id_index >= 0) json_copy_string(json, &tokens[id_index], entity->entity_id, sizeof(entity->entity_id));
        if (state_index >= 0) json_copy_string(json, &tokens[state_index], entity->state, sizeof(entity->state));
        set_domain_from_entity(entity);
        parse_attributes(json, tokens, attr_index, entity);

        entity->is_available = strcmp(entity->state, "unavailable") != 0;
        entity->numeric_state = strtof(entity->state, NULL);
        entity->has_numeric_state = isdigit((unsigned char)entity->state[0]) || entity->state[0] == '-' || entity->state[0] == '+';
        if (strcmp(entity->domain, "person") == 0) {
            strncpy(entity->zone, entity->state, sizeof(entity->zone) - 1);
            entity->zone[sizeof(entity->zone) - 1] = '\0';
        }
        if (strcmp(entity->domain, "weather") == 0) {
            strncpy(entity->condition, entity->state, sizeof(entity->condition) - 1);
            entity->condition[sizeof(entity->condition) - 1] = '\0';
        }
        if (entity->friendly_name[0] == '\0') {
            strncpy(entity->friendly_name, entity->entity_id, sizeof(entity->friendly_name) - 1);
            entity->friendly_name[sizeof(entity->friendly_name) - 1] = '\0';
        }

        app->store.count++;
        cursor += (int)json_token_span(tokens, cursor);
    }

    app->store.loaded = true;
    app->store.last_poll_ms = osGetTime();
    free(tokens);
    return true;
}

bool ha_poll_states(AppState* app) {
    char* response = (char*)malloc(HA3DS_HTTP_BUFFER);
    if (!response) {
        app_set_status(app, "Out of memory");
        return false;
    }
    if (!app->config_loaded || app->config.base_url[0] == '\0' || app->config.access_token[0] == '\0') {
        app_set_status(app, "Config incomplete");
        free(response);
        return false;
    }

    bool ok = http_request_json(app, HTTPC_METHOD_GET, "/api/states", NULL, response, HA3DS_HTTP_BUFFER);
    if (ok) {
        ok = parse_states_payload(app, response);
    }
    if (!ok) {
        snprintf(app->store.last_error, sizeof(app->store.last_error), "Home Assistant poll failed");
        app_set_status(app, "Poll failed - check URL/token/network");
    } else {
        app->store.last_error[0] = '\0';
        app_set_status(app, "Updated %d entities", app->store.count);
    }
    free(response);
    return ok;
}

static const char* resolve_service(const char* domain) {
    if (strcmp(domain, "light") == 0 || strcmp(domain, "switch") == 0 || strcmp(domain, "fan") == 0) {
        return "toggle";
    }
    if (strcmp(domain, "scene") == 0 || strcmp(domain, "script") == 0) {
        return "turn_on";
    }
    return NULL;
}

static bool ha_call_service(AppState* app, const char* domain, const char* service, const char* body) {
    char path[HA3DS_STR_MEDIUM];
    char response[2048];
    snprintf(path, sizeof(path), "/api/services/%s/%s", domain, service);
    return http_request_json(app, HTTPC_METHOD_POST, path, body, response, sizeof(response));
}

static bool climate_cycle_mode(AppState* app, const EntityState* entity) {
    if (entity->hvac_mode_count <= 0) {
        return false;
    }

    int next_index = 0;
    for (int i = 0; i < entity->hvac_mode_count; i++) {
        if (strcmp(entity->hvac_modes[i], entity->state) == 0) {
            next_index = (i + 1) % entity->hvac_mode_count;
            break;
        }
    }

    char body[HA3DS_STR_MEDIUM + 48];
    snprintf(body, sizeof(body), "{\"entity_id\":\"%s\",\"hvac_mode\":\"%s\"}", entity->entity_id, entity->hvac_modes[next_index]);
    if (!ha_call_service(app, "climate", "set_hvac_mode", body)) {
        return false;
    }

    EntityState* mutable_entity = app_find_entity_mut(app, entity->entity_id);
    if (mutable_entity) {
        snprintf(mutable_entity->state, sizeof(mutable_entity->state), "%s", entity->hvac_modes[next_index]);
    }
    app->store.last_poll_ms = 0;
    app_set_status(app, "Climate mode -> %s", entity->hvac_modes[next_index]);
    return true;
}

static bool climate_adjust_target(AppState* app, const EntityState* entity, float delta) {
    float step = entity->target_temp_step > 0.0f ? entity->target_temp_step : 1.0f;
    float target = entity->target_temperature > -9000.0f ? entity->target_temperature : entity->temperature;
    if (target <= -9000.0f) {
        return false;
    }

    target += delta * step;
    if (entity->min_temp > -9000.0f && target < entity->min_temp) {
        target = entity->min_temp;
    }
    if (entity->max_temp > -9000.0f && target > entity->max_temp) {
        target = entity->max_temp;
    }

    char body[HA3DS_STR_MEDIUM + 64];
    snprintf(body, sizeof(body), "{\"entity_id\":\"%s\",\"temperature\":%.1f}", entity->entity_id, target);
    if (!ha_call_service(app, "climate", "set_temperature", body)) {
        return false;
    }

    EntityState* mutable_entity = app_find_entity_mut(app, entity->entity_id);
    if (mutable_entity) {
        mutable_entity->target_temperature = target;
    }
    app->store.last_poll_ms = 0;
    app_set_status(app, "Climate target -> %.1f", target);
    return true;
}

bool ha_trigger_entity(AppState* app, const char* entity_id) {
    const EntityState* entity = app_find_entity(app, entity_id);
    if (!entity) {
        app_set_status(app, "Entity not loaded");
        return false;
    }
    if (app->service_busy) {
        app_set_status(app, "Action already in progress");
        return false;
    }
    if (strcmp(entity->domain, "climate") == 0) {
        app->service_busy = true;
        bool ok = climate_cycle_mode(app, entity);
        app->service_busy = false;
        if (!ok) {
            app_set_status(app, "Climate mode change failed");
        }
        return ok;
    }
    const char* service = resolve_service(entity->domain);
    if (!service) {
        app_set_status(app, "No action for %s", entity->domain);
        return false;
    }

    char path[HA3DS_STR_MEDIUM];
    char body[HA3DS_STR_MEDIUM + 32];
    char response[2048];
    snprintf(path, sizeof(path), "/api/services/%s/%s", entity->domain, service);
    snprintf(body, sizeof(body), "{\"entity_id\":\"%s\"}", entity_id);

    app->service_busy = true;
    bool ok = http_request_json(app, HTTPC_METHOD_POST, path, body, response, sizeof(response));
    app->service_busy = false;
    if (!ok) {
        app_set_status(app, "Action failed for %s", entity->friendly_name);
        return false;
    }

    if (strcmp(entity->domain, "light") == 0 || strcmp(entity->domain, "switch") == 0 || strcmp(entity->domain, "fan") == 0) {
        EntityState* mutable_entity = app_find_entity_mut(app, entity_id);
        if (mutable_entity) {
            snprintf(mutable_entity->state, sizeof(mutable_entity->state), "%s", strcmp(entity->state, "on") == 0 ? "off" : "on");
        }
    } else if (strcmp(entity->domain, "scene") == 0 || strcmp(entity->domain, "script") == 0) {
        app->store.last_poll_ms = 0;
    }
    app_set_status(app, "Triggered %s", entity->friendly_name);
    return true;
}

bool ha_climate_adjust(AppState* app, const char* entity_id, int direction) {
    const EntityState* entity = app_find_entity(app, entity_id);
    if (!entity || strcmp(entity->domain, "climate") != 0) {
        app_set_status(app, "Climate entity not loaded");
        return false;
    }
    if (app->service_busy) {
        app_set_status(app, "Action already in progress");
        return false;
    }

    app->service_busy = true;
    bool ok = climate_adjust_target(app, entity, direction < 0 ? -1.0f : 1.0f);
    app->service_busy = false;
    if (!ok) {
        app_set_status(app, "Climate temp change failed");
    }
    return ok;
}
