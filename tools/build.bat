@echo off
pushd X:\build
call shell.bat
cl /DDEBUG=%1 /Wall /Zi X:\code\ratwurst.c
popd
