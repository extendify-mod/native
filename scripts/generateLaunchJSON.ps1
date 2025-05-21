Write-Output "Finding Spotify executable";
Push-Location $env:APPDATA;
if (-not (Test-Path -Path .\Spotify\Spotify.exe)) {
    Write-Error "Spotify.exe not found in $PWD, please install Spotify first.";
}
[String] $spotify = Resolve-Path -Path .\Spotify\Spotify.exe;
Pop-Location;
Write-Output "Finished finding Spotify executable";
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