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
