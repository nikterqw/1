#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
run_metal.py

Запуск Geant4-приложения с металлической сферой (W) и первичным пучком.
Генерирует imageN_XXXXXX.txt и PNG, пишет info.txt.
"""

import os
import subprocess
import time
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt


# =========================
# НАСТРОЙКИ
# =========================

EXE = Path("./exampleB1")
RUN_MAC = Path("./run.mac")
OUT_DIR = Path("runs_metal_W")

N_RUNS = 1
BASE_SEED = 0

# Визуализация источника (первые N событий)
SOURCE_SAMPLE_N = 0  # поставьте 10, если хотите видеть треки в визуализаторе

# Порог детектора (keV)
THRESHOLD_KEV = 6.0

PROGRESS_EVERY_SEC = 5.0


def load_matrix_256(path: Path) -> np.ndarray:
    rows = []
    with path.open("r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            rows.append([float(x) for x in line.split()])
    arr = np.array(rows, dtype=float)
    if arr.shape != (256, 256):
        raise RuntimeError(f"{path} has shape {arr.shape}, expected (256,256)")
    return arr


def save_png_from_matrix(mat: np.ndarray, png_path: Path) -> None:
    plt.figure(figsize=(6, 6), dpi=200)
    plt.imshow(mat, origin="lower", cmap="inferno")
    plt.axis("off")
    plt.tight_layout(pad=0)
    plt.savefig(png_path, bbox_inches="tight", pad_inches=0)
    plt.close()


def run_one(exe: Path, run_mac: Path, workdir: Path, log_path: Path,
            env: dict, img_out: Path) -> int:
    cmd = [str(exe.resolve()), str(run_mac.resolve()),
           "--image-txt", str(img_out.resolve())]
    with log_path.open("w", encoding="utf-8") as f:
        f.write("CMD: " + " ".join(cmd) + "\n")
        f.write("CWD: " + str(workdir) + "\n")
        f.flush()
        p = subprocess.run(cmd, cwd=str(workdir), stdout=f,
                           stderr=subprocess.STDOUT, env=env)
    return p.returncode


def main() -> None:
    workdir = Path(".").resolve()
    exe = (workdir / EXE).resolve()
    run_mac = (workdir / RUN_MAC).resolve()

    if not exe.exists():
        raise FileNotFoundError(f"Executable not found: {exe}")
    if not run_mac.exists():
        raise FileNotFoundError(f"run.mac not found: {run_mac}")

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    info_path = OUT_DIR / "info.txt"
    log_path = OUT_DIR / "run_last.log"

    print("Starting simulation...", flush=True)
    t0 = time.time()
    last_print = t0

    with info_path.open("a", encoding="utf-8") as info:
        for i in range(N_RUNS):
            now = time.time()
            if (now - last_print) >= PROGRESS_EVERY_SEC:
                done = i
                dt = now - t0
                rate = done / dt if dt > 0 else 0.0
                eta = (N_RUNS - done) / rate if rate > 0 else float("inf")
                print(f"[{done:6d}/{N_RUNS}] {rate:.4f} run/s, "
                      f"ETA {eta/60:.1f} min", flush=True)
                last_print = now

            seed_run = BASE_SEED + i

            env = os.environ.copy()
            env["G4SEED"] = str(seed_run)
            env["SPECT_THRESHOLD_KEV"] = f"{THRESHOLD_KEV:.6f}"
            env["SPECT_SOURCE_SAMPLE"] = str(SOURCE_SAMPLE_N)

            dst_txt = OUT_DIR / f"imageN_{i:06d}.txt"

            rc = run_one(exe, run_mac, workdir, log_path, env, dst_txt)
            if rc != 0:
                info.write(f"{i}\tseed={seed_run}\tFAILED rc={rc}\n")
                info.flush()
                raise RuntimeError(f"Run {i} failed (rc={rc}). See {log_path}")

            if not dst_txt.exists():
                out_files = sorted([p.name for p in OUT_DIR.iterdir() if p.is_file()])
                wd_files = sorted([p.name for p in workdir.iterdir() if p.is_file()])
                raise RuntimeError(
                    f"{dst_txt} not found after run {i}. "
                    f"OUT_DIR files: {out_files}. "
                    f"Workdir files: {wd_files}. "
                    f"Check {log_path}"
                )

            mat = load_matrix_256(dst_txt)
            dst_png = OUT_DIR / f"imageN_{i:06d}.png"
            save_png_from_matrix(mat, dst_png)

            info.write(
                f"{i}\tseed={seed_run}\t"
                f"metal=W\tradius=2mm\tpos=(0,0,-5)cm\t"
                f"beam=80keV\tpos=(5,0,-5)cm\tcone=5deg\n"
            )
            info.flush()

    dt = time.time() - t0
    rate = N_RUNS / dt if dt > 0 else 0.0
    print(f"[{N_RUNS:6d}/{N_RUNS}] DONE. avg {rate:.4f} run/s", flush=True)
    print(f"OK. Saved {N_RUNS} maps to: {OUT_DIR}", flush=True)
    print(f"Info: {info_path}", flush=True)


if __name__ == "__main__":
    main()
