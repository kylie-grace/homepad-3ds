#include "app.h"
#include "font8x8_basic.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#define TOP_W 400
#define BOTTOM_W 320
#define SCREEN_H 240

typedef struct {
    u8* fb;
    int width;
    int height;
} Canvas;

typedef struct {
    u8 r;
    u8 g;
    u8 b;
} Color;

static const Color C_BG = {28, 30, 46};
static const Color C_PANEL = {44, 47, 69};
static const Color C_PANEL_ALT = {55, 59, 85};
static const Color C_TEXT = {220, 226, 255};
static const Color C_SUB = {155, 162, 196};
static const Color C_ACCENT = {187, 152, 233};
static const Color C_WARM = {217, 196, 139};
static const Color C_GOOD = {140, 202, 145};
static const Color C_WARN = {225, 148, 146};
static const Color C_INFO = {113, 170, 222};
static const Color C_COOL = {121, 180, 235};
static const Color C_SCENE = {195, 145, 224};
static const Color C_SWITCH = {154, 206, 173};
static const Color C_SENSOR = {220, 182, 119};

static const char* PAGE_NAMES[PAGE_COUNT] = {"Overview", "Rooms", "People", "Weather", "Quick"};

static void copy_ellipsized(char* dest, size_t dest_size, const char* src, int max_chars) {
    if (dest_size == 0) {
        return;
    }
    if (!src) {
        dest[0] = '\0';
        return;
    }

    size_t src_len = strlen(src);
    size_t copy_len = src_len < (size_t)max_chars ? src_len : (size_t)max_chars;
    if (copy_len >= dest_size) {
        copy_len = dest_size - 1;
    }

    memcpy(dest, src, copy_len);
    dest[copy_len] = '\0';

    if (src_len > (size_t)max_chars && max_chars >= 3 && dest_size >= 4) {
        dest[max_chars - 3] = '.';
        dest[max_chars - 2] = '.';
        dest[max_chars - 1] = '.';
        dest[max_chars] = '\0';
    }
}

static void put_px(Canvas* canvas, int x, int y, Color color) {
    if (x < 0 || y < 0 || x >= canvas->width || y >= canvas->height) {
        return;
    }
    size_t offset = (size_t)(x * canvas->height + (canvas->height - 1 - y)) * 3;
    canvas->fb[offset + 0] = color.b;
    canvas->fb[offset + 1] = color.g;
    canvas->fb[offset + 2] = color.r;
}

static void fill_rect(Canvas* canvas, int x, int y, int w, int h, Color color) {
    for (int px = x; px < x + w; px++) {
        for (int py = y; py < y + h; py++) {
            put_px(canvas, px, py, color);
        }
    }
}

static void fill_rounded_rect(Canvas* canvas, int x, int y, int w, int h, int radius, Color color) {
    fill_rect(canvas, x + radius, y, w - 2 * radius, h, color);
    fill_rect(canvas, x, y + radius, radius, h - 2 * radius, color);
    fill_rect(canvas, x + w - radius, y + radius, radius, h - 2 * radius, color);
    for (int dx = 0; dx < radius; dx++) {
        for (int dy = 0; dy < radius; dy++) {
            if (dx * dx + dy * dy <= radius * radius) {
                put_px(canvas, x + radius - dx, y + radius - dy, color);
                put_px(canvas, x + w - radius - 1 + dx, y + radius - dy, color);
                put_px(canvas, x + radius - dx, y + h - radius - 1 + dy, color);
                put_px(canvas, x + w - radius - 1 + dx, y + h - radius - 1 + dy, color);
            }
        }
    }
}

static void draw_char(Canvas* canvas, int x, int y, char c, int scale, Color color) {
    unsigned char glyph = (unsigned char)c;
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (font8x8_basic[glyph][row] & (1u << col)) {
                fill_rect(canvas, x + col * scale, y + row * scale, scale, scale, color);
            }
        }
    }
}

static void draw_text(Canvas* canvas, int x, int y, const char* text, int scale, Color color) {
    int cursor = x;
    for (size_t i = 0; text[i] != '\0'; i++) {
        draw_char(canvas, cursor, y, text[i], scale, color);
        cursor += 8 * scale;
    }
}

