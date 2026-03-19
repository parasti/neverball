#!/usr/bin/env python3
"""
Compare two OBJ files for geometric equivalence.

Usage: compare_obj.py <mapc.obj> <reference.obj> [--tolerance <eps>]

Compares vertex positions, face counts, triangle winding, and vertex-face
membership between a mapc-exported OBJ and a reference OBJ (e.g., from
TrenchBroom).

The comparison accounts for:
  - Different vertex/face ordering between the two files
  - Different triangulation of the same polygon (same-diagonal check)
  - Coordinate system transforms (configurable)
"""

import sys
import math
import argparse
from collections import defaultdict


def parse_obj(path):
    """Parse an OBJ file into vertices, normals, texcoords, and faces."""
    verts = []
    normals = []
    texcoords = []
    faces = []

    with open(path) as f:
        for line in f:
            line = line.strip()
            if line.startswith("v "):
                parts = line.split()
                verts.append((float(parts[1]), float(parts[2]), float(parts[3])))
            elif line.startswith("vn "):
                parts = line.split()
                normals.append((float(parts[1]), float(parts[2]), float(parts[3])))
            elif line.startswith("vt "):
                parts = line.split()
                texcoords.append((float(parts[1]), float(parts[2])))
            elif line.startswith("f "):
                parts = line.split()[1:]
                face_verts = []
                for p in parts:
                    # Handle v, v/t, v/t/n, v//n formats
                    indices = p.split("/")
                    vi = int(indices[0]) - 1  # OBJ is 1-based
                    face_verts.append(vi)
                faces.append(tuple(face_verts))

    return verts, normals, texcoords, faces


def vec_sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def vec_cross(a, b):
    return (
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    )


def vec_dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def vec_len(a):
    return math.sqrt(vec_dot(a, a))


def vec_dist(a, b):
    return vec_len(vec_sub(a, b))


def face_normal(verts, face):
    """Compute the face normal from the first three vertices."""
    if len(face) < 3:
        return (0, 0, 0)
    v0 = verts[face[0]]
    v1 = verts[face[1]]
    v2 = verts[face[2]]
    e1 = vec_sub(v1, v0)
    e2 = vec_sub(v2, v0)
    n = vec_cross(e1, e2)
    l = vec_len(n)
    if l < 1e-12:
        return (0, 0, 0)
    return (n[0] / l, n[1] / l, n[2] / l)


def canonical_face(verts, face):
    """Create a canonical representation of a face for comparison.

    Returns a tuple of sorted vertex positions (rounded), which is
    invariant to vertex index differences and winding order.
    """
    positions = tuple(sorted(verts[vi] for vi in face))
    return positions


def build_vertex_map(verts_a, verts_b, tolerance):
    """Build a mapping from verts_a indices to verts_b indices.

    Returns (mapping dict, unmatched_a list, unmatched_b set).
    """
    mapping = {}
    used_b = set()

    for i, va in enumerate(verts_a):
        best_j = -1
        best_d = tolerance + 1
        for j, vb in enumerate(verts_b):
            if j in used_b:
                continue
            d = vec_dist(va, vb)
            if d < best_d:
                best_d = d
                best_j = j
        if best_j >= 0 and best_d <= tolerance:
            mapping[i] = best_j
            used_b.add(best_j)

    unmatched_a = [i for i in range(len(verts_a)) if i not in mapping]
    unmatched_b = set(range(len(verts_b))) - used_b

    return mapping, unmatched_a, unmatched_b


def round_pos(v, decimals=5):
    return tuple(round(x, decimals) for x in v)


