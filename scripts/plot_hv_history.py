from pathlib import Path
import argparse
import pandas as pd
import matplotlib.pyplot as plt

ap = argparse.ArgumentParser()
ap.add_argument("--repo", required=True)
ap.add_argument("--method", required=True)
ap.add_argument("--inst", required=True)
ap.add_argument("--runs", type=int, default=10)
args = ap.parse_args()

root = Path(args.repo) / "results" / "experiments" / args.method / args.inst
files = [root / f"run_{i}" / "hv_history.csv" for i in range(args.runs)]

missing = [str(f) for f in files if not f.is_file()]
if missing:
    print("Missing hv_history.csv files:")
    for m in missing[:50]:
        print("  ", m)
    raise SystemExit(1)

dfs = []
for i, f in enumerate(files):
    df = pd.read_csv(f, sep=";")
    df["run"] = i
    dfs.append(df[["gen", "hv", "run"]])

all_df = pd.concat(dfs, ignore_index=True)

pv = all_df.pivot_table(index="gen", columns="run", values="hv", aggfunc="mean").sort_index()
mean = pv.mean(axis=1)
std = pv.std(axis=1)

plt.figure(figsize=(10, 6))
plt.plot(mean.index, mean.values, label=f"{args.method} mean HV ({args.runs} runs)")
plt.fill_between(mean.index, (mean - std).values, (mean + std).values, alpha=0.2, label="±1 std")
plt.title(f"HV vs Generation ({args.method}, inst={args.inst})")
plt.xlabel("Generation")
plt.ylabel("Hypervolume (ref=(1,1), normalized)")
plt.grid(True)
plt.legend()
plt.show()