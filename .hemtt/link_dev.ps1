<#
.SYNOPSIS
    Makes <Arma 3>\x\kh a junction that points at this repo's .hemttout\dev, so the
    game (and `hemtt launch -Q`, which requires that link) always sees the latest
    dev build without copying anything.

.DESCRIPTION
    Arma 3 is located the same way HEMTT does it:
      1. KH_ARMA3_DIR environment variable (override)
      2. Steam: HKCU\Software\Valve\Steam\SteamPath (or HKLM InstallPath), then every
         library in steamapps\libraryfolders.vdf is checked for appmanifest_107410.acf
      3. HKLM\SOFTWARE\WOW6432Node\Bohemia Interactive\Arma 3\main
    The link name comes from .hemtt\project.toml (mainprefix\prefix = x\kh).

    If the link already exists and points at the dev folder nothing is done. If it
    points elsewhere it is replaced. If x\kh is a REAL folder (not a link) it is left
    alone and the script tells you to move it - it never deletes your files.
    Junctions need no admin rights.

.PARAMETER Remove
    Remove the link instead of creating it.
#>
[CmdletBinding()]
param(
    [switch] $Remove,
    [string] $Arma3Dir = ""
)
Set-StrictMode -Version 2
$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot  = (Resolve-Path (Join-Path $ScriptDir "..")).Path
$DevDir    = Join-Path $RepoRoot ".hemttout\dev"

function Read-Reg([string]$path, [string]$name) {
    try { return [string](Get-ItemProperty -Path $path -Name $name -ErrorAction Stop).$name } catch { return "" }
}
function Test-Arma3([string]$dir) {
    return ($dir -and (Test-Path -LiteralPath (Join-Path $dir "arma3_x64.exe")))
}

