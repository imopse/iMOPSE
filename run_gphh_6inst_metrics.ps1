param(
    [string]$Repo        = "C:\Users\awesd\Source\Repos\iMOPSE",
    [string]$Cfg         = "",
    [string]$Problem     = "MSRCPSP_TA2",
    [int]$Runs           = 1,
    [int]$Seed0          = 100,
    [string]$BntgaZip    = "",
    [switch]$RebuildBntgaStage,
    [switch]$PrepareCompareResultsLayout
)

$ErrorActionPreference = "Stop"

function Find-Exe($root, $name) {
    if (!(Test-Path $root)) { return $null }

    $all = Get-ChildItem -Path $root -Recurse -Filter $name -ErrorAction SilentlyContinue
    if (!$all) { return $null }

    $preferred = $all | Where-Object { $_.FullName -match '\\Release\\' } | Select-Object -First 1
    if ($preferred) { return $preferred.FullName }

    return ($all | Select-Object -First 1).FullName
}

function Get-CfgValue([string]$text, [string]$key) {
    $m = [regex]::Match($text, "(?im)^\s*$([regex]::Escape($key))\s+([^\s#;]+)")
    if ($m.Success) { return $m.Groups[1].Value }
    return $null
}

function Parse-ParetoAnalyzerLine([string]$line) {
    if ([string]::IsNullOrWhiteSpace($line)) { return $null }
    if ($line -notmatch '^[^;]+;') { return $null }

    $parts = $line -split ';'
    if ($parts.Count -lt 2) { return $null }

    $obj = [ordered]@{}
    $obj["Method"] = $parts[0].Trim()

    for ($i = 1; $i -lt $parts.Count; $i++) {
        $p = $parts[$i].Trim()
        if ([string]::IsNullOrWhiteSpace($p)) { continue }

        $kv = $p -split ':', 2
        if ($kv.Count -ne 2) { continue }

        $k = $kv[0].Trim()
        $v = $kv[1].Trim()
        $obj[$k] = $v
    }

    return [pscustomobject]$obj
}

function Ensure-Directory([string]$path) {
    New-Item -ItemType Directory -Force -Path $path | Out-Null
}

function Convert-BntgaToParetoFormat {
    param(
        [string]$ZipPath,
        [string]$StageRoot,
        [switch]$ForceRebuild
    )

    $finalRoot = Join-Path $StageRoot "BNTGA"

    if ((Test-Path $finalRoot) -and -not $ForceRebuild) {
        Write-Host "[OK] BNTGA staged already exists:"
        Write-Host "     $finalRoot"
        return $finalRoot
    }

    if (!(Test-Path $ZipPath)) {
        throw "BNTGA zip not found: $ZipPath"
    }

    if (Test-Path $StageRoot) {
        Remove-Item -Recurse -Force $StageRoot
    }
    Ensure-Directory $StageRoot

    $unzRoot = Join-Path $StageRoot "_unzipped"
    Ensure-Directory $unzRoot
    Write-Host "[INFO] Expanding BNTGA zip..."
    Expand-Archive -Path $ZipPath -DestinationPath $unzRoot -Force

    Ensure-Directory $finalRoot

    $instanceMarkers = Get-ChildItem -Path $unzRoot -Recurse -Filter "merged_nd_front.json" -File
    if (!$instanceMarkers) {
        throw "Could not find merged_nd_front.json inside $ZipPath"
    }

    foreach ($marker in $instanceMarkers) {
        $instanceDir = $marker.Directory.FullName
        $instanceName = Split-Path $instanceDir -Leaf
        $dstInst = Join-Path $finalRoot $instanceName
        Ensure-Directory $dstInst

        $runDirs = Get-ChildItem -Path $instanceDir -Directory | Where-Object { $_.Name -match '^run_\d+$' }
        foreach ($rd in $runDirs) {
            $src = Join-Path $rd.FullName "results.json"
            if (!(Test-Path $src)) { continue }

            $dstRun = Join-Path $dstInst $rd.Name
            Ensure-Directory $dstRun

            $dst = Join-Path $dstRun "results.csv"
            Copy-Item -Force $src $dst
        }
    }

    Write-Host "[OK] BNTGA staged for paretoAnalyzer:"
    Write-Host "     $finalRoot"

    return $finalRoot
}

