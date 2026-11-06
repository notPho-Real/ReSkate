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
        $hash = (Get-FileHash target/release/reskate_spotify.lib -Algorithm SHA256).Hash.ToLowerInvariant()
        $report.measurements += [ordered]@{mode=$mode; seconds=$timer.Elapsed.TotalSeconds; sha256=$hash}
    }
    $report.equal = @($report.measurements.sha256 | Select-Object -Unique).Count -eq 1
    if (-not $report.equal) { throw 'Clean and cached Spotify libraries differ' }
} finally {
    Pop-Location
    $report | ConvertTo-Json -Depth 5 | Set-Content artifacts/cargo-audit.json
    $report | ConvertTo-Json -Depth 5 | Add-Content $env:GITHUB_STEP_SUMMARY
}

