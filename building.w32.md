# Install and add to path
- llvm
- cmake
- ninja
- patch (unix command, can be installed with git by adding unix tools to path in the installer)
## Windows 10 SDK
https://developer.microsoft.com/en-us/windows/downloads/windows-sdk/
## visual studio build tools (scroll down on https://visualstudio.microsoft.com/downloads/)
install
- MSVC v143 build tools for your arch. You can start searching `MSVC v143 - VS <YOUR_VS_VERSION_HERE> C++` to easily find it
# Building
## Configure
```ps1
cmake -GNinja -B build
```
## Build
```ps1
ninja -C build
```