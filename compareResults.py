#!/usr/bin/env python3

import math
from pathlib import Path
from typing import Dict, List, Tuple, Optional, Any

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt


Point = Tuple[float, float]

METHOD_ROOTS: Dict[str, Path] = {
    "BNTGAI": Path(r"experiments/bntgai/BNTGA_MSRCPSP/MSRCPSP_TA"),
    "other": Path(r"experiments/other"),
}

BASELINE_NAME = "BNTGAI"

OUT_DIR = Path(r"results/reports")
OUT_LATEX = OUT_DIR / "latex"
OUT_PAIRWISE_PLOTS = OUT_DIR / "pairwise_plots"
OUT_ALL_RUN_FRONT_PLOTS = OUT_DIR / "all_run_fronts"

MAX_RUNS_PER_INSTANCE: Optional[int] = 30
RESULT_FILE_CANDIDATES = ("results.csv", "results.json")

PLOT_DPI = 180
FIGSIZE_PAIR = (8, 5)
FIGSIZE_ALL_RUNS = (8, 5)

MARKER_SIZE_TPFS = 22
MARKER_SIZE_METHOD = 28
MARKER_SIZE_RUN = 14
MARKER_SIZE_MERGED = 30

HV_DIGITS = 6
IGD_DIGITS = 6
NDT_DIGITS = 4

HV_REF: Point = (1.0, 1.0)

NADIR: Dict[str, Tuple[float, float]] = {
    "1000_20_1024_5_A": (24374, 2395960),
    "1000_40_4096_10_A": (24616, 2427140),
    "500_10_512_10_A": (12300, 1055340),
    "500_40_2048_10_A": (12079, 1151130),
    "200_10_50_15": (4831, 483100),
    "200_40_130_9_D4": (3890, 155211),
    "200_20_0_0": (5007, 484677),
    "100_20_22_15": (2455, 239608),
    "100_5_48_9": (2448, 226930),
    "100_20_0_0": (2451, 240688),
    "500_20_0_0": (12332, 1113580),
    "1000_40_0_0": (24433, 2362670),
}


def instance_sort_key(name: str):
    try:
        return (int(name.split("_")[0]), name)
    except Exception:
        return (999999, name)


def run_sort_key(p: Path):
    s = p.name
    for prefix in ("run_", "run"):
        if s.startswith(prefix):
            s = s[len(prefix):]
            break
    try:
        return (0, int(s))
    except Exception:
        return (1, p.name)


def method_marker(method: str) -> str:
    if method == "ANGLE":
        return "^"
    if method == "GENO":
        return "s"
    if "escape" in method.lower():
        return "D"
    return "o"


def is_bad(x: float) -> bool:
    return math.isnan(x) or math.isinf(x)


def latex_escape(s: str) -> str:
    return s.replace("_", r"\_")


def bold(s: str) -> str:
    return r"\textbf{" + s + "}"


def dominates(a: Point, b: Point) -> bool:
    return (a[0] <= b[0] and a[1] <= b[1]) and (a[0] < b[0] or a[1] < b[1])


def nondominated(points: List[Point]) -> List[Point]:
    if not points:
        return []

    pts = sorted(set(points), key=lambda p: (p[0], p[1]))

    nd: List[Point] = []
    best_y = float("inf")

    for x, y in pts:
        if y < best_y:
            nd.append((x, y))
            best_y = y

    return nd


def merged_front(fronts: List[List[Point]]) -> List[Point]:
    pts: List[Point] = []
    for fr in fronts:
        pts.extend(fr)
    return nondominated(pts)


def number_of_non_dominated_by(eval_front: List[Point], true_front: List[Point]) -> int:
    if not eval_front:
        return 0
    if not true_front:
        return len(eval_front)

    cnt = 0
    for p in eval_front:
        dominated_by_true = False
        for q in true_front:
            if dominates(q, p):
                dominated_by_true = True
                break
        if not dominated_by_true:
            cnt += 1
    return cnt


def hv_2d_cpp(front: List[Point], ref: Point = HV_REF) -> float:
    if not front:
        return float("nan")

    pts = sorted(front, key=lambda t: t[0])

    hyper_volume = 0.0
    prev_cost = ref[1]
    for x, y in pts:
        hyper_volume += (ref[0] - x) * (prev_cost - y)
        prev_cost = y

    return hyper_volume


def calc_dist2(vec1: Point, vec2: Point) -> float:
    dx = vec2[0] - vec1[0]
    dy = vec2[1] - vec1[1]
    return dx * dx + dy * dy


def igd_cpp(true_pf: List[Point], approx_pf: List[Point]) -> float:
    if not true_pf or not approx_pf:
        return float("nan")

    dist_sum = 0.0
    for p in true_pf:
        min_dist2 = float("inf")
        for q in approx_pf:
            d2 = calc_dist2(p, q)
            if d2 < min_dist2:
                min_dist2 = d2
        dist_sum += min_dist2

    return math.sqrt(dist_sum) / float(len(true_pf))


def nd_tpfs_cpp(approx: List[Point], true_pf: List[Point]) -> float:
    if not approx or not true_pf:
        return float("nan")
    non_dominated = float(number_of_non_dominated_by(approx, true_pf))
    return non_dominated / float(len(true_pf))


def mean_std(xs: List[float]) -> Tuple[float, float]:
    vals = [x for x in xs if not is_bad(x)]
    if not vals:
        return float("nan"), float("nan")
    mu = sum(vals) / len(vals)
    var = sum((x - mu) ** 2 for x in vals) / len(vals)
    return mu, math.sqrt(var)


def find_results_file(run_dir: Path) -> Optional[Path]:
    for name in RESULT_FILE_CANDIDATES:
        p = run_dir / name
        if p.exists() and p.is_file():
            return p
    return None


def parse_points_from_text(text: str) -> List[Point]:
    points: List[Point] = []
    for line in text.splitlines():
        s = line.strip()
        if not s or ";" not in s:
            continue
        a, b = s.split(";", 1)
        try:
            x = int(float(a.strip()))
            y = int(float(b.strip()))
            points.append((float(x), float(y)))
        except ValueError:
            continue
    return points


def read_run_raw_points(run_dir: Path) -> List[Point]:
    fp = find_results_file(run_dir)
    if fp is None:
        return []
    return parse_points_from_text(fp.read_text(encoding="utf-8", errors="ignore"))


def normalize_points(points_raw: List[Point], nadir: Tuple[float, float]) -> List[Point]:
    n0, n1 = nadir
    if n0 == 0 or n1 == 0:
        return []
    return [(x / float(n0), y / float(n1)) for x, y in points_raw]


def list_valid_run_dirs(instance_dir: Path) -> List[Path]:
    runs = [p for p in instance_dir.iterdir() if p.is_dir()]
    runs.sort(key=run_sort_key)
    runs = [r for r in runs if find_results_file(r) is not None]
    if MAX_RUNS_PER_INSTANCE is not None:
        runs = runs[:MAX_RUNS_PER_INSTANCE]
    return runs


def empty_method_record() -> Dict[str, Any]:
    return {
        "run_dirs": [],
        "raw_runs": [],
        "raw_nd_runs": [],
        "raw_best_points": [],
        "raw_merged_nd": [],
        "norm_nd_runs": [],
        "norm_merged_nd": [],
    }


def collect_instance_method_data(instance: str, method: str, method_root: Path) -> Dict[str, Any]:
    rec = empty_method_record()
    inst_dir = method_root / instance
    if not inst_dir.is_dir():
        return rec

    run_dirs = list_valid_run_dirs(inst_dir)
    rec["run_dirs"] = [str(p) for p in run_dirs]

    nadir = NADIR[instance]

    raw_runs: List[List[Point]] = []
    raw_nd_runs: List[List[Point]] = []
    raw_best_points: List[Point] = []
    norm_nd_runs: List[List[Point]] = []

    for run_dir in run_dirs:
        raw_pts = read_run_raw_points(run_dir)
        if raw_pts:
            raw_runs.append(raw_pts)
            raw_nd = nondominated(raw_pts)
            raw_nd_runs.append(raw_nd)
            raw_best_points.extend(raw_nd)
            norm_nd = normalize_points(raw_nd, nadir)
            norm_nd_runs.append(norm_nd)

    raw_merged_nd = merged_front(raw_nd_runs)
    norm_merged_nd = normalize_points(raw_merged_nd, nadir)

    rec["raw_runs"] = raw_runs
    rec["raw_nd_runs"] = raw_nd_runs
    rec["raw_best_points"] = raw_best_points
    rec["raw_merged_nd"] = raw_merged_nd
    rec["norm_nd_runs"] = norm_nd_runs
    rec["norm_merged_nd"] = norm_merged_nd

    return rec