static void draw_status_pill(Canvas* canvas, int x, int y, const char* text, Color bg, Color fg) {
    fill_rounded_rect(canvas, x, y, 90, 20, 7, bg);
    draw_text(canvas, x + 8, y + 6, text, 1, fg);
}

static Color domain_color(const EntityState* entity) {
    if (!entity) return C_PANEL_ALT;
    if (strcmp(entity->domain, "light") == 0) return C_WARM;
    if (strcmp(entity->domain, "switch") == 0) return C_SWITCH;
    if (strcmp(entity->domain, "fan") == 0) return C_GOOD;
    if (strcmp(entity->domain, "climate") == 0) return C_COOL;
    if (strcmp(entity->domain, "scene") == 0 || strcmp(entity->domain, "script") == 0) return C_SCENE;
    if (strcmp(entity->domain, "sensor") == 0 || strcmp(entity->domain, "binary_sensor") == 0) return C_SENSOR;
    if (strcmp(entity->domain, "weather") == 0) return C_ACCENT;
    return C_INFO;
}

static const char* domain_tag(const EntityState* entity) {
    if (!entity) return "--";
    if (strcmp(entity->domain, "light") == 0) return "LT";
    if (strcmp(entity->domain, "switch") == 0) return "SW";
    if (strcmp(entity->domain, "fan") == 0) return "FN";
    if (strcmp(entity->domain, "climate") == 0) return "CL";
    if (strcmp(entity->domain, "scene") == 0) return "SC";
    if (strcmp(entity->domain, "script") == 0) return "SR";
    if (strcmp(entity->domain, "sensor") == 0) return "SN";
    if (strcmp(entity->domain, "binary_sensor") == 0) return "BN";
    if (strcmp(entity->domain, "media_player") == 0) return "MD";
    if (strcmp(entity->domain, "person") == 0) return "PR";
    return "ET";
}

static void climate_summary(const EntityState* entity, char* out, size_t out_size) {
    char raw[48];
    if (!entity) {
        snprintf(out, out_size, "--");
        return;
    }
    if (entity->target_temperature > -9000.0f && entity->temperature > -9000.0f) {
        snprintf(raw, sizeof(raw), "%s %.0f/%.0f", entity->state, entity->target_temperature, entity->temperature);
    } else if (entity->target_temperature > -9000.0f) {
        snprintf(raw, sizeof(raw), "%s %.0f", entity->state, entity->target_temperature);
    } else {
        snprintf(raw, sizeof(raw), "%s", entity->state);
    }
    copy_ellipsized(out, out_size, raw, 16);
}

static void draw_panel_label(Canvas* canvas, int x, int y, const char* title, const char* value, const char* subtext, Color panel, Color accent) {
    fill_rounded_rect(canvas, x, y, 120, 70, 8, panel);
    draw_text(canvas, x + 10, y + 10, title, 1, C_SUB);
    draw_text(canvas, x + 10, y + 28, value, 2, accent);
    if (subtext && subtext[0]) {
        draw_text(canvas, x + 10, y + 52, subtext, 1, C_SUB);
    }
}

static void add_button(AppState* app, int x, int y, int w, int h, ActionType type, int value, bool enabled) {
    if (app->button_count >= HA3DS_MAX_UI_BUTTONS) {
        return;
    }
    UiButton* button = &app->buttons[app->button_count++];
    button->x = x;
    button->y = y;
    button->w = w;
    button->h = h;
    button->type = type;
    button->value = value;
    button->enabled = enabled;
}

static void clear_buttons(AppState* app) {
    app->button_count = 0;
    if (app->focused_button >= HA3DS_MAX_UI_BUTTONS) {
        app->focused_button = 0;
    }
}

static const char* safe_name(const EntityState* entity, const char* fallback) {
    if (!entity) {
        return fallback;
    }
    return entity->friendly_name[0] ? entity->friendly_name : entity->entity_id;
}

static void format_temp(char* out, size_t out_size, const EntityState* entity) {
    if (entity && entity->temperature > -9000.0f) {
        snprintf(out, out_size, "%.1fF", entity->temperature);
    } else if (entity && entity->has_numeric_state) {
        snprintf(out, out_size, "%.1f%s", entity->numeric_state, entity->unit[0] ? entity->unit : "");
    } else {
        snprintf(out, out_size, "--");
    }
}

