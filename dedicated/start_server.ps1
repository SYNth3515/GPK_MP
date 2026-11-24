# GPK_MP: выделенный сервер на Windows. Запуск (из папки игры или откуда угодно):
#   powershell -ExecutionPolicy Bypass -File dedicated\start_server.ps1 -Config Deathmatch
#   powershell -ExecutionPolicy Bypass -File dedicated\start_server.ps1 -Config D:\my_server.ini
# -Config: имя файла из dedicated\presets (без .ini) или путь к своему ini (образец - любой файл из presets).
# Сервер работает в своей папке dedicated\run\<имя> (ссылки на файлы игры + свои engine.ini, dedicated.ini и логи),
# поэтому несколько серверов (с разными Port) идут одновременно. Игра упала или все игроки вышли - она запускается снова.
# Остановка: Ctrl+C в этом окне (игра сервера закрывается тоже).
param([string]$Config = 'Deathmatch', [int]$RestartDelay = 5, [int]$KeepLogs = 20)
$ErrorActionPreference = 'Stop'
$game = Split-Path -Parent $PSScriptRoot
if (-not (Test-Path (Join-Path $game 'engine.exe'))) { Write-Host "Нет engine.exe в $game" -ForegroundColor Red; exit 1 }

$cfg = $Config
if (-not (Test-Path -LiteralPath $cfg)) { $cfg = Join-Path $PSScriptRoot ("presets\" + $Config + '.ini') }
if (-not (Test-Path -LiteralPath $cfg)) { Write-Host "Нет файла конфига: $Config" -ForegroundColor Red; exit 1 }
$cfg = (Resolve-Path -LiteralPath $cfg).Path
$name = [IO.Path]::GetFileNameWithoutExtension($cfg)
$run = Join-Path $PSScriptRoot ("run\" + $name)
New-Item -ItemType Directory -Force -Path $run | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $run 'logs') | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $run 'SAVE') | Out-Null

# the game: folders as junctions, files as hard links (no admin rights needed; the same disk)
foreach ($d in @('PROGRAM', 'RESOURCE', 'modules')) {
    $dst = Join-Path $run $d
    if (-not (Test-Path -LiteralPath $dst)) { New-Item -ItemType Junction -Path $dst -Target (Join-Path $game $d) | Out-Null }
}
foreach ($f in (Get-ChildItem -LiteralPath $game -File | Where-Object { $_.Extension -in '.exe', '.dll', '.dat' -and $_.Name -notlike 'unins*' -and $_.Name -ne 'config.exe' })) {
    $dst = Join-Path $run $f.Name
    if (Test-Path -LiteralPath $dst) {
        if ((Get-Item -LiteralPath $dst).Length -eq $f.Length) { continue }
        Remove-Item -LiteralPath $dst -Force
    }
    try { New-Item -ItemType HardLink -Path $dst -Target $f.FullName | Out-Null }
    catch { Copy-Item -LiteralPath $f.FullName -Destination $dst }
}

# engine.ini of the server: the one of the game (the master server), a small window, few frames, no fps counter
$ini = Get-Content -LiteralPath (Join-Path $game 'engine.ini') -Encoding Default
$set = @{ 'full_screen' = '0'; 'screen_x' = '640'; 'screen_y' = '480'; 'max_fps' = '0'; 'show_fps' = '0'; 'PostProcess' = '0' }
$ini = $ini | ForEach-Object {
    $l = $_
    foreach ($k in $set.Keys) { if ($l -match ('^\s*' + [regex]::Escape($k) + '\s*=')) { $l = $k + ' = ' + $set[$k] } }
    $l
}
Set-Content -LiteralPath (Join-Path $run 'engine.ini') -Value $ini -Encoding Default
# dedicated.ini = the config + every missing / empty key as "default" (the game crashes on a missing key)
$keys = @('Enabled','Name','Port','Mode','Teams','Password','AdminPassword','MaxPlayers','Island','Land','Weather','Respawn','RespawnTime','TimeLimit','GameSpeed','LateJoin','StartPosition','Credit','MaxShipClass','BotSkill','NoBoarding','PassivePerks','ActivePerks','FastBoarding','PistolReload','Internet','Lan','P2P','MaxFps','MinPlayers','StartDelay','RoundEndDelay','QuitWhenEmpty','RotateMap','Render')
$have = @{}
$out = New-Object System.Collections.Generic.List[string]
foreach ($l in [IO.File]::ReadAllLines($cfg)) {
    if ($l -match '^\s*([A-Za-z0-9_]+)\s*=\s*(.*?)\s*$') {
        $k = $Matches[1]; $v = $Matches[2]; $have[$k.ToLower()] = 1
        if ($k -eq 'Enabled') { $v = '1' }
        if ($v -eq '') { $v = 'default' }
        $l = "$k = $v"
    }
    $out.Add($l)
}
if (-not ($out | Where-Object { $_ -match '^\s*\[Server\]' })) { $out.Insert(0, '[Server]') }
foreach ($k in $keys) { if (-not $have.ContainsKey($k.ToLower())) { if ($k -eq 'Enabled') { $out.Add('Enabled = 1') } else { $out.Add("$k = default") } } }
[IO.File]::WriteAllLines((Join-Path $run 'dedicated.ini'), $out, (New-Object System.Text.UTF8Encoding $false))

$exe = Join-Path $run 'engine.exe'
Write-Host "Сервер $name ($cfg), папка $run. Остановка: Ctrl+C" -ForegroundColor Green
$p = $null
try {
    while ($true) {
        Write-Host ("[{0:HH:mm:ss}] запуск" -f (Get-Date))
        $p = Start-Process -FilePath $exe -WorkingDirectory $run -PassThru
        $p.WaitForExit()
        $code = $p.ExitCode
        $p = $null
        $stamp = Get-Date -Format 'yyyy-MM-dd_HH-mm-ss'
        foreach ($log in @('compile.log', 'system.log', 'error.log')) {
            $src = Join-Path $run $log
            if (Test-Path -LiteralPath $src) { Move-Item -LiteralPath $src -Destination (Join-Path $run ("logs\" + $stamp + '_' + $log)) -Force }
        }
        Get-ChildItem -LiteralPath (Join-Path $run 'logs') -File | Sort-Object LastWriteTime -Descending | Select-Object -Skip ($KeepLogs * 3) | Remove-Item -Force
        Write-Host ("[{0:HH:mm:ss}] игра закрылась (код {1}), перезапуск через {2} с" -f (Get-Date), $code, $RestartDelay) -ForegroundColor Yellow
        Start-Sleep -Seconds $RestartDelay
    }
}
finally {
    if ($p -ne $null -and -not $p.HasExited) { $p.Kill() }
}
