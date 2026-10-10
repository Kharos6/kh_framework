# Binary dependencies

The C++ extensions link against prebuilt third-party libraries (LuaJIT, Intercept,
llama.cpp, sherpa-onnx, Ultralight, MinHook, Vulkan, CUDA) and the mod folder needs
their runtime DLLs. Those binaries are **not in git** (`*.lib`, `*.dll`, `*.exe` are
ignored) because rebuilding them from source with the exact flags is fragile. Instead,
`manifest.json` points at **one zip** (`kh_framework_deps.zip`) on cloud storage whose
internal paths mirror the repository; it is downloaded, verified and extracted over
the repository root.

The headers (`sol/`, `lz4/`, `llama/include/`, `ultralight/include/`, ...) *are* in git;
the zip carries only the `lib/` folders, the runtime DLLs, `hemtt.exe` and the
Ultralight resources:

| inside the zip | purpose |
|---|---|
| `hemtt.exe` | the pinned HEMTT build tool, repo root |
| `extensions/kh_framework/<lib>/lib/*.lib` (intercept, luajit, sherpa, vulkan, llama, ultralight, minhook) | link-time libraries |
| `extensions/kh_framework/cuda/lib/*.lib` | the four CUDA import libs (`cuda`, `cudart_static`, `cublas`, `cublasLt`), so **no CUDA Toolkit install is needed to link**; `build.bat` still falls back to the toolkit path if present |
| `.hemttout/dev/*.dll`, `ultralight_resources/`, `LICENSES.txt` | runtime files of the mod folder |

## For contributors: nothing to do

Every build entry point (`extensions\*\build.bat`, `.hemtt\*.bat`) runs
`dependencies\get_dependencies.bat` first. The first run downloads the zip (a few
hundred MB); later runs take a fraction of a second. You can also run it yourself:

```
get_dependencies.bat                   fetch what is missing (instant if nothing is)
get_dependencies.bat -Force            re-download and re-extract
get_dependencies.bat -Verify           re-hash the cached zip, re-extract if anything is off
get_dependencies.bat -Profile release  put the runtime files in .hemttout\release instead of dev
```

Requirements: Windows 10/11 with the built-in Windows PowerShell 5.1 (PowerShell 7 also
works). No other software; MEGA links are handled without the MEGA client.

The download is cached in `dependencies\.cache\` and the installed file list is
recorded in `dependencies\.state\`. If `.hemttout\dev` gets wiped (plain `hemtt dev`
does that), the next build restores the runtime files from the cache without
downloading. The zip is verified against the sha256 in the manifest before extraction.

## For the maintainer: publishing / updating

### 1. Pack

On a machine where everything is in place (your working tree, with `CUDA_PATH` set by
the CUDA installer - open a fresh terminal if in doubt):

```
dependencies\pack_dependencies.bat -Check   # lists every file that would go in; reports missing ones
dependencies\pack_dependencies.bat          # builds dependencies\upload\kh_framework_deps.zip, updates manifest.json
```

The packer only reads your files; it writes nothing outside `dependencies\upload\` and
`dependencies\manifest.json`. It fingerprints the sources, so running it again without
changes leaves the zip and the recorded sha256 untouched.

Updating a library later: replace the files in its `lib\` (and/or the DLLs in
`.hemttout\dev`), run `pack_dependencies.bat`, upload the new zip, commit
`manifest.json`. On their next build users re-download the zip; files that vanished
from it (e.g. a renamed `.lib`) are removed from their tree too.

### 2. Upload

**GitHub Releases (recommended).** Free, no bandwidth cap, 2 GB per file, direct
download URL, and the zip lives next to the code:

1. github.com → Releases → *Draft a new release*. Tag e.g. `deps-2026.10` (clearly not
   a mod version), tick **pre-release** so it is not shown as the mod's latest release.
2. Drag `dependencies\upload\kh_framework_deps.zip` into the assets box. Publish.
3. ```
   dependencies\pack_dependencies.bat -Url https://github.com/Kharos6/kh_framework/releases/download/deps-2026.10/kh_framework_deps.zip
   ```
4. Commit `dependencies\manifest.json`.

For an update, either replace the asset on the same release (delete the old one, upload
the new one - same URL, the new sha256 in the manifest does the rest) or make a new
`deps-YYYY.MM` release and change `-Url`. The second keeps old commits buildable.

**MEGA.** Supported directly: the script talks to MEGA's public API and decrypts the
file itself, the user needs no MEGA software.

1. Upload `kh_framework_deps.zip` to MEGA.
2. Right-click it → *Get link* → **choose the option that includes the decryption key**
   (the link must end in `#` followed by 43 characters).
3. `dependencies\pack_dependencies.bat -Url "https://mega.nz/file/XXXXXXXX#KEY..."` and
   commit `manifest.json`.

   A folder link (`https://mega.nz/folder/XXXXXXXX#KEY`) also works: the zip is then
   found by name inside that folder, so an update is just replacing the file there.

   Caveats: free MEGA accounts have a **transfer quota per downloader IP** (a few GB
   per ~6 h) which a large zip can hit; an outage or "over quota" makes the build stop
   with a message saying so. The MEGA client code was tested against a mock of MEGA's
   API, not against mega.nz itself - after setting it up, do one test from a fresh
   clone. If it misbehaves, GitHub Releases is a one-flag switch.

**Anything else with a direct link** works as `-Url` (Cloudflare R2 / Backblaze B2
public bucket, a web server, a Dropbox link with `?dl=1`). Google Drive is **not**
suitable: files over ~100 MB get a virus-scan interstitial page instead of a download.

### 3. Adding a dependency or moving headers out of git

Add a line to `sources` in `manifest.json`:

```json
{ "from": "extensions/kh_framework/foo/lib/**", "to": "extensions/kh_framework/foo/lib" }
```

`from` accepts a repo-relative file, `dir/**` (recursive), `dir/*.lib`, or an absolute
path with `${ENV_VAR}`; `to` is the folder (relative to the repo root) it is extracted
into. Files under `to` keep their relative path, anything else is placed flat. Then
pack, upload, commit. To take e.g. `sol/` out of git later, add its folder here and
delete it from the repo - the bootstrap delivers it from then on.
