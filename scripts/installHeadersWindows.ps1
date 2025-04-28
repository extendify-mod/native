$ErrorActionPreference = "Stop"
#Requires -Modules Pscx
# Install-Module Pscx -Scope CurrentUser -AllowClobber
Push-Location $PSScriptRoot
# Should be installed on all systems (see: https://techcommunity.microsoft.com/blog/containers/tar-and-curl-come-to-windows/382409), since we are using it for unzip, which GNU tar doesnt support
# we use the absolute path to avoid any other tar
$tar = "C:\Windows\System32\tar.exe"
Get-Command "curl" -ErrorAction Stop > $null
Get-Command "git" -ErrorAction Stop > $null
Get-Command "cmake" -ErrorAction Stop > $null
Get-Command "make" -ErrorAction Stop > $null
Get-Command "patch" -ErrorAction Stop > $null
# needed because diff is a default alias from powershell
[System.Management.Automation.CommandInfo[]] $diffCmd = Get-Command "diff" -CommandType Application -ErrorAction SilentlyContinue
if ($null -eq $diffCmd[0]) {
    throw "Diff is not installed. NOTE: powershell has a default alias called diff, which is not diff";
}
[String] $diff = $diffCmd[0].Source
function mktempName {
    $tmp = [System.IO.Path]::GetTempPath() # Not $env:TEMP, see https://stackoverflow.com/a/946017
    $name = (New-Guid).ToString("N")
    (Join-Path $tmp $name)
}
[String] $version = (Select-String -Path .\..\nix\libcefPatched.nix -Pattern '(?<=\bversion ?= ?").+(?=";)').Matches[0].Value
[String] $gitRev = (Select-String -Path .\..\nix\libcefPatched.nix -Pattern '(?<=\bgitRevision ?= ?").+(?=";)').Matches[0].Value
[String] $chromiumVersion = (Select-String -Path .\..\nix\libcefPatched.nix -Pattern '(?<=\bchromiumVersion ?= ?").+(?=";)').Matches[0].Value

