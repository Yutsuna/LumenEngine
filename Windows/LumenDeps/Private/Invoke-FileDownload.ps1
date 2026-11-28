<#
.SYNOPSIS
Downloads a file from a URI to a local path.

.DESCRIPTION
Downloads the resource specified by the URI to the destination file. Temporarily
suppresses PowerShell's progress display and restores the previous setting when
the download completes or fails.

.PARAMETER Uri
Specifies the URI of the resource to download.

.PARAMETER OutFile
Specifies the destination path for the downloaded file.
#>
#Requires -Version 5.1

function Invoke-FileDownload
{
    [CmdletBinding()]
    param
    (
        [Parameter(Mandatory)]
        [string]$Uri,

        [Parameter(Mandatory)]
        [string]$OutFile
    )

    $prev = $ProgressPreference
    try
    {
        $ProgressPreference = 'SilentlyContinue'
        Invoke-WebRequest -Uri $Uri -OutFile $OutFile -UseBasicParsing
    }
    finally
    {
        $ProgressPreference = $prev
    }
}