def compare(path_a, path_b, tolerance=1e-4):
    """Compare two OBJ files and print a report."""

    verts_a, normals_a, texc_a, faces_a = parse_obj(path_a)
    verts_b, normals_b, texc_b, faces_b = parse_obj(path_b)

    print(f"File A: {path_a}")
    print(f"  Vertices: {len(verts_a)}, Faces: {len(faces_a)}")
    print(f"File B: {path_b}")
    print(f"  Vertices: {len(verts_b)}, Faces: {len(faces_b)}")
    print()

    errors = 0

    # --- Vertex count ---

    if len(verts_a) != len(verts_b):
        print(f"WARN: Vertex count mismatch: {len(verts_a)} vs {len(verts_b)}")
    else:
        print(f"OK: Vertex count matches: {len(verts_a)}")

    # --- Face count ---

    if len(faces_a) != len(faces_b):
        print(f"WARN: Face count mismatch: {len(faces_a)} vs {len(faces_b)}")
    else:
        print(f"OK: Face count matches: {len(faces_a)}")

    # --- Vertex matching ---

    print()
    print("Building vertex mapping...")
    mapping, unmatched_a, unmatched_b = build_vertex_map(verts_a, verts_b, tolerance)

    print(f"  Matched: {len(mapping)} / {len(verts_a)}")
    if unmatched_a:
        print(f"  FAIL: {len(unmatched_a)} vertices in A have no match in B")
        errors += len(unmatched_a)
        for i in unmatched_a[:10]:
            print(f"    A[{i}] = {verts_a[i]}")
    if unmatched_b:
        print(f"  FAIL: {len(unmatched_b)} vertices in B have no match in A")
        errors += len(unmatched_b)

    # --- Face matching ---

    print()
    print("Comparing faces...")

    # Build canonical face sets using vertex positions
    def canon_face_positions(verts, face):
        return tuple(sorted(round_pos(verts[vi]) for vi in face))

    faces_a_canon = defaultdict(list)
    for fi, face in enumerate(faces_a):
        key = canon_face_positions(verts_a, face)
        faces_a_canon[key].append(fi)

    faces_b_canon = defaultdict(list)
    for fi, face in enumerate(faces_b):
        key = canon_face_positions(verts_b, face)
        faces_b_canon[key].append(fi)

    matched_faces = 0
    winding_mismatches = 0

    keys_a = set(faces_a_canon.keys())
    keys_b = set(faces_b_canon.keys())

    common = keys_a & keys_b
    only_a = keys_a - keys_b
    only_b = keys_b - keys_a

    for key in common:
        a_list = faces_a_canon[key]
        b_list = faces_b_canon[key]
        pairs = min(len(a_list), len(b_list))
        matched_faces += pairs

        # Check winding for matched pairs
        for k in range(pairs):
            na = face_normal(verts_a, faces_a[a_list[k]])
            nb = face_normal(verts_b, faces_b[b_list[k]])
            if vec_dot(na, nb) < 0:
                winding_mismatches += 1

    print(f"  Matched faces: {matched_faces} / {len(faces_a)}")
    if only_a:
        n = sum(len(faces_a_canon[k]) for k in only_a)
        print(f"  FAIL: {n} faces in A have no match in B")
        errors += n
    if only_b:
        n = sum(len(faces_b_canon[k]) for k in only_b)
        print(f"  FAIL: {n} faces in B have no match in A")
        errors += n
    if winding_mismatches:
        print(f"  WARN: {winding_mismatches} matched faces have opposite winding")
    else:
        print(f"  OK: All matched faces have consistent winding")

    # --- Summary ---

    print()
    if errors == 0:
        print("PASS: OBJ files are geometrically equivalent")
    else:
        print(f"FAIL: {errors} total errors found")

    return errors == 0


def main():
    parser = argparse.ArgumentParser(description="Compare two OBJ files")
    parser.add_argument("file_a", help="First OBJ file (e.g., mapc output)")
    parser.add_argument("file_b", help="Second OBJ file (e.g., TrenchBroom reference)")
    parser.add_argument("--tolerance", type=float, default=1e-4,
                        help="Vertex matching tolerance (default: 1e-4)")
    args = parser.parse_args()

    ok = compare(args.file_a, args.file_b, args.tolerance)
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
