$ErrorActionPreference = "Stop";
Push-Location $PSScriptRoot\..;
if (-not (Test-Path -Path .\build\setdll.exe)) {
    Write-Error "setdll.exe not found, please build the project first. For more information on building, please see building.w32.md";
}
[String] $setdll = Resolve-Path -Path .\build\setdll.exe;
if (-not (Test-Path -Path .\build\extendify_wrapper.dll)) {
    Write-Error "extendify_wrapper.dll not found, please build the project first. For more information on building, please see building.w32.md";
}
[String] $extendify_wrapper = Resolve-Path -Path .\build\extendify_wrapper.dll;

Pop-Location;
Push-Location $env:APPDATA;
if (-not (Test-Path -Path .\Spotify\Spotify.exe)) {
    Write-Error "Spotify.exe not found in $PWD, please install Spotify first.";
}
[String] $spotify = Resolve-Path -Path .\Spotify\Spotify.exe;
Write-Output "Removing old DLLs";
& $setdll /r $spotify;
Write-Output "Removed old DLLs";
Write-Output "Injecting new DLL: $extendify_wrapper";
& $setdll /d:$extendify_wrapper $spotify;
Write-Output "Injected new DLL";
Pop-Location;
Write-Output "Generating vscode launch configuration";
$baseConfig = Get-Content -Path $PSScriptRoot\launch.json -Raw | ConvertFrom-Json -Depth 100;
$baseConfig.configurations | ForEach-Object {
    $_.program = $spotify;
}
# use gdb-multiarch for arm64 as normal gdb does not support arm64
if ([System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture -eq [System.Runtime.InteropServices.Architecture]::Arm64) {
    $baseConfig.configurations | ForEach-Object {
        if ($_.miDebuggerPath -eq "gdb") {
            $_.miDebuggerPath = "gdb-multiarch";
        }
    }
}
ConvertTo-Json -InputObject $baseConfig -Depth 100 | Set-Content -Path $PSScriptRoot\..\.vscode\launch.json -Force;
Write-Output "Generated vscode launch configuration";