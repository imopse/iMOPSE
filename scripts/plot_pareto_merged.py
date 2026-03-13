from pathlib import Path
import argparse
import pandas as pd
import matplotlib.pyplot as plt

ap = argparse.ArgumentParser()
ap.add_argument("--merged_dir", required=True)
ap.add_argument("--methods", nargs="+", required=True)
ap.add_argument("--title", default="Pareto Front")
ap.add_argument("--out", default="")
ap.add_argument("--show", action="store_true")
ap.add_argument("--global_front", action="store_true")
args = ap.parse_args()

merged_dir = Path(args.merged_dir)

def read_csv_anysep(path: Path) -> pd.DataFrame:
    try:
        return pd.read_csv(path, sep=";")
    except Exception:
        return pd.read_csv(path)

def pick_objective_cols(df: pd.DataFrame):
    cols = [c.strip() for c in df.columns]
    low = [c.lower() for c in cols]

    def find(preds):
        for p in preds:
            for c, lc in zip(cols, low):
                if p in lc:
                    return c
        return None

    ms = find(["makespan", "cmax", "span"])
    cost = find(["cost", "totalcost", "total_cost", "koszt"])

    if ms and cost:
        return ms, cost

    num = []
    for c in cols:
        if pd.api.types.is_numeric_dtype(df[c]):
            num.append(c)
    if len(num) >= 2:
        return num[0], num[1]

    raise ValueError(f"Cannot infer objective columns from {cols}")

def nondominated(points):
    n = len(points)
    nd = [True]*n
    for i in range(n):
        if not nd[i]:
            continue
        xi, yi = points[i]
        for j in range(n):
            if i == j:
                continue
            xj, yj = points[j]
            if (xj <= xi and yj <= yi) and (xj < xi or yj < yi):
                nd[i] = False
                break
    return nd

plt.figure(figsize=(10, 6))
all_union = []

for m in args.methods:
    f = merged_dir / f"{m}_merged.csv"
    if not f.is_file():
        print(f"[skip] missing: {f}")
        continue
    df = read_csv_anysep(f)
    ms_col, cost_col = pick_objective_cols(df)

    x = df[ms_col].astype(float).to_numpy()
    y = df[cost_col].astype(float).to_numpy()

    plt.scatter(x, y, s=14, alpha=0.75, label=m)
    all_union.extend(list(zip(x, y)))

if not all_union:
    print("No merged data found to plot.")
    raise SystemExit(0)

if args.global_front:
    nd_mask = nondominated(all_union)
    nd_pts = [p for p, keep in zip(all_union, nd_mask) if keep]
    nd_pts.sort(key=lambda t: t[0])
    xs = [p[0] for p in nd_pts]
    ys = [p[1] for p in nd_pts]
    plt.plot(xs, ys, linewidth=1.5, label="GLOBAL ND (union)")

plt.title(args.title)
plt.xlabel("Makespan")
plt.ylabel("Cost")
plt.grid(True)
plt.legend()

if args.out:
    Path(args.out).parent.mkdir(parents=True, exist_ok=True)
    plt.savefig(args.out, dpi=160)
    print(f"[ok] saved: {args.out}")

if args.show:
    plt.show()
