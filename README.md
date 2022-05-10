# ShutdownBuddy

Detect interactive sessions, run a timer and then shut down when the timer expires still with no interactive
sessions running.

Experimental.

## Test Installation

    sc.exe create ShutdownBuddy binPath= "\path\to\ShutdownBuddy.exe"
    Start-Service ShutdownBuddy