# --- 1. find Arma 3 -----------------------------------------------------------------
function Find-Arma3 {
    if ($env:KH_ARMA3_DIR) {
        if (Test-Arma3 $env:KH_ARMA3_DIR) { return (Resolve-Path -LiteralPath $env:KH_ARMA3_DIR).Path }
        Write-Host "WARNING: KH_ARMA3_DIR is set but '$($env:KH_ARMA3_DIR)' has no arma3_x64.exe - ignoring it." -ForegroundColor Yellow
    }

    # Steam root
    $steamRoots = @()
    foreach ($p in @((Read-Reg "HKCU:\Software\Valve\Steam" "SteamPath"),
                     (Read-Reg "HKLM:\SOFTWARE\WOW6432Node\Valve\Steam" "InstallPath"),
                     (Read-Reg "HKLM:\SOFTWARE\Valve\Steam" "InstallPath"),
                     $(if ([Environment]::GetEnvironmentVariable("ProgramFiles(x86)")) { Join-Path ([Environment]::GetEnvironmentVariable("ProgramFiles(x86)")) "Steam" } else { "" }))) {
        if ($p) { $p = $p -replace '/', '\'; if ((Test-Path -LiteralPath $p) -and ($steamRoots -notcontains $p)) { $steamRoots += $p } }
    }

    # every Steam library, as listed in libraryfolders.vdf (plus the root itself)
    $libraries = @()
    foreach ($root in $steamRoots) {
        $libraries += $root
        $vdf = Join-Path $root "steamapps\libraryfolders.vdf"
        if (Test-Path -LiteralPath $vdf) {
            foreach ($m in [regex]::Matches((Get-Content -LiteralPath $vdf -Raw), '"path"\s+"([^"]+)"')) {
                $lib = $m.Groups[1].Value -replace '\\\\', '\'
                if ((Test-Path -LiteralPath $lib) -and ($libraries -notcontains $lib)) { $libraries += $lib }
            }
        }
    }
    foreach ($lib in $libraries) {
        $acf = Join-Path $lib "steamapps\appmanifest_107410.acf"
        if (-not (Test-Path -LiteralPath $acf)) { continue }
        $m = [regex]::Match((Get-Content -LiteralPath $acf -Raw), '"installdir"\s+"([^"]+)"')
        $installDir = if ($m.Success) { $m.Groups[1].Value } else { "Arma 3" }
        $dir = Join-Path $lib ("steamapps\common\" + $installDir)
        if (Test-Arma3 $dir) { return (Resolve-Path -LiteralPath $dir).Path }
    }

    # Bohemia's own registry key (written by the game / launcher)
    foreach ($k in @("HKLM:\SOFTWARE\WOW6432Node\Bohemia Interactive\Arma 3", "HKLM:\SOFTWARE\Bohemia Interactive\Arma 3")) {
        $dir = Read-Reg $k "main"
        if (Test-Arma3 $dir) { return (Resolve-Path -LiteralPath $dir).Path }
    }
    return $null
}

# --- 2. link name from project.toml -------------------------------------------------------
$mainprefix = "x"; $prefix = "kh"
$toml = Join-Path $ScriptDir "project.toml"
if (Test-Path -LiteralPath $toml) {
    $t = Get-Content -LiteralPath $toml -Raw
    $m = [regex]::Match($t, '(?m)^\s*mainprefix\s*=\s*"([^"]+)"'); if ($m.Success) { $mainprefix = $m.Groups[1].Value }
    $m = [regex]::Match($t, '(?m)^\s*prefix\s*=\s*"([^"]+)"');     if ($m.Success) { $prefix = $m.Groups[1].Value }
}

$arma = if ($Arma3Dir) { $Arma3Dir } else { Find-Arma3 }
if (-not $arma) {
    Write-Host "ERROR: Arma 3 was not found (checked Steam's libraries and the Bohemia Interactive registry key)." -ForegroundColor Red
    Write-Host "       Set the environment variable KH_ARMA3_DIR to the folder that contains arma3_x64.exe and run again." -ForegroundColor Red
    exit 1
}
$prefixDir = Join-Path $arma $mainprefix
$link      = Join-Path $prefixDir $prefix
Write-Host "Arma 3  : $arma"
Write-Host "link    : $link"
Write-Host "target  : $DevDir"

# --- 3. inspect what is there -------------------------------------------------------------
$item = $null
if (Test-Path -LiteralPath $link) { $item = Get-Item -LiteralPath $link -Force }
$isLink = ($null -ne $item) -and (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0)

function Remove-Link([string]$path) {
    # rmdir without /s removes ONLY the junction, never what it points at.
    # (Remove-Item on a junction can follow it into the target - do not use it here.)
    $out = cmd /c rmdir "$path" 2>&1
    if (Test-Path -LiteralPath $path) { throw "could not remove the existing link '$path': $out" }
}

if ($Remove) {
    if ($null -eq $item) { Write-Host "Nothing to remove - the link does not exist." -ForegroundColor Green; exit 0 }
    if (-not $isLink) { Write-Host "ERROR: '$link' is a real folder, not a link - not touching it." -ForegroundColor Red; exit 1 }
    Remove-Link $link
    Write-Host "Link removed." -ForegroundColor Green
    exit 0
}

if (-not (Test-Path -LiteralPath $DevDir)) {
    New-Item -ItemType Directory -Path $DevDir -Force | Out-Null
    Write-Host "Created '$DevDir' (it did not exist yet)." -ForegroundColor Yellow
}
$DevDir = (Resolve-Path -LiteralPath $DevDir).Path
if (-not (Test-Path -LiteralPath (Join-Path $DevDir "addons"))) {
    Write-Host "WARNING: '$DevDir' has no addons folder - nothing has been built yet. Run build_all.bat (or .hemtt\build.bat) or the mod will load without its PBOs." -ForegroundColor Yellow
}

if ($null -ne $item -and -not $isLink) {
    Write-Host "ERROR: '$link' already exists as a REAL folder (not a link), so it would shadow the dev build." -ForegroundColor Red
    Write-Host "       It was not touched. Move or delete that folder yourself, then run this again." -ForegroundColor Red
    exit 1
}

if ($isLink) {
    $target = ""
    try { $target = [string]$item.Target } catch { }
    $target = $target -replace '^\\\\\?\\', ''
    if ($target.TrimEnd('\') -ieq $DevDir.TrimEnd('\')) {
        Write-Host "Link already in place." -ForegroundColor Green
        exit 0
    }
    Write-Host "Link exists but points at '$target' - replacing it." -ForegroundColor Yellow
    Remove-Link $link
}

# --- 4. create ---------------------------------------------------------------------------
if (-not (Test-Path -LiteralPath $prefixDir)) { New-Item -ItemType Directory -Path $prefixDir -Force | Out-Null }
try {
    New-Item -ItemType Junction -Path $link -Target $DevDir -ErrorAction Stop | Out-Null
} catch {
    # fall back to cmd's mklink /J (same thing, different code path)
    $out = cmd /c mklink /J "$link" "$DevDir" 2>&1
    if (-not (Test-Path -LiteralPath $link)) {
        Write-Host "ERROR: could not create the junction: $($_.Exception.Message) / $out" -ForegroundColor Red
        Write-Host "       (Is the Arma 3 folder writable? Junctions need no admin rights, but a protected folder does.)" -ForegroundColor Red
        exit 1
    }
}
Write-Host "Link created: $link -> $DevDir" -ForegroundColor Green
exit 0