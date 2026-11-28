#Requires -Version 5.1

function Invoke-LumenSetup
{
    <#
    .SYNOPSIS
    Sets up Lumen's Windows development toolchain.

    .DESCRIPTION
    Bootstraps the repository-local MSYS2 UCRT64 environment and installs the
    configured development packages. Unless -NoShell is specified, starts an
    interactive toolchain-configured PowerShell session or runs the supplied
    command in that environment.

    .PARAMETER Command
    Runs the specified command in a toolchain-configured PowerShell process.
    If omitted, opens an interactive development shell.

    .PARAMETER RepoRoot
    Specifies the repository root. Defaults to the repository containing this
    module.

    .PARAMETER Status
    Displays the installation status and version of each configured package.

    .PARAMETER Update
    Updates the MSYS2 package databases before installing missing packages.

    .PARAMETER Force
    Reinstalls the repository-local MSYS2 environment.

    .PARAMETER Isolated
    Starts the development shell with an isolated PATH containing the toolchain
    and essential Windows system directories.

    .PARAMETER NoShell
    Sets up the toolchain and packages without opening a development shell.

    .EXAMPLE
    Invoke-LumenSetup -Status
    Displays the status of configured packages.

    .EXAMPLE
    Invoke-LumenSetup -NoShell -Update
    Updates MSYS2 and installs required packages without opening a shell.

    .EXAMPLE
    Invoke-LumenSetup -Command 'meson setup build'
    Runs the specified command in the configured toolchain environment.

    .NOTES
    Requires Windows PowerShell 5.1 or later. Supports -WhatIf and -Confirm.
    #>
    [CmdletBinding(SupportsShouldProcess)]
    param
    (
        [Parameter(Position = 0)]
        [string]$Command,

        [string]$RepoRoot,

        [switch]$Status,
        [switch]$Update,
        [switch]$Force,
        [switch]$Isolated,
        [switch]$NoShell
    )

    if (-not $RepoRoot)
    {
        $RepoRoot = (Resolve-Path -LiteralPath (Join-Path -Path $PSScriptRoot -ChildPath '../../..')).Path
    }

    $toolchain = [LumenToolchain]::new($RepoRoot, $LumenConfig)

    if ($Status)
    {
        $toolchain.GetPackageStatus() |
            Sort-Object -Property Category, Name |
            Format-Table -Property Name, Category, Installed, Version -AutoSize
        return
    }

    if ($PSCmdlet.ShouldProcess($toolchain.WinEnvDir, 'Set up the MSYS2 UCRT64 toolchain'))
    {
        $toolchain.BootstrapMsys2($Force.IsPresent)
        $toolchain.SyncPackages($Update.IsPresent)
    }

    if ($NoShell)
    {
        return
    }

    $hasCmd = $PSBoundParameters.ContainsKey('Command')

    if (-not $hasCmd)
    {
        Write-Host 'Toolchain ready. Opening the Lumen dev shell - type "exit" to leave it.' -ForegroundColor Green
    }

    $exitCode = $toolchain.EnterShell($Command, $hasCmd, $Isolated.IsPresent)
    if ($hasCmd)
    {
        exit $exitCode
    }
}
