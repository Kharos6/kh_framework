<#
.SYNOPSIS
    AUTHOR SIDE. Builds the single dependency zip that dependencies/get_dependencies.ps1
    downloads, from the files present in YOUR working tree, and records its
    sha256 / size in manifest.json.

.DESCRIPTION
    Resolves every "sources" entry of manifest.json ('from': repo-relative path, glob
    with * or dir/**, or absolute path with ${ENV_VAR} expansion; 'to': folder relative
    to the repo root), zips them into dependencies/upload/<file> with paths that mirror
    the repository, and writes sha256, size and a fingerprint of the sources into
    manifest.json.

    If no source file changed since the last run (same fingerprint) and the zip still
    exists in upload/, nothing is rebuilt, so the recorded sha256 stays stable.

    Afterwards: upload dependencies/upload/<file> to your storage, make sure
    manifest.json's url points at it (-Url), commit manifest.json.
    See dependencies/README.md.

.PARAMETER Force
    Rebuild even when the fingerprint says nothing changed.

.PARAMETER Url
    Write this value into manifest.json as the download url.

.PARAMETER Check
    Only report which sources are present / missing; write nothing.

.EXAMPLE
    powershell -NoProfile -ExecutionPolicy Bypass -File dependencies\pack_dependencies.ps1 -Check
    powershell -NoProfile -ExecutionPolicy Bypass -File dependencies\pack_dependencies.ps1 -Url https://github.com/Kharos6/kh_framework/releases/download/deps-2026.10/kh_framework_deps.zip
#>
[CmdletBinding()]
param(
    [switch]   $Force,
    [string]   $Url = "",
    [switch]   $Check,
    [string]   $Manifest = ""
)

Set-StrictMode -Version 2
$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.IO.Compression | Out-Null
Add-Type -AssemblyName System.IO.Compression.FileSystem | Out-Null

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot  = (Resolve-Path (Join-Path $ScriptDir "..")).Path
if (-not $Manifest) { $Manifest = Join-Path $ScriptDir "manifest.json" }
$UploadDir = Join-Path $ScriptDir "upload"
$Sep = [System.IO.Path]::DirectorySeparatorChar

function Get-Prop($obj, [string]$name, $default = $null) {
    if ($null -eq $obj) { return $default }
    $p = $obj.PSObject.Properties[$name]
    if ($null -eq $p -or $null -eq $p.Value) { return $default }
    return $p.Value
}
function Get-Sha256([string]$path) { return (Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant() }
function Sha256-String([string]$s) {
    $sha = [System.Security.Cryptography.SHA256]::Create()
    try { return ([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($s))) -replace '-', '').ToLowerInvariant() } finally { $sha.Dispose() }
}
function Normalize-Rel([string]$p) {
    $p = ($p -replace '\\', '/').Trim()
    while ($p.StartsWith('./')) { $p = $p.Substring(2) }
    $p = $p.TrimEnd('/')
    if ($p -eq '') { $p = '.' }
    return $p
}
function Expand-EnvVars([string]$s) {
    return [regex]::Replace($s, '\$\{([A-Za-z_][A-Za-z0-9_]*)\}', {
        param($m)
        $v = [Environment]::GetEnvironmentVariable($m.Groups[1].Value)
        if ($null -eq $v) { throw "environment variable `${$($m.Groups[1].Value)} is not set (needed by a manifest source)" }
        return $v
    })
}
function Is-Absolute([string]$p) { return ($p -match '^[A-Za-z]:[\\/]' -or $p.StartsWith('/') -or $p.StartsWith('\\')) }

# Resolves one source spec to a list of absolute file paths.
function Resolve-Source([string]$spec) {
    $spec = Expand-EnvVars $spec
    $norm = $spec -replace '\\', '/'
    $recursive = $false
    if ($norm.EndsWith('/**')) { $recursive = $true; $norm = $norm.Substring(0, $norm.Length - 3) }
    $native = $norm -replace '/', $Sep
    $abs = if (Is-Absolute $native) { $native } else { [System.IO.Path]::Combine($RepoRoot, $native) }
    $out = @()
    if ($recursive) {
        if (Test-Path -LiteralPath $abs -PathType Container) {
            $out = @(Get-ChildItem -LiteralPath $abs -File -Recurse | ForEach-Object { $_.FullName })
        }
    } elseif ($native.Contains('*') -or $native.Contains('?')) {
        $out = @(Get-ChildItem -Path $abs -File -ErrorAction SilentlyContinue | ForEach-Object { $_.FullName })
    } elseif (Test-Path -LiteralPath $abs -PathType Leaf) {
        $out = @($abs)
    } elseif (Test-Path -LiteralPath $abs -PathType Container) {
        $out = @(Get-ChildItem -LiteralPath $abs -File -Recurse | ForEach-Object { $_.FullName })
    }
    return ,$out
}

# Path of $file inside the zip: relative to dest when it lives under dest, else just its name.
function Entry-Name([string]$file, [string]$destAbs) {
    $f = $file -replace '\\', '/'
    $d = ($destAbs -replace '\\', '/').TrimEnd('/') + '/'
    if ($f.StartsWith($d, [StringComparison]::OrdinalIgnoreCase)) { return $f.Substring($d.Length) }
    return [System.IO.Path]::GetFileName($file)
}

function New-Package([string]$zipPath, $entries) {
    if (Test-Path -LiteralPath $zipPath) { Remove-Item -LiteralPath $zipPath -Force }
    $zip = [System.IO.Compression.ZipFile]::Open($zipPath, [System.IO.Compression.ZipArchiveMode]::Create)
    try {
        foreach ($e in $entries) {
            [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile($zip, $e.Path, $e.Name, [System.IO.Compression.CompressionLevel]::Optimal) | Out-Null
        }
    } finally { $zip.Dispose() }
}

# --- manifest writer that keeps the file readable and diff-friendly -------------
function J([string]$s) { return ($s | ConvertTo-Json -Compress) }
function JArr($arr, [string]$indent) {
    $arr = @($arr)
    if ($arr.Count -eq 0) { return "[]" }
    $parts = $arr | ForEach-Object { $indent + "  " + (J ([string]$_)) }
    return "[`n" + ($parts -join ",`n") + "`n" + $indent + "]"
}
function Write-Manifest($man, [string]$path) {
    $sb = New-Object System.Text.StringBuilder
    [void]$sb.Append("{`n")
    $comment = Get-Prop $man "_comment" $null
    if ($null -ne $comment) { [void]$sb.Append('  "_comment": ' + (JArr $comment "  ") + ",`n") }
    [void]$sb.Append('  "manifest_version": ' + [int](Get-Prop $man "manifest_version" 2) + ",`n")
    [void]$sb.Append('  "url": '      + (J ([string](Get-Prop $man "url" ""))) + ",`n")
    [void]$sb.Append('  "file": '     + (J ([string](Get-Prop $man "file" "kh_framework_deps.zip"))) + ",`n")
    [void]$sb.Append('  "src_hash": ' + (J ([string](Get-Prop $man "src_hash" ""))) + ",`n")
    [void]$sb.Append('  "sha256": '   + (J ([string](Get-Prop $man "sha256" ""))) + ",`n")
    [void]$sb.Append('  "size": '     + [long](Get-Prop $man "size" 0) + ",`n")
    [void]$sb.Append('  "sources": [')
    $first = $true
    foreach ($src in @(Get-Prop $man "sources" @())) {
        if (-not $first) { [void]$sb.Append(",") }
        $first = $false
        [void]$sb.Append("`n    { `"from`": " + (J ([string]$src.from)) + ", `"to`": " + (J ([string]$src.to)) + " }")
    }
    [void]$sb.Append("`n  ]`n}`n")
    [System.IO.File]::WriteAllText($path, $sb.ToString(), (New-Object System.Text.UTF8Encoding($false)))
}

# --- main -------------------------------------------------------------------
if (-not (Test-Path -LiteralPath $Manifest)) { Write-Host "manifest not found: $Manifest" -ForegroundColor Red; exit 2 }
$man = Get-Content -LiteralPath $Manifest -Raw | ConvertFrom-Json
function Set-Prop($obj, [string]$name, $value) {
    if ($obj.PSObject.Properties[$name]) { $obj.$name = $value } else { $obj | Add-Member -NotePropertyName $name -NotePropertyValue $value }
}
if ($Url) { Set-Prop $man "url" $Url }
$file = [string](Get-Prop $man "file" "kh_framework_deps.zip")

Write-Host ""
Write-Host "KH Framework dependency packer"
Write-Host "  root   : $RepoRoot"
Write-Host "  output : $(Join-Path $UploadDir $file)"
Write-Host ""

# 1. resolve sources -> zip entries (name inside the zip = path relative to the repo root)
$entries = @()
$missing = @()
foreach ($src in @(Get-Prop $man "sources" @())) {
    $from = [string]$src.from
    $toRel = Normalize-Rel ([string]$src.to)
    $toAbs = if ($toRel -eq '.') { $RepoRoot } else { [System.IO.Path]::Combine($RepoRoot, ($toRel -replace '/', $Sep)) }
    try { $r = Resolve-Source $from } catch { $missing += "$from ($($_.Exception.Message))"; continue }
    if ($r.Count -eq 0) { $missing += $from; continue }
    foreach ($f in $r) {
        $inside = Entry-Name $f $toAbs
        $name = if ($toRel -eq '.') { $inside } else { $toRel + '/' + $inside }
        $entries += @{ Path = $f; Name = $name }
    }
}
$entries = @($entries | Sort-Object { $_.Name })
$dupes = @($entries | Group-Object { $_.Name } | Where-Object { $_.Count -gt 1 })
if ($missing.Count -gt 0) {
    Write-Host "Nothing found for these sources:" -ForegroundColor Red
    foreach ($m in $missing) { Write-Host "   $m" -ForegroundColor Red }
    Write-Host ""
    Write-Host "Put the files in place (CUDA: is CUDA_PATH set in this shell?) or remove the entry from manifest.json, then run again. Nothing was written." -ForegroundColor Red
    exit 1
}
if ($dupes.Count -gt 0) {
    Write-Host ("Duplicate zip entries: " + (($dupes | ForEach-Object { $_.Name }) -join ", ")) -ForegroundColor Red
    exit 1
}

# 2. fingerprint of the sources (name + sha256 of every file)
$fpLines = foreach ($e in $entries) { $e.Name + "  " + (Get-Sha256 $e.Path) }
$fp = Sha256-String ($fpLines -join "`n")
$totalSrc = 0; foreach ($e in $entries) { $totalSrc += (Get-Item -LiteralPath $e.Path).Length }
$zipPath = Join-Path $UploadDir $file

foreach ($e in $entries) { Write-Host ("  {0}" -f $e.Name) -ForegroundColor DarkGray }
Write-Host ("  {0} file(s), {1:0.0} MB raw" -f $entries.Count, ($totalSrc / 1MB))
Write-Host ""

if ($Check) {
    if ($fp -eq [string](Get-Prop $man "src_hash" "")) { Write-Host "Sources unchanged since the last pack." -ForegroundColor Green }
    else { Write-Host "Sources CHANGED since the last pack (or never packed) - run without -Check to rebuild." -ForegroundColor Yellow }
    exit 0
}

$recordedSha = [string](Get-Prop $man "sha256" "")
if (-not $Force -and (Test-Path -LiteralPath $zipPath) -and $fp -eq [string](Get-Prop $man "src_hash" "") -and $recordedSha -and (Get-Sha256 $zipPath) -eq $recordedSha) {
    Write-Host "Sources unchanged - $file left as is (use -Force to rebuild)." -ForegroundColor DarkGray
} else {
    if (-not (Test-Path -LiteralPath $UploadDir)) { New-Item -ItemType Directory -Path $UploadDir -Force | Out-Null }
    Write-Host "Packing $file ..."
    New-Package $zipPath $entries
    Set-Prop $man "src_hash" $fp
    Set-Prop $man "sha256" (Get-Sha256 $zipPath)
    Set-Prop $man "size" ((Get-Item -LiteralPath $zipPath).Length)
    Write-Host ("Packed: {0:0.0} MB" -f ((Get-Item -LiteralPath $zipPath).Length / 1MB)) -ForegroundColor Green
}

Write-Manifest $man $Manifest
Write-Host ""
Write-Host "manifest.json updated."
Write-Host "  upload : $zipPath"
Write-Host "  url    : $([string](Get-Prop $man 'url' '(none - set it with -Url)'))"
Write-Host "Then commit dependencies/manifest.json (never the upload/ folder)."
exit 0