function Remove-ExistingLinkOrDir([string]$path) {
    if (Test-Path $path) {
        Remove-Item -Recurse -Force $path
    }
}

function Create-Junction([string]$linkPath, [string]$targetPath) {
    if (!(Test-Path $targetPath)) {
        throw "Junction target does not exist: $targetPath"
    }

    $parent = Split-Path $linkPath -Parent
    Ensure-Directory $parent
    Remove-ExistingLinkOrDir $linkPath

    $cmd = 'mklink /J "{0}" "{1}"' -f $linkPath, $targetPath
    cmd /c $cmd | Out-Null

    if (!(Test-Path $linkPath)) {
        throw "Failed to create junction: $linkPath -> $targetPath"
    }
}


if ([string]::IsNullOrWhiteSpace($Cfg)) {
    $Cfg = Join-Path $Repo "configurations\methods\GPHH\GPHH_MSRCPSP.cfg"
}
if (!(Test-Path $Cfg)) {
    throw "Missing GPHH cfg: $Cfg"
}

$cfgText = Get-Content $Cfg -Raw
$pop = Get-CfgValue $cfgText "PopulationSize"
$gen = Get-CfgValue $cfgText "Generations"

if (-not $pop) { throw "PopulationSize not found in $Cfg" }
if (-not $gen) { throw "Generations not found in $Cfg" }

[int]$popInt = $pop
[int]$genInt = $gen
[int]$ffe = $popInt * $genInt

Write-Host "[OK] Using existing cfg:"
Write-Host "     $Cfg"
Write-Host ("     PopulationSize={0}, Generations={1}, FFE={2}" -f $popInt, $genInt, $ffe)


$imopseExe = Find-Exe (Join-Path $Repo "build") "imopse.exe"
if (!(Test-Path $imopseExe)) {
    $imopseExe = Find-Exe (Join-Path $Repo "optimizer") "imopse.exe"
}
if (!(Test-Path $imopseExe)) {
    throw "imopse.exe not found under $Repo"
}

$paretoExe = Find-Exe (Join-Path $Repo "build") "paretoAnalyzer.exe"
if (!(Test-Path $paretoExe)) {
    $paretoExe = Find-Exe (Join-Path $Repo "paretoAnalyzer") "paretoAnalyzer.exe"
}
if (!(Test-Path $paretoExe)) {
    throw "paretoAnalyzer.exe not found under $Repo"
}

Write-Host "[OK] imopse.exe:"
Write-Host "     $imopseExe"
Write-Host "[OK] paretoAnalyzer.exe:"
Write-Host "     $paretoExe"


if ([string]::IsNullOrWhiteSpace($BntgaZip)) {
    $BntgaZip = Join-Path $Repo "bntgai.zip"
}
if (!(Test-Path $BntgaZip)) {
    throw "BNTGA zip not found. Pass it with -BntgaZip <path>"
}


$instances = @(
    @{ Name = "100_20_0_0";      Def = "configurations\problems\MSRCPSP\NoConstr\100_20_0_0.def" },
    @{ Name = "100_20_22_15";    Def = "configurations\problems\MSRCPSP\d36\100_20_22_15.def" },
    @{ Name = "100_5_48_9";      Def = "configurations\problems\MSRCPSP\d36\100_5_48_9.def" },
    @{ Name = "200_10_50_15";    Def = "configurations\problems\MSRCPSP\d36\200_10_50_15.def" },
    @{ Name = "200_40_130_9_D4"; Def = "configurations\problems\MSRCPSP\d36\200_40_130_9_D4.def" },
    @{ Name = "200_20_0_0";      Def = "configurations\problems\MSRCPSP\NoConstr\200_20_0_0.def" }
)


