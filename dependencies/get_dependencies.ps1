<#
.SYNOPSIS
    Downloads the binary dependencies of KH Framework (prebuilt .lib / .dll files,
    hemtt.exe, Ultralight resources, ...) as one zip described by
    dependencies/manifest.json and extracts it over the repository root.

.DESCRIPTION
    Runs on stock Windows PowerShell 5.1 (every Windows 10/11) and on PowerShell 7.
    Nothing needs to be installed. Every build.bat in extensions/ and every .bat in
    .hemtt/ calls this script first, so a fresh clone becomes buildable with one
    double-click.

    The zip can be served from any plain https:// host (GitHub Releases is the
    recommended one) or from MEGA (file or folder links; MEGA's client-side
    decryption is implemented here, no MEGA software is required).

    The download is verified against the sha256 in the manifest, kept in
    dependencies/.cache/, and extracted. A state file in dependencies/.state/
    records what was installed, so a second run is instant, and a wiped
    destination (for example .hemttout/dev after `hemtt dev`) is restored from
    the cache without downloading again.

.PARAMETER Force
    Re-download and re-extract even if everything is up to date.

.PARAMETER Verify
    Re-hash the cached zip and re-extract if anything is missing or changed.

.PARAMETER Profile
    HEMTT output profile for the runtime files: zip entries under ".hemttout/dev/"
    are placed under ".hemttout/<Profile>/" instead. Default: dev.

.PARAMETER Url
    Override the manifest's url (handy for testing a new upload before committing).

.EXAMPLE
    powershell -NoProfile -ExecutionPolicy Bypass -File dependencies\get_dependencies.ps1
    powershell -NoProfile -ExecutionPolicy Bypass -File dependencies\get_dependencies.ps1 -Profile release
#>
[CmdletBinding()]
param(
    [switch]   $Force,
    [switch]   $Verify,
    [string]   $Profile = "dev",
    [string]   $Manifest = "",
    [string]   $Url = "",
    [switch]   $Quiet
)

Set-StrictMode -Version 2
$ErrorActionPreference = "Stop"

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot  = (Resolve-Path (Join-Path $ScriptDir "..")).Path
if (-not $Manifest) { $Manifest = Join-Path $ScriptDir "manifest.json" }
$CacheDir  = Join-Path $ScriptDir ".cache"
$StateDir  = Join-Path $ScriptDir ".state"
$MegaApi   = "https://g.api.mega.co.nz/cs"
if ($env:KH_MEGA_API) { $MegaApi = $env:KH_MEGA_API }   # test hook (mock server)

function Write-Info($msg)  { if (-not $Quiet) { Write-Host $msg } }
function Write-Ok($msg)    { if (-not $Quiet) { Write-Host $msg -ForegroundColor Green } }
function Write-Warn2($msg) { Write-Host $msg -ForegroundColor Yellow }
function Write-Err($msg)   { Write-Host $msg -ForegroundColor Red }

# Windows PowerShell 5.1 defaults to TLS 1.0 which every host now rejects.
try {
    [Net.ServicePointManager]::SecurityProtocol = [Net.ServicePointManager]::SecurityProtocol -bor [Net.SecurityProtocolType]::Tls12
} catch { }

# ---------------------------------------------------------------------------
# Native helper (C# 5 syntax only: Windows PowerShell 5.1 compiles with the
# .NET Framework compiler). Compiled lazily - only when something must be
# downloaded - so the "everything is already there" path stays instant.
# ---------------------------------------------------------------------------
$script:NativeLoaded = $false
$NativeSource = @'
using System;
using System.IO;
using System.Net;
using System.Text;
using System.Security.Cryptography;

namespace KhDeps
{
    public static class Util
    {
        public static byte[] B64UrlDecode(string s)
        {
            s = s.Trim().Replace('-', '+').Replace('_', '/');
            switch (s.Length % 4) { case 2: s += "=="; break; case 3: s += "="; break; }
            return Convert.FromBase64String(s);
        }

        public static string Sha256File(string path)
        {
            using (SHA256 sha = SHA256.Create())
            using (FileStream fs = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read, 1 << 20))
            {
                byte[] h = sha.ComputeHash(fs);
                StringBuilder sb = new StringBuilder(64);
                foreach (byte b in h) sb.Append(b.ToString("x2"));
                return sb.ToString();
            }
        }

        public static bool Silent = false;
        static DateTime lastProgress = DateTime.MinValue;
        public static void Progress(string label, long done, long total, bool final)
        {
            if (Silent) return;
            DateTime now = DateTime.UtcNow;
            if (!final && (now - lastProgress).TotalMilliseconds < 250) return;
            lastProgress = now;
            string s;
            if (total > 0)
                s = string.Format("\r    {0}  {1:0.0} / {2:0.0} MB ({3}%)   ", label, done / 1048576.0, total / 1048576.0, (int)(done * 100 / total));
            else
                s = string.Format("\r    {0}  {1:0.0} MB   ", label, done / 1048576.0);
            Console.Write(s);
            if (final) Console.WriteLine();
        }

        public static HttpWebRequest MakeRequest(string url)
        {
            HttpWebRequest req = (HttpWebRequest)WebRequest.Create(url);
            req.UserAgent = "kh_framework-get_dependencies/1.0";
            req.AllowAutoRedirect = true;
            req.Timeout = 60000;
            req.ReadWriteTimeout = 120000;
            return req;
        }

        // Plain streaming download (https hosts: GitHub Releases, R2, Dropbox, ...).
        public static void Download(string url, string dest, string label)
        {
            HttpWebRequest req = MakeRequest(url);
            using (HttpWebResponse resp = (HttpWebResponse)req.GetResponse())
            using (Stream input = resp.GetResponseStream())
            using (FileStream output = new FileStream(dest, FileMode.Create, FileAccess.Write, FileShare.None, 1 << 20))
            {
                long total = resp.ContentLength;
                byte[] buf = new byte[1 << 18];
                long done = 0;
                int n;
                while ((n = input.Read(buf, 0, buf.Length)) > 0)
                {
                    output.Write(buf, 0, n);
                    done += n;
                    Progress(label, done, total, false);
                }
                Progress(label, done, total, true);
            }
        }
    }

    // MEGA public-link client side: the key split, AES-ECB node-key decryption,
    // AES-CBC attribute decryption and the AES-128-CTR streaming file decryption.
    public static class Mega
    {
        static Aes MakeAes(byte[] key, CipherMode mode)
        {
            Aes aes = Aes.Create();
            aes.KeySize = 128;
            aes.BlockSize = 128;
            aes.Mode = mode;
            aes.Padding = PaddingMode.None;
            aes.Key = key;
            aes.IV = new byte[16];
            return aes;
        }

        public static byte[] EcbDecrypt(byte[] key, byte[] data)
        {
            if (data.Length % 16 != 0) throw new ArgumentException("ECB data must be a multiple of 16 bytes");
            using (Aes aes = MakeAes(key, CipherMode.ECB))
            using (ICryptoTransform t = aes.CreateDecryptor())
            {
                byte[] outp = new byte[data.Length];
                t.TransformBlock(data, 0, data.Length, outp, 0);
                return outp;
            }
        }

        public static byte[] CbcDecryptZeroIv(byte[] key, byte[] data)
        {
            int len = data.Length - (data.Length % 16);
            using (Aes aes = MakeAes(key, CipherMode.CBC))
            using (ICryptoTransform t = aes.CreateDecryptor())
            {
                byte[] outp = new byte[len];
                if (len > 0) t.TransformBlock(data, 0, len, outp, 0);
                return outp;
            }
        }

        // A 32-byte MEGA file key -> 16-byte AES key and 16-byte CTR start value.
        public static void SplitFileKey(byte[] k32, out byte[] key, out byte[] iv)
        {
            if (k32.Length != 32) throw new ArgumentException("MEGA file key must be 32 bytes, got " + k32.Length);
            key = new byte[16];
            for (int i = 0; i < 16; i++) key[i] = (byte)(k32[i] ^ k32[i + 16]);
            iv = new byte[16];
            Array.Copy(k32, 16, iv, 0, 8); // nonce = key words 4..5, counter starts at 0
        }

        // Decrypted attribute block: "MEGA{...json...}" followed by zero padding.
        public static string DecryptAttributes(byte[] key16, byte[] attrs)
        {
            byte[] plain = CbcDecryptZeroIv(key16, attrs);
            string s = Encoding.UTF8.GetString(plain).TrimEnd('\0');
            int i = s.IndexOf('{');
            int j = s.LastIndexOf('}');
            if (!s.StartsWith("MEGA") || i < 0 || j < i)
                throw new InvalidDataException("MEGA attribute block did not decrypt (wrong key?)");
            return s.Substring(i, j - i + 1);
        }

        static void Increment(byte[] counter)
        {
            for (int i = 15; i >= 0; i--) { if (++counter[i] != 0) break; }
        }

        // Download url and AES-128-CTR decrypt on the fly into dest.
        public static void DownloadDecrypt(string url, byte[] key16, byte[] iv16, long expectedSize, string dest, string label)
        {
            HttpWebRequest req = Util.MakeRequest(url);
            using (Aes aes = MakeAes(key16, CipherMode.ECB))
            using (ICryptoTransform enc = aes.CreateEncryptor())
            using (HttpWebResponse resp = (HttpWebResponse)req.GetResponse())
            using (Stream input = resp.GetResponseStream())
            using (FileStream output = new FileStream(dest, FileMode.Create, FileAccess.Write, FileShare.None, 1 << 20))
            {
                long total = resp.ContentLength > 0 ? resp.ContentLength : expectedSize;
                const int BLOCKS = 16384;                 // 256 KiB of keystream per batch
                byte[] counters = new byte[BLOCKS * 16];
                byte[] keystream = new byte[BLOCKS * 16];
                byte[] counter = (byte[])iv16.Clone();
                byte[] data = new byte[BLOCKS * 16];
                int have = 0;                             // bytes in data not yet decrypted
                long done = 0;

                while (true)
                {
                    int n = input.Read(data, have, data.Length - have);
                    if (n <= 0 && have == 0) break;
                    have += n;
                    bool eof = n <= 0;
                    // Decrypt whole blocks; keep a partial tail for the next read (unless eof).
                    int usable = eof ? have : have - (have % 16);
                    int nblocks = (usable + 15) / 16;
                    for (int b = 0; b < nblocks; b++)
                    {
                        Array.Copy(counter, 0, counters, b * 16, 16);
                        Increment(counter);
                    }
                    enc.TransformBlock(counters, 0, nblocks * 16, keystream, 0);
                    for (int i = 0; i < usable; i++) data[i] ^= keystream[i];
                    output.Write(data, 0, usable);
                    done += usable;
                    int rest = have - usable;
                    if (rest > 0) Array.Copy(data, usable, data, 0, rest);
                    have = rest;
                    Util.Progress(label, done, total, false);
                    if (eof) break;
                }
                Util.Progress(label, done, total, true);
            }
        }
    }
}
'@

function Ensure-Native {
    if ($script:NativeLoaded) { return }
    if (-not ("KhDeps.Util" -as [type])) {
        # -IgnoreWarnings: PowerShell 7 flags HttpWebRequest as obsolete (SYSLIB0014) and
        # would otherwise refuse to compile; it is the one API that exists on both runtimes.
        Add-Type -TypeDefinition $NativeSource -Language CSharp -IgnoreWarnings -WarningAction SilentlyContinue | Out-Null
    }
    [KhDeps.Util]::Silent = [bool]$Quiet
    $script:NativeLoaded = $true
}

# ---------------------------------------------------------------------------
# Small helpers
# ---------------------------------------------------------------------------
function Get-Sha256([string]$path) {
    return (Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant()
}

$Sep = [System.IO.Path]::DirectorySeparatorChar

function Normalize-Rel([string]$p) {
    $p = ($p -replace '\\', '/').Trim()
    while ($p.StartsWith('./')) { $p = $p.Substring(2) }
    $p = $p.TrimEnd('/')
    if ($p -eq '') { $p = '.' }
    return $p
}

function Get-Prop($obj, [string]$name, $default = $null) {
    if ($null -eq $obj) { return $default }
    $p = $obj.PSObject.Properties[$name]
    if ($null -eq $p -or $null -eq $p.Value) { return $default }
    return $p.Value
}

function Format-Size([long]$bytes) {
    if ($bytes -ge 1048576) { return ("{0:0.0} MB" -f ($bytes / 1048576.0)) }
    if ($bytes -ge 1024)    { return ("{0:0.0} KB" -f ($bytes / 1024.0)) }
    return "$bytes B"
}

# ---------------------------------------------------------------------------
# MEGA public links
# ---------------------------------------------------------------------------
function Test-MegaUrl([string]$url) {
    return ($url -match '^https?://(www\.)?mega(\.co)?\.nz/')
}

# Returns @{ Type = 'file'|'folder'; Handle; Key (bytes); SubHandle (folder links that point at one file) }
function Parse-MegaLink([string]$url) {
    $u = $url.Trim()
    $m = [regex]::Match($u, 'mega(?:\.co)?\.nz/(?:file/|#!)([A-Za-z0-9_-]{8})[#!]([A-Za-z0-9_-]{43})')
    if ($m.Success) {
        return @{ Type = 'file'; Handle = $m.Groups[1].Value; Key = [KhDeps.Util]::B64UrlDecode($m.Groups[2].Value); SubHandle = $null }
    }
    $m = [regex]::Match($u, 'mega(?:\.co)?\.nz/(?:folder/|#F!)([A-Za-z0-9_-]{8})[#!]([A-Za-z0-9_-]{22})(?:/file/([A-Za-z0-9_-]{8}))?')
    if ($m.Success) {
        $sub = $null
        if ($m.Groups[3].Success) { $sub = $m.Groups[3].Value }
        return @{ Type = 'folder'; Handle = $m.Groups[1].Value; Key = [KhDeps.Util]::B64UrlDecode($m.Groups[2].Value); SubHandle = $sub }
    }
    throw "Unrecognised MEGA link: $url (expected https://mega.nz/file/XXXX#KEY or https://mega.nz/folder/XXXX#KEY)"
}

$script:MegaSeq = [int](Get-Random -Minimum 100000 -Maximum 999999)
$script:MegaFolderCache = @{}

# One MEGA API request. $query: extra query-string parameters (e.g. "&n=FOLDER").
function Invoke-MegaApi([hashtable]$request, [string]$query = "") {
    $body = "[" + ($request | ConvertTo-Json -Compress -Depth 5) + "]"
    $attempt = 0
    while ($true) {
        $attempt++
        $uri = "$MegaApi" + "?id=" + $script:MegaSeq + $query
        $script:MegaSeq++
        $raw = $null
        try {
            $raw = Invoke-RestMethod -Method Post -Uri $uri -Body $body -ContentType "application/json" -TimeoutSec 60
        } catch {
            if ($attempt -ge 5) { throw "MEGA API request failed: $($_.Exception.Message)" }
            Start-Sleep -Seconds ([Math]::Min(2 * $attempt, 10))
            continue
        }
        $first = @($raw)[0]
        if ($first -is [int] -or $first -is [long] -or $first -is [double] -or $first -is [decimal]) {
            $code = [int]$first
            if ($code -eq -3 -and $attempt -lt 8) {    # EAGAIN: server asks us to retry
                Start-Sleep -Seconds ([Math]::Min(2 * $attempt, 15))
                continue
            }
            $reason = switch ($code) {
                -2  { "EARGS: malformed request" }
                -3  { "EAGAIN: server busy, giving up after retries" }
                -4  { "ERATELIMIT: too many requests, try again later" }
                -9  { "ENOENT: not found (link removed, or wrong handle)" }
                -11 { "EACCESS: access denied" }
                -16 { "EBLOCKED: this link has been blocked by MEGA" }
                -17 { "EOVERQUOTA: the owner's / your transfer quota is exhausted" }
                -18 { "ETEMPUNAVAIL: temporarily unavailable" }
                default { "error code $code" }
            }
            throw "MEGA API error $code ($reason)"
        }
        return $first
    }
}

# Lists a MEGA folder link once per run: name -> @{ Handle; Key; Iv; Size }.
function Get-MegaFolderIndex([hashtable]$link) {
    $cacheKey = $link.Handle
    if ($script:MegaFolderCache.ContainsKey($cacheKey)) { return $script:MegaFolderCache[$cacheKey] }

    Write-Info "    listing MEGA folder $($link.Handle) ..."
    $resp = Invoke-MegaApi @{ a = "f"; c = 1; r = 1 } ("&n=" + $link.Handle)
    $nodes = Get-Prop $resp "f" @()
    $index = @{ ByName = @{}; ByHandle = @{} }
    foreach ($node in $nodes) {
        $type = [int](Get-Prop $node "t" -1)
        if ($type -ne 0) { continue }   # 0 = file
        $kField = [string](Get-Prop $node "k" "")
        if (-not $kField) { continue }
        $encKey = $null
        foreach ($part in $kField.Split('/')) {
            $kv = $part.Split(':')
            if ($kv.Length -eq 2 -and ($kv[0] -eq $link.Handle -or $null -eq $encKey)) { $encKey = $kv[1] }
        }
        if (-not $encKey) { continue }
        try {
            $k32 = [KhDeps.Mega]::EcbDecrypt($link.Key, [KhDeps.Util]::B64UrlDecode($encKey))
            if ($k32.Length -ne 32) { continue }
            $key = $null; $iv = $null
            [KhDeps.Mega]::SplitFileKey($k32, [ref]$key, [ref]$iv)
            $attrJson = [KhDeps.Mega]::DecryptAttributes($key, [KhDeps.Util]::B64UrlDecode([string](Get-Prop $node "a" "")))
            $attrs = $attrJson | ConvertFrom-Json
            $name = [string](Get-Prop $attrs "n" "")
            $entry = @{ Handle = [string](Get-Prop $node "h"); Key = $key; Iv = $iv; Size = [long](Get-Prop $node "s" 0); Name = $name }
            $index.ByHandle[$entry.Handle] = $entry
            if ($name -and -not $index.ByName.ContainsKey($name)) { $index.ByName[$name] = $entry }
        } catch {
            Write-Warn2 "    (skipping a MEGA node that could not be decrypted: $($_.Exception.Message))"
        }
    }
    $script:MegaFolderCache[$cacheKey] = $index
    return $index
}

# Downloads one package from MEGA (file link, or folder link + file name) to $dest.
function Get-MegaPackage([string]$url, [string]$fileName, [string]$dest, [string]$label) {
    $link = Parse-MegaLink $url
    if ($link.Type -eq 'file') {
        $key = $null; $iv = $null
        [KhDeps.Mega]::SplitFileKey($link.Key, [ref]$key, [ref]$iv)
        $g = Invoke-MegaApi @{ a = "g"; g = 1; p = $link.Handle }
        $dlUrl = [string](Get-Prop $g "g" "")
        if (-not $dlUrl) { throw "MEGA did not return a download URL for file $($link.Handle) (response: $($g | ConvertTo-Json -Compress))" }
        $size = [long](Get-Prop $g "s" 0)
        try {
            $attrs = [KhDeps.Mega]::DecryptAttributes($key, [KhDeps.Util]::B64UrlDecode([string](Get-Prop $g "at" ""))) | ConvertFrom-Json
            $remoteName = [string](Get-Prop $attrs "n" "")
            if ($remoteName -and $fileName -and $remoteName -ne $fileName) {
                Write-Warn2 "    note: MEGA file is named '$remoteName', manifest expects '$fileName' (continuing; sha256 decides)"
            }
        } catch { Write-Warn2 "    note: could not read the MEGA file name: $($_.Exception.Message)" }
        [KhDeps.Mega]::DownloadDecrypt($dlUrl, $key, $iv, $size, $dest, $label)
        return
    }

    # folder link
    $index = Get-MegaFolderIndex $link
    $entry = $null
    if ($link.SubHandle) {
        $entry = $index.ByHandle[$link.SubHandle]
        if ($null -eq $entry) { throw "MEGA folder $($link.Handle) has no file with handle $($link.SubHandle)" }
    } else {
        if ($index.ByName.ContainsKey($fileName)) { $entry = $index.ByName[$fileName] }
        else {
            foreach ($k in $index.ByName.Keys) { if ($k -ieq $fileName) { $entry = $index.ByName[$k]; break } }
        }
        if ($null -eq $entry) {
            $have = ($index.ByName.Keys | Sort-Object) -join ", "
            throw "MEGA folder $($link.Handle) does not contain '$fileName'. Files found: $have"
        }
    }
    $g = Invoke-MegaApi @{ a = "g"; g = 1; n = $entry.Handle } ("&n=" + $link.Handle)
    $dlUrl = [string](Get-Prop $g "g" "")
    if (-not $dlUrl) { throw "MEGA did not return a download URL for '$fileName' (response: $($g | ConvertTo-Json -Compress))" }
    $size = [long](Get-Prop $g "s" $entry.Size)
    [KhDeps.Mega]::DownloadDecrypt($dlUrl, $entry.Key, $entry.Iv, $size, $dest, $label)
}

# ---------------------------------------------------------------------------
# Generic download with verification -> cache
# ---------------------------------------------------------------------------
function Get-Package([string]$url, [string]$fileName, [string]$expected, [string]$cachePath) {
    Ensure-Native
    $tmp = "$cachePath.part"
    $attempt = 0
    while ($true) {
        $attempt++
        try {
            if (Test-Path -LiteralPath $tmp) { Remove-Item -LiteralPath $tmp -Force }
            if (Test-MegaUrl $url) {
                Get-MegaPackage $url $fileName $tmp $fileName
            } else {
                [KhDeps.Util]::Download($url, $tmp, $fileName)
            }
            break
        } catch {
            $msg = $_.Exception.Message
            if ($_.Exception.InnerException) { $msg += " / " + $_.Exception.InnerException.Message }
            if ($msg -match '509|quota|EOVERQUOTA') {
                throw "Download of $fileName refused: bandwidth/transfer quota exceeded ($msg). Wait a while, use a VPN, or switch the manifest url to GitHub Releases."
            }
            if ($attempt -ge 3) { throw "Download of $fileName failed after $attempt attempts: $msg" }
            Write-Warn2 "    retry $attempt after error: $msg"
            Start-Sleep -Seconds (3 * $attempt)
        }
    }
    if ($expected) {
        $actual = Get-Sha256 $tmp
        if ($actual -ne $expected.ToLowerInvariant()) {
            Remove-Item -LiteralPath $tmp -Force
            throw "sha256 mismatch for ${fileName}: expected $expected, got $actual. The upload does not match the manifest (re-run pack_dependencies.ps1 and re-upload, or fix manifest.json)."
        }
    } else {
        Write-Warn2 "    manifest has no sha256 for $fileName - download not verified"
    }
    Move-Item -LiteralPath $tmp -Destination $cachePath -Force
}

# ---------------------------------------------------------------------------
# Extraction
# ---------------------------------------------------------------------------
Add-Type -AssemblyName System.IO.Compression.FileSystem | Out-Null

# Extracts $zipPath into $destDir (overwrite). Returns the list of relative file paths written.
function Expand-Package([string]$zipPath, [string]$destDir) {
    $written = New-Object System.Collections.Generic.List[string]
    if (-not (Test-Path -LiteralPath $destDir)) { New-Item -ItemType Directory -Path $destDir -Force | Out-Null }
    $destFull = (Resolve-Path -LiteralPath $destDir).Path
    $zip = [System.IO.Compression.ZipFile]::OpenRead($zipPath)
    try {
        foreach ($entry in $zip.Entries) {
            $rel = ($entry.FullName -replace '\\', '/').TrimStart('/')
            if ($rel.EndsWith('/') -or $entry.Name -eq '' -or $rel -eq '') { continue }     # directory entry
            if ($rel -match '(^|/)\.\.(/|$)' -or $rel -match '^[A-Za-z]:') {
                throw "refusing unsafe zip entry '$($entry.FullName)' in $zipPath"
            }
            if ($Profile -ne "dev" -and $rel -match '^\.hemttout/dev/') {
                $rel = $rel -replace '^\.hemttout/dev/', (".hemttout/" + $Profile + "/")
            }
            $target = [System.IO.Path]::Combine($destFull, ($rel -replace '/', $Sep))
            $parent = Split-Path -Parent $target
            if (-not (Test-Path -LiteralPath $parent)) { New-Item -ItemType Directory -Path $parent -Force | Out-Null }
            try {
                [System.IO.Compression.ZipFileExtensions]::ExtractToFile($entry, $target, $true)
            } catch [System.IO.IOException] {
                throw "cannot overwrite '$target' - it is probably in use (close Arma 3 / TeamSpeak / the editor holding it) : $($_.Exception.Message)"
            }
            $written.Add($rel)
        }
    } finally { $zip.Dispose() }
    return ,$written
}

# ---------------------------------------------------------------------------
# State file
# ---------------------------------------------------------------------------
$StatePath = Join-Path $StateDir ("deps" + $(if ($Profile -ne "dev") { "@" + $Profile } else { "" }) + ".json")

function Read-State {
    if (-not (Test-Path -LiteralPath $StatePath)) { return $null }
    try { return (Get-Content -LiteralPath $StatePath -Raw | ConvertFrom-Json) } catch { return $null }
}

function Write-State([string]$file, [string]$sha, $files) {
    if (-not (Test-Path -LiteralPath $StateDir)) { New-Item -ItemType Directory -Path $StateDir -Force | Out-Null }
    $obj = [ordered]@{ file = $file; sha256 = $sha; profile = $Profile; installed = (Get-Date).ToString("s"); files = @($files) }
    ($obj | ConvertTo-Json -Depth 5) | Set-Content -LiteralPath $StatePath -Encoding UTF8
}

function Test-Installed($state, [string]$want) {
    if ($null -eq $state) { return $false }
    if ($want -and ([string](Get-Prop $state "sha256" "")) -ne $want) { return $false }
    $files = @(Get-Prop $state "files" @())
    if ($files.Count -eq 0) { return $false }
    foreach ($f in $files) {
        if (-not (Test-Path -LiteralPath ([System.IO.Path]::Combine($RepoRoot, ($f -replace '/', $Sep))))) { return $false }
    }
    return $true
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
if (-not (Test-Path -LiteralPath $Manifest)) { Write-Err "manifest not found: $Manifest"; exit 2 }
$man = Get-Content -LiteralPath $Manifest -Raw | ConvertFrom-Json
$url = [string](Get-Prop $man "url" "")
if ($Url) { $url = $Url }
$file = [string](Get-Prop $man "file" "kh_framework_deps.zip")
$want = ([string](Get-Prop $man "sha256" "")).ToLowerInvariant()
$size = [long](Get-Prop $man "size" 0)
$cachePath = Join-Path $CacheDir $file

Write-Info ""
Write-Info "KH Framework dependencies  (profile '$Profile')"
Write-Info "  root   : $RepoRoot"
Write-Info "  source : $url"
Write-Info ""

$sw = [System.Diagnostics.Stopwatch]::StartNew()
$state = Read-State
if (-not $Force -and -not $Verify -and (Test-Installed $state $want)) {
    Write-Ok ("  [ok]      {0} - all {1} files present and up to date." -f $file, @(Get-Prop $state "files" @()).Count)
    exit 0
}
$files = @()
try {
    if (-not $url) { throw "manifest.json has no url - the maintainer has not published the dependencies yet (see dependencies/README.md)" }
    if (-not (Test-Path -LiteralPath $CacheDir)) { New-Item -ItemType Directory -Path $CacheDir -Force | Out-Null }

    $haveCache = $false
    if (-not $Force -and (Test-Path -LiteralPath $cachePath)) {
        if ($want) { $haveCache = ((Get-Sha256 $cachePath) -eq $want) } else { $haveCache = $true }
        if (-not $haveCache) { Remove-Item -LiteralPath $cachePath -Force }
    }
    if ($haveCache) {
        Write-Info ("  [cached]  {0}" -f $file)
    } else {
        $hint = if ($size -gt 0) { " (" + (Format-Size $size) + ")" } else { "" }
        Write-Info ("  [get]     {0}{1}" -f $file, $hint)
        Get-Package $url $file $want $cachePath
    }

    Write-Info ("  [extract] {0} -> {1}" -f $file, $RepoRoot)
    $files = Expand-Package $cachePath $RepoRoot

    # Remove files placed by an earlier version of the package that are gone now.
    if ($null -ne $state) {
        foreach ($old in @(Get-Prop $state "files" @())) {
            if ($files -notcontains $old) {
                $p = [System.IO.Path]::Combine($RepoRoot, ($old -replace '/', $Sep))
                if (Test-Path -LiteralPath $p) {
                    try { Remove-Item -LiteralPath $p -Force; Write-Info "            removed stale $old" } catch { }
                }
            }
        }
    }
    $sha = if ($want) { $want } else { Get-Sha256 $cachePath }
    Write-State $file $sha $files
} catch {
    Write-Err ("  [FAILED]  {0}" -f $_.Exception.Message)
    Write-Err "Fix the problem above and run dependencies\get_dependencies.bat again."
    exit 1
}
$sw.Stop()
Write-Ok ("All dependencies present: {0} files installed ({1:0.0}s)." -f @($files).Count, $sw.Elapsed.TotalSeconds)
exit 0
