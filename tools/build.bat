@echo off
pushd X:\build
call shell.bat
cl /DDEBUG=1 /W4 /Zi X:\code\ratwurst.cpp
popd
