/*
 * Copyright (C) 2024 Jānis Rūcis
 *
 * NEVERBALL is  free software; you can redistribute  it and/or modify
 * it under the  terms of the GNU General  Public License as published
 * by the Free  Software Foundation; either version 2  of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of
 * MERCHANTABILITY or  FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
 * General Public License for more details.
 */

#include "transition.h"
#include "common.h"
#include "gui.h"
#include "log.h"


/*
 * Widget IDs with exit transitions.
 */
static int transition_ids[16];

/*---------------------------------------------------------------------------*/

void transition_init(void)
{
    memset(transition_ids, 0, sizeof (transition_ids));
}

void transition_quit(void)
{
    memset(transition_ids, 0, sizeof (transition_ids));
}

void transition_add(int id)
{
    int i;

    for (i = 0; i < ARRAYSIZE(transition_ids); ++i)
        if (!transition_ids[i])
        {
            transition_ids[i] = id;
            break;
        }

    if (i == ARRAYSIZE(transition_ids))
    {
        log_printf("Out of transition slots\n");

        gui_remove(id);
    }
}

void transition_remove(int id)
{
    int i;

    for (i = 0; i < ARRAYSIZE(transition_ids); ++i)
        if (transition_ids[i] == id)
        {
            transition_ids[i] = 0;
            break;
        }
}

void transition_timer(float dt)
{
    int i;

    for (i = 0; i < ARRAYSIZE(transition_ids); ++i)
        if (transition_ids[i])
            gui_timer(transition_ids[i], dt);
}

void transition_paint(void)
{
    int i;

    for (i = 0; i < ARRAYSIZE(transition_ids); ++i)
        if (transition_ids[i])
            gui_paint(transition_ids[i]);
}

/*---------------------------------------------------------------------------*/

int transition_slide(int id, int in, int intent)
{
    if (in)
    {
        gui_slide(id, (intent == INTENT_BACK ? GUI_W : GUI_E) | GUI_FLING, 0, 0.16f, 0);
    }
    else
    {
        gui_slide(id, (intent == INTENT_BACK ? GUI_E : GUI_W) | GUI_BACKWARD | GUI_FLING | GUI_REMOVE, 0, 0.16f, 0);

        transition_add(id);
    }

    return id;
}

/*---------------------------------------------------------------------------*/