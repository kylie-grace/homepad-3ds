#include "app.h"

#include <3ds.h>
#include <3ds/services/httpc.h>
#include <3ds/services/sslc.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

static u32* g_http_sharedmem = NULL;

static void init_services(void) {
    gfxInitDefault();
    gfxSet3D(false);
    g_http_sharedmem = (u32*)memalign(0x1000, 0x4000);
    if (g_http_sharedmem) {
        memset(g_http_sharedmem, 0, 0x4000);
    }
    httpcInit(g_http_sharedmem ? 0x4000 : 0);
    sslcInit(0);
}

static void clamp_state(AppState* app) {
    if (app->page >= PAGE_COUNT) {
        app->page = PAGE_OVERVIEW;
    }
    if (app->config.room_count <= 0) {
        app->selected_room = 0;
        if (app->page == PAGE_ROOM) {
            app->page = PAGE_OVERVIEW;
        }
        return;
    }
    if (app->selected_room < 0) {
        app->selected_room = 0;
    }
    if (app->selected_room >= app->config.room_count) {
        app->selected_room = app->config.room_count - 1;
    }
}

static void exit_services(void) {
    sslcExit();
    httpcExit();
    free(g_http_sharedmem);
    gfxExit();
}

int main(int argc, char** argv) {
    AppState app;
    memset(&app, 0, sizeof(app));
    app.page = PAGE_OVERVIEW;
    app.focused_button = 0;

    init_services();

    if (!config_load(&app.config, APP_CONFIG_PATH)) {
        app.config_loaded = false;
        app_set_status(&app, "Add sdmc:/3ds/homepad/config.json");
    } else {
        app.config_loaded = true;
        app_set_status(&app, "Config loaded");
        ha_poll_states(&app);
    }

    while (aptMainLoop() && !app.quit_requested) {
        hidScanInput();
        u32 keys_down = hidKeysDown();
        u32 keys_held = hidKeysHeld();
        touchPosition touch = {0};
        hidTouchRead(&touch);
        clamp_state(&app);
        ui_handle_input(&app, keys_down, keys_held, &touch);

        if ((keys_down & KEY_X) && app.config_loaded) {
            ha_poll_states(&app);
        }

        if (app.config_loaded &&
            app.config.poll_interval_seconds > 0 &&
            app.config.base_url[0] != '\0' &&
            osGetTime() - app.store.last_poll_ms > (u64)app.config.poll_interval_seconds * 1000ULL) {
            ha_poll_states(&app);
        }

        ui_render(&app);

        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }

    exit_services();
    return 0;
}