static void draw_overview_top(Canvas* top, AppState* app) {
    char line[64];
    char clock_text[16];
    char weather_temp[32];
    const EntityState* weather = app_find_entity(app, app->config.weather_entity);
    const EntityState* indoor = app_find_entity(app, app->config.indoor_temp_entity);
    time_t now = time(NULL);
    struct tm* local = localtime(&now);
    snprintf(clock_text, sizeof(clock_text), "%02d:%02d", local ? local->tm_hour : 0, local ? local->tm_min : 0);
    format_temp(weather_temp, sizeof(weather_temp), weather);

    draw_text(top, 16, 14, app_greeting(), 1, C_SUB);
    snprintf(line, sizeof(line), "%s!", app->config.display_name[0] ? app->config.display_name : "Resident");
    draw_text(top, 16, 30, line, 2, C_TEXT);
    if (!app->config_loaded) {
        draw_status_pill(top, 292, 18, "SETUP", C_WARN, C_BG);
    } else if (app->store.last_error[0]) {
        draw_status_pill(top, 292, 18, "OFFLINE", C_WARN, C_BG);
    } else {
        draw_status_pill(top, 292, 18, "ONLINE", C_GOOD, C_BG);
    }

    draw_panel_label(top, 16, 64, "Time", clock_text, "Local", C_PANEL, C_TEXT);
    draw_panel_label(top, 146, 64, "Weather", weather ? weather->condition : "--", weather_temp, C_PANEL, C_ACCENT);

    char indoor_buf[32];
    format_temp(indoor_buf, sizeof(indoor_buf), indoor);
    draw_panel_label(top, 276, 64, "Indoor", indoor_buf, indoor ? safe_name(indoor, "") : "", C_PANEL, C_WARM);

    fill_rounded_rect(top, 16, 146, 380, 78, 10, C_PANEL_ALT);
    draw_text(top, 28, 158, "Whole Home", 1, C_SUB);
    snprintf(line, sizeof(line), "Lights on: %d", app_count_domain_state(app, "light", "on"));
    draw_text(top, 28, 178, line, 2, C_TEXT);
    snprintf(line, sizeof(line), "Active devices: %d", app_count_active_devices(app));
    draw_text(top, 28, 200, line, 1, C_SUB);
    snprintf(line, sizeof(line), "People home: %d/%d", app_count_domain_state(app, "person", "home"), app->config.person_count);
    draw_text(top, 200, 200, line, 1, C_SUB);

    if (weather) {
        snprintf(line, sizeof(line), "Feels %.0fF  High %.0fF  Low %.0fF", weather->feels_like, weather->high, weather->low);
        draw_text(top, 200, 178, line, 1, C_SUB);
    }
    if (app->store.last_error[0]) {
        draw_text(top, 16, 228, app->store.last_error, 1, C_WARN);
    }
}

static void draw_room_top(Canvas* top, AppState* app) {
    if (app->config.room_count == 0) {
        draw_text(top, 18, 18, "No rooms configured", 2, C_TEXT);
        return;
    }

    const RoomConfig* room = &app->config.rooms[app->selected_room];
    const EntityState* temp = app_find_entity(app, room->temp_sensor);
    const EntityState* humidity = app_find_entity(app, room->humidity_sensor);
    char temp_buf[32];
    char hum_buf[32];
    format_temp(temp_buf, sizeof(temp_buf), temp);
    snprintf(hum_buf, sizeof(hum_buf), "%.0f%%", humidity && humidity->humidity > -9000.0f ? humidity->humidity : (humidity && humidity->has_numeric_state ? humidity->numeric_state : 0.0f));

    draw_text(top, 18, 18, room->name, 2, C_TEXT);
    draw_panel_label(top, 16, 56, "Temperature", temp_buf, safe_name(temp, room->temp_sensor), C_PANEL, C_WARM);
    draw_panel_label(top, 146, 56, "Humidity", humidity ? hum_buf : "--", safe_name(humidity, room->humidity_sensor), C_PANEL, C_ACCENT);

    fill_rounded_rect(top, 276, 56, 108, 70, 8, C_PANEL);
    draw_text(top, 288, 66, "Controls", 1, C_SUB);
    snprintf(hum_buf, sizeof(hum_buf), "%d ready", room->control_count);
    draw_text(top, 288, 92, hum_buf, 2, C_TEXT);

    fill_rounded_rect(top, 16, 140, 368, 84, 10, C_PANEL_ALT);
    draw_text(top, 28, 150, "Highlights", 1, C_SUB);
    int y = 170;
    for (int i = 0; i < room->highlight_count; i++) {
        const EntityState* entity = app_find_entity(app, room->highlight_entities[i]);
        char line[128];
        if (entity && entity->has_numeric_state) {
            snprintf(line, sizeof(line), "%s: %.1f%s", safe_name(entity, room->highlight_entities[i]), entity->numeric_state, entity->unit);
        } else if (entity) {
            snprintf(line, sizeof(line), "%s: %s", safe_name(entity, room->highlight_entities[i]), entity->state);
        } else {
            snprintf(line, sizeof(line), "%s: --", room->highlight_entities[i]);
        }
        draw_text(top, 28, y, line, 1, C_TEXT);
        y += 16;
    }
}

static void draw_people_top(Canvas* top, AppState* app) {
    draw_text(top, 18, 18, "Household Presence", 2, C_TEXT);
    for (int i = 0; i < app->config.person_count; i++) {
        const EntityState* person = app_find_entity(app, app->config.people_entities[i]);
        int x = 16 + (i % 2) * 188;
        int y = 56 + (i / 2) * 64;
        char state[32];
        snprintf(state, sizeof(state), "%s", person ? person->state : "--");
        draw_panel_label(top, x, y, person ? safe_name(person, "Person") : "Person", state, person ? person->zone : "", C_PANEL, person && strcmp(state, "home") == 0 ? C_GOOD : C_WARN);
    }
}

static void draw_weather_top(Canvas* top, AppState* app) {
    const EntityState* weather = app_find_entity(app, app->config.weather_entity);
    const EntityState* sun = app_find_entity(app, "sun.sun");
    char line[64];
    draw_text(top, 18, 18, "Weather", 2, C_TEXT);
    fill_rounded_rect(top, 16, 56, 368, 160, 10, C_PANEL_ALT);
    draw_text(top, 28, 70, weather ? safe_name(weather, "Weather") : "Weather", 1, C_SUB);
    snprintf(line, sizeof(line), "%s %.0fF", weather ? weather->condition : "--", weather ? weather->temperature : 0.0f);
    draw_text(top, 28, 92, line, 2, C_TEXT);
    snprintf(line, sizeof(line), "Feels %.0fF  High %.0fF  Low %.0fF", weather ? weather->feels_like : 0.0f, weather ? weather->high : 0.0f, weather ? weather->low : 0.0f);
    draw_text(top, 28, 126, line, 1, C_SUB);
    snprintf(line, sizeof(line), "Wind %.0f mph  Rain %.0f%%", weather ? weather->wind_speed : 0.0f, weather ? weather->precipitation_chance : 0.0f);
    draw_text(top, 28, 148, line, 1, C_SUB);
    snprintf(line, sizeof(line), "Sunrise %s", sun && sun->next_rising[0] ? sun->next_rising : "--");
    draw_text(top, 28, 176, line, 1, C_SUB);
    snprintf(line, sizeof(line), "Sunset  %s", sun && sun->next_setting[0] ? sun->next_setting : "--");
    draw_text(top, 28, 194, line, 1, C_SUB);
}

static void draw_quick_top(Canvas* top, AppState* app) {
    draw_text(top, 18, 18, "Quick Actions", 2, C_TEXT);
    fill_rounded_rect(top, 16, 56, 368, 160, 10, C_PANEL_ALT);
    draw_text(top, 28, 68, "Scenes, scripts, and favorite toggles", 1, C_SUB);
    int y = 96;
    for (int i = 0; i < app->config.quick_action_count && i < 6; i++) {
        const EntityState* entity = app_find_entity(app, app->config.quick_action_entities[i]);
        char line[128];
        snprintf(line, sizeof(line), "%s [%s]", entity ? safe_name(entity, app->config.quick_action_entities[i]) : app->config.quick_action_entities[i], entity ? entity->state : "--");
        draw_text(top, 28, y, line, 1, C_TEXT);
        y += 20;
    }
}

