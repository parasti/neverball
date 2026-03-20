# Neverball Development Notes

## How to verify mapc geometry changes

This describes how to test changes to `share/mapclib.c` (the map compiler) to
ensure they don't regress geometry output across all shipped maps.

### Step 1: Build mapc on the baseline (master)

```sh
git stash  # if you have uncommitted changes
git checkout master
make mapc -j$(nproc)
```

### Step 2: Generate baseline output for all maps

Run mapc in CSV mode on every `.map` file and save the output:

```sh
for f in data/map-*/*.map; do
    echo "=== $f ==="
    ./mapc "$f" data --csv 2>&1
done > /tmp/mapc_baseline.txt
```

The CSV output includes columns like `mtrl,vert,edge,side,texc,offs,geom,...`.
The `geom` column is the most important — it counts how many renderable
triangles were generated. A drop in `geom` count for a map means visible
faces were lost.

### Step 3: Build mapc on your branch and generate new output

```sh
git checkout <your-branch>
git stash pop  # if needed
make mapc -j$(nproc)

for f in data/map-*/*.map; do
    echo "=== $f ==="
    ./mapc "$f" data --csv 2>&1
done > /tmp/mapc_branch.txt
```

### Step 4: Diff the results

```sh
diff /tmp/mapc_baseline.txt /tmp/mapc_branch.txt
```

What to look for:
- **`geom` count decreased** for any map: your change lost visible geometry.
  This is a regression and must be investigated.
- **`geom` count increased** for a map: your change recovered previously
  invisible faces. This is the desired outcome for precision fixes.
- **`vert`, `edge`, `side` changed**: expected side effects of geometry changes.
  Not a problem unless `geom` decreased.
- **No diff at all for a map**: your change didn't affect that map. Fine.

### Key maps to watch

- `data/map-easy/bumper.map` — contains thin brushes (0.125 units wide, valid
  in TrenchBroom). These produce sides with 0 geoms because the brush is so
  narrow that `clip_vert` can't find enough vertex intersections on those faces.
  This is a separate issue from plane snapping.
- `data/map-easy/mazebump.map` — same thin-brush issue as bumper.
- `data/map-easy/invisible_face2.map` — the test map from GitHub issue #397.
  This is the primary map for verifying the `snap_plane` fix. On master, some
  faces are invisible. After the fix, geom count should increase.

### What the numbers mean

Example mapc CSV output line:
```
bumper.sol,55,88,0.106,10,8440,5216,28408,10196,29730,9910,348,20,79,73,...
```

The columns are (in order):
`file, n, c, t, mtrl, vert, edge, side, texc, offs, geom, lump, path, node, body, item, goal, view, jump, swch, bill, ball, char, dict, indx`

- `n` = brush count, `c` = brush capacity used
- `t` = compile time in seconds
- `geom` = number of renderable geometry primitives (triangles)
- `side` = number of brush sides processed
- `vert` = number of vertices in the compiled output
