#
# Install ShutdownBuddy
#
#
Param(
    [bool]$DebugLog = $false,
    [int]$EvaluationIntervalSeconds = 60,
    [int]$ShutdownAfterIdleForSeconds = 3600
)


$VerbosePreference="Continue"
$ErrorActionPreference="Continue"

$sdbPath = "C:\Program Files\upfold.org.uk\ShutdownBuddy"
$regRoot = "HKLM:\SOFTWARE\upfold.org.uk\ShutdownBuddy"

try {
    New-Item -ItemType Directory $sdbPath -Force
}
catch {
    Write-Host "$sdbPath already exists, or no permissions?"
}

if (-not (Test-Path $regRoot)) {
    New-Item $regRoot -Force
}

if ($DebugLog) {
    Set-ItemProperty -Path $regRoot -Name "DebugLog" -Value 1 -Force
}
else {
    Set-ItemProperty -Path $regRoot -Name "DebugLog" -Value 0 -Force
}

Set-ItemProperty -Path $regRoot -Name "EvaluationIntervalSeconds" -Value $EvaluationIntervalSeconds -Force
Set-ItemProperty -Path $regRoot -Name "ShutdownAfterIdleForSeconds" -Value $ShutdownAfterIdleForSeconds -Force

# copy files
Copy-Item ShutdownBuddy.exe "$sdbPath" -Force

# service installation and config
sc.exe create ShutdownBuddy binPath= "$sdbPath\ShutdownBuddy.exe"
sc.exe config ShutdownBuddy start= delayed-auto

Start-Service ShutdownBuddy