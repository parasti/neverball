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

#include <math.h>

#include "dvec3.h"

#define A 10
#define B 11
#define C 12
#define D 13
#define E 14
#define F 15

#define TINY 1e-15

/*---------------------------------------------------------------------------*/

void d_nrm(double *n, const double *v)
{
    double d = d_len(v);

    if (d == 0.0)
    {
        n[0] = 0.0;
        n[1] = 0.0;
        n[2] = 0.0;
    }
    else
    {
        n[0] = v[0] / d;
        n[1] = v[1] / d;
        n[2] = v[2] / d;
    }
}

void d_crs(double *u, const double *v, const double *w)
{
    u[0] = v[1] * w[2] - v[2] * w[1];
    u[1] = v[2] * w[0] - v[0] * w[2];
    u[2] = v[0] * w[1] - v[1] * w[0];
}

/*---------------------------------------------------------------------------*/

void dm_basis(double *M,
              const double e0[3],
              const double e1[3],
              const double e2[3])
{
    M[0] = e0[0]; M[4] = e1[0]; M[8] = e2[0]; M[C] = 0.0;
    M[1] = e0[1]; M[5] = e1[1]; M[9] = e2[1]; M[D] = 0.0;
    M[2] = e0[2]; M[6] = e1[2]; M[A] = e2[2]; M[E] = 0.0;
    M[3] =   0.0; M[7] =   0.0; M[B] =   0.0; M[F] = 1.0;
}

void dm_xps(double *M, const double *N)
{
    M[0] = N[0]; M[1] = N[4]; M[2] = N[8]; M[3] = N[C];
    M[4] = N[1]; M[5] = N[5]; M[6] = N[9]; M[7] = N[D];
    M[8] = N[2]; M[9] = N[6]; M[A] = N[A]; M[B] = N[E];
    M[C] = N[3]; M[D] = N[7]; M[E] = N[B]; M[F] = N[F];
}

int dm_inv(double *I, const double *M)
{
    double T[16];
    double d;

    T[0] = +(M[5] * (M[A] * M[F] - M[B] * M[E]) -
             M[9] * (M[6] * M[F] - M[7] * M[E]) +
             M[D] * (M[6] * M[B] - M[7] * M[A]));
    T[1] = -(M[4] * (M[A] * M[F] - M[B] * M[E]) -
             M[8] * (M[6] * M[F] - M[7] * M[E]) +
             M[C] * (M[6] * M[B] - M[7] * M[A]));
    T[2] = +(M[4] * (M[9] * M[F] - M[B] * M[D]) -
             M[8] * (M[5] * M[F] - M[7] * M[D]) +
             M[C] * (M[5] * M[B] - M[7] * M[9]));
    T[3] = -(M[4] * (M[9] * M[E] - M[A] * M[D]) -
             M[8] * (M[5] * M[E] - M[6] * M[D]) +
             M[C] * (M[5] * M[A] - M[6] * M[9]));

    T[4] = -(M[1] * (M[A] * M[F] - M[B] * M[E]) -
             M[9] * (M[2] * M[F] - M[3] * M[E]) +
             M[D] * (M[2] * M[B] - M[3] * M[A]));
    T[5] = +(M[0] * (M[A] * M[F] - M[B] * M[E]) -
             M[8] * (M[2] * M[F] - M[3] * M[E]) +
             M[C] * (M[2] * M[B] - M[3] * M[A]));
    T[6] = -(M[0] * (M[9] * M[F] - M[B] * M[D]) -
             M[8] * (M[1] * M[F] - M[3] * M[D]) +
             M[C] * (M[1] * M[B] - M[3] * M[9]));
    T[7] = +(M[0] * (M[9] * M[E] - M[A] * M[D]) -
             M[8] * (M[1] * M[E] - M[2] * M[D]) +
             M[C] * (M[1] * M[A] - M[2] * M[9]));

    T[8] = +(M[1] * (M[6] * M[F] - M[7] * M[E]) -
             M[5] * (M[2] * M[F] - M[3] * M[E]) +
             M[D] * (M[2] * M[7] - M[3] * M[6]));
    T[9] = -(M[0] * (M[6] * M[F] - M[7] * M[E]) -
             M[4] * (M[2] * M[F] - M[3] * M[E]) +
             M[C] * (M[2] * M[7] - M[3] * M[6]));
    T[A] = +(M[0] * (M[5] * M[F] - M[7] * M[D]) -
             M[4] * (M[1] * M[F] - M[3] * M[D]) +
             M[C] * (M[1] * M[7] - M[3] * M[5]));
    T[B] = -(M[0] * (M[5] * M[E] - M[6] * M[D]) -
             M[4] * (M[1] * M[E] - M[2] * M[D]) +
             M[C] * (M[1] * M[6] - M[2] * M[5]));

    T[C] = -(M[1] * (M[6] * M[B] - M[7] * M[A]) -
             M[5] * (M[2] * M[B] - M[3] * M[A]) +
             M[9] * (M[2] * M[7] - M[3] * M[6]));
    T[D] = +(M[0] * (M[6] * M[B] - M[7] * M[A]) -
             M[4] * (M[2] * M[B] - M[3] * M[A]) +
             M[8] * (M[2] * M[7] - M[3] * M[6]));
    T[E] = -(M[0] * (M[5] * M[B] - M[7] * M[9]) -
             M[4] * (M[1] * M[B] - M[3] * M[9]) +
             M[8] * (M[1] * M[7] - M[3] * M[5]));
    T[F] = +(M[0] * (M[5] * M[A] - M[6] * M[9]) -
             M[4] * (M[1] * M[A] - M[2] * M[9]) +
             M[8] * (M[1] * M[6] - M[2] * M[5]));

    d = M[0] * T[0] + M[4] * T[4] + M[8] * T[8] + M[C] * T[C];

    if (fabs(d) > TINY)
    {
        I[0] = T[0] / d;
        I[1] = T[4] / d;
        I[2] = T[8] / d;
        I[3] = T[C] / d;
        I[4] = T[1] / d;
        I[5] = T[5] / d;
        I[6] = T[9] / d;
        I[7] = T[D] / d;
        I[8] = T[2] / d;
        I[9] = T[6] / d;
        I[A] = T[A] / d;
        I[B] = T[E] / d;
        I[C] = T[3] / d;
        I[D] = T[7] / d;
        I[E] = T[B] / d;
        I[F] = T[F] / d;

        return 1;
    }
    return 0;
}

/*---------------------------------------------------------------------------*/

void dm_vxfm(double *v, const double *M, const double *w)
{
    v[0] = (w[0] * M[0] + w[1] * M[4] + w[2] * M[8]);
    v[1] = (w[0] * M[1] + w[1] * M[5] + w[2] * M[9]);
    v[2] = (w[0] * M[2] + w[1] * M[6] + w[2] * M[A]);
}

/*---------------------------------------------------------------------------*/