static void draw_nav_bar(Canvas* bottom, AppState* app) {
    const int tab_w = 58;
    for (int i = 0; i < PAGE_COUNT; i++) {
        int x = 8 + i * (tab_w + 4);
        Color panel = app->page == i ? C_ACCENT : C_PANEL;
        fill_rounded_rect(bottom, x, 200, tab_w, 28, 7, panel);
        draw_text(bottom, x + 6, 210, PAGE_NAMES[i], 1, app->page == i ? C_BG : C_TEXT);
        add_button(app, x, 200, tab_w, 28, ACTION_PAGE, i, true);
    }
}

static void draw_entity_button(Canvas* bottom, AppState* app, int x, int y, int w, int h, const char* entity_id, int button_index) {
    const EntityState* entity = app_find_entity(app, entity_id);
    bool focused = app->focused_button == button_index;
    Color panel = focused ? C_ACCENT : C_PANEL;
    Color accent = domain_color(entity);
    char label[20];
    char state[24];

    copy_ellipsized(label, sizeof(label), entity ? safe_name(entity, entity_id) : entity_id, 16);
    if (entity && strcmp(entity->domain, "climate") == 0) {
        climate_summary(entity, state, sizeof(state));
    } else {
        copy_ellipsized(state, sizeof(state), entity ? entity->state : "--", 16);
    }

    fill_rounded_rect(bottom, x, y, w, h, 8, panel);
    fill_rounded_rect(bottom, x + 6, y + 8, 20, 12, 4, focused ? C_BG : accent);
    draw_text(bottom, x + 10, y + 10, domain_tag(entity), 1, focused ? accent : C_BG);
    draw_text(bottom, x + 32, y + 8, label, 1, focused ? C_BG : C_TEXT);
    draw_text(bottom, x + 32, y + 26, state, 1, focused ? C_BG : C_SUB);
    add_button(app, x, y, w, h, ACTION_ENTITY, button_index, entity != NULL);

    if (entity && strcmp(entity->domain, "climate") == 0) {
        fill_rounded_rect(bottom, x + w - 42, y + 6, 16, 14, 4, focused ? C_BG : C_COOL);
        fill_rounded_rect(bottom, x + w - 22, y + 6, 16, 14, 4, focused ? C_BG : C_COOL);
        draw_text(bottom, x + w - 38, y + 9, "-", 1, focused ? C_COOL : C_BG);
        draw_text(bottom, x + w - 18, y + 9, "+", 1, focused ? C_COOL : C_BG);
        add_button(app, x + w - 42, y + 6, 16, 14, ACTION_CLIMATE_DOWN, button_index, true);
        add_button(app, x + w - 22, y + 6, 16, 14, ACTION_CLIMATE_UP, button_index, true);
    }
}

static void draw_overview_bottom(Canvas* bottom, AppState* app) {
    draw_text(bottom, 12, 12, "Favorites", 2, C_TEXT);
    for (int i = 0; i < app->config.favorite_count; i++) {
        int row = i / 2;
        int col = i % 2;
        int x = 12 + col * 148;
        int y = 46 + row * 52;
        int button_index = app->button_count;
        draw_entity_button(bottom, app, x, y, 136, 42, app->config.favorite_entities[i], button_index);
    }
}