[System.Runtime.InteropServices.Architecture] $arch = [System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture
[String] $platformStr;
[String] $headersPath = Join-Path $PSScriptRoot ".." "windowsHeaders";
[String] $libsPath = Join-Path $PSScriptRoot ".." "windowsLibs";
# module paths
[String] $spdlogPath = Join-Path $PSScriptRoot ".." "spdlog";
if (Test-Path $headersPath) {
    throw "Headers already installed";
}
elseif (Test-Path $libsPath) {
    throw "Libs already installed";
}
elseif (Test-Path .\..\detours) {
    throw "Detours already downloaded";
}
elseif (Test-Path $spdlogPath) {
    throw "spdlog already downloaded";
}

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
[String] $url = "https://cef-builds.spotifycdn.com/$basepath.tar.bz2"
[String] $tempfile = "..\tmp.tar.bz2"
try {
    curl $url -o $tempfile
    if (!$?) {
        throw "Curl failed"
    }
    New-Item -ItemType Directory -Path .\..\cef_build_temp -ErrorAction Stop
    New-Item -ItemType Directory -Path $headersPath -ErrorAction Stop
    New-Item -ItemType Directory -Path $libsPath -ErrorAction Stop
    # Windows tar is painfully slow
    # i was able to install a multi threaded version of bzip and extract it and by the time i was done, it was still extracting
    # it took 35 minutes in the time it took pbzip2 18 seconds
    # If pbzip2 is in the PATH, use it instead
    Write-Output "Extracting Archive"
    if ($null -ne (Get-Command pbzip2 -ErrorAction SilentlyContinue)) {
        # 2G ram, decmopress, keep orig. archive file, output to stdout | untar
        pbzip2 -m2000 -d -k -c $tempfile | & $tar -v --directory ./../cef_build_temp/ --file - --strip-components 1 --extract
    }
    else {
        & $tar -v --directory ./../cef_build_temp/ --file $tempfile --strip-components 1 --bzip2 --extract
    }
    Write-Output "Copying headers and libcef"
    Copy-Item -Path .\..\cef_build_temp\include\* -Destination $headersPath -Recurse
    Copy-Item -Path .\..\cef_build_temp\Release\* -Destination $libsPath -Recurse
    New-Item -Type SymbolicLink -Path $headersPath\include -Value .
    Write-Output "Finished writing headers and libcef"
    Write-Output "Starting build of libcef_dll_wrapper"
    try {
        Push-Location .\..\cef_build_temp
        patch .\cmake\cef_variables.cmake ..\clangCL.patch
        if (!$?) {
            throw "Failed to patch for clang-cl";
        }
        cmake -G "Unix Makefiles" -B dist -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl -DCMAKE_BUILD_TYPE=Release -DCMAKE_MT=llvm-mt
        make -j -C dist libcef_dll_wrapper
        Copy-Item -Path .\dist\libcef_dll_wrapper\libcef_dll_wrapper.lib $libsPath
        Write-Output "Build libcef_dll_wrapper"
    }
    finally {
        Pop-Location
    }
}
catch {
    Write-Output $_
    throw "Failed to setup libcef"
}
finally {
    Remove-Item $tempfile -ErrorAction Continue
    Remove-Item .\..\cef_build_temp -Recurse -Force -ErrorAction Continue
    Pop-Location
}

# TODO: do this in the nix build
$DETOURS_SHA = "c3ffaef25a26dc9788d52da24956bb610d27cff4"
Push-Location $PSScriptRoot

New-Item -ItemType Directory -Path .\..\windowsLibs -ErrorAction SilentlyContinue
try {
    git clone --depth 1 --progress https://github.com/microsoft/Detours.git ..\detours
    Push-Location ..\detours
    git fetch origin $DETOURS_SHA
    git checkout --detach $DETOURS_SHA
    Invoke-BatchFile "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" $([System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture.ToString())
    Push-Location src
    nmake
    $DetoursSourceDir = $(Get-Location).Path
    Pop-Location
    $maybeoutdir = (Get-ChildItem -Path . -Filter lib.*)
    if ($maybeoutdir.Length -ne 1) {
        throw "more one than one dir matched lib.*"
    }
    $outpath = $maybeoutdir[0].FullName
    Pop-Location
    Copy-Item -Path $outpath\* -Destination $libsPath
    Copy-Item -Path $DetoursSourceDir\*.h -Destination $headersPath
}
finally {
    Pop-Location
    Remove-Item -Recurse -Force $(Join-Path $PSScriptRoot "..\detours")
}

[String] $spdlogHash = "548b264254b7cbbf68f9003315ea958edacb91e5";
Push-Location $PSScriptRoot
Write-Output "Starting build of spdlog"
try {
    git clone --depth 1 --progress https://github.com/gabime/spdlog.git $spdlogPath
    Write-Output "spdlog cloned successfully"
    Push-Location $spdlogPath
    try {
        git fetch origin $spdlogHash
        git checkout --detach $spdlogHash
        Write-Output "spdlog checked out successfully"
        cmake -G "Unix Makefiles" -B build -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release
        make -j -C build spdlog
        [String] $outfile = (Join-Path $spdlogPath "build" "spdlog.lib")
        if (-not (Test-Path $outfile)) {
            throw "spdlog.lib not found"
        }
        Copy-Item -Path $outfile -Destination $libsPath
        Write-Output "spdlog built successfully"
        Copy-Item -Path $spdlogPath\include\* -Destination $headersPath -Recurse
        Write-Output "spdlog headers copied successfully"
    } catch {
        Write-Output $_
        throw "Failed to clone spdlog"
    } finally {
        Pop-Location
        Remove-Item -Recurse -Force $spdlogPath
    }
    Write-Output "Finished building spdlog"
}
finally {
    Pop-Location
}