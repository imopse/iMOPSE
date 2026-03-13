from pathlib import Path
import argparse
import pandas as pd
import matplotlib.pyplot as plt

ap = argparse.ArgumentParser()
ap.add_argument("--repo", required=True)
ap.add_argument("--inst", default="")
ap.add_argument("--methods", nargs="+", required=True)
args = ap.parse_args()

base = Path(args.repo) / "results" / "paretoAnalyzer"
plt.figure(figsize=(10, 6))

plotted_any = False
for m in args.methods:
    f = base / f"{m}_merged.csv"
    if not f.is_file():
        print(f"[skip] missing: {f}")
        continue

    df = pd.read_csv(f, sep=";", header=None)
    if df.shape[1] < 2:
        print(f"[skip] invalid format (need >=2 cols): {f}")
        continue

    plt.scatter(df.iloc[:,0], df.iloc[:,1], label=m, s=18)
    plotted_any = True

if not plotted_any:
    print("No merged pareto files found. Nothing to plot.")
    raise SystemExit(0)

plt.title("Merged Pareto Fronts" + (f" (inst={args.inst})" if args.inst else ""))
plt.xlabel("Objective 1")
plt.ylabel("Objective 2")
plt.grid(True)
plt.legend()
plt.show()
