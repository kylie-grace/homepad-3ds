#ifndef HA3DS_APP_H
#define HA3DS_APP_H

#include <3ds.h>
#include <stdbool.h>
#include <stddef.h>

#define HA3DS_MAX_ENTITIES 256
#define HA3DS_MAX_ROOMS 12
#define HA3DS_MAX_PEOPLE 8
#define HA3DS_MAX_ROOM_CONTROLS 6
#define HA3DS_MAX_ROOM_HIGHLIGHTS 6
#define HA3DS_MAX_QUICK_ACTIONS 12
#define HA3DS_MAX_FAVORITES 8
#define HA3DS_MAX_UI_BUTTONS 24

#define HA3DS_STR_SMALL 32
#define HA3DS_STR_MEDIUM 64
#define HA3DS_STR_LARGE 128
#define HA3DS_STR_URL 256
#define HA3DS_STR_TOKEN 512

typedef struct {
    char name[HA3DS_STR_SMALL];
    char temp_sensor[HA3DS_STR_MEDIUM];
    char humidity_sensor[HA3DS_STR_MEDIUM];
    int control_count;
    char control_entities[HA3DS_MAX_ROOM_CONTROLS][HA3DS_STR_MEDIUM];
    int highlight_count;
    char highlight_entities[HA3DS_MAX_ROOM_HIGHLIGHTS][HA3DS_STR_MEDIUM];
} RoomConfig;

typedef struct {
    char base_url[HA3DS_STR_URL];
    char access_token[HA3DS_STR_TOKEN];
    char display_name[HA3DS_STR_SMALL];
    char weather_entity[HA3DS_STR_MEDIUM];
    char indoor_temp_entity[HA3DS_STR_MEDIUM];
    int poll_interval_seconds;
    int person_count;
    char people_entities[HA3DS_MAX_PEOPLE][HA3DS_STR_MEDIUM];
    int favorite_count;
    char favorite_entities[HA3DS_MAX_FAVORITES][HA3DS_STR_MEDIUM];
    int quick_action_count;
    char quick_action_entities[HA3DS_MAX_QUICK_ACTIONS][HA3DS_STR_MEDIUM];
    int room_count;
    RoomConfig rooms[HA3DS_MAX_ROOMS];
} AppConfig;

typedef struct {
    char entity_id[HA3DS_STR_MEDIUM];
    char domain[HA3DS_STR_SMALL];
    char state[HA3DS_STR_SMALL];
    char friendly_name[HA3DS_STR_MEDIUM];
    char unit[16];
    char device_class[HA3DS_STR_SMALL];
    char zone[HA3DS_STR_SMALL];
    char condition[HA3DS_STR_SMALL];
    char next_rising[HA3DS_STR_SMALL];
    char next_setting[HA3DS_STR_SMALL];
    bool is_available;
    bool has_numeric_state;
    float numeric_state;
    float temperature;
    float feels_like;
    float humidity;
    float high;
    float low;
    float wind_speed;
    float precipitation_chance;
} EntityState;

typedef struct {
    EntityState entities[HA3DS_MAX_ENTITIES];
    int count;
    bool loaded;
    u64 last_poll_ms;
    char last_error[HA3DS_STR_LARGE];
} EntityStore;

typedef enum {
    PAGE_OVERVIEW = 0,
    PAGE_ROOM = 1,
    PAGE_PEOPLE = 2,
    PAGE_WEATHER = 3,
    PAGE_QUICK = 4,
    PAGE_COUNT = 5
} PageId;

typedef enum {
    ACTION_NONE = 0,
    ACTION_PAGE,
    ACTION_ROOM,
    ACTION_ENTITY,
    ACTION_QUICK
} ActionType;

typedef struct {
    int x;
    int y;
    int w;
    int h;
    ActionType type;
    int value;
    bool enabled;
} UiButton;

typedef struct {
    AppConfig config;
    EntityStore store;
    PageId page;
    int selected_room;
    int focused_button;
    UiButton buttons[HA3DS_MAX_UI_BUTTONS];
    int button_count;
    bool quit_requested;
    bool service_busy;
    bool config_loaded;
    char status_line[HA3DS_STR_LARGE];
} AppState;

bool config_load(AppConfig* config, const char* path);
bool ha_poll_states(AppState* app);
bool ha_trigger_entity(AppState* app, const char* entity_id);
const EntityState* app_find_entity(const AppState* app, const char* entity_id);
EntityState* app_find_entity_mut(AppState* app, const char* entity_id);
int app_count_domain_state(const AppState* app, const char* domain, const char* desired_state);
int app_count_active_devices(const AppState* app);
const char* app_greeting(void);
void app_set_status(AppState* app, const char* fmt, ...);
void ui_render(AppState* app);
void ui_handle_input(AppState* app, u32 keys_down, u32 keys_held, touchPosition* touch);

#endif
