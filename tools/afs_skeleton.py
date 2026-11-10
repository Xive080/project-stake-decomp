#!/usr/bin/env python3
"""Esqueleto por tipo de cuerpo: arbol de partes (BODYPT) en DATA00.BIN (tabla +0xF730 de raices, idx en +0x20, nhijos en +0x22,
punteros a hijos en +0x24, cargado en 0x1F1A000) y desplazamientos de hueso (registros de 20 B {id, x, y, z, escala}, indice parte-1)
en GAMEMOD.BIN (tabla de punteros en 0x9B5580). Uso como modulo: skeleton(tipo) -> (padres, offsets)."""
import struct
from pathlib import Path
import numpy as np
TBL = Path(__file__).resolve().parent.parent / "disc" / "TBL"
DB, GL, GT = 0x1F1A000, 0x92B000, 0x9B5580

def skeleton(t):
    D = (TBL / "DATA00.BIN").read_bytes(); G = (TBL / "GAMEMOD.BIN").read_bytes(); n = struct.unpack_from("<h", D, 0xF860 + 2 * t)[0]
    par = [-1] * n; ch = {}
    def walk(p, pa):
        o = p - DB; idx, c = struct.unpack_from("<hh", D, o + 0x20); par[idx] = pa
        for q in struct.unpack_from("<%dI" % c, D, o + 0x24): walk(q, idx)
    walk(struct.unpack_from("<I", D, 0xF730 + 4 * t)[0], -1)
    base = struct.unpack_from("<I", G, GT - GL + 4 * t)[0] - GL
    off = np.zeros((n, 3)); 
    for i in range(1, n): off[i] = struct.unpack_from("<3f", G, base + 20 * (i - 1) + 4)
    return par, off

def euler(a, order="zyx"):
    """a = (ax, ay, az) radianes; matriz R = Rorden[0] @ Rorden[1] @ Rorden[2]"""
    R = np.eye(3)
    for ax in order:
        i = "xyz".index(ax); c, s = np.cos(a[i]), np.sin(a[i]); M = np.eye(3); j, k = [(1, 2), (2, 0), (0, 1)][i]
        M[j, j] = c; M[k, k] = c; M[j, k] = -s; M[k, j] = s; R = R @ M
    return R

def pose(par, off, ang, root=(0, 0, 0), order="zyx", flip=1.0):
    """posiciones de cada articulacion para un cuadro (ang: n x 3 radianes)"""
    n = len(par); P = np.zeros((n, 3)); R = [None] * n
    for i in range(n):
        Ri = euler(ang[i], order)
        if par[i] < 0: R[i] = Ri; P[i] = root
        else: R[i] = R[par[i]] @ Ri; P[i] = P[par[i]] + R[par[i]] @ (off[i] * flip)
    return P
