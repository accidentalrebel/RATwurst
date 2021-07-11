# RATwurst

![logo](https://github.com/accidentalrebel/RATwurst/raw/master/images/ratwurst.png)

The aim of this project is for me to learn about the techniques used by malware by making a Remote Access Tool.

The more I understand the inner workings of malware and the reasoning behind how they were built, the better I can protect against them.

Only used for educational purposes. See disclaimer below.

## Noteworthy features (so far)

* Runtime loading of DLLs
* Anti-virus evasion via simple strings obfuscation
* Deletes itself and runs from temporary folder
* Anti-debugging via rdtsc timing

## TODOs

* [x] Client-server communication via sockets
* [x] Multiple clients
* [x] Fetch client host information
* [x] Shutdown client
* [x] Remote command execution
* [x] Client to server file transfer
* [x] Server to client file transfer
* [x] Persistence
* [x] Encrypt messages
* [x] Anti-debugging
* [ ] Anti-emulation/Anti-sandbox
* [ ] and more... (?)

## How to build

Currently only works with Visual Studio 2019 with Visual C++ build tools. Makes use of the MVSC Compiler.

Check out `build.bat` under the tools folder on how a build is done.

## Resources

These resources helped me a lot when developing this project:

  * [ParadoxiaRAT](https://github.com/quantumcore/paradoxiaRAT) - Native Windows Remote access Tool project
  * [Ghost](https://github.com/AHXR/ghost) -  RAT (Remote Access Trojan) - Silent Botnet - Full Remote Command-Line Access - Download & Execute Programs - Spread Virus' & Malware
  * [DarkRAT](https://github.com/yatt-ze/The-Collection/tree/master/Source%20Codes/Botnets/DarkRat%20Loader/derkrut) - DarkRAT loader leaked source code
  * [WinAPI-Tricks](https://github.com/vxunderground/WinAPI-Tricks) - Collection of various WINAPI tricks / features used or abused by Malware
  * [Engineering Anti-Virus Evasion](https://blog.scrt.ch/2020/07/15/engineering-antivirus-evasion-part-ii/) - Blog post about anti-virus evasion techniques for malware
  * [Sandbox detection and evasion techniques](https://www.ptsecurity.com/ww-en/analytics/antisandbox-techniques/) - Research that shows how sandbox evasion techniques have evolved in the last 10 years.

## Disclaimer

Usage of this tool for attacking targets without prior mutual consent is illegal.
It is the end user's responsibility to obey all applicable local, state, federal, and international laws.
The developers to this repository assume no liability and are not responsible for any misuse or damage caused by this program.
