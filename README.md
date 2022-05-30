# ShutdownBuddy

Detect interactive sessions, run a timer and then shut down when the timer expires still with no interactive
sessions running.

Experimental.

## Test Installation

Ensure that the VC redist for 

    sc.exe create ShutdownBuddy binPath= "\path\to\ShutdownBuddy.exe"
    Start-Service ShutdownBuddy