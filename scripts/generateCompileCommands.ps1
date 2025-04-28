$ErrorActionPreference = "Stop"
[System.IO.FileInfo[]] $paths = Get-ChildItem -Path dist -Filter *.json -Name -Recurse -ErrorAction SilentlyContinue
if ($paths.Length -eq 0) {
    Write-Error "No compile_commands.json files found in dist directory."
    exit 1
}
$paths = $paths | ForEach-Object { "dist/$_" }
"[$(Get-Content $paths)]" -replace '.{2}$', ']' > .\compile_commands.json
