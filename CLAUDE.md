voice-typing-shim adds an editing layer to Windows voice typing
It creates a window containing a large edit box
It is written in C++ using plain Win32
And compiled from the Windows command line
When the edit box is created or otherwise gains focus, it initiates Windows voice typing, as if the user had pressed Win-H
The most reliable way to do this is probably the Win32 SendInput API 
