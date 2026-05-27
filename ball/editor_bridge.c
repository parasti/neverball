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

EMSCRIPTEN_KEEPALIVE
void emscripten_play_test_map(const char *map_data)
{
    fs_file map_file;
    mapc_context ctx;

    printf("Compiling test map from editor...\n");

    // Write the map data to a virtual file
    map_file = fs_open_write("/test.map");
    if (!map_file)
    {
        printf("Error: Could not create /test.map\n");
        return;
    }

    fs_write(map_data, strlen(map_data), map_file);
    fs_close(map_file);

    // Initialize mapc context
    if (mapc_init(&ctx) != 0)
    {
        printf("Error: Could not init mapc context\n");
        return;
    }

    // Compile it
    mapc_set_src(ctx, "/test.map");
    mapc_set_dst(ctx, "/test.sol");

    if (!mapc_compile(ctx))
    {
        printf("Error: mapc_compile failed\n");
        mapc_quit(&ctx);
        return;
    }

    mapc_dump(ctx);
    mapc_quit(&ctx);

    // Standalone mode map loading
    static struct level level;
    if (level_load("/test.sol", &level))
    {
        progress_init(MODE_STANDALONE);
        if (progress_play(&level))
        {
            goto_state(&st_play_ready);
        }
    }
    else
    {
        printf("Error: Failed to load /test.sol\n");
    }
}
