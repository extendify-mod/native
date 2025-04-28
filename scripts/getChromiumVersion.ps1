$ErrorActionPreference = "Stop"
#Requires -Modules Pscx
Push-Location $PSScriptRoot
[String] $version = (Select-String -Path .\nix\libcefPatched.nix -Pattern '(?<=\bversion ?= ?").+(?=";)').Matches[0].Value
[String] $gitRev = (Select-String -Path .\nix\libcefPatched.nix -Pattern '(?<=\bgitRevision ?= ?").+(?=";)').Matches[0].Value
[String] $chromiumVersion = (Select-String -Path .\nix\libcefPatched.nix -Pattern '(?<=\bchromiumVersion ?= ?").+(?=";)').Matches[0].Value
Write-Output "Version: $version"
Write-Output "GitRev: g$gitRev"
Write-Output "Chromium Version: $chromiumVersion"
[System.Runtime.InteropServices.Architecture] $arch = [System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture
[String] $platformStr;
switch ($arch) {
    "Arm64" {
        $platformStr = "windowsarm64";
        break;
    }
    "X64" {
        $platformStr = "windows64";
        break;
    }
    Default {
        throw "Unsupported architecture: $arch"
    }
}
# curl https://cef-builds.spotifycdn.com/cef_binary_128.4.12%2Bg1d7a1f9%2Bchromium-128.0.6613.138_windowsarm64_minimal.tar.bz2 | tar --extract cef_binary_128.4.12+g1d7a1f9+chromium-128.0.6613.138_windowsarm64_minimal/include --strip-components 2 --bzip2 --file - --directory .\windowsHeaders\
[String] $basepath = "cef_binary_$version+g$gitRev+chromium-${chromiumVersion}_$platformStr"
Write-Output "Basepath: $basepath"
[String] $url = "https://cef-builds.spotifycdn.com/$basepath.tar.bz2"
Write-Output "Archive URL: $url"
Pop-Location
