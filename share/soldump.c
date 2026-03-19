/*
 * soldump - Dump SOL file contents in a human-readable, diffable format.
 *
 * Usage: soldump <file.sol>
 */

#include <stdio.h>
#include <stdlib.h>

#include "solid_base.h"
#include "fs.h"

static void dump_summary(const struct s_base *fp)
{
    printf("summary:\n");
    printf("  materials:  %d\n", fp->mc);
    printf("  vertices:   %d\n", fp->vc);
    printf("  edges:      %d\n", fp->ec);
    printf("  sides:      %d\n", fp->sc);
    printf("  texcoords:  %d\n", fp->tc);
    printf("  offsets:    %d\n", fp->oc);
    printf("  geoms:      %d\n", fp->gc);
    printf("  lumps:      %d\n", fp->lc);
    printf("  nodes:      %d\n", fp->nc);
    printf("  paths:      %d\n", fp->pc);
    printf("  bodies:     %d\n", fp->bc);
    printf("  items:      %d\n", fp->hc);
    printf("  goals:      %d\n", fp->zc);
    printf("  jumps:      %d\n", fp->jc);
    printf("  switches:   %d\n", fp->xc);
    printf("  billboards: %d\n", fp->rc);
    printf("  balls:      %d\n", fp->uc);
    printf("  views:      %d\n", fp->wc);
    printf("  dicts:      %d\n", fp->dc);
    printf("  indices:    %d\n", fp->ic);
    printf("\n");
}

static void dump_materials(const struct s_base *fp)
{
    int i;

    for (i = 0; i < fp->mc; i++)
    {
        const struct b_mtrl *mp = fp->mv + i;

        printf("mtrl %d:\n", i);
        printf("  file:  \"%s\"\n", mp->f);
        printf("  flags: 0x%08x\n", mp->fl);
        printf("  d: %g %g %g %g\n", mp->d[0], mp->d[1], mp->d[2], mp->d[3]);
        printf("  a: %g %g %g %g\n", mp->a[0], mp->a[1], mp->a[2], mp->a[3]);
        printf("  s: %g %g %g %g\n", mp->s[0], mp->s[1], mp->s[2], mp->s[3]);
        printf("  e: %g %g %g %g\n", mp->e[0], mp->e[1], mp->e[2], mp->e[3]);
        printf("  h: %g\n", mp->h[0]);
    }
    if (fp->mc) printf("\n");
}

static void dump_verts(const struct s_base *fp)
{
    int i;

    for (i = 0; i < fp->vc; i++)
    {
        const struct b_vert *vp = fp->vv + i;

        printf("vert %d: %+.8e %+.8e %+.8e\n", i, vp->p[0], vp->p[1], vp->p[2]);
    }
    if (fp->vc) printf("\n");
}

static void dump_edges(const struct s_base *fp)
{
    int i;

    for (i = 0; i < fp->ec; i++)
    {
        const struct b_edge *ep = fp->ev + i;

        printf("edge %d: %d %d\n", i, ep->vi, ep->vj);
    }
    if (fp->ec) printf("\n");
}

static void dump_sides(const struct s_base *fp)
{
    int i;

    for (i = 0; i < fp->sc; i++)
    {
        const struct b_side *sp = fp->sv + i;

        printf("side %d: n=(%+.8e %+.8e %+.8e) d=%+.8e\n",
               i, sp->n[0], sp->n[1], sp->n[2], sp->d);
    }
    if (fp->sc) printf("\n");
}

static void dump_texcoords(const struct s_base *fp)
{
    int i;

    for (i = 0; i < fp->tc; i++)
    {
        const struct b_texc *tp = fp->tv + i;

        printf("texc %d: %+.8e %+.8e\n", i, tp->u[0], tp->u[1]);
    }
    if (fp->tc) printf("\n");
}

static void dump_offsets(const struct s_base *fp)
{
    int i;

    for (i = 0; i < fp->oc; i++)
    {
        const struct b_offs *op = fp->ov + i;

        printf("offs %d: ti=%d si=%d vi=%d\n", i, op->ti, op->si, op->vi);
    }
    if (fp->oc) printf("\n");
}

static void dump_geoms(const struct s_base *fp)
{
    int i;

    for (i = 0; i < fp->gc; i++)
    {
        const struct b_geom *gp = fp->gv + i;

        printf("geom %d: mi=%d oi=%d oj=%d ok=%d\n",
               i, gp->mi, gp->oi, gp->oj, gp->ok);
    }
    if (fp->gc) printf("\n");
}

