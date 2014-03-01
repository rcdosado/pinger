@echo off

echo Your directory is cleanse...
rc.exe ping.rc
cl /GS- /GA /MD /O1 /FdPing ping.c /link /FILEALIGN:512 /ENTRY:main  /SUBSYSTEM:WINDOWS /MERGE:.data=.text /MERGE:.rdata=.text /SECTION:.text,EWR /IGNORE:4078 /MACHINE:IX86 ping.res
del *.bak
del *.obj
del *.aps
pause
exit