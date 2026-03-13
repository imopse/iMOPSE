from pathlib import Path
import argparse
import pandas as pd
import matplotlib.pyplot as plt

ap = argparse.ArgumentParser()
ap.add_argument("--repo", required=True)
ap.add_argument("--inst", required=True)
ap.add_argument("--runs", type=int, required=True)
ap.add_argument("--methods", nargs="+", required=True)
ap.add_argument("--show", action="store_true")
ap.add_argument("--out", default="")
args = ap.parse_args()

plt.figure(figsize=(10, 6))
plotted = False

def read_hv(path):
    try:
        return pd.read_csv(path, sep=";")
    except Exception:
        return pd.read_csv(path)

for method in args.methods:
    root = Path(args.repo) / "results" / "experiments" / method / args.inst
    files = []
    for i in range(args.runs):
        a = root / f"run_{i}" / "hv_history.csv"
        b = root / f"run_{i}" / "results" / "hv_history.csv"
        f = a if a.is_file() else b
        files.append(f)

    missing = [f for f in files if not f.is_file()]
    if missing:
        print(f"[skip] {method}: missing hv_history.csv (e.g. {missing[0]})")
        continue

    dfs = []
    for i, f in enumerate(files):
        df = read_hv(f)
        df.columns = [c.strip().lower() for c in df.columns]
        if "gen" not in df.columns or "hv" not in df.columns:
            print(f"[skip] {method}: hv_history has unexpected columns: {list(df.columns)}")
            dfs = []
            break
        df["run"] = i
        dfs.append(df[["gen","hv","run"]])

    if not dfs:
        continue

    all_df = pd.concat(dfs, ignore_index=True)
    pv = all_df.pivot_table(index="gen", columns="run", values="hv", aggfunc="mean").sort_index()
    mean = pv.mean(axis=1)
    std  = pv.std(axis=1)

    plt.plot(mean.index, mean.values, label=f"{method}")
    plt.fill_between(mean.index, (mean-std).values, (mean+std).values, alpha=0.15)
    plotted = True

if not plotted:
    print("No HV histories found for the selected methods.")
    raise SystemExit(0)

plt.title(f"HV vs Generation (inst={args.inst})")
plt.xlabel("Generation")
plt.ylabel("Hypervolume (ref=(1,1), normalized)")
plt.grid(True)
plt.legend()

if args.out:
    Path(args.out).parent.mkdir(parents=True, exist_ok=True)
    plt.savefig(args.out, dpi=160)
    print(f"[ok] saved: {args.out}")

if args.show:
    plt.show()