$resultsRoot = Join-Path $Repo "results\gphh_6inst_metrics"
$tag = "pop${popInt}_gen${genInt}"
$tagRoot = Join-Path $resultsRoot $tag
$gphhRoot = Join-Path $tagRoot "GPHH"
$paretoOutRoot = Join-Path $tagRoot "paretoAnalyzer"
$bntgaStageRoot = Join-Path $resultsRoot "_bntga_stage"

Ensure-Directory $resultsRoot
Ensure-Directory $tagRoot
Ensure-Directory $gphhRoot
Ensure-Directory $paretoOutRoot


$bntgaRoot = Convert-BntgaToParetoFormat -ZipPath $BntgaZip -StageRoot $bntgaStageRoot -ForceRebuild:$RebuildBntgaStage


$runtimeCsv = Join-Path $tagRoot "runtime_summary.csv"
$metricsCsv = Join-Path $tagRoot "metrics_by_method.csv"
$compareCsv = Join-Path $tagRoot "metrics_compare_gphh_vs_bntga.csv"
$compareCfg = Join-Path $tagRoot "compare_methods.cfg"

"instance,pop,gen,ffe,seed,exit_code,seconds,gphh_dir" | Set-Content -Path $runtimeCsv -Encoding ASCII
"instance,method,runs,mpfs,mnd,hv,hv_std,gd,gd_std,igd,igd_std,pfs,pfs_std,nd,nd_std,nd_tpfs,nd_tpfs_std" | Set-Content -Path $metricsCsv -Encoding ASCII
"instance,pop,gen,ffe,seed,gphh_seconds,gphh_hv,bntga_hv,gphh_igd,bntga_igd,gphh_nd_tpfs,bntga_nd_tpfs" | Set-Content -Path $compareCsv -Encoding ASCII

@(
    $gphhRoot
    $bntgaRoot
) | Set-Content -Path $compareCfg -Encoding ASCII

Write-Host "[OK] compare cfg for paretoAnalyzer:"
Write-Host "     $compareCfg"

