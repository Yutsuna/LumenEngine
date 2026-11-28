#Requires -Version 5.1

[CmdletBinding(SupportsShouldProcess)]
param
(
    [Parameter(Position = 0)]
    [string]$Command,

    [switch]$Status,
    [switch]$Update,
    [switch]$Force,
    [switch]$Isolated,
    [switch]$NoShell,
    [switch]$Help
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if ($Help)
{
    Write-Host @'

Lumen - Windows dev environment setup

WHAT IT DOES
  1. Installs a portable MSYS2 (UCRT64) in <repo>\.win-env\msys64 (first run only)
  2. Installs the missing packages (GCC, Meson, Ninja, SDL3, Vulkan, ...)
  3. Opens a PowerShell dev shell with the toolchain in PATH.
     The script then WAITS until you type 'exit'. This is normal.

USAGE
  .\Windows\Setup.ps1 [-Command <cmd>] [-Status] [-Update] [-Force]
                      [-Isolated] [-NoShell] [-Help]

OPTIONS
  -Command <cmd>  Run <cmd> inside the toolchain environment, then exit
                  (instead of opening an interactive shell)
  -Status         Show installed / missing packages and their versions
  -Update         Refresh the MSYS2 package databases (pacman -Syuu) first
  -Force          Delete and reinstall the local MSYS2
  -Isolated       Dev shell with a minimal PATH (toolchain + Windows system only)
  -NoShell        Only install; do not open the dev shell
  -Help           Show this help

EXAMPLES
  .\Windows\Setup.ps1                          Install if needed + open dev shell
  .\Windows\Setup.ps1 -NoShell                 Install only
  .\Windows\Setup.ps1 -Status                  List packages
  .\Windows\Setup.ps1 -Update -NoShell         Update + install only
  .\Windows\Setup.ps1 -Command "meson setup build"

'@
    return
}

Import-Module (Join-Path -Path $PSScriptRoot -ChildPath 'LumenDeps/LumenDeps.psd1') -Force

$repoRoot = (Resolve-Path -LiteralPath (Join-Path -Path $PSScriptRoot -ChildPath '..')).Path

$invokeArgs = @{ RepoRoot = $repoRoot }
foreach ($k in $PSBoundParameters.Keys)
{
    $invokeArgs[$k] = $PSBoundParameters[$k]
}
if ($PSBoundParameters.ContainsKey('WhatIf'))
{
    $invokeArgs['WhatIf']  = $PSBoundParameters['WhatIf']
}
if ($PSBoundParameters.ContainsKey('Confirm'))
{
    $invokeArgs['Confirm'] = $PSBoundParameters['Confirm']
}

Invoke-LumenSetup @invokeArgs
