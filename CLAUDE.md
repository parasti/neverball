# Neverball Development Notes

## How mapc geometry changes were verified

Changes to `share/mapclib.c` (the map compiler) were verified using the
project's GitHub Pages deployment pipeline and a dedicated test map.

### Visual verification via GitHub Pages

1. Push changes to a `claude/*` branch.
2. GitHub Actions (`.github/workflows/pages.yml`) automatically:
   - Builds `mapc` and compiles all `.map` files to `.sol` files (`make sols`).
   - Builds Neverball to WebAssembly via Emscripten (`emscripten/ball.mk`).
   - Deploys the result to `https://play.neverball.org/<branch-name>/`.
3. Open the deployed URL in a browser.
4. Click the **"Test Map"** button in `js/index.html` (line 558). This button
   launches the game directly into `map-easy/invisible_face2.sol`, bypassing
   the normal menu flow.
5. Visually inspect the level. The `invisible_face2.map` (from GitHub issue
   #397) contains geometry that produces invisible faces on master due to
   floating-point precision issues in plane deduplication. After the
   `snap_plane` fix, those faces should be visible.

### soldump for before/after diffing

The `soldump` tool (`share/soldump.c`) was added to produce human-readable,
diffable text output from compiled `.sol` files. Usage:

```sh
# On master:
make soldump
./soldump data/map-easy/invisible_face2.sol > /tmp/before.txt

# On your branch:
make soldump
./soldump data/map-easy/invisible_face2.sol > /tmp/after.txt

diff /tmp/before.txt /tmp/after.txt
```

This prints every material, vertex, edge, side, texcoord, geom, lump, node,
etc. in the file, so you can see exactly which geometry primitives changed.

### Regression check across all maps

To verify no other maps lost geometry, run `mapc --csv` on every map and
compare output between master and your branch:

```sh
for f in data/map-*/*.map; do
    echo "=== $f ==="
    ./mapc "$f" data --csv 2>&1
done > /tmp/mapc_output.txt
```

The CSV columns are:
`file, n, c, t, mtrl, vert, edge, side, texc, offs, geom, lump, path, node, body, item, goal, view, jump, swch, bill, ball, char, dict, indx`

The `geom` column counts renderable geometry primitives. A decrease means
visible faces were lost (regression). An increase means previously invisible
faces were recovered (desired).

### Known affected maps

- `data/map-easy/invisible_face2.map` — primary test case from issue #397.
  On master, some faces are invisible. The `snap_plane` fix increases the
  geom count by recovering those faces.
- `data/map-easy/bumper.map` — contains thin brushes (0.125 units wide, the
  smallest grid size in TrenchBroom). Some sides produce 0 geoms because the
  brush is so narrow that `clip_vert` can't find enough vertex intersections.
  This is a separate issue from plane snapping.
- `data/map-easy/mazebump.map` — same thin-brush issue as bumper.
