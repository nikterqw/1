#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
run_metal.py (адаптированный)
Запускает exampleB1, затем забирает готовый imageN.txt и imageE.txt из рабочей папки.
"""

import os
import subprocess
import time
import shutil
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
THRESHOLD_KEV = 6.0

def load_matrix_256(path: Path) -> np.ndarray:
    rows = []
    with path.open("r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line: continue
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

def run_one(exe: Path, run_mac: Path, workdir: Path, log_path: Path, env: dict) -> int:
    # Мы больше не передаем --image-txt, чтобы не путать программу
    cmd = [str(exe.resolve()), str(run_mac.resolve())]
    
    with log_path.open("w", encoding="utf-8") as f:
        f.write("CMD: " + " ".join(cmd) + "\n")
        f.write("CWD: " + str(workdir) + "\n")
        f.flush()
        p = subprocess.run(cmd, cwd=str(workdir), stdout=f, stderr=subprocess.STDOUT, env=env)
    return p.returncode

def main() -> None:
    workdir = Path(".").resolve()
    exe = (workdir / EXE).resolve()
    run_mac = (workdir / RUN_MAC).resolve()

    if not exe.exists(): raise FileNotFoundError(f"Executable not found: {exe}")
    if not run_mac.exists(): raise FileNotFoundError(f"run.mac not found: {run_mac}")

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    info_path = OUT_DIR / "info.txt"
    log_path = OUT_DIR / "run_last.log"

    print("Starting simulation...", flush=True)

    with info_path.open("a", encoding="utf-8") as info:
        for i in range(N_RUNS):
            seed_run = BASE_SEED + i

            env = os.environ.copy()
            env["G4SEED"] = str(seed_run)
            env["SPECT_THRESHOLD_KEV"] = f"{THRESHOLD_KEV:.6f}"
            env["SPECT_SOURCE_SAMPLE"] = "0"

            rc = run_one(exe, run_mac, workdir, log_path, env)
            if rc != 0:
                raise RuntimeError(f"Run {i} failed (rc={rc}). See {log_path}")

            # Ищем, куда Geant4 сохранил файлы
            src_txt = workdir / "imageN.txt"
            src_png_txt = workdir / "imageE.txt"
            
            if not src_txt.exists():
                raise RuntimeError(f"imageN.txt not found in {workdir}. Check {log_path}")

            # Копируем результаты в нашу папку
            dst_txt = OUT_DIR / f"imageN_{i:06d}.txt"
            shutil.copy(src_txt, dst_txt)
            
            if src_png_txt.exists():
                dst_e_txt = OUT_DIR / f"imageE_{i:06d}.txt"
                shutil.copy(src_png_txt, dst_e_txt)

            # Делаем PNG
            mat = load_matrix_256(dst_txt)
            dst_png = OUT_DIR / f"imageN_{i:06d}.png"
            save_png_from_matrix(mat, dst_png)

            info.write(f"{i}\tseed={seed_run}\tmetal=W\tradius=2mm\tpos=(0,0,-5)cm\tbeam=80keV\n")
            info.flush()

    print(f"OK. Saved to: {OUT_DIR}", flush=True)

if __name__ == "__main__":
    main()
