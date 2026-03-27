# Neverball

## Build

Build individual targets with `make <target> -j$(nproc)`. Key targets: `neverball`, `neverputt`, `mapc`, `soldump`.

## Testing mapc changes

To verify mapc output before and after a change, compare soldump output of compiled SOL files:

```sh
# 1. Build mapc from master in a worktree and compile a test map
git worktree add /tmp/neverball-master master
cd /tmp/neverball-master && make mapc -j$(nproc)
/tmp/neverball-master/mapc data/map-easy/test.map data
cp data/map-easy/test.sol v1.sol

# 2. Build mapc on the current branch and compile the same map
cd /path/to/neverball
make mapc -j$(nproc)
./mapc data/map-easy/test.map data
cp data/map-easy/test.sol v2.sol

# 3. Build soldump, dump both SOL files to text, and diff
make soldump -j$(nproc)
./soldump v1.sol > v1.txt 2>&1
./soldump v2.sol > v2.txt 2>&1
diff -u v1.txt v2.txt

# 4. Clean up
rm v1.sol v2.sol v1.txt v2.txt
git worktree remove /tmp/neverball-master
```

Note: `mapc` uses a virtual filesystem, so the output SOL path is derived from the input map path (always writes to `test.sol` next to `test.map`). The second argument to mapc is the data directory, not the output file. `soldump` also uses the virtual FS, so SOL files must be in the current directory (absolute paths won't work).

## Isolating mapc geometry bugs

When debugging missing triangles or wrong geometry, disable non-essential passes to get a deterministic, comparable output between master and the branch:

1. **Disable smth, uniq, and OBJ loading** in `share/mapclib.c`:
   - Comment out `read_obj(ctx, v[i], mi);` (~line 1444) — prevents OBJ model geometry
   - Comment out `uniq_file(ctx);` (~line 3612) — prevents deduplication from reordering
   - Comment out `smth_file(ctx);` (~line 3617) — prevents smoothing from merging normals

2. **Apply the same changes to the master worktree** (`/tmp/neverball-master/share/mapclib.c`).

3. **Compare per-lump geom count distributions** (not per-lump indices, since lump ordering may differ between pipelines):
   ```sh
   # Extract gc values and compare distributions
   grep "^lump " v1.txt | sed 's/.* gc=\([0-9]*\) .*/\1/' | sort -n | uniq -c | sort -rn
   grep "^lump " v2.txt | sed 's/.* gc=\([0-9]*\) .*/\1/' | sort -n | uniq -c | sort -rn
   ```

4. **Use `easy.map`** as the primary test — it has simple axis-aligned blocks plus diagonal fence brushes, making it a good mix. For stress testing, compile all easy maps: `for f in data/map-easy/*.map; do ./mapc "$f" data; done`

### Key diagnostic: per-face geom counts

For a correct box brush (6 planes), expect 6 faces with either 4 or 6 geoms per lump (2 or 3 quads = 4 or 6 triangles). A lump with `gc=0` but `sc=6` means the clipper failed to produce geometry for that brush intersection.

### Hull initialization

The polyhedron clipper starts from an axis-aligned bounding box and clips it by each brush plane. The initial AABB must fully contain the brush. `brush_aabb` computes a tight box via O(n³) plane-triple intersection. A simpler approach (large fixed box, e.g. `S=4096`) would also be correct — the clipper trims it down regardless — but wastes intermediate clipping work on faces far from the brush.
