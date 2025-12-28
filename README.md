# Siren1
Direct Device-to-Device File Sharing

# Important
Working in a distributed network, play by its rules. Install the client and server on your device. Remember, a holistic node is when a device is able to transmit and receive.

# Build
1. Install Tools:
Deb:
```
#   apt install build-essential cmake
```
RPM:
```
#   dnf groupinstall "Development Tools" 
#   dnf install cmake
```
How to patch this for FreeBSD?
```
#   pkg install cmake
```
Android:
Use Termux.
```
$   pkg install make clang cmake
```
Windows: use qmake.

2. Begin:
```
$   cd siren1/<*>/build/template
$   mkdir build
$   cd build
$   cmake ..
$   make
```
* - is where to insert "sirenC" and "sirenS".

# How to start
1. Start demon Yggdrasil.
2. You need to know the IPv6 address of the receiver and its port.
```
$   ./sirenC <IPv6 adress> <port> <file1 file2>
$   ./sirenS <port>
```
# Does it work without the Internet?
Yes.
Tested on older IBSS devices. To quickly enter mesh mode, use powermesh.sh (to exit mesh mode RESTART NetworkManager).

