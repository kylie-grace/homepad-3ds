#include "app.h"
#include "jsmn.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONFIG_TOKEN_CAPACITY 2048

static size_t json_token_span(const jsmntok_t* tokens, int index) {
    size_t span = 1;
    int child = tokens[index].size;
    int cursor = index + 1;
    while (child-- > 0) {
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
    if (tokens[object_index].type != JSMN_OBJECT) {
        return -1;
    }
    int cursor = object_index + 1;
    int pair_count = tokens[object_index].size / 2;
    for (int pair = 0; pair < pair_count; pair++) {
        int key_index = cursor;
        int value_index = key_index + 1;
        if (json_eq(json, &tokens[key_index], key)) {
            return value_index;
        }
        cursor = value_index + (int)json_token_span(tokens, value_index);
    }
    return -1;
}

static void json_copy_string(const char* json, const jsmntok_t* token, char* dest, size_t dest_size) {
    if (token == NULL || token->start < 0 || token->end < 0 || dest_size == 0) {
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

static int json_to_int(const char* json, const jsmntok_t* token, int default_value) {
    char buffer[32];
    json_copy_string(json, token, buffer, sizeof(buffer));
    return buffer[0] ? atoi(buffer) : default_value;
}

static void trim_trailing_slashes(char* value) {
    size_t len = strlen(value);
    while (len > 0 && value[len - 1] == '/') {
        value[len - 1] = '\0';
        len--;
    }
}

static void parse_string_array(const char* json, const jsmntok_t* tokens, int array_index, char items[][HA3DS_STR_MEDIUM], int max_items, int* out_count) {
    *out_count = 0;
    if (array_index < 0 || tokens[array_index].type != JSMN_ARRAY) {
        return;
    }
    int cursor = array_index + 1;
    int limit = tokens[array_index].size < max_items ? tokens[array_index].size : max_items;
    for (int i = 0; i < limit; i++) {
        json_copy_string(json, &tokens[cursor], items[i], HA3DS_STR_MEDIUM);
        (*out_count)++;
        cursor += (int)json_token_span(tokens, cursor);
    }
}

static bool load_file(const char* path, char** out_buffer, size_t* out_size) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        return false;
    }
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);
    if (size <= 0) {
        fclose(file);
        return false;
    }
    char* buffer = (char*)malloc((size_t)size + 1);
    if (!buffer) {
        fclose(file);
        return false;
    }
    if (fread(buffer, 1, (size_t)size, file) != (size_t)size) {
        free(buffer);
        fclose(file);
        return false;
    }
    buffer[size] = '\0';
    fclose(file);
    *out_buffer = buffer;
    *out_size = (size_t)size;
    return true;
}

bool config_load(AppConfig* config, const char* path) {
    memset(config, 0, sizeof(*config));
    snprintf(config->display_name, sizeof(config->display_name), "Resident");
    config->poll_interval_seconds = 15;

    char* json = NULL;
    size_t size = 0;
    if (!load_file(path, &json, &size) && !load_file(APP_FALLBACK_CONFIG_PATH, &json, &size)) {
        return false;
    }

    jsmn_parser parser;
    jsmntok_t tokens[CONFIG_TOKEN_CAPACITY];
    jsmn_init(&parser);
    int token_count = jsmn_parse(&parser, json, (unsigned int)size, tokens, CONFIG_TOKEN_CAPACITY);
    if (token_count < 1 || tokens[0].type != JSMN_OBJECT) {
        free(json);
        return false;
    }

    int index = json_find_key(json, tokens, 0, "home_assistant_url");
    if (index >= 0) json_copy_string(json, &tokens[index], config->base_url, sizeof(config->base_url));
    index = json_find_key(json, tokens, 0, "access_token");
    if (index >= 0) json_copy_string(json, &tokens[index], config->access_token, sizeof(config->access_token));
    index = json_find_key(json, tokens, 0, "display_name");
    if (index >= 0) json_copy_string(json, &tokens[index], config->display_name, sizeof(config->display_name));
    index = json_find_key(json, tokens, 0, "weather_entity");
    if (index >= 0) json_copy_string(json, &tokens[index], config->weather_entity, sizeof(config->weather_entity));
    index = json_find_key(json, tokens, 0, "indoor_temp_entity");
    if (index >= 0) json_copy_string(json, &tokens[index], config->indoor_temp_entity, sizeof(config->indoor_temp_entity));
    index = json_find_key(json, tokens, 0, "poll_interval_seconds");
    if (index >= 0) config->poll_interval_seconds = json_to_int(json, &tokens[index], 15);
    trim_trailing_slashes(config->base_url);
    if (config->poll_interval_seconds < 10) config->poll_interval_seconds = 10;
    if (config->poll_interval_seconds > 300) config->poll_interval_seconds = 300;

    parse_string_array(json, tokens, json_find_key(json, tokens, 0, "people_entities"), config->people_entities, HA3DS_MAX_PEOPLE, &config->person_count);
    parse_string_array(json, tokens, json_find_key(json, tokens, 0, "favorite_entities"), config->favorite_entities, HA3DS_MAX_FAVORITES, &config->favorite_count);
    parse_string_array(json, tokens, json_find_key(json, tokens, 0, "quick_action_entities"), config->quick_action_entities, HA3DS_MAX_QUICK_ACTIONS, &config->quick_action_count);

    int rooms_index = json_find_key(json, tokens, 0, "rooms");
    if (rooms_index >= 0 && tokens[rooms_index].type == JSMN_ARRAY) {
        int cursor = rooms_index + 1;
        int limit = tokens[rooms_index].size < HA3DS_MAX_ROOMS ? tokens[rooms_index].size : HA3DS_MAX_ROOMS;
        for (int i = 0; i < limit; i++) {
            RoomConfig* room = &config->rooms[config->room_count];
            int name_index = json_find_key(json, tokens, cursor, "name");
            int temp_index = json_find_key(json, tokens, cursor, "temp_sensor");
            int humidity_index = json_find_key(json, tokens, cursor, "humidity_sensor");
            if (name_index >= 0) json_copy_string(json, &tokens[name_index], room->name, sizeof(room->name));
            if (temp_index >= 0) json_copy_string(json, &tokens[temp_index], room->temp_sensor, sizeof(room->temp_sensor));
            if (humidity_index >= 0) json_copy_string(json, &tokens[humidity_index], room->humidity_sensor, sizeof(room->humidity_sensor));
            parse_string_array(json, tokens, json_find_key(json, tokens, cursor, "control_entities"), room->control_entities, HA3DS_MAX_ROOM_CONTROLS, &room->control_count);
            parse_string_array(json, tokens, json_find_key(json, tokens, cursor, "highlight_entities"), room->highlight_entities, HA3DS_MAX_ROOM_HIGHLIGHTS, &room->highlight_count);
            config->room_count++;
            cursor += (int)json_token_span(tokens, cursor);
        }
    }

    free(json);
    return config->base_url[0] != '\0' && config->access_token[0] != '\0';
}
