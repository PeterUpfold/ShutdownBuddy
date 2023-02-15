# ShutdownBuddy

Detect the computer being idle, based on having no interactive signed in Windows sessions, and shut down the computer fully after a configurable period of time.

Machines running ShutdownBuddy will "properly" shut down when no-one is signed in, and not just sleep or hibernate.

## Installation

An installer is provided in releases.

## Silent installation

To perform a silent installation, use the installer and provide the `/verysilent` switch. You may also set the configuration parameters to set in the registry as the example below.

    install-ShutdownBuddy.exe /verysilent /EvaluationIntervalSeconds=60 /ShutdownAfterIdleForSeconds=3600 /DebugLog=0

## Silent uninstallation

To uninstall, run the uninstaller:

    "C:\Program Files\ShutdownBuddy\unins000.exe" /verysilent

## Configuration

Configuration uses the Windows registry.

    HKLM\SOFTWARE\upfold.org.uk\ShutdownBuddy

Possible config options in this registry key:

`DebugLog` -- DWORD. Set to `1` to log activity to a temporary file in `C:\WINDOWS\TEMP\Sdb*.tmp`

`EvaluationIntervalSeconds` -- DWORD. How frequently, in seconds, to evaluate for interactive sessions.

`ShutdownAfterIdleForSeconds` -- DWORD. How many seconds of idle computer (i.e. no interactive sessions) before issuing a shutdown. This is periodically evaluated as above.