def build_instance_data() -> Dict[str, Dict[str, Dict[str, Any]]]:
    instance_data: Dict[str, Dict[str, Dict[str, Any]]] = {}
    active_instances = sorted(NADIR.keys(), key=instance_sort_key)

    print("Reading runs")
    for instance in active_instances:
        instance_data[instance] = {}
        for method, root in METHOD_ROOTS.items():
            if root.is_dir():
                instance_data[instance][method] = collect_instance_method_data(instance, method, root)
            else:
                instance_data[instance][method] = empty_method_record()
    print("Finished reading runs")
    return instance_data


def build_pair_tpfs_raw(
    instance_methods: Dict[str, Dict[str, Any]],
    method_a: str,
    method_b: str
) -> List[Point]:
    pts: List[Point] = []
    pts.extend(instance_methods[method_a]["raw_merged_nd"])
    pts.extend(instance_methods[method_b]["raw_merged_nd"])
    return nondominated(pts)


def build_pair_tpfs_norm(instance: str, tpfs_raw: List[Point]) -> List[Point]:
    return normalize_points(tpfs_raw, NADIR[instance])


def compute_pair_metrics_for_instance(
    instance: str,
    instance_methods: Dict[str, Dict[str, Any]],
    baseline: str,
    other: str,
) -> Optional[Dict[str, Any]]:
    b = instance_methods.get(baseline)
    o = instance_methods.get(other)
    if not b or not o:
        return None
    if not b["norm_nd_runs"] or not o["norm_nd_runs"]:
        return None

    tpfs_raw = build_pair_tpfs_raw(instance_methods, baseline, other)
    if not tpfs_raw:
        return None

    tpfs_norm = build_pair_tpfs_norm(instance, tpfs_raw)
    if not tpfs_norm:
        return None

    b_hv_runs = [hv_2d_cpp(fr, ref=HV_REF) for fr in b["norm_nd_runs"]]
    o_hv_runs = [hv_2d_cpp(fr, ref=HV_REF) for fr in o["norm_nd_runs"]]

    b_igd_runs = [igd_cpp(tpfs_norm, fr) for fr in b["norm_nd_runs"]]
    o_igd_runs = [igd_cpp(tpfs_norm, fr) for fr in o["norm_nd_runs"]]

    b_hv_mu, b_hv_sd = mean_std(b_hv_runs)
    o_hv_mu, o_hv_sd = mean_std(o_hv_runs)
    b_igd_mu, b_igd_sd = mean_std(b_igd_runs)
    o_igd_mu, o_igd_sd = mean_std(o_igd_runs)

    b_ndt = nd_tpfs_cpp(b["norm_merged_nd"], tpfs_norm)
    o_ndt = nd_tpfs_cpp(o["norm_merged_nd"], tpfs_norm)

    return {
        "instance": instance,
        "baseline_hv_mean": b_hv_mu,
        "baseline_hv_std": b_hv_sd,
        "other_hv_mean": o_hv_mu,
        "other_hv_std": o_hv_sd,
        "baseline_igd_mean": b_igd_mu,
        "baseline_igd_std": b_igd_sd,
        "other_igd_mean": o_igd_mu,
        "other_igd_std": o_igd_sd,
        "baseline_nd_tpfs": b_ndt,
        "other_nd_tpfs": o_ndt,
    }


def compute_all_pairwise_metrics(
    instance_data: Dict[str, Dict[str, Dict[str, Any]]]
) -> Dict[str, Dict[str, Dict[str, Any]]]:
    pair_metrics: Dict[str, Dict[str, Dict[str, Any]]] = {}
    others = [m for m in METHOD_ROOTS if m != BASELINE_NAME]

    print("Computing metrics")
    for other in others:
        pair_metrics[other] = {}
        for instance in sorted(instance_data.keys(), key=instance_sort_key):
            row = compute_pair_metrics_for_instance(
                instance=instance,
                instance_methods=instance_data[instance],
                baseline=BASELINE_NAME,
                other=other,
            )
            if row is not None:
                pair_metrics[other][instance] = row
    print("Finished computing metrics")
    return pair_metrics


