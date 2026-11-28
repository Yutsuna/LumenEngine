# BuildMeson.ps1
# Script d'invocation parallèle de Meson sous Windows

$NATIVE_FILE = "Meson/Native.ini"

function InvokeMeson {
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

    if ($Sanitize -ne "") {
        $MesonArgs += "-Db_sanitize=$Sanitize"
    }

    # Création d'un fichier temporaire pour stderr (requis par PowerShell)
    $TempErrorLog = [System.IO.Path]::GetTempFileName()

    # Lancement en arrière-plan avec redirections séparées
    $process = Start-Process -FilePath "meson" -ArgumentList $MesonArgs -NoNewWindow -PassThru -RedirectStandardOutput NUL -RedirectStandardError $TempErrorLog

    # Nettoyage asynchrone du fichier temporaire une fois le processus terminé
    $process | ForEach-Object {
        $proc = $_
        Register-ObjectEvent -InputObject $proc -EventName Exited -Action {
            Remove-Item $TempErrorLog -ErrorAction SilentlyContinue
            Unregister-Event -SourceIdentifier $Event.SourceIdentifier
        } | Out-Null
    }


    return $process
}

$Processes = @()

$Processes += InvokeMeson "Build"         "debug"   "address,undefined"
$Processes += InvokeMeson "Build-TSAN"    "debug"   "thread"
$Processes += InvokeMeson "Build-Release" "release" ""

$Status = 0
foreach ($proc in $Processes) {
    $proc.WaitForExit()
    if ($proc.ExitCode -ne 0) {
        $Status = 1
    }
}

exit $Status
