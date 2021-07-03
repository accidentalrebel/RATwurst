@echo off
pushd X:\build
call shell.bat
cl /Zi X:\code\ratwurst.cpp Advapi32.lib
popd