def fmt_pm(mu: float, sd: float, digits: int) -> str:
    if is_bad(mu) or is_bad(sd):
        return r"--"
    return f"{mu:.{digits}f} $\\pm$ {sd:.{digits}f}"


def fmt_val(v: float, digits: int) -> str:
    if is_bad(v):
        return r"--"
    return f"{v:.{digits}f}"


def best_of_two(a: float, b: float, mode: str) -> Tuple[bool, bool]:
    if is_bad(a) and is_bad(b):
        return (False, False)
    if is_bad(a):
        return (False, True)
    if is_bad(b):
        return (True, False)

    eps = 1e-15
    if mode == "max":
        m = max(a, b)
        return (a >= m - eps, b >= m - eps)
    else:
        m = min(a, b)
        return (a <= m + eps, b <= m + eps)


def write_pair_tables(pair_metrics: Dict[str, Dict[str, Dict[str, Any]]]) -> None:
    OUT_LATEX.mkdir(parents=True, exist_ok=True)

    print("Writing LaTeX tables")
    for other, by_instance in pair_metrics.items():
        tex_lines: List[str] = []
        tex_lines.append(r"\begin{tabular}{l|ccc|ccc}")
        tex_lines.append(r"\toprule")
        tex_lines.append(
            r"Instance & \multicolumn{3}{c|}{" + latex_escape(BASELINE_NAME) + r"} & \multicolumn{3}{c}{" + latex_escape(other) + r"} \\"
        )
        tex_lines.append(r"\midrule")
        tex_lines.append(
            r" & HV $\uparrow$ & IGD $\downarrow$ & ND/TPFS $\uparrow$ & HV $\uparrow$ & IGD $\downarrow$ & ND/TPFS $\uparrow$ \\"
        )
        tex_lines.append(r"\midrule")

        for instance in sorted(by_instance.keys(), key=instance_sort_key):
            row = by_instance[instance]

            b_hv = row["baseline_hv_mean"]
            o_hv = row["other_hv_mean"]
            b_igd = row["baseline_igd_mean"]
            o_igd = row["other_igd_mean"]
            b_ndt = row["baseline_nd_tpfs"]
            o_ndt = row["other_nd_tpfs"]

            hv_b_best, hv_o_best = best_of_two(b_hv, o_hv, "max")
            igd_b_best, igd_o_best = best_of_two(b_igd, o_igd, "min")
            ndt_b_best, ndt_o_best = best_of_two(b_ndt, o_ndt, "max")

            b_hv_s = fmt_pm(row["baseline_hv_mean"], row["baseline_hv_std"], HV_DIGITS)
            o_hv_s = fmt_pm(row["other_hv_mean"], row["other_hv_std"], HV_DIGITS)
            b_igd_s = fmt_pm(row["baseline_igd_mean"], row["baseline_igd_std"], IGD_DIGITS)
            o_igd_s = fmt_pm(row["other_igd_mean"], row["other_igd_std"], IGD_DIGITS)
            b_ndt_s = fmt_val(b_ndt, NDT_DIGITS)
            o_ndt_s = fmt_val(o_ndt, NDT_DIGITS)

            if hv_b_best and b_hv_s != r"--":
                b_hv_s = bold(b_hv_s)
            if hv_o_best and o_hv_s != r"--":
                o_hv_s = bold(o_hv_s)
            if igd_b_best and b_igd_s != r"--":
                b_igd_s = bold(b_igd_s)
            if igd_o_best and o_igd_s != r"--":
                o_igd_s = bold(o_igd_s)
            if ndt_b_best and b_ndt_s != r"--":
                b_ndt_s = bold(b_ndt_s)
            if ndt_o_best and o_ndt_s != r"--":
                o_ndt_s = bold(o_ndt_s)

            tex_lines.append(
                f"{latex_escape(instance)} & {b_hv_s} & {b_igd_s} & {b_ndt_s} & {o_hv_s} & {o_igd_s} & {o_ndt_s} \\\\"
            )

        tex_lines.append(r"\bottomrule")
        tex_lines.append(r"\end{tabular}")
        tex_lines.append("")

        out_file = OUT_LATEX / f"{BASELINE_NAME}_vs_{other}.tex"
        out_file.write_text("\n".join(tex_lines), encoding="utf-8")

    print("Finished writing LaTeX tables")


