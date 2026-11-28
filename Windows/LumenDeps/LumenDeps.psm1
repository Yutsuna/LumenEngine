#Requires -Version 5.1

using namespace System.Collections.Generic
using namespace System.IO

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

<#
 # @class LumenToolchain
 # @brief Represents the repository-local MSYS2 UCRT64 toolchain and provides operations
 #        for bootstrapping its environment, synchronizing packages, and launching a
 #        configured development shell.
 #>
class LumenToolchain
{
    [string]    $RepoRoot
    [string]    $WinEnvDir
    [string]    $MsysRoot
    [string]    $UcrtRoot
    [string]    $UcrtBin
    [string]    $BashExe
    [string]    $CcacheDir
    [hashtable] $Config

    LumenToolchain([string]$RepoRoot, [hashtable]$Config)
    {
        $this.RepoRoot  = (Resolve-Path -LiteralPath $RepoRoot).Path
        $this.Config    = $Config
        $this.WinEnvDir = [Path]::Combine($this.RepoRoot, '.win-env')
        $this.MsysRoot  = [Path]::Combine($this.WinEnvDir, 'msys64')
        $this.UcrtRoot  = [Path]::Combine($this.MsysRoot, 'ucrt64')
        $this.UcrtBin   = [Path]::Combine($this.UcrtRoot, 'bin')
        $this.BashExe   = [Path]::Combine($this.MsysRoot, 'usr', 'bin', 'bash.exe')
        $this.CcacheDir = [Path]::Combine($this.WinEnvDir, 'ccache')
    }

    [bool] IsBootstrapped()
    {
        return (Test-Path -LiteralPath $this.BashExe)
    }

    [string[]] InvokeBash([string]$BashCommand, [bool]$CaptureOutput)
    {
        if (-not $this.IsBootstrapped())
        {
            throw "MSYS2 has not been initialized in '$($this.WinEnvDir)'."
        }

        $prevChere   = [Environment]::GetEnvironmentVariable('CHERE_INVOKING', 'Process')
        $prevMsystem = [Environment]::GetEnvironmentVariable('MSYSTEM', 'Process')
        $prevEap     = Get-Variable -Name ErrorActionPreference -ValueOnly

        [Environment]::SetEnvironmentVariable('CHERE_INVOKING', 'yes', 'Process')
        [Environment]::SetEnvironmentVariable('MSYSTEM', 'UCRT64', 'Process')

        try
        {
            $ErrorActionPreference = 'Continue'
            $output = @()

            if ($CaptureOutput)
            {
                $output = @(& $this.BashExe -lc $BashCommand 2>&1 | ForEach-Object { "$_" })
            }
            else
            {
                & $this.BashExe -lc $BashCommand 2>&1 | ForEach-Object { Write-Host "$_" }
            }

            $code = $LASTEXITCODE
            $ErrorActionPreference = $prevEap

            if ($code -ne 0)
            {
                throw "MSYS2 command failed with exit code $code`: $BashCommand"
            }

            return [string[]]$output
        }
        finally
        {
            $ErrorActionPreference = $prevEap
            [Environment]::SetEnvironmentVariable('CHERE_INVOKING', $prevChere, 'Process')
            [Environment]::SetEnvironmentVariable('MSYSTEM', $prevMsystem, 'Process')
        }
    }

    [hashtable] GetInstalledPackageMap()
    {
        $map = @{}
        if (-not $this.IsBootstrapped())
        {
            return $map
        }

        $lines = $this.InvokeBash('pacman -Q', $true)
        foreach ($line in $lines)
        {
            $parts = $line -split '\s+', 2
            if ($parts.Count -eq 2)
            {
                $map[$parts[0]] = $parts[1]
            }
        }

        return $map
    }

    [PSCustomObject[]] GetPackageStatus()
    {
        $installed = $this.GetInstalledPackageMap()
        $results   = [List[PSCustomObject]]::new()

        foreach ($category in $this.Config.Packages.Keys)
        {
            foreach ($pkg in $this.Config.Packages[$category])
            {
                $hasPkg = $installed.ContainsKey($pkg)
                $results.Add([PSCustomObject]@{
                    Name      = $pkg
                    Category  = $category
                    Installed = $hasPkg
                    Version   = if ($hasPkg) { $installed[$pkg] } else { '-' }
                })
            }
        }

        return $results.ToArray()
    }

    [void] BootstrapMsys2([bool]$ForceReinstall)
    {
        if ($this.IsBootstrapped() -and -not $ForceReinstall)
        {
            return
        }

        if ($ForceReinstall -and (Test-Path -LiteralPath $this.MsysRoot))
        {
            Write-Host 'Removing the existing MSYS2 installation...' -ForegroundColor Yellow
            Remove-Item -LiteralPath $this.MsysRoot -Recurse -Force
        }

        New-Item -ItemType Directory -Force -Path $this.WinEnvDir | Out-Null
        $sfxPath = [Path]::Combine($this.WinEnvDir, 'msys2-base.sfx.exe')

        try
        {
            if (-not (Test-Path -LiteralPath $sfxPath))
            {
                Write-Host 'Downloading portable MSYS2 to .win-env...' -ForegroundColor Cyan
                Invoke-FileDownload -Uri $this.Config.MsysUrl -OutFile $sfxPath
            }

            Write-Host 'Extracting MSYS2...' -ForegroundColor Cyan
            $proc = Start-Process -FilePath $sfxPath -ArgumentList @('-y', "-o$($this.WinEnvDir)") -Wait -NoNewWindow -PassThru
            if ($proc.ExitCode -ne 0)
            {
                throw "MSYS2 extraction failed with exit code $($proc.ExitCode)."
            }

            $null = $this.InvokeBash('true', $false)
            $null = $this.InvokeBash('gpgconf --kill all >/dev/null 2>&1 || true', $false)
        }
        finally
        {
            if (Test-Path -LiteralPath $sfxPath)
            {
                Remove-Item -LiteralPath $sfxPath -Force
            }
        }
    }

    [void] SyncPackages([bool]$UpdateDb)
    {
        $missing = @($this.GetPackageStatus() | Where-Object Installed -eq $false)

        if ($missing.Count -eq 0 -and -not $UpdateDb)
        {
            Write-Host 'All dependencies are already installed; no downloads are required.' -ForegroundColor Green
            return
        }

        $syncDb = [Path]::Combine($this.MsysRoot, 'var', 'lib', 'pacman', 'sync', 'ucrt64.db')
        if ($UpdateDb -or -not (Test-Path -LiteralPath $syncDb))
        {
            Write-Host 'Updating the MSYS2 package database...' -ForegroundColor Cyan
            $null = $this.InvokeBash('pacman --noconfirm -Syuu', $false)
            $null = $this.InvokeBash('pacman --noconfirm -Syuu', $false)
        }

        if ($missing.Count -gt 0)
        {
            $names = [List[string]]::new()
            foreach ($item in $missing)
            {
                $names.Add($item.Name)
            }

            $pkgArgs = $names -join ' '
            Write-Host "Installing $($missing.Count) missing package(s)..." -ForegroundColor Cyan
            $null = $this.InvokeBash("pacman --noconfirm -S --needed $pkgArgs", $false)
        }
    }

    [hashtable] GetEnvironmentVariables([bool]$IsolatedMode)
    {
        $sysRoot     = [Environment]::GetEnvironmentVariable('SystemRoot', 'Process')
        $currentPath = [Environment]::GetEnvironmentVariable('PATH', 'Process')

        $pathPrefix = if ($IsolatedMode)
        {
            "$($this.UcrtBin);$sysRoot\System32;$sysRoot"
        }
        else
        {
            "$($this.UcrtBin);$currentPath"
        }

        return @{
            PATH            = $pathPrefix
            CC              = 'gcc'
            CXX             = 'g++'
            PKG_CONFIG      = [Path]::Combine($this.UcrtBin, 'pkgconf.exe')
            PKG_CONFIG_PATH = "$([Path]::Combine($this.UcrtRoot, 'lib', 'pkgconfig'));$([Path]::Combine($this.UcrtRoot, 'share', 'pkgconfig'))"
            VK_LAYER_PATH   = $this.UcrtBin
            CCACHE_DIR      = $this.CcacheDir
        }
    }

    [int] EnterShell([string]$ShellCommand, [bool]$HasCommand, [bool]$IsolatedMode)
    {
        if (-not (Test-Path -LiteralPath $this.UcrtBin))
        {
            throw "The UCRT64 environment was not found at '$($this.UcrtBin)'."
        }

        if (-not $HasCommand -and [Environment]::GetEnvironmentVariable('LUMEN_SHELL', 'Process'))
        {
            Write-Host 'You are already inside a Lumen shell (type exit to leave it).' -ForegroundColor Yellow
            return 0
        }

        $shellExe = [System.Diagnostics.Process]::GetCurrentProcess().MainModule.FileName

        $envVars = $this.GetEnvironmentVariables($IsolatedMode)
        $envVars['LUMEN_SHELL'] = '1'

        $previous = @{}
        $exitCode = 0

        foreach ($key in $envVars.Keys)
        {
            $previous[$key] = [Environment]::GetEnvironmentVariable($key, 'Process')
            [Environment]::SetEnvironmentVariable($key, $envVars[$key], 'Process')
        }

        try
        {
            if ($HasCommand)
            {
                $script  = "$ShellCommand`nexit `$LASTEXITCODE"
                $encoded = [Convert]::ToBase64String([System.Text.Encoding]::Unicode.GetBytes($script))
                $psArgs  = @('-NoLogo', '-NoProfile', '-EncodedCommand', $encoded)
            }
            else
            {
                $initScript = @'
function global:prompt
{
    "[Lumen:win] $(Get-Location)> "
}
function global:Show-LumenVersions
{
    gcc --version | Select-Object -First 1
    meson --version
}
Write-Host "Lumen dev shell ready. Type 'exit' to leave it. (Show-LumenVersions for tool versions)" -ForegroundColor Green
'@
                $encoded = [Convert]::ToBase64String([System.Text.Encoding]::Unicode.GetBytes($initScript))
                $psArgs  = @('-NoLogo', '-NoProfile', '-NoExit', '-EncodedCommand', $encoded)
            }

            $proc     = Start-Process -FilePath $shellExe -ArgumentList $psArgs -NoNewWindow -Wait -PassThru
            $exitCode = $proc.ExitCode
        }
        finally
        {
            foreach ($key in $previous.Keys)
            {
                [Environment]::SetEnvironmentVariable($key, $previous[$key], 'Process')
            }
        }

        return $exitCode
    }

}

. (Join-Path -Path $PSScriptRoot -ChildPath 'Private/Deps.ps1')
. (Join-Path -Path $PSScriptRoot -ChildPath 'Private/Invoke-FileDownload.ps1')
. (Join-Path -Path $PSScriptRoot -ChildPath 'Public/Invoke-LumenSetup.ps1')

Export-ModuleMember -Function Invoke-LumenSetup