static void draw_room_bottom(Canvas* bottom, AppState* app) {
    if (app->config.room_count == 0) {
        return;
    }
    const RoomConfig* room = &app->config.rooms[app->selected_room];
    draw_text(bottom, 12, 10, room->name, 2, C_TEXT);
    for (int i = 0; i < room->control_count; i++) {
        int row = i / 2;
        int col = i % 2;
        int x = 12 + col * 148;
        int y = 44 + row * 52;
        int button_index = app->button_count;
        draw_entity_button(bottom, app, x, y, 136, 42, room->control_entities[i], button_index);
    }
    int room_y = 150;
    int room_start = (app->selected_room / 4) * 4;
    const int room_tab_x = 36;
    const int room_tab_w = 58;
    const int room_tab_gap = 2;
    if (app->config.room_count > 4 && room_start > 0) {
        fill_rounded_rect(bottom, 12, room_y, 20, 28, 6, C_PANEL);
        draw_text(bottom, 18, room_y + 10, "<", 1, C_TEXT);
        add_button(app, 12, room_y, 20, 28, ACTION_ROOM, room_start - 1, true);
    }
    for (int i = 0; i < 4 && room_start + i < app->config.room_count; i++) {
        int room_index = room_start + i;
        int x = room_tab_x + i * (room_tab_w + room_tab_gap);
        bool focused = app->page == PAGE_ROOM && room_index == app->selected_room;
        char label[12];
        copy_ellipsized(label, sizeof(label), app->config.rooms[room_index].name, 8);
        fill_rounded_rect(bottom, x, room_y, room_tab_w, 28, 6, focused ? C_ACCENT : C_PANEL);
        draw_text(bottom, x + 6, room_y + 10, label, 1, focused ? C_BG : C_TEXT);
        add_button(app, x, room_y, room_tab_w, 28, ACTION_ROOM, room_index, true);
    }
    if (app->config.room_count > 4 && room_start + 4 < app->config.room_count) {
        fill_rounded_rect(bottom, 308 - 20, room_y, 20, 28, 6, C_PANEL);
        draw_text(bottom, 308 - 14, room_y + 10, ">", 1, C_TEXT);
        add_button(app, 308 - 20, room_y, 20, 28, ACTION_ROOM, room_start + 4, true);
    }
}

static void draw_people_bottom(Canvas* bottom, AppState* app) {
    draw_text(bottom, 12, 14, "People summary is read-only in v1", 1, C_SUB);
    draw_text(bottom, 12, 32, "Zones come from person entity state.", 1, C_SUB);
}

static void draw_weather_bottom(Canvas* bottom, AppState* app) {
    draw_text(bottom, 12, 14, "Weather page is passive. Use Overview for fast glance.", 1, C_SUB);
    draw_text(bottom, 12, 32, "Sunrise/sunset reads from sun.sun when present.", 1, C_SUB);
}

static void draw_quick_bottom(Canvas* bottom, AppState* app) {
    draw_text(bottom, 12, 12, "Quick actions", 2, C_TEXT);
    for (int i = 0; i < app->config.quick_action_count && i < 6; i++) {
        int row = i / 2;
        int col = i % 2;
        int x = 12 + col * 148;
        int y = 44 + row * 52;
        int button_index = app->button_count;
        draw_entity_button(bottom, app, x, y, 136, 42, app->config.quick_action_entities[i], button_index);
    }
}

static const char* action_entity_id(const AppState* app, const UiButton* button) {
    if (button->type != ACTION_ENTITY && button->type != ACTION_CLIMATE_DOWN && button->type != ACTION_CLIMATE_UP) {
        return NULL;
    }
    if (app->page == PAGE_OVERVIEW && button->value < app->config.favorite_count) {
        return app->config.favorite_entities[button->value];
    }
    if (app->page == PAGE_ROOM && app->config.room_count > 0 && button->value < app->config.rooms[app->selected_room].control_count) {
        return app->config.rooms[app->selected_room].control_entities[button->value];
    }
    if (app->page == PAGE_QUICK && button->value < app->config.quick_action_count) {
        return app->config.quick_action_entities[button->value];
    }
    return NULL;
}

static bool focused_climate_entity(const AppState* app) {
    if (app->focused_button < 0 || app->focused_button >= app->button_count) {
        return false;
    }
    const UiButton* button = &app->buttons[app->focused_button];
    const char* entity_id = action_entity_id(app, button);
    if (!entity_id) {
        return false;
    }
    const EntityState* entity = app_find_entity(app, entity_id);
    return entity && strcmp(entity->domain, "climate") == 0;
}