foreach ($inst in $instances) {
    $instName = $inst.Name
    $instDef  = Join-Path $Repo $inst.Def

    if (!(Test-Path $instDef)) {
        Write-Host ""
        Write-Host ("[SKIP] Missing instance: {0}" -f $instDef)
        continue
    }

    $outDir = Join-Path $gphhRoot $instName
    if (Test-Path $outDir) {
        Remove-Item -Recurse -Force $outDir
    }
    Ensure-Directory $outDir

    Write-Host ""
    Write-Host ("==================== {0} ====================" -f $instName)
    Write-Host ("Instance: {0}" -f $instDef)
    Write-Host ("Output  : {0}" -f $outDir)

    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    & $imopseExe $Cfg $Problem $instDef $outDir $Runs $Seed0
    $exitCode = $LASTEXITCODE
    $sw.Stop()

    $secs = [math]::Round($sw.Elapsed.TotalSeconds, 3)
    Write-Host ("ExitCode: {0}" -f $exitCode)
    Write-Host ("Time [s]: {0}" -f $secs)

    "{0},{1},{2},{3},{4},{5},{6},""{7}""" -f `
        $instName, $popInt, $genInt, $ffe, $Seed0, $exitCode, $secs, $outDir `
        | Add-Content -Path $runtimeCsv -Encoding ASCII

    $gphhRun0 = Join-Path $outDir "run_0\results.csv"
    if (!(Test-Path $gphhRun0)) {
        Write-Host ("[WARN] Missing GPHH results.csv for {0}, skip metrics." -f $instName)
        continue
    }

    $bntgaRun0 = Join-Path $bntgaRoot (Join-Path $instName "run_0\results.csv")
    if (!(Test-Path $bntgaRun0)) {
        Write-Host ("[WARN] Missing staged BNTGA results for {0}, skip metrics against BNTGA." -f $instName)
        continue
    }

    $instParetoOut = Join-Path $paretoOutRoot $instName
    Ensure-Directory $instParetoOut

    Write-Host ("[INFO] Calculating HV / IGD / ND-TPFS for {0}" -f $instName)

    $paLines = & $paretoExe $compareCfg $instName $instParetoOut 2>&1
    $paLines | ForEach-Object { Write-Host $_ }

    $parsed = @()
    foreach ($line in $paLines) {
        $obj = Parse-ParetoAnalyzerLine $line
        if ($null -ne $obj) {
            $parsed += $obj
        }
    }

    $gphhMetrics  = $parsed | Where-Object { $_.Method -eq "GPHH" }  | Select-Object -First 1
    $bntgaMetrics = $parsed | Where-Object { $_.Method -eq "BNTGA" } | Select-Object -First 1

    foreach ($m in @($gphhMetrics, $bntgaMetrics)) {
        if ($null -eq $m) { continue }

        "{0},{1},{2},{3},{4},{5},{6},{7},{8},{9},{10},{11},{12},{13},{14},{15},{16}" -f `
            $instName, `
            $m.Method, `
            $m.runs, `
            $m.MPFS, `
            $m.MND, `
            $m.HV, `
            $m.'HV Std', `
            $m.GD, `
            $m.'GD Std', `
            $m.IGD, `
            $m.'IGD Std', `
            $m.PFS, `
            $m.'PFS Std', `
            $m.ND, `
            $m.'ND Std', `
            $m.'ND/TPFS', `
            $m.'ND/TPFS Std' `
            | Add-Content -Path $metricsCsv -Encoding ASCII
    }

    if (($null -ne $gphhMetrics) -and ($null -ne $bntgaMetrics)) {
        "{0},{1},{2},{3},{4},{5},{6},{7},{8},{9},{10},{11}" -f `
            $instName, `
            $popInt, `
            $genInt, `
            $ffe, `
            $Seed0, `
            $secs, `
            $gphhMetrics.HV, `
            $bntgaMetrics.HV, `
            $gphhMetrics.IGD, `
            $bntgaMetrics.IGD, `
            $gphhMetrics.'ND/TPFS', `
            $bntgaMetrics.'ND/TPFS' `
            | Add-Content -Path $compareCsv -Encoding ASCII
    }
}

if ($PrepareCompareResultsLayout) {
    Write-Host ""
    Write-Host "[INFO] Preparing experiments layout for compareResults.py ..."

    $expRoot = Join-Path $Repo "experiments"
    $expBntga = Join-Path $expRoot "bntgai\BNTGA_MSRCPSP\MSRCPSP_TA"
    $expOther = Join-Path $expRoot "other"

    Ensure-Directory (Join-Path $expRoot "bntgai\BNTGA_MSRCPSP")
    Ensure-Directory $expRoot

    Create-Junction -linkPath $expBntga -targetPath $bntgaRoot
    Create-Junction -linkPath $expOther -targetPath $gphhRoot

    Write-Host "[OK] compareResults.py layout ready:"
    Write-Host ("     baseline: {0}" -f $expBntga)
    Write-Host ("     other   : {0}" -f $expOther)
    Write-Host ""
    Write-Host "Now run:"
    Write-Host ("     cd {0}" -f $Repo)
    Write-Host "     py .\compareResults.py"
}

Write-Host ""
Write-Host "==================== DONE ===================="
Write-Host ("Runtime summary  : {0}" -f $runtimeCsv)
Write-Host ("Metrics by method: {0}" -f $metricsCsv)
Write-Host ("Compare summary  : {0}" -f $compareCsv)
Write-Host ("Pareto outputs   : {0}" -f $paretoOutRoot)