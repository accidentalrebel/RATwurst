@echo off
pushd X:\build
call shell.bat
cl /W4 /Zi X:\code\ratwurst.cpp
popd
