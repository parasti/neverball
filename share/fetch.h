/*
 * Copyright (C) 2021-2024 Jānis Rūcis
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

#ifndef FETCH_H
#define FETCH_H 1

#include <stddef.h>

#define FETCH_MAX 5
#define FETCH_BODY_MAX 256u

/*
 * Progress callback extra_data.
 */
struct fetch_progress
{
    double total;
    double now;
};

/*
 * Done callback extra_data.
 */
struct fetch_done
{
    unsigned int success:1;
    long status;
    long retry_after;
    char body[FETCH_BODY_MAX];
    size_t body_size;
};

/*
 * Callbacks.
 */
struct fetch_callback
{
    void (*progress)(void *data, void *progress_data);
    void (*done)(void *data, void *done_data);
    void *data;
};

void fetch_init(void (*dispatch_event)(void *));
void fetch_handle_event(void *);
void fetch_quit(void);

unsigned int fetch_file(const char *url,
                        const char *dst,
                        struct fetch_callback);

unsigned int fetch_post(const char *url,
                        struct fetch_callback callback);

#endif