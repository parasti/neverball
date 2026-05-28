#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif
#include <stdio.h>
#include <string.h>

#include "fs.h"
#include "mapclib.h"
#include "config.h"
#include "state.h"
#include "st_play.h"
#include "level.h"
#include "progress.h"
#include "log.h"

EMSCRIPTEN_KEEPALIVE
void emscripten_play_test_map(const char *map_data)
{
    fs_file map_file;
    mapc_context ctx;

    log_printf("EDITOR_BRIDGE: Compiling test map from editor...\n");
    log_printf("EDITOR_BRIDGE: Map data size: %zu bytes\n", strlen(map_data));

    // Write the map data to a virtual file
    map_file = fs_open_write("/test.map");
    if (!map_file)
    {
        log_printf("EDITOR_BRIDGE: Error: Could not create /test.map\n");
        return;
    }

    if (fs_write(map_data, strlen(map_data), map_file) != strlen(map_data))
    {
        log_printf("EDITOR_BRIDGE: Error: fs_write failed to write all bytes\n");
    }
    fs_close(map_file);

    log_printf("EDITOR_BRIDGE: Wrote /test.map to MemFS\n");

    // Initialize mapc context
    if (!mapc_init(&ctx))
    {
        log_printf("EDITOR_BRIDGE: Error: Could not init mapc context\n");
        return;
    }

    log_printf("EDITOR_BRIDGE: Starting mapc_compile\n");

    // Compile it
    mapc_set_src(ctx, "/test.map");
    mapc_set_dst(ctx, "/test.sol");

    if (!mapc_compile(ctx))
    {
        log_printf("EDITOR_BRIDGE: Error: mapc_compile failed\n");
        mapc_quit(&ctx);
        return;
    }

    log_printf("EDITOR_BRIDGE: mapc_compile succeeded, dumping sol...\n");

    mapc_dump(ctx);
    mapc_quit(&ctx);

    log_printf("EDITOR_BRIDGE: Finished compiling /test.sol, attempting level_load\n");

    // Standalone mode map loading
    static struct level level;
    if (level_load("/test.sol", &level))
    {
        log_printf("EDITOR_BRIDGE: level_load succeeded, initializing standalone progress mode...\n");
        progress_init(MODE_STANDALONE);
        if (progress_play(&level))
        {
            log_printf("EDITOR_BRIDGE: progress_play succeeded. Triggering state transition to st_play_ready...\n");
            goto_state(&st_play_ready);
        }
        else
        {
            log_printf("EDITOR_BRIDGE: Error: progress_play failed.\n");
        }
    }
    else
    {
        log_printf("EDITOR_BRIDGE: Error: Failed to load /test.sol via level_load\n");
    }
}
