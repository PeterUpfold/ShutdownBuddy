#
# Uninstall ShutdownBuddy
#
#
Param(
    [bool]$RemoveConfig = $false
)

$VerbosePreference="Continue"
$ErrorActionPreference="Continue"

$sdbPath = "C:\Program Files\upfold.org.uk\ShutdownBuddy"
$regRoot = "HKLM:\SOFTWARE\upfold.org.uk\ShutdownBuddy"

Stop-Service ShutdownBuddy

Remove-Item "$sdbPath" -Force -Recurse

if ($RemoveConfig) {
    Remove-Item $regRoot -Force
}

sc.exe delete ShutdownBuddy