static void dump_lumps(const struct s_base *fp)
{
    int i;

    for (i = 0; i < fp->lc; i++)
    {
        const struct b_lump *lp = fp->lv + i;

        printf("lump %d: fl=0x%x v0=%d vc=%d e0=%d ec=%d g0=%d gc=%d s0=%d sc=%d\n",
               i, lp->fl, lp->v0, lp->vc, lp->e0, lp->ec,
               lp->g0, lp->gc, lp->s0, lp->sc);
    }
    if (fp->lc) printf("\n");
}

static void dump_nodes(const struct s_base *fp)
{
    int i;

    for (i = 0; i < fp->nc; i++)
    {
        const struct b_node *np = fp->nv + i;

        printf("node %d: si=%d ni=%d nj=%d l0=%d lc=%d\n",
               i, np->si, np->ni, np->nj, np->l0, np->lc);
    }
    if (fp->nc) printf("\n");
}

static void dump_paths(const struct s_base *fp)
{
    int i;

    for (i = 0; i < fp->pc; i++)
    {
        const struct b_path *pp = fp->pv + i;

        printf("path %d: p=(%+.8e %+.8e %+.8e) t=%g pi=%d f=%d s=%d fl=0x%x\n",
               i, pp->p[0], pp->p[1], pp->p[2], pp->t, pp->pi, pp->f, pp->s, pp->fl);
    }
    if (fp->pc) printf("\n");
}

static void dump_bodies(const struct s_base *fp)
{
    int i;

    for (i = 0; i < fp->bc; i++)
    {
        const struct b_body *bp = fp->bv + i;

        printf("body %d: p0=%d p1=%d ni=%d l0=%d lc=%d g0=%d gc=%d\n",
               i, bp->p0, bp->p1, bp->ni, bp->l0, bp->lc, bp->g0, bp->gc);
    }
    if (fp->bc) printf("\n");
}

static void dump_items(const struct s_base *fp)
{
    int i;

    for (i = 0; i < fp->hc; i++)
    {
        const struct b_item *hp = fp->hv + i;

        printf("item %d: p=(%+.8e %+.8e %+.8e) t=%d n=%d\n",
               i, hp->p[0], hp->p[1], hp->p[2], hp->t, hp->n);
    }
    if (fp->hc) printf("\n");
}

static void dump_goals(const struct s_base *fp)
{
    int i;

    for (i = 0; i < fp->zc; i++)
    {
        const struct b_goal *zp = fp->zv + i;

        printf("goal %d: p=(%+.8e %+.8e %+.8e) r=%g\n",
               i, zp->p[0], zp->p[1], zp->p[2], zp->r);
    }
    if (fp->zc) printf("\n");
}

static void dump_views(const struct s_base *fp)
{
    int i;

    for (i = 0; i < fp->wc; i++)
    {
        const struct b_view *wp = fp->wv + i;

        printf("view %d: p=(%+.8e %+.8e %+.8e) q=(%+.8e %+.8e %+.8e)\n",
               i, wp->p[0], wp->p[1], wp->p[2], wp->q[0], wp->q[1], wp->q[2]);
    }
    if (fp->wc) printf("\n");
}

static void dump_dicts(const struct s_base *fp)
{
    int i;

    for (i = 0; i < fp->dc; i++)
    {
        const struct b_dict *dp = fp->dv + i;
        const char *key = (dp->ai < fp->ac) ? fp->av + dp->ai : "???";
        const char *val = (dp->aj < fp->ac) ? fp->av + dp->aj : "???";

        printf("dict %d: \"%s\" = \"%s\"\n", i, key, val);
    }
    if (fp->dc) printf("\n");
}

static void dump_indices(const struct s_base *fp)
{
    int i;

    for (i = 0; i < fp->ic; i++)
    {
        printf("index %d: %d\n", i, fp->iv[i]);
    }
    if (fp->ic) printf("\n");
}

int main(int argc, char *argv[])
{
    struct s_base base;

    if (argc < 2)
    {
        fprintf(stderr, "Usage: soldump <file.sol>\n");
        return 1;
    }

    if (!fs_init(argv[0]))
    {
        fprintf(stderr, "Failed to init filesystem: %s\n", fs_error());
        return 1;
    }

    fs_add_path(".");

    if (!sol_load_base(&base, argv[1]))
    {
        fprintf(stderr, "Failed to load SOL: %s\n", argv[1]);
        return 1;
    }

    dump_summary(&base);
    dump_materials(&base);
    dump_verts(&base);
    dump_edges(&base);
    dump_sides(&base);
    dump_texcoords(&base);
    dump_offsets(&base);
    dump_geoms(&base);
    dump_lumps(&base);
    dump_nodes(&base);
    dump_paths(&base);
    dump_bodies(&base);
    dump_items(&base);
    dump_goals(&base);
    dump_views(&base);
    dump_dicts(&base);
    dump_indices(&base);

    sol_free_base(&base);

    return 0;
}
