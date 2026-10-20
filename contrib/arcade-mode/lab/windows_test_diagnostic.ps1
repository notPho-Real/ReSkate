$ErrorActionPreference = 'Stop'
$source = Join-Path $env:GITHUB_WORKSPACE 'source'
$evidence = Join-Path $env:GITHUB_WORKSPACE 'diagnostic-evidence'
$raw = Join-Path $env:RUNNER_TEMP 'arcade-test-diagnostic'
New-Item -ItemType Directory -Force $evidence, $raw | Out-Null
$manifest = [ordered]@{
  source_sha = '29301b36a27f6deec969415136464a319d43271d'
  orchestration_sha = $env:GITHUB_SHA
  target = 'dingosdk_arcade_mode_tests'
  configuration = 'Release'
  diagnostic_only = $true
  hardware_or_deployment = $false
  result = 'not_started'
  symbol_evidence_requires_review = $true
}
function Clean([string]$value) {
  foreach ($prefix in @($source, $env:GITHUB_WORKSPACE, $env:RUNNER_TEMP, $env:USERPROFILE)) {
    if ($prefix) { $value = $value.Replace($prefix, '<runner>').Replace($prefix.Replace('\','/'), '<runner>') }
  }
  $value = [regex]::Replace($value, '[A-Za-z]:[\\/][^\s"<>]*', '<runner-path>')
  # Reject accidental credential-shaped text instead of publishing it.
  if ($value -match '(?i)(gh[pousr]_[A-Za-z0-9]{20,}|github_pat_[A-Za-z0-9_]+|-----BEGIN .*PRIVATE KEY-----)') {
    throw 'Sanitization rejected sensitive content.'
  }
  return $value
}
try {
  $cdb = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\Debuggers\x64\cdb.exe'
  if (!(Test-Path $cdb)) { $manifest.result='debugger_unavailable'; throw 'CDB missing; no implicit installation.' }
  Push-Location $source
  try {
    $actual = (& git rev-parse HEAD).Trim()
    if ($LASTEXITCODE -ne 0 -or $actual -ne $manifest.source_sha) { throw 'Frozen source mismatch.' }
    $manifest.source_tree = (& git rev-parse 'HEAD^{tree}').Trim()
    if ($manifest.source_tree -ne 'd8b902fced41592b0217b8cfaaf6648d66598180') { throw 'Frozen tree mismatch.' }
    $manifest.cdb_sha256 = (Get-FileHash $cdb -Algorithm SHA256).Hash.ToLowerInvariant()
    $overlay = Join-Path $raw 'symbols-only.cmake'
    @'
if(PROJECT_NAME STREQUAL "DingoSDK")
  cmake_language(DEFER CALL target_compile_options dingosdk_arcade_mode_tests PRIVATE /Zi)
  cmake_language(DEFER CALL target_link_options dingosdk_arcade_mode_tests PRIVATE /DEBUG:FULL /INCREMENTAL:NO /OPT:REF /OPT:ICF)
endif()
'@ | Set-Content $overlay -Encoding utf8
    & cargo fetch --locked --manifest-path SpotifyHelper/Cargo.toml --target x86_64-pc-windows-msvc *> (Join-Path $raw 'dependencies.log')
    if ($LASTEXITCODE -ne 0) { throw 'Dependency fetch failed.' }
    & cmake --preset vs2022-x64 -DDINGOSDK_BUILD_ARCADE_MODE_TESTS=ON -DDINGOSDK_BUILD_LAUNCHER_TESTS=ON -DDINGOSDK_BUILD_ARCADE_SCORE_ASSET_TOOLS=ON -DDINGOSDK_BUILD_PARK_EDITOR_TESTS=ON "-DCMAKE_PROJECT_INCLUDE=$overlay" *> (Join-Path $raw 'configure.log')
    if ($LASTEXITCODE -ne 0) { throw 'Diagnostic configure failed.' }
    & cmake --build --preset release --parallel 4 --target dingosdk_arcade_mode_tests -- /v:normal *> (Join-Path $raw 'build.log')
    if ($LASTEXITCODE -ne 0) { $manifest.result='build_failed'; throw 'Focused target build failed.' }
    $bin = Join-Path $source 'build/vs2022-x64/Release'
    $exe = Join-Path $bin 'dingosdk_arcade_mode_tests.exe'
    $pdb = Join-Path $bin 'dingosdk_arcade_mode_tests.pdb'
    if (!(Test-Path $exe) -or !(Test-Path $pdb)) { throw 'Matching build executable/PDB absent.' }
    [xml]$project = Get-Content (Join-Path $source 'build/vs2022-x64/dingosdk_arcade_mode_tests.vcxproj') -Raw
    $units = @($project.SelectNodes('//*[local-name()="ClCompile"][@Include]'))
    if ($units.Count -ne 13) { throw 'Combined source list changed.' }
    $manifest.combined_sources = @($units | ForEach-Object {
      $file = [IO.Path]::GetFullPath($_.Include)
      if (!$file.StartsWith($source + [IO.Path]::DirectorySeparatorChar)) { throw 'Source outside frozen checkout.' }
      @{path=[IO.Path]::GetRelativePath($source,$file);sha256=(Get-FileHash $file -Algorithm SHA256).Hash.ToLowerInvariant()}
    })
    $manifest.exe_sha256=(Get-FileHash $exe -Algorithm SHA256).Hash.ToLowerInvariant()
    $manifest.pdb_sha256=(Get-FileHash $pdb -Algorithm SHA256).Hash.ToLowerInvariant()
    $manifest.added_compile_options=@('/Zi')
    $manifest.added_link_options=@('/DEBUG:FULL','/INCREMENTAL:NO','/OPT:REF','/OPT:ICF')
    $manifest.cdb_commands='.reload /f dingosdk_arcade_mode_tests.exe; lmvm dingosdk_arcade_mode_tests; sxe 0xc0000005; sxe 0xc00000fd; g; .lastevent; .exr -1; .ecxr; kv; lm; q'
    $manifest.result='debugger_started'
    $manifest.working_directory_relative='build/vs2022-x64'
    Push-Location (Join-Path $source $manifest.working_directory_relative)
    try {
      & $cdb -y $bin -logo (Join-Path $raw 'debugger.log') -c $manifest.cdb_commands $exe *> (Join-Path $raw 'debugger-console.log')
      $manifest.debugger_exit=$LASTEXITCODE
    } finally { Pop-Location }
    $text=Get-Content (Join-Path $raw 'debugger.log') -Raw
    if ($text -match '(?i)(c0000005|c00000fd).*(first chance|second chance)|(?i)Access violation - code|Stack overflow - code') {
      $manifest.result='exception_captured'
      throw 'Windows test exception captured; original full CI remains failed.'
    }
    if ($manifest.debugger_exit -ne 0 -or $text -notmatch 'exited with code 0') {
      $manifest.result='debugger_outcome_unverified'
      throw 'Debugger result requires review.'
    }
    $manifest.result='single_run_non_reproduction_not_ci_clearance'
  } finally { Pop-Location }
} catch {
  Clean($_.Exception.Message) | Set-Content (Join-Path $evidence 'diagnostic-status.txt') -Encoding utf8
  throw 'Focused diagnostic stopped; inspect sanitized evidence. No acceptance implied.'
} finally {
  foreach ($file in Get-ChildItem $raw -Filter '*.log') {
    Clean(Get-Content $file.FullName -Raw) | Set-Content (Join-Path $evidence $file.Name) -Encoding utf8
  }
  $manifest | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $evidence 'manifest.json') -Encoding utf8
  # Raw EXE/PDB, debugger files and memory dumps are never uploaded.
}
