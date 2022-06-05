# ShutdownBuddy

Detect the computer being idle, based on zero interactive signed in Windows sessions, and shut down the computer fully if
there continually are no interactive sessions open.

Experimental.

## Test Installation

Ensure that the VC redist for 

    sc.exe create ShutdownBuddy binPath= "\path\to\ShutdownBuddy.exe"
    Start-Service ShutdownBuddy

## Configuration

Configuration will use the Windows registry.

    HKLM\SOFTWARE\upfold.org.uk\ShutdownBuddy

Possible config options in this registry key:

`DebugLog` -- DWORD. Set to `1` to log activity to a temporary file in `C:\WINDOWS\TEMP\Sdb*.tmp`

`EvaluationIntervalSeconds` -- DWORD. How frequently, in seconds, to evaluate for interactive sessions.

`ShutdownAfterIdleForSeconds` -- DWORD. How many seconds of idle computer (i.e. no interactive sessions) before issuing a shutdown. This is periodically evaluated as above.