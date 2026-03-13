# =========================
# run_5methods_10runs_plot.ps1
# =========================
param(
  [string]$repo  = "C:\Users\awesd\Source\Repos\iMOPSE",
  [int]$runs     = 10,
  [int]$seed0    = 100,
  [string]$problem = "MSRCPSP_TA2",
  [string]$instDefRel = "configurations\problems\MSRCPSP\d36\200_10_50_9.def",

  # allow forcing exact exe paths (recommended)
  [string]$imopseExeOverride = "",
  [string]$paretoExeOverride = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# =========================
# 0) SETTINGS
# =========================
$instDef  = Join-Path $repo $instDefRel
$instName = [IO.Path]::GetFileNameWithoutExtension($instDef)
if (!(Test-Path $instDef)) { throw "Missing instance: $instDef" }

# =========================
# 1) FIND EXE (prefer Release) OR use override
# =========================
function Find-Exe($root, $name) {
  $all = Get-ChildItem -Path $root -Recurse -Filter $name -ErrorAction SilentlyContinue
  if (!$all) { return $null }
  $rel = $all | Where-Object { $_.FullName -match '\\Release\\' } |
         Sort-Object LastWriteTime -Descending | Select-Object -First 1
  if ($rel) { return $rel.FullName }
  return ($all | Sort-Object LastWriteTime -Descending | Select-Object -First 1).FullName
}

# --- imopse.exe ---
if ($imopseExeOverride -and (Test-Path $imopseExeOverride)) {
  $imopseExe = $imopseExeOverride
} else {
  $imopseExe = Find-Exe (Join-Path $repo "build") "imopse.exe"
  if (!(Test-Path $imopseExe)) { $imopseExe = Find-Exe (Join-Path $repo "optimizer") "imopse.exe" }
}
if (!(Test-Path $imopseExe)) { throw "imopse.exe not found under $repo (and no valid -imopseExeOverride)" }

# --- paretoAnalyzer.exe ---
if ($paretoExeOverride -and (Test-Path $paretoExeOverride)) {
  $paretoExe = $paretoExeOverride
} else {
  $paretoExe = Find-Exe (Join-Path $repo "build") "paretoAnalyzer.exe"
  if (!(Test-Path $paretoExe)) { $paretoExe = Find-Exe (Join-Path $repo "paretoAnalyzer") "paretoAnalyzer.exe" }
}
if (!(Test-Path $paretoExe)) { throw "paretoAnalyzer.exe not found under $repo (and no valid -paretoExeOverride)" }

Write-Host "imopseExe: $imopseExe"
Write-Host "paretoExe: $paretoExe"

# show stamp + hash so we know exactly which binary ran
$im = Get-Item $imopseExe
$ph = (Get-FileHash $imopseExe -Algorithm SHA256).Hash.Substring(0,16)
Write-Host ("imopse.exe stamp: {0} | size: {1} | sha256: {2}..." -f $im.LastWriteTime, $im.Length, $ph)

Write-Host "inst: $instName | runs: $runs | seed0: $seed0 | problem: $problem"

# =========================
# 2) Ensure python plotting scripts
# =========================
$scriptsDir = Join-Path $repo "scripts"
New-Item -ItemType Directory -Force -Path $scriptsDir | Out-Null

# --- HV multi plot (mean +/- std) ---
$hvMulti = Join-Path $scriptsDir "plot_hv_multi.py"
@"
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
"@ | Set-Content -Path $hvMulti -Encoding UTF8

# --- Pareto plot from *_merged.csv ---
$paretoPlot = Join-Path $scriptsDir "plot_pareto_merged.py"
@"
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
"@ | Set-Content -Path $paretoPlot -Encoding UTF8

# =========================
# 3) Prepare GPHH cfg variant (BNTGA mode)
# =========================
$gphhBase  = Join-Path $repo "configurations\methods\GPHH\GPHH_MSRCPSP.cfg"
$gphhBntga = Join-Path $repo "configurations\methods\GPHH\GPHH_MSRCPSP_BNTGA.cfg"
if (!(Test-Path $gphhBase)) { throw "Missing GPHH cfg: $gphhBase" }

$text = Get-Content $gphhBase -Raw
if ($text -match '(?im)^\s*UseNSGA2\s+\S+') { $text = [regex]::Replace($text,'(?im)^\s*UseNSGA2\s+\S+.*$','UseNSGA2 0') } else { $text += "`r`nUseNSGA2 0" }
if ($text -match '(?im)^\s*UseBNTGA\s+\S+') { $text = [regex]::Replace($text,'(?im)^\s*UseBNTGA\s+\S+.*$','UseBNTGA 1') } else { $text += "`r`nUseBNTGA 1" }

$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($gphhBntga, $text, $utf8NoBom)
Write-Host "GPHH BNTGA cfg: $gphhBntga"

# =========================
# 4) Configs for 5 methods
# =========================
$methodCfg = [ordered]@{
  "GPHH"   = $gphhBntga
  "NSGAII" = (Join-Path $repo "configurations\methods\NSGAII\NSGAII_MSRCPSP.cfg")
  "MOEAD"  = (Join-Path $repo "configurations\methods\MOEAD\MOEAD_MSRCPSP.cfg")
  "SPEA2"  = (Join-Path $repo "configurations\methods\SPEA2\SPEA2_MSRCPSP.cfg")
  "BNTGA"  = (Join-Path $repo "configurations\methods\BNTGA\BNTGA_MSRCPSP.cfg")
}

if (!(Test-Path $methodCfg["NSGAII"])) {
  $alt = Join-Path $repo "configurations\methods\NSGAII\NSGAII_MSRCPSP_compare.cfg"
  if (Test-Path $alt) { $methodCfg["NSGAII"] = $alt }
}

# =========================
# 5) Results folders
# =========================
$resultsRoot = Join-Path $repo "results"
$experiments = Join-Path $resultsRoot "experiments"
$paretoOut   = Join-Path $resultsRoot "paretoAnalyzer"
$plotsOut    = Join-Path $resultsRoot ("plots\" + $instName)

New-Item -ItemType Directory -Force -Path $experiments | Out-Null
New-Item -ItemType Directory -Force -Path $paretoOut   | Out-Null
New-Item -ItemType Directory -Force -Path $plotsOut    | Out-Null

function Has-ResultsCsv($outDir) {
  $a = Join-Path $outDir "run_0\results.csv"
  $b = Join-Path $outDir "run_0\results\results.csv"
  return (Test-Path $a) -or (Test-Path $b)
}
function Has-HVHistory($outDir) {
  $a = Join-Path $outDir "run_0\hv_history.csv"
  $b = Join-Path $outDir "run_0\results\hv_history.csv"
  return (Test-Path $a) -or (Test-Path $b)
}

# =========================
# 6) Run each method (10 runs)
# =========================
$ranMethodsForMerge = @()
$ranMethodsForHV    = @()

foreach ($m in $methodCfg.Keys) {
  $cfg = $methodCfg[$m]
  if (!(Test-Path $cfg)) { Write-Host "[skip] $m cfg not found: $cfg"; continue }

  $methodRoot = Join-Path $experiments $m
  $outDir     = Join-Path $methodRoot $instName

  New-Item -ItemType Directory -Force -Path $methodRoot | Out-Null
  if (Test-Path $outDir) { Remove-Item -Recurse -Force $outDir }
  New-Item -ItemType Directory -Force -Path $outDir | Out-Null

  Write-Host ("`n=== RUN {0}: {1} runs ===" -f $m, $runs)
  & $imopseExe $cfg $problem $instDef $outDir $runs $seed0
  Write-Host ("exit code {0}" -f $LASTEXITCODE)

  Write-Host "[check] diag files (search res_rule_diag.txt, res_tree_report.txt) ..."
  $diag1 = Get-ChildItem -Path $outDir -Recurse -Filter "res_rule_diag.txt" -ErrorAction SilentlyContinue | Select-Object -First 1
  if ($diag1) { Write-Host "  found: $($diag1.FullName)" } else { Write-Host "  NOT FOUND: res_rule_diag.txt" }

  $diag2 = Get-ChildItem -Path $outDir -Recurse -Filter "res_tree_report.txt" -ErrorAction SilentlyContinue | Select-Object -First 1
  if ($diag2) { Write-Host "  found: $($diag2.FullName)" } else { Write-Host "  NOT FOUND: res_tree_report.txt" }

  if (Has-ResultsCsv $outDir) { $ranMethodsForMerge += $m } else { Write-Host ("[warn] {0}: results.csv not found -> skip merge" -f $m) }
  if (Has-HVHistory $outDir)  { $ranMethodsForHV += $m }   else { Write-Host ("[info] {0}: hv_history.csv not found -> skip HV plot" -f $m) }
}

# =========================
# 7) Merge per method using paretoAnalyzer
# =========================
$mergedFiles = @{}

foreach ($m in $ranMethodsForMerge) {
  $methodRoot = Join-Path $experiments $m
  $cfgList = Join-Path $resultsRoot ("merge_" + $m.ToLower() + ".cfg")
  Set-Content -Path $cfgList -Value $methodRoot -Encoding ASCII

  Write-Host ("`n=== MERGE {0} ===" -f $m)
  & $paretoExe $cfgList $instName $paretoOut

  $mf = Join-Path $paretoOut ("{0}_merged.csv" -f $m)
  if (Test-Path $mf) { $mergedFiles[$m] = $mf }
}

Write-Host "`nMerged CSVs:"
Get-ChildItem $paretoOut -Filter "*_merged.csv" | Select-Object Name, FullName

# =========================
# 8) Plot Pareto: GPHH only
# =========================
if ($mergedFiles.ContainsKey("GPHH")) {
  $outPng = Join-Path $plotsOut "pareto_GPHH.png"
  Write-Host "`n=== Plot: Pareto Front (GPHH) ==="
  Push-Location $scriptsDir
  py .\plot_pareto_merged.py --merged_dir "$paretoOut" --methods GPHH --title "Pareto Front: GPHH (inst=$instName)" --out "$outPng"
  Pop-Location
  Write-Host "[ok] saved: $outPng"
} else {
  Write-Host "[warn] Missing merged file for GPHH -> no GPHH plot"
}

# =========================
# 9) Plot Pareto: all 5 methods (overlay)
# =========================
$allMethods = @("GPHH","NSGAII","MOEAD","SPEA2","BNTGA")
$methodsWithMerged = @($allMethods | Where-Object { $mergedFiles.ContainsKey($_) })

if ($methodsWithMerged.Count -gt 0) {
  $outPngAll = Join-Path $plotsOut "pareto_ALL_5.png"
  Write-Host "`n=== Plot: Pareto Front (ALL methods) ==="
  Push-Location $scriptsDir
  py .\plot_pareto_merged.py --merged_dir "$paretoOut" --methods @($methodsWithMerged) `
    --title "Pareto Front: ALL (inst=$instName)" --out "$outPngAll"
  Pop-Location
  Write-Host "[ok] saved: $outPngAll"
} else {
  Write-Host "[warn] No merged files found -> no ALL Pareto plot"
}

# =========================
# 10) HV plot: all methods on one chart
# =========================
$methodsWithHV = @($allMethods | Where-Object { $ranMethodsForHV -contains $_ })

if ($methodsWithHV.Count -gt 0) {
  $outHvPng = Join-Path $plotsOut "hv_ALL_5.png"
  Write-Host "`n=== Plot: HV vs Generation (ALL methods) ==="
  Push-Location $scriptsDir
  py .\plot_hv_multi.py --repo "$repo" --inst "$instName" --runs $runs --methods @($methodsWithHV) --out "$outHvPng"
  Pop-Location
  Write-Host "[ok] saved: $outHvPng"
} else {
  Write-Host "[warn] No hv_history.csv files found -> no HV plot"
}

Write-Host "`nOK."
Write-Host "Results root: $resultsRoot"
Write-Host "Merged CSVs:  $paretoOut"
Write-Host "Plots:        $plotsOut"