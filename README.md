# RATwurst

The aim of this project is for me to learn about the techniques used by malware by making a Remote Access Tool.

The more I understand the inner workings of malware and the reasoning behind how they were built, the better I can protect against them.

Only used for educational purposes. See disclaimer below.

## Noteworthy features (so far)

* Runtime loading of DLLs
* Anti-virus evasion via simple strings obfuscation

## TODOs

* [x] Client-server communication via sockets
* [x] Multiple clients
* [ ] Fetch client host information
* [ ] Encrypt messages
* [ ] Server to client file transfer
* [ ] Client to server file transfer
* [ ] Persistence
* [ ] Anti-debugging
* [ ] Anti-emulation/Anti-sandbox
* [ ] and more... (?)

## How to build

Currently only works with Visual Studio 2019 with Visual C++ build tools. Makes use of the MVSC Compiler.

Check out `build.bat` under the tools folder on how a build is done.

## Disclaimer

Usage of this tool for attacking targets without prior mutual consent is illegal.
It is the end user's responsibility to obey all applicable local, state, federal, and international laws.
The developers to this repository assume no liability and are not responsible for any misuse or damage caused by this program.
