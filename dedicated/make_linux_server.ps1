# GPK_MP: облегчённая копия игры для выделенного сервера на Linux (VPS) из вашей установленной игры с модом.
# Сервер ничего не рисует и не играет звук, поэтому ему не нужны локации, текстуры, звуки и видео: ~0.45 ГБ вместо ~9 ГБ.
# Запуск (папка dedicated лежит в папке игры):
#   powershell -ExecutionPolicy Bypass -File dedicated\make_linux_server.ps1
#   powershell -ExecutionPolicy Bypass -File dedicated\make_linux_server.ps1 -Out D:\GPK_MP_server
# Результат: папка GPK_MP_server и архив gpk_server.tgz рядом с папкой игры. Архив + dedicated\linux\install.sh -> на VPS.
#   -Game D:\Games\GPK_MP   папка игры, если dedicated лежит не в ней
param([string]$Out = '', [string]$Game = '', [switch]$NoArchive)
$ErrorActionPreference = 'Stop'
$game = $Game
if ($game -eq '') { $game = Split-Path -Parent $PSScriptRoot }
if (-not (Test-Path (Join-Path $game 'engine.exe'))) { Write-Host "Нет engine.exe в $game (папка dedicated должна лежать в папке игры)" -ForegroundColor Red; exit 1 }
if ($Out -eq '') { $Out = Join-Path (Split-Path -Parent $game) 'GPK_MP_server' }
New-Item -ItemType Directory -Force -Path $Out | Out-Null
$Out = (Resolve-Path -LiteralPath $Out).Path
Write-Host "Игра:  $game"
Write-Host "Копия: $Out"

$dirs = @('PROGRAM', 'modules',
          'RESOURCE\INI', 'RESOURCE\Sea', 'RESOURCE\Downloads', 'RESOURCE\Particles',
          'RESOURCE\MODELS\Islands', 'RESOURCE\MODELS\Ships', 'RESOURCE\MODELS\Ammo', 'RESOURCE\MODELS\Weather',
          'RESOURCE\MODELS\Tornado', 'RESOURCE\MODELS\SAILORSPOINTS',
          'RESOURCE\Textures\FONTS', 'RESOURCE\Textures\NET')
foreach ($d in $dirs) {
    $src = Join-Path $game $d
    if (-not (Test-Path -LiteralPath $src)) { Write-Host "  нет $d - пропущено" -ForegroundColor Yellow; continue }
    $xf = @('*.log', '*.bak', '*.orig')
    # острова: серверу нужна только геометрия (.gm); свет по часам (.col) и трава (.grs) только рисуются (-0.7 ГБ)
    if ($d -eq 'RESOURCE\MODELS\Islands') { $xf += @('*.col', '*.grs') }
    $null = & robocopy $src (Join-Path $Out $d) /E /PURGE /NFL /NDL /NJH /NJS /NP /R:1 /W:1 /XF $xf
    if ($LASTEXITCODE -ge 8) { Write-Host "  ошибка копирования $d (robocopy $LASTEXITCODE)" -ForegroundColor Red; exit 1 }
}
foreach ($f in @('engine.exe', 'engine.ini', 'fmodex.dll', 'stormex2.dll', 'stormext.dll', 'resource0.dat', 'resource1.dat')) {
    $src = Join-Path $game $f
    if (-not (Test-Path -LiteralPath $src)) { Write-Host "  нет $f - пропущено" -ForegroundColor Yellow; continue }
    $dst = Join-Path $Out $f
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $dst) | Out-Null
    Copy-Item -LiteralPath $src -Destination $dst -Force
}
# папка dedicated - та, где лежит этот скрипт (пресеты, скрипты Linux), без run (рабочие папки серверов Windows)
$null = & robocopy $PSScriptRoot (Join-Path $Out 'dedicated') /E /PURGE /NFL /NDL /NJH /NJS /NP /R:1 /W:1 /XD run dist /XF *.log
if ($LASTEXITCODE -ge 8) { Write-Host "  ошибка копирования dedicated (robocopy $LASTEXITCODE)" -ForegroundColor Red; exit 1 }
# dedicated.ini в копию не берём: на сервере его пишет скрипт запуска (иначе копия на Windows стала бы сервером)
$mb = [math]::Round(((Get-ChildItem -LiteralPath $Out -Recurse -File | Measure-Object Length -Sum).Sum) / 1MB)
Write-Host "Копия готова: $mb МБ" -ForegroundColor Green

if ($NoArchive) { exit 0 }
$tar = Join-Path $env:SystemRoot 'System32\tar.exe'
if (-not (Test-Path $tar)) { Write-Host "Нет tar.exe (Windows 10 1803+): запакуйте папку $Out в .tar.gz сами (7-Zip)" -ForegroundColor Yellow; exit 0 }
$parent = Split-Path -Parent $Out
$leaf = Split-Path -Leaf $Out
$tgz = Join-Path $parent 'gpk_server.tgz'
if (Test-Path -LiteralPath $tgz) { Remove-Item -LiteralPath $tgz -Force }
Write-Host "Пакую $tgz ..."
& $tar --exclude "$leaf/dedicated/run" -czf $tgz -C $parent $leaf
if ($LASTEXITCODE -ne 0) { Write-Host "tar: ошибка $LASTEXITCODE" -ForegroundColor Red; exit 1 }
Write-Host ("Архив готов: {0} ({1} МБ)" -f $tgz, [math]::Round((Get-Item $tgz).Length / 1MB)) -ForegroundColor Green
Write-Host "Дальше: залейте gpk_server.tgz и dedicated\linux\install.sh на VPS (в /root) и выполните:"
Write-Host "  sudo bash install.sh gpk_server.tgz Deathmatch"
