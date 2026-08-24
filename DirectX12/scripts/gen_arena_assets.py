# -*- coding: utf-8 -*-
# 最小アリーナ用の.mstc/.mmat資産を生成するスクリプト.
# 使い方: py -3 scripts\gen_arena_assets.py   (DirectX12ディレクトリで実行)
#
# 生成物:
#   Data\Model\mmdl\mstc\ArenaFloor.mstc  直径40mの床円盤(y=0平面)
#   Data\Model\mmdl\mstc\ArenaRing.mstc   外周r=20mに立つ高さ1.2mの境界リング
#   Data\Model\mmdl\mmat\ArenaFloor.mmat / ArenaRing.mmat
#
# 頂点のワインディングはCube.mstcと同じ規約(cross(B-A, C-A)が面の表方向)に揃えている.
# Mstcパイプラインはカリング無しのため表裏どちらからでも描画されるが、法線は外向きで統一.

import math
import os
import struct

MSTC_DIR = os.path.join("Data", "Model", "mmdl", "mstc")
MMAT_DIR = os.path.join("Data", "Model", "mmdl", "mmat")

FLOOR_RADIUS = 20.0     # アリーナ半径(m). 直径40m = 「直径数十m」.
FLOOR_SEGMENTS = 64
RING_HEIGHT = 1.2       # 境界リングの高さ(m). 視覚化のみで当たり判定は無し.


def write_mstc(path, vertices, indices, material_name):
    """StaticVertex(32B)配列+uint16インデックス+マテリアル名の.mstcを出力する."""
    payload = b"".join(struct.pack("<8f",
                                   v[0], v[1], v[2],
                                   n[0], n[1], n[2],
                                   uv[0], uv[1])
                       for v, n, uv in vertices)
    payload += b"".join(struct.pack("<H", i) for i in indices)
    material = material_name.encode("ascii")
    header = struct.pack("<4sIIII", b"MSTC", 1,
                         len(vertices), len(indices), len(material))
    with open(path, "wb") as f:
        f.write(header + payload + material)


def write_mmat(path, diffuse, ambient):
    """テクスチャ無し単色の.mmat(v3)を出力する. 発光っぽさはAmbientの底上げで作る."""
    header = struct.pack("<4sIIIII", b"MMAT", 3, 0, 0, 0, 0)
    values = struct.pack("<11f2I",
                         diffuse[0], diffuse[1], diffuse[2], diffuse[3],
                         0.0, 0.0, 0.0,          # Specular
                         0.0,                     # SpecularPower
                         ambient[0], ambient[1], ambient[2],
                         0, 0)                    # UseSphereMap / UseToonMap
    with open(path, "wb") as f:
        f.write(header + values + b"\x00" * 512)


def gen_floor():
    verts = [((0.0, 0.0, 0.0), (0.0, 1.0, 0.0), (0.5, 0.5))]
    for i in range(FLOOR_SEGMENTS + 1):
        theta = 2.0 * math.pi * i / FLOOR_SEGMENTS
        x, z = FLOOR_RADIUS * math.cos(theta), FLOOR_RADIUS * math.sin(theta)
        verts.append(((x, 0.0, z), (0.0, 1.0, 0.0),
                      (x / (2.0 * FLOOR_RADIUS) + 0.5, z / (2.0 * FLOOR_RADIUS) + 0.5)))
    indices = []
    for i in range(FLOOR_SEGMENTS):
        indices += [0, i + 2, i + 1]  # 中心扇. 表が+y方向になる順序.
    return verts, indices


def gen_ring():
    verts = []
    for i in range(FLOOR_SEGMENTS + 1):
        theta = 2.0 * math.pi * i / FLOOR_SEGMENTS
        x, z = FLOOR_RADIUS * math.cos(theta), FLOOR_RADIUS * math.sin(theta)
        nx, nz = math.cos(theta), math.sin(theta)
        verts.append(((x, 0.0, z), (nx, 0.0, nz), (i / FLOOR_SEGMENTS * 16.0, 1.0)))
        verts.append(((x, RING_HEIGHT, z), (nx, 0.0, nz), (i / FLOOR_SEGMENTS * 16.0, 0.0)))
    indices = []
    for i in range(FLOOR_SEGMENTS):
        b0, t0 = i * 2, i * 2 + 1
        b1, t1 = i * 2 + 2, i * 2 + 3
        indices += [b0, t0, t1, b0, t1, b1]  # 法線(外向き)に対して正しい向きのワインディング.
    return verts, indices


def main():
    os.makedirs(MSTC_DIR, exist_ok=True)
    os.makedirs(MMAT_DIR, exist_ok=True)

    floor_diffuse = (0.35, 0.38, 0.43, 1.0)
    floor_ambient = (0.22, 0.24, 0.28)
    ring_diffuse = (0.10, 0.75, 0.95, 1.0)
    ring_ambient = (0.30, 1.50, 1.90)

    write_mmat(os.path.join(MMAT_DIR, "ArenaFloor.mmat"), floor_diffuse, floor_ambient)
    write_mmat(os.path.join(MMAT_DIR, "ArenaRing.mmat"), ring_diffuse, ring_ambient)

    verts, indices = gen_floor()
    write_mstc(os.path.join(MSTC_DIR, "ArenaFloor.mstc"),
               verts, indices, "ArenaFloor.mmat")

    verts, indices = gen_ring()
    write_mstc(os.path.join(MSTC_DIR, "ArenaRing.mstc"),
               verts, indices, "ArenaRing.mmat")

    print("generated: ArenaFloor.mstc / ArenaRing.mstc / ArenaFloor.mmat / ArenaRing.mmat")


if __name__ == "__main__":
    main()