def scatter_front(front: List[Point], label: Optional[str], marker: str, size: int, alpha: float):
    if not front:
        return
    xs = [p[0] for p in front]
    ys = [p[1] for p in front]
    if label is None:
        plt.scatter(xs, ys, marker=marker, s=size, alpha=alpha)
    else:
        plt.scatter(xs, ys, marker=marker, s=size, alpha=alpha, label=label)


def plot_pairwise_comparisons(instance_data: Dict[str, Dict[str, Dict[str, Any]]]) -> None:
    OUT_PAIRWISE_PLOTS.mkdir(parents=True, exist_ok=True)
    others = [m for m in METHOD_ROOTS if m != BASELINE_NAME]

    print("Making pairwise plots")
    for other in others:
        pair_dir = OUT_PAIRWISE_PLOTS / f"{BASELINE_NAME}_vs_{other}"
        pair_dir.mkdir(parents=True, exist_ok=True)

        for instance in sorted(instance_data.keys(), key=instance_sort_key):
            methods = instance_data[instance]
            b = methods.get(BASELINE_NAME, {})
            o = methods.get(other, {})

            b_front = nondominated(b.get("raw_best_points", []))
            o_front = nondominated(o.get("raw_best_points", []))

            if not b_front or not o_front:
                continue

            tpfs_pair = nondominated(b_front + o_front)

            plt.figure(figsize=FIGSIZE_PAIR)
            scatter_front(tpfs_pair, "TPFS", "x", MARKER_SIZE_TPFS, 0.35)
            scatter_front(b_front, BASELINE_NAME, method_marker(BASELINE_NAME), MARKER_SIZE_METHOD, 0.9)
            scatter_front(o_front, other, method_marker(other), MARKER_SIZE_METHOD, 0.9)

            plt.title(f"{instance} — {BASELINE_NAME} vs {other}")
            plt.xlabel("Makespan")
            plt.ylabel("Cost")
            plt.legend(fontsize=8)
            plt.tight_layout()
            plt.savefig(pair_dir / f"{instance}.png", dpi=PLOT_DPI)
            plt.close()
    print("Finished pairwise plots")


def plot_all_run_fronts(instance_data: Dict[str, Dict[str, Dict[str, Any]]]) -> None:
    OUT_ALL_RUN_FRONT_PLOTS.mkdir(parents=True, exist_ok=True)

    print("Making single-method plots")
    for method in METHOD_ROOTS:
        method_dir = OUT_ALL_RUN_FRONT_PLOTS / method
        method_dir.mkdir(parents=True, exist_ok=True)

        for instance in sorted(instance_data.keys(), key=instance_sort_key):
            rec = instance_data[instance].get(method, {})
            raw_nd_runs = rec.get("raw_nd_runs", [])
            raw_merged_nd = rec.get("raw_merged_nd", [])

            if not raw_nd_runs or not raw_merged_nd:
                continue

            plt.figure(figsize=FIGSIZE_ALL_RUNS)

            run_label_used = False
            for fr in raw_nd_runs:
                if not fr:
                    continue
                if not run_label_used:
                    scatter_front(fr, "run front", method_marker(method), MARKER_SIZE_RUN, 0.18)
                    run_label_used = True
                else:
                    scatter_front(fr, None, method_marker(method), MARKER_SIZE_RUN, 0.18)

            scatter_front(raw_merged_nd, "merged ND", "x", MARKER_SIZE_MERGED, 0.95)

            plt.title(f"{instance} — {method}")
            plt.xlabel("Makespan")
            plt.ylabel("Cost")
            plt.legend(fontsize=8)
            plt.tight_layout()
            plt.savefig(method_dir / f"{instance}.png", dpi=PLOT_DPI)
            plt.close()
    print("Finished single-method plots")


def main():
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    print("Checking method roots")
    for method, root in METHOD_ROOTS.items():
        print(f"{method}: {root} {'OK' if root.is_dir() else 'MISSING'}")

    instance_data = build_instance_data()
    pair_metrics = compute_all_pairwise_metrics(instance_data)
    write_pair_tables(pair_metrics)
    plot_pairwise_comparisons(instance_data)
    plot_all_run_fronts(instance_data)

    print("Done")
    print(f"LaTeX tables: {OUT_LATEX}")
    print(f"Pairwise plots: {OUT_PAIRWISE_PLOTS}")
    print(f"Single-method plots: {OUT_ALL_RUN_FRONT_PLOTS}")


if __name__ == "__main__":
    main()