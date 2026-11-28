$NATIVE_FILE = "Meson/Native.ini"

$RepoRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path

function Test-SanitizerSupport
{
    param (
        [string]$Sanitize
    )

    # Meson probes sanitizer support at link time. MinGW GCC (MSYS2 UCRT64)
    # ships no libasan/libubsan/libtsan, so -Db_sanitize=... fails at setup.
    # Probe with the same compilers Meson uses; ccache only forwards calls.
    $CC  = Get-Command 'gcc' -ErrorAction SilentlyContinue
    $CXX = Get-Command 'g++' -ErrorAction SilentlyContinue
    if ($null -eq $CC -or $null -eq $CXX)
    {
        return $false
    }

    $ProbeDir = Join-Path ([System.IO.Path]::GetTempPath()) ("lumen-sanitize-" + [guid]::NewGuid().ToString('N'))
    try
    {
        New-Item -ItemType Directory -Path $ProbeDir -Force | Out-Null
        Set-Content -LiteralPath (Join-Path $ProbeDir 'probe.c')   -Value 'int main(void) { return 0; }' -Encoding ASCII
        Set-Content -LiteralPath (Join-Path $ProbeDir 'probe.cpp') -Value 'int main() { return 0; }' -Encoding ASCII

        foreach ($Probe in @(
            @{ Compiler = $CC.Source;  Source = 'probe.c' },
            @{ Compiler = $CXX.Source; Source = 'probe.cpp' }
        ))
        {
            & $Probe.Compiler (Join-Path $ProbeDir $Probe.Source) -o (Join-Path $ProbeDir 'probe.exe') "-fsanitize=$Sanitize" 2>&1 | Out-Null
            if ($LASTEXITCODE -ne 0)
            {
                return $false
            }
        }
        return $true
    }
    finally
    {
        Remove-Item -LiteralPath $ProbeDir -Recurse -Force -ErrorAction SilentlyContinue
    }
}

function InvokeMeson
{
    param (
        [string]$BuildDir,
        [string]$BuildType,
        [string]$Sanitize
    )

    $EnableBenchmarking = if ($BuildType -eq "release") { "true" } else { "false" }

    $MesonArgs = @(
        "setup", $BuildDir,
        "--buildtype=$BuildType",
        "-Denable_testing=true",
        "-Denable_benchmarking=$EnableBenchmarking"
    )

    if (Test-Path -LiteralPath (Join-Path $BuildDir 'build.ninja'))
    {
        $MesonArgs += "--reconfigure"
    }

    $MesonArgs += "--native-file=$NATIVE_FILE"

    if ($Sanitize -ne "")
    {
        $MesonArgs += "-Db_sanitize=$Sanitize"
    }

    $StartInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $StartInfo.FileName        = 'meson'
    $StartInfo.Arguments       = ( $MesonArgs | ForEach-Object { '"' + ( $_ -replace '"', '\"' ) + '"' } ) -join ' '
    $StartInfo.UseShellExecute = $false
    $StartInfo.WorkingDirectory = $RepoRoot

    return [System.Diagnostics.Process]::Start( $StartInfo )
}

$AsanSupported = Test-SanitizerSupport 'address,undefined'
$TsanSupported = Test-SanitizerSupport 'thread'

if (-not $AsanSupported)
{
    Write-Warning "No ASan/UBSan runtime in this toolchain: Build is configured as plain Debug (no -Db_sanitize)."
}
if (-not $TsanSupported)
{
    Write-Warning "No TSan runtime in this toolchain: Build-TSAN is skipped. Sanitizer builds require the Linux/Nix devshell."
}

$Processes = @()

$Processes += InvokeMeson "Build"         "debug"   $(if ($AsanSupported) { "address,undefined" } else { "" })
if ($TsanSupported)
{
    $Processes += InvokeMeson "Build-TSAN"    "debug"   "thread"
}
$Processes += InvokeMeson "Build-Release" "release" ""

$Status = 0
foreach ($Process in $Processes)
{
    $Process.WaitForExit()
    if ($Process.ExitCode -ne 0)
    {
        $Status = 1
    }
}

exit $Status
