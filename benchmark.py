#!/usr/bin/env python3
import argparse
import csv
import pathlib
import re
import subprocess
import time

import matplotlib.pyplot as plt

def run_command(cmd):
    start = time.perf_counter()
    result = subprocess.run(cmd, capture_output=True, text=True, check=True)
    elapsed = time.perf_counter() - start
    return result.stdout, elapsed

def parse_program_time(output):
    m = re.search(r"Tempo de execucao\s*:\s*([0-9.]+)\s*segundos", output)
    return float(m.group(1)) if m else None

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--log", required=True)
    parser.add_argument("--output_dir", default="benchmark_out")
    args = parser.parse_args()

    outdir = pathlib.Path(args.output_dir)
    outdir.mkdir(parents=True, exist_ok=True)

    threads = [1, 2, 4, 8, 16]
    rows = []

    seq_out, _ = run_command(["./sensor_analyzer_seq", args.log])
    seq_time = parse_program_time(seq_out)
    (outdir / "saida_seq.txt").write_text(seq_out, encoding="utf-8")

    rows.append(["sequencial", 1, seq_time, 1.0, 1.0])

    for exe_name in ["sensor_analyzer_par", "sensor_analyzer_optimized"]:
        times = []
        for t in threads:
            out, _ = run_command([f"./{exe_name}", str(t), args.log])
            prog_time = parse_program_time(out)
            times.append((t, prog_time))
            (outdir / f"{exe_name}_{t}.txt").write_text(out, encoding="utf-8")

        for t, prog_time in times:
            speedup = seq_time / prog_time if prog_time else 0.0
            efficiency = speedup / t
            rows.append([exe_name, t, prog_time, speedup, efficiency])

    with open(outdir / "benchmark.csv", "w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(["versao", "threads", "tempo_s", "speedup", "eficiencia"])
        writer.writerows(rows)

    x = threads
    par_y = [r[3] for r in rows if r[0] == "sensor_analyzer_par"]
    opt_y = [r[3] for r in rows if r[0] == "sensor_analyzer_optimized"]

    plt.figure(figsize=(8, 5))
    plt.plot(x, par_y, marker="o", label="Paralela com mutex")
    plt.plot(x, opt_y, marker="o", label="Paralela otimizada")
    plt.plot(x, x, linestyle="--", label="Speedup ideal")
    plt.xlabel("Numero de threads")
    plt.ylabel("Speedup")
    plt.title("Speedup vs Numero de Threads")
    plt.xticks(x)
    plt.grid(True, alpha=0.3)
    plt.legend()
    plt.tight_layout()
    plt.savefig(outdir / "grafico_speedup.png", dpi=180)
    print(f"Benchmark concluido em {outdir}")

if __name__ == "__main__":
    main()
