from __future__ import annotations
from pathlib import Path
import argparse
import re
import csv
import zipfile
import tempfile
import statistics
from collections import Counter

TAG_RE = re.compile(r"^(START_RES_TREE|GEN0_BEST_RES_TREE|FINAL_BEST_RES_TREE)\s*$")
NODES_RE = re.compile(r"^nodes=(\d+)\s+depth=(\d+)\s*$")
EXPR_RE = re.compile(r"^expr=(.*)\s*$")
FEAT_RE = re.compile(r"\b(RES_[A-Z0-9_]+)\b")

def parse_report_txt(path: Path) -> dict:
    lines = path.read_text(encoding="utf-8", errors="ignore").splitlines()
    out = {"path": str(path)}
    i = 0
    while i < len(lines):
        m = TAG_RE.match(lines[i].strip())
        if not m:
            i += 1
            continue

        tag = m.group(1)
        if i + 1 < len(lines):
            mn = NODES_RE.match(lines[i + 1].strip())
            if mn:
                out[f"{tag}_nodes"] = int(mn.group(1))
                out[f"{tag}_depth"] = int(mn.group(2))
        if i + 2 < len(lines):
            me = EXPR_RE.match(lines[i + 2])
            if me:
                out[f"{tag}_expr"] = me.group(1).strip()
        i += 3
    return out

def open_input(input_path: Path) -> Path:
    """Returns directory that contains run_* folders."""
    if input_path.is_dir():
        return input_path

    if input_path.is_file() and input_path.suffix.lower() == ".zip":
        tmp = Path(tempfile.mkdtemp(prefix="res_tree_div_"))
        with zipfile.ZipFile(input_path, "r") as z:
            z.extractall(tmp)
        return tmp

    raise SystemExit(f"Input must be a directory or .zip, got: {input_path}")

def safe_mean(xs):
    return float(statistics.mean(xs)) if xs else float("nan")

def safe_median(xs):
    return float(statistics.median(xs)) if xs else float("nan")

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--input", required=True, help="Path to folder with run_* or a .zip containing run_*")
    ap.add_argument("--out_csv", default="", help="Output CSV path (optional)")
    ap.add_argument("--print_examples", action="store_true", help="Print final expr per run")
    args = ap.parse_args()

    root = open_input(Path(args.input).resolve())

    reports = []
    for run_dir in sorted(root.glob("run_*")):
        txt = run_dir / "res_tree_report.txt"
        if txt.is_file():
            d = parse_report_txt(txt)
            d["run"] = run_dir.name
            reports.append(d)

    if not reports:
        raise SystemExit(f"No run_*/res_tree_report.txt found under: {root}")

    start_expr = [r.get("START_RES_TREE_expr", "") for r in reports]
    gen0_expr  = [r.get("GEN0_BEST_RES_TREE_expr", "") for r in reports]
    final_expr = [r.get("FINAL_BEST_RES_TREE_expr", "") for r in reports]

    final_nodes = [r.get("FINAL_BEST_RES_TREE_nodes", 0) for r in reports]
    final_depth = [r.get("FINAL_BEST_RES_TREE_depth", 0) for r in reports]
    gen0_nodes  = [r.get("GEN0_BEST_RES_TREE_nodes", 0) for r in reports]

    uniq_start = len(set(start_expr))
    uniq_gen0  = len(set(gen0_expr))
    uniq_final = len(set(final_expr))

    final_leaf = sum(1 for n in final_nodes if n == 1)

    final_eq_start = sum(1 for a, b in zip(final_expr, start_expr) if a == b)
    final_eq_gen0  = sum(1 for a, b in zip(final_expr, gen0_expr) if a == b)


    ratios = []
    for n0, nf in zip(gen0_nodes, final_nodes):
        if n0 and nf:
            ratios.append(nf / n0)

    feat_counter = Counter()
    for ex in final_expr:
        feat_counter.update(FEAT_RE.findall(ex))

    print("=== Resource-tree diversity report ===")
    print(f"runs: {len(reports)}")
    print(f"unique START_RES_TREE: {uniq_start}/{len(reports)}")
    print(f"unique GEN0_BEST_RES_TREE: {uniq_gen0}/{len(reports)}")
    print(f"unique FINAL_BEST_RES_TREE: {uniq_final}/{len(reports)}")
    print(f"FINAL leaf trees (nodes==1): {final_leaf}/{len(reports)}")
    print(f"FINAL == START: {final_eq_start}/{len(reports)}")
    print(f"FINAL == GEN0:  {final_eq_gen0}/{len(reports)}")
    print("")
    print("FINAL size stats:")
    print(f"  nodes mean={safe_mean(final_nodes):.3f}  median={safe_median(final_nodes):.3f}  max={max(final_nodes)}")
    print(f"  depth mean={safe_mean(final_depth):.3f}  median={safe_median(final_depth):.3f}  max={max(final_depth)}")
    print("")
    if ratios:
        print("FINAL/GEN0 node ratio (smaller => stronger shrink):")
        print(f"  mean={safe_mean(ratios):.3f}  median={safe_median(ratios):.3f}  min={min(ratios):.6f}  max={max(ratios):.3f}")
    print("")
    print("Feature frequency in FINAL trees (counts = #runs where feature appears, approximate):")
    feat_presence = Counter()
    for ex in final_expr:
        feat_presence.update(set(FEAT_RE.findall(ex)))
    for f, c in feat_presence.most_common():
        print(f"  {f}: {c}/{len(reports)}")

    if args.print_examples:
        print("\nFINAL expressions per run:")
        for r in sorted(reports, key=lambda x: x["run"]):
            print(f"  {r['run']}: {r.get('FINAL_BEST_RES_TREE_expr','')}")

    if args.out_csv:
        out_csv = Path(args.out_csv).resolve()
        out_csv.parent.mkdir(parents=True, exist_ok=True)
        cols = [
            "run",
            "START_RES_TREE_nodes","START_RES_TREE_depth","START_RES_TREE_expr",
            "GEN0_BEST_RES_TREE_nodes","GEN0_BEST_RES_TREE_depth","GEN0_BEST_RES_TREE_expr",
            "FINAL_BEST_RES_TREE_nodes","FINAL_BEST_RES_TREE_depth","FINAL_BEST_RES_TREE_expr",
        ]
        with out_csv.open("w", newline="", encoding="utf-8") as f:
            w = csv.DictWriter(f, fieldnames=cols, extrasaction="ignore")
            w.writeheader()
            for r in sorted(reports, key=lambda x: x["run"]):
                w.writerow(r)
        print(f"\n[ok] CSV saved: {out_csv}")

if __name__ == "__main__":
    main()