---
name: Windows Workflow
description: How Lumen's Windows dev environment works — repo-local MSYS2 UCRT64 via Windows/Setup.ps1, direnv/.envrc activation, and running Meson builds locally
---

## Architecture (verified layout)

```
Windows/Setup.ps1             — entry point: bootstrap, packages, dev shell
Windows/LumenDeps/
  LumenDeps.psm1              — LumenToolchain class (install / env / EnterShell)
  Private/Deps.ps1            — MSYS2 pacman package list = Windows deps source of truth
.win-env/msys64/              — portable MSYS2, repo-local, gitignored (never global)
  ucrt64/bin/                 — gcc, meson, ninja, pkgconf, … prepended to PATH
.envrc                        — OS-aware: Windows branch activates this same UCRT64 env
```

- Dependencies are **never installed globally**. The Windows source of truth is
  `Windows/LumenDeps/Private/Deps.ps1`; every tool lives under `.win-env/`.
- `LumenToolchain.GetEnvironmentVariables()` exports `CC`, `CXX`, `PKG_CONFIG`,
  `PKG_CONFIG_PATH`, `VK_LAYER_PATH`, `CCACHE_DIR` and `LUMEN_SHELL`. These are
  mirrored by the Windows branch of `.envrc`; keep both in sync when changing
  the toolchain contract.
- `Nix/deps.nix` is the equivalent source of truth for Linux/macOS. Do not mix
  the two dependency lists.

## Setup (once per clone)

```powershell
.\Windows\Setup.ps1 -NoShell          # install MSYS2 UCRT64 + missing packages
.\Windows\Setup.ps1 -Status           # audit installed package versions
.\Windows\Setup.ps1 -Update -NoShell  # refresh the pacman databases
.\Windows\Setup.ps1                   # interactive dev shell (type exit to leave)
.\Windows\Setup.ps1 -Command "meson --version"   # one command inside the env
```

`-Isolated` limits PATH to the UCRT64 bin plus Windows system directories: use
it when you need certainty that no global tool can be picked up. `-Force`
deletes and reinstalls the local MSYS2.

## direnv (automatic activation)

`.envrc` branches on `uname -s` (`MINGW*|MSYS*|CYGWIN*|Windows_NT`). On Windows
it activates `.win-env/msys64/ucrt64`; on Linux/macOS it runs
`use flake .#default --accept-flake-config` + `PATH_add result/bin`.

Windows prerequisites:

- `direnv` with `pwsh` export support and PowerShell **7.2+** (`direnv hook pwsh`);
- a real bash (Git Bash or MSYS2). Set `DIRENV_BASH` if plain `bash.exe`
  resolves to the WSL store stub;
- `HOME` or `XDG_CONFIG_HOME` must be set — direnv stores its allow DB and
  config under XDG paths;
- `direnv allow` after each `.envrc` change, `direnv reload` after edits.

The repo's PowerShell profile wires the hook. On a fresh machine the equivalent
belongs in the user's `$PROFILE`:

```powershell
$env:HOME = $env:USERPROFILE
$env:DIRENV_BASH = 'C:\Program Files\Git\bin\bash.exe'
Invoke-Expression "$(direnv hook pwsh)"
```

direnv's `pwsh` export passes env strings through verbatim, so `.envrc`
prepends the UCRT64 bin in **POSIX** form and lets MSYS convert it when dumping
the environment. Do not "fix" that by assigning a Windows-form PATH inside the
bash evaluation: MSYS re-splits `C:` and corrupts every entry.

## Builds

Once the env is active (direnv on `cd`, or inside a `Setup.ps1` dev shell), the
usual Meson commands use local tools:

```powershell
meson setup Build --reconfigure
meson compile -C Build
meson test -C Build
```

Without direnv, prefix each one:
`.\Windows\Setup.ps1 -Command "meson compile -C Build"`.

Example executables are self-contained: a post-build `custom_target`
(`Meson/MesonStageRuntimeDlls.py`) copies the engine DLLs next to
`Build*/Examples/LuminaExample.exe`, so running an example needs no PATH entry
beyond the UCRT64 bin. Test and benchmark binaries still live beside their
targets in `Build*/Source/Meson/` and resolve the DLLs through the
`Build*/Source/Meson/Private` directory added by `.envrc`.

`Scripts/BuildMeson.ps1` reconfigures all three build trees in parallel but
calls a bare `meson`: it is only safe **from an activated shell**. Never run it
from a plain PowerShell where global tools could win.
