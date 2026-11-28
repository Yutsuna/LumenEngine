$NATIVE_FILE = "Meson/Native.ini"

$RepoRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path

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
        "-Denable_benchmarking=$EnableBenchmarking",
        "--reconfigure",
        "--native-file=$NATIVE_FILE"
    )

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

$Processes = @()

$Processes += InvokeMeson "Build"         "debug"   "address,undefined"
$Processes += InvokeMeson "Build-TSAN"    "debug"   "thread"
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
