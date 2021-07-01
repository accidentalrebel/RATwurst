@echo off
pushd X:\build
call shell.bat
cl /Zi X:\code\ratwurst.cpp
popd
