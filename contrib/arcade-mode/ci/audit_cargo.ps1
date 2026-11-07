$ErrorActionPreference = 'Stop'
$report = [ordered]@{
    source_sha = $env:GITHUB_SHA
    toolchain = $env:ARCADE_TOOLCHAIN
    measurements = @()
    equal = $false
}
New-Item -ItemType Directory artifacts -Force | Out-Null
Push-Location SpotifyHelper
try {
    foreach ($mode in @('restored', 'warm', 'clean')) {
        # Only generated Cargo outputs on this disposable hosted runner.
        if ($mode -eq 'clean' -and (Test-Path target)) {
            Remove-Item -Recurse -Force target
        }
        $timer = [Diagnostics.Stopwatch]::StartNew()
        & cargo build --locked --release --lib
        if ($LASTEXITCODE) { throw "Cargo $mode failed" }
        $timer.Stop()
        $fingerprint = & python ../contrib/arcade-mode/ci/archive_fingerprint.py target/release/reskate_spotify.lib
        if ($LASTEXITCODE) { throw 'Archive parsing failed' }
        $fingerprint = $fingerprint | ConvertFrom-Json
        if ($mode -ne 'warm') {
            Copy-Item target/release/reskate_spotify.lib "../artifacts/spotify-$mode.lib"
        }
        $report.measurements += [ordered]@{mode=$mode; seconds=$timer.Elapsed.TotalSeconds; fingerprint=$fingerprint}
    }
    $report.raw_equal = @($report.measurements.fingerprint.raw_sha256 | Select-Object -Unique).Count -eq 1
    $report.equal = @($report.measurements.fingerprint.content_sha256 | Select-Object -Unique).Count -eq 1
    $report.comparison = 'All archive bytes except documented archive/COFF timestamps'
    if (-not $report.equal) { throw 'Clean and cached Spotify library content differs' }
} finally {
    Pop-Location
    $report | ConvertTo-Json -Depth 5 | Set-Content artifacts/cargo-audit.json
    $report | ConvertTo-Json -Depth 5 | Add-Content $env:GITHUB_STEP_SUMMARY
}

