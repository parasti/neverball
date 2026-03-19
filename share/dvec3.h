/*
 * Copyright (C) 2025 Jānis Rūcis
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

/*
 * Double-precision vector and matrix operations for the map compiler.
 *
 * These mirror the conventions in vec3.h but operate on double arrays.
 * Used only by mapc to maintain full precision during CSG computation.
 * The binary SOL format remains float32; conversion happens at output.
 */

#ifndef DVEC3_H
#define DVEC3_H

#include <math.h>

/*---------------------------------------------------------------------------*/

#define d_dot(u, v) ((u)[0] * (v)[0] + (u)[1] * (v)[1] + (u)[2] * (v)[2])
#define d_len(u)    sqrt(d_dot(u, u))

#define d_cpy(u, v) do { \
    (u)[0] = (v)[0];     \
    (u)[1] = (v)[1];     \
    (u)[2] = (v)[2];     \
} while (0)

#define d_inv(u, v) do { \
    (u)[0] = -(v)[0];    \
    (u)[1] = -(v)[1];    \
    (u)[2] = -(v)[2];    \
} while (0)

#define d_scl(u, v, k) do { \
    (u)[0] = (v)[0] * (k);  \
    (u)[1] = (v)[1] * (k);  \
    (u)[2] = (v)[2] * (k);  \
} while (0)

#define d_add(u, v, w) do {   \
    (u)[0] = (v)[0] + (w)[0]; \
    (u)[1] = (v)[1] + (w)[1]; \
    (u)[2] = (v)[2] + (w)[2]; \
} while (0)

#define d_sub(u, v, w) do {   \
    (u)[0] = (v)[0] - (w)[0]; \
    (u)[1] = (v)[1] - (w)[1]; \
    (u)[2] = (v)[2] - (w)[2]; \
} while (0)

#define d_mad(u, p, v, t) do {      \
    (u)[0] = (p)[0] + (v)[0] * (t); \
    (u)[1] = (p)[1] + (v)[1] * (t); \
    (u)[2] = (p)[2] + (v)[2] * (t); \
} while (0)

/*---------------------------------------------------------------------------*/

void   d_nrm(double *, const double *);
void   d_crs(double *, const double *, const double *);

void   dm_basis(double *, const double e0[3],
                          const double e1[3],
                          const double e2[3]);
void   dm_xps(double *, const double *);
int    dm_inv(double *, const double *);
void   dm_vxfm(double *, const double *, const double *);

/*---------------------------------------------------------------------------*/

#endif
