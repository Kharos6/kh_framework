<h1 align="center">
  DESCRIPTION
</h1>

KH Framework is a tool package designed to facilitate the creation of missions and addons for Arma 3. It contains a library of commands, functions, editor attributes, events, modules, and other various utilities.

<h1 align="center">
  COMPATIBILITY
</h1>

Likely compatible with just about everything on the workshop.

<h1 align="center">
  NOTES
</h1>

- No guarantees that every aspect of the mod functions perfectly are made. Any issues should be reported.
- This mod must be installed on every machine and the server.

<h1 align="center">
  BUILDING
</h1>

Requirements: Windows 10/11 and **Visual Studio 2022** (any edition - the free Community edition or the standalone "Build Tools for Visual Studio 2022" is enough) with the "Desktop development with C++" workload. The build scripts locate it automatically via `vswhere` wherever it is installed; set `KH_VCVARS` to a `vcvars64.bat` path to override. Nothing else - the prebuilt third-party libraries, runtime DLLs and `hemtt.exe` are not in git; they are downloaded automatically from the location listed in [`dependencies/manifest.json`](dependencies/manifest.json) the first time any build script runs (see [`dependencies/README.md`](dependencies/README.md)).

**Why Visual Studio 2022 specifically:** the compiler shipped with Visual Studio 2026 (MSVC 14.50, as of 18.10) crashes with an internal compiler error (`C1001` in `lua_wrappers_sqf.hpp`) on the function that registers the ~1,000 SQF command wrappers for Lua. The 2022 toolset (MSVC 14.4x) compiles it fine, so the build scripts prefer a VS 2022 install when one exists and fall back to VS 2026 with a warning. If you only have Visual Studio 2026, you do not need a second install: in the Visual Studio Installer choose *Modify* → *Individual components* and add **"MSVC v143 - VS 2022 C++ x64/x86 build tools (Latest)"**, then build with the environment variable `KH_VCVARS_VER` set to that toolset's version (e.g. `set KH_VCVARS_VER=14.44` in the terminal, or add it as a user environment variable). `vsenv.bat` prints the compiler version at the start of every build, so a mismatch is easy to spot. If all else fails, simply install the 2022 version.

- `build_all.bat` - builds all of the below in one go, meaning the three extensions and the mod itself
- `release_final.bat` - builds the full release: dependencies, the three extensions rebuilt, `.hemtt\release.bat`, then everything from `.hemttout\dev` except `addons`, `keys` and `mod.cpp`, then `Documents\Arma 3\kh_framework\cache`, all into `.hemttout\release`
- `launch.bat` - launches Arma 3 with the dev build
- `extensions\kh_framework\build.bat` - builds `kh_framework_x64.dll` into `.hemttout\dev\intercept\`
- `extensions\kh_rv_extension\build.bat` - builds `kh_rv_extension_x64.dll` into `.hemttout\dev\`
- `extensions\kh_framework_teamspeak\build.bat` - builds the Teamspeak plugin and its `.ts3_plugin` package into `.hemttout\dev\`
- `.hemtt\build.bat` - builds the PBOs and copies them into `.hemttout\dev\addons\`
- `.hemtt\launch_no_build.bat` - launches Arma 3 with `.hemttout\dev` as the mod folder
- `.hemtt\release.bat` - builds the mod release
- `dependencies\get_dependencies.bat` - manual fetch for the dependencies.

<h1 align="center">
  DISCLAIMER
</h1>

So long as it is done in good will, and credit to the original author(s) is provided alongside the necessary links to the original content, you are allowed to modify, redistribute, and repurpose this mod as you see fit.