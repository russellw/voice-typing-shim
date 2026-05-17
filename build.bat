@echo off
cl /nologo /W4 /O2 /EHsc main.cpp /link /SUBSYSTEM:WINDOWS user32.lib comctl32.lib gdi32.lib
