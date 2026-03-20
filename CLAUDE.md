# Neverball Development Notes

## Testing mapc changes

### Verification method for geometry precision fixes

1. **Build mapc**: `make -j$(nproc)` from repo root
2. **Run mapc on affected maps** and compare output before/after:
   ```
   ./mapc data/map-medium/bumper.map
   ./mapc data/map-medium/mazebump.map
   ```
3. **Check for regressions**: Ensure no maps produce fewer geoms than before.
   Run mapc on the full set of maps and diff the output:
   ```
   for f in data/map-*/*.map; do echo "=== $f ==="; ./mapc "$f" 2>&1 | grep -E "geom|vert|side"; done > mapc_output.txt
   ```
4. **Key metrics to watch in mapc output**:
   - `s` (sides with 0 geoms) — fewer is better
   - Total geom/vert/side counts — should stay the same or improve
   - No crashes or errors on any map

### Known affected maps

- `data/map-medium/bumper.map` — has sides with 0 geoms (thin 0.125-unit brushes)
- `data/map-medium/mazebump.map` — same issue

### Plane precision context

- The snap_plane fix addresses near-duplicate planes that differ only by floating-point noise
- Thin brushes (0.125 units, valid in TrenchBroom) are a separate issue where `clip_vert` can't find enough vertices on narrow faces
- Both issues manifest as "sides with 0 geoms" in mapc output but have different root causes