static void perform_button(AppState* app, int index) {
    if (index < 0 || index >= app->button_count) {
        return;
    }
    const UiButton* button = &app->buttons[index];
    if (!button->enabled) {
        return;
    }
    switch (button->type) {
        case ACTION_PAGE:
            app->page = (PageId)button->value;
            break;
        case ACTION_ROOM:
            app->selected_room = button->value;
            app->page = PAGE_ROOM;
            break;
        case ACTION_ENTITY: {
            const char* entity_id = action_entity_id(app, button);
            if (entity_id) {
                ha_trigger_entity(app, entity_id);
            }
            break;
        }
        case ACTION_CLIMATE_DOWN: {
            const char* entity_id = action_entity_id(app, button);
            if (entity_id) {
                ha_climate_adjust(app, entity_id, -1);
            }
            break;
        }
        case ACTION_CLIMATE_UP: {
            const char* entity_id = action_entity_id(app, button);
            if (entity_id) {
                ha_climate_adjust(app, entity_id, 1);
            }
            break;
        }
        default:
            break;
    }
}

void ui_handle_input(AppState* app, u32 keys_down, u32 keys_held, touchPosition* touch) {
    if (keys_down & KEY_START) {
        app->quit_requested = true;
    }
    if (keys_down & KEY_R) {
        app->page = (PageId)((app->page + 1) % PAGE_COUNT);
    }
    if (keys_down & KEY_L) {
        app->page = (PageId)((app->page + PAGE_COUNT - 1) % PAGE_COUNT);
    }
    if ((keys_down & KEY_DUP) && app->focused_button > 0) {
        app->focused_button--;
    }
    if ((keys_down & KEY_DDOWN) && app->focused_button + 1 < app->button_count) {
        app->focused_button++;
    }
    if ((keys_down & KEY_DLEFT) && app->page == PAGE_ROOM && app->selected_room > 0) {
        app->selected_room--;
    }
    if ((keys_down & KEY_DRIGHT) && app->page == PAGE_ROOM && app->selected_room + 1 < app->config.room_count) {
        app->selected_room++;
    }
    if (keys_down & KEY_A) {
        perform_button(app, app->focused_button);
    }
    if (keys_held & KEY_TOUCH) {
        for (int i = 0; i < app->button_count; i++) {
            UiButton* button = &app->buttons[i];
            if (touch->px >= button->x && touch->px < button->x + button->w &&
                touch->py >= button->y && touch->py < button->y + button->h) {
                app->focused_button = i;
                if (keys_down & KEY_TOUCH) {
                    perform_button(app, i);
                }
                break;
            }
        }
    }
}

void ui_render(AppState* app) {
    u16 top_w = 0, top_h = 0, bottom_w = 0, bottom_h = 0;
    Canvas top = {gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &top_w, &top_h), TOP_W, SCREEN_H};
    Canvas bottom = {gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, &bottom_w, &bottom_h), BOTTOM_W, SCREEN_H};

    fill_rect(&top, 0, 0, top.width, top.height, C_BG);
    fill_rect(&bottom, 0, 0, bottom.width, bottom.height, C_BG);
    clear_buttons(app);

    switch (app->page) {
        case PAGE_OVERVIEW: draw_overview_top(&top, app); break;
        case PAGE_ROOM: draw_room_top(&top, app); break;
        case PAGE_PEOPLE: draw_people_top(&top, app); break;
        case PAGE_WEATHER: draw_weather_top(&top, app); break;
        case PAGE_QUICK: draw_quick_top(&top, app); break;
        default: break;
    }

    switch (app->page) {
        case PAGE_OVERVIEW: draw_overview_bottom(&bottom, app); break;
        case PAGE_ROOM: draw_room_bottom(&bottom, app); break;
        case PAGE_PEOPLE: draw_people_bottom(&bottom, app); break;
        case PAGE_WEATHER: draw_weather_bottom(&bottom, app); break;
        case PAGE_QUICK: draw_quick_bottom(&bottom, app); break;
        default: break;
    }

    draw_nav_bar(&bottom, app);
    if (app->focused_button >= app->button_count) {
        app->focused_button = app->button_count > 0 ? app->button_count - 1 : 0;
    }

    fill_rect(&bottom, 0, 232, bottom.width, 8, C_PANEL_ALT);
    draw_text(&bottom, 8, 233, app->status_line[0] ? app->status_line : "Ready", 1, C_SUB);
    if (focused_climate_entity(app)) {
        draw_text(&bottom, 178, 233, "A Mode +/- Touch", 1, C_INFO);
    } else {
        draw_text(&bottom, 222, 233, "X Refresh", 1, C_INFO);
    }
}
