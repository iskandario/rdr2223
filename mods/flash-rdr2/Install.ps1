[CmdletBinding()]
param([string]$GameDirectory)

$ErrorActionPreference = 'Stop'

try {
    Write-Host 'FlashRDR2 0.2.0 - EXPERIMENTAL - STORY MODE ONLY' -ForegroundColor Yellow

    foreach ($file in @('FlashRDR2.asi','FlashRDR2.ini','SHA256SUMS.txt')) {
        if (-not (Test-Path -LiteralPath (Join-Path $PSScriptRoot $file) -PathType Leaf)) {
            throw "Missing $file. Extract the complete Windows ZIP first."
        }
    }

    $expected = @{}

    foreach ($line in Get-Content -LiteralPath (Join-Path $PSScriptRoot 'SHA256SUMS.txt')) {
        if ($line -match '^([A-Fa-f0-9]{64})  (.+)$') {
            $expected[$Matches[2]] = $Matches[1]
        }
    }

    foreach ($file in @('FlashRDR2.asi','FlashRDR2.ini')) {
        $actual = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $PSScriptRoot $file)).Hash

        if (-not $expected.ContainsKey($file) -or $actual -ne $expected[$file]) {
            throw "Checksum mismatch: $file"
        }
    }

    if ([string]::IsNullOrWhiteSpace($GameDirectory)) {
        Add-Type -AssemblyName System.Windows.Forms

        $dialog = New-Object System.Windows.Forms.OpenFileDialog
        $dialog.Title = 'Select RDR2.exe'
        $dialog.Filter = 'RDR2 executable (RDR2.exe)|RDR2.exe'

        try {
            if ($dialog.ShowDialog() -ne [System.Windows.Forms.DialogResult]::OK) {
                throw 'Cancelled.'
            }

            $GameDirectory = Split-Path -Parent $dialog.FileName
        }
        finally {
            $dialog.Dispose()
        }
    }

    $GameDirectory = (Resolve-Path -LiteralPath $GameDirectory).Path

    if (-not (Test-Path -LiteralPath (Join-Path $GameDirectory 'RDR2.exe') -PathType Leaf)) {
        throw 'Selected folder is not RDR2.'
    }

    if (Get-Process -Name RDR2 -ErrorAction SilentlyContinue) {
        throw 'Exit RDR2 first.'
    }

    $hook = Test-Path -LiteralPath (Join-Path $GameDirectory 'ScriptHookRDR2.dll') -PathType Leaf

    $loader =
        (Test-Path -LiteralPath (Join-Path $GameDirectory 'dinput8.dll') -PathType Leaf) -or
        (Test-Path -LiteralPath (Join-Path $GameDirectory 'version.dll') -PathType Leaf)

    if (-not $hook -or -not $loader) {
        throw 'ScriptHookRDR2 or ASI loader missing.'
    }

    foreach ($file in @('FlashRDR2.asi','FlashRDR2.ini')) {
        if (Test-Path -LiteralPath (Join-Path $GameDirectory $file)) {
            throw "$file already exists. Remove/back it up first."
        }
    }

    Write-Host "Game folder: $GameDirectory"
    Write-Host 'F6 = Flash mode ON/OFF'
    Write-Host 'W + Shift = super speed'
    Write-Host 'F9 = emergency reset'
    Write-Host 'Do NOT enter Red Dead Online with ASI mods installed.' -ForegroundColor Yellow

    if ((Read-Host 'Type INSTALL to continue') -cne 'INSTALL') {
        throw 'Cancelled.'
    }

    [IO.File]::Copy(
        (Join-Path $PSScriptRoot 'FlashRDR2.asi'),
        (Join-Path $GameDirectory 'FlashRDR2.asi'),
        $false
    )

    [IO.File]::Copy(
        (Join-Path $PSScriptRoot 'FlashRDR2.ini'),
        (Join-Path $GameDirectory 'FlashRDR2.ini'),
        $false
    )

    Write-Host 'Installed.' -ForegroundColor Green
    Write-Host 'Steam -> RDR2 -> Story Mode -> load save -> F6.'
}
catch {
    Write-Host $_.Exception.Message -ForegroundColor Red
    exit 1
}
