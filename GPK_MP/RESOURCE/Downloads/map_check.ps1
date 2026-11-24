# COOP s47: the check of the maps of RESOURCE\Downloads (see README_карты.txt).
#   map_check.ps1              - checks every map folder
#   map_check.ps1 MyMap        - checks the folder MyMap
#   map_check.ps1 MyMap -Fix   - also copies into the folder the textures its models need from RESOURCE\Textures
#   map_check.ps1 MyMap -New land|sea  - makes a map.ini template in the folder (when it has none)
param([string]$Map = '', [switch]$Fix, [string]$New = '')
$ErrorActionPreference = 'Stop'
$Dl  = $PSScriptRoot
$Res = Split-Path $Dl -Parent                       # RESOURCE
$Tex = Join-Path $Res 'Textures'
$Enc = [Text.Encoding]::GetEncoding(1251)
$Ext = @('gm','col','tx','ptc','grs','ini','txt','tga')    # the files the server sends (NetDl_PathOk)

function Say($s, $c = 'Gray') { Write-Host $s -ForegroundColor $c }

function Read-MapIni($path) {
    $h = @{}; $sec = ''
    foreach ($l in [IO.File]::ReadAllLines($path, $Enc)) {
        $t = $l.Trim()
        if ($t -match '^\[(.+)\]$') { $sec = $Matches[1].ToLower(); continue }
        if ($sec -ne 'map' -or $t -eq '' -or $t.StartsWith(';')) { continue }
        $i = $t.IndexOf('=')
        if ($i -gt 0) { $h[$t.Substring(0, $i).Trim().ToLower()] = $t.Substring($i + 1).Trim() }
    }
    return $h
}

# the texture names (.tga) written in a .gm file
function Get-GmTextures($gm) {
    $s = $Enc.GetString([IO.File]::ReadAllBytes($gm))
    return [regex]::Matches($s, '[A-Za-z0-9_\- .\\]{2,}\.tga', 'IgnoreCase') | ForEach-Object { $_.Value.TrimStart(' ', '\') }
}

$texIndex = $null
function Find-Texture($name) {
    # $name: "x.tga" or "dir\x.tga"; the root of RESOURCE\Textures first, then any of its folders
    $p = Join-Path $Tex ($name + '.tx')
    if (Test-Path -LiteralPath $p) { return $p }
    if ($null -eq $script:texIndex) {
        $script:texIndex = @{}
        Get-ChildItem $Tex -Recurse -File -Filter '*.tx' | ForEach-Object {
            $k = $_.Name.ToLower(); if (-not $script:texIndex.ContainsKey($k)) { $script:texIndex[$k] = $_.FullName }
        }
    }
    $k = ([IO.Path]::GetFileName($name) + '.tx').ToLower()
    if ($script:texIndex.ContainsKey($k)) { return $script:texIndex[$k] }
    return $null
}

function New-MapIni($dir, $type) {
    $ini = Join-Path $dir 'map.ini'
    if (Test-Path $ini) { Say "  map.ini уже есть, не трогаю" Yellow; return }
    $d = Split-Path $dir -Leaf
    if ($type -eq 'land') {
        $a = @("models.always.main=$d", "models.always.locators=${d}_locators", "models.day.charactersPatch=${d}_patch",
               "models.night.charactersPatch=${d}_patch", 'environment.weather=true', 'environment.sea=false',
               'reload.l1.name=reload1', 'reload.l1.emerge=reload1')
    } else {
        $a = @("model=$d", "refl_model=${d}_refl", "locators=${d}_locators",
               'reload.l1.label=Shore1', 'reload.l1.name=reload_1', 'reload.l1.go=Shore1', 'reload.l1.emerge=sea', 'reload.l1.radius=600.0')
    }
    $lines = @('[map]', "name=$d", "type=$type", "count=$($a.Count)")
    for ($i = 0; $i -lt $a.Count; $i++) { $lines += "a$($i + 1)=$($a[$i])" }
    [IO.File]::WriteAllText($ini, ($lines -join "`r`n") + "`r`n", $Enc)
    Say "  создан шаблон map.ini ($type): поправь имена моделей и локаторов под свою карту" Green
}

function Check-Map($dir) {
    $d = Split-Path $dir -Leaf
    $err = 0; $warn = 0
    Say "=== $d" Cyan
    if ($d -notmatch '^[A-Za-z0-9_\-]{1,24}$') { Say "  ОШИБКА: имя папки - только латиница, цифры, _ и -, до 24 знаков" Red; $err++ }
    if ($New) { New-MapIni $dir $New.ToLower() }
    $ini = Join-Path $dir 'map.ini'
    if (-not (Test-Path $ini)) { Say "  ОШИБКА: нет map.ini (сделать шаблон: map_check.ps1 $d -New land или -New sea)" Red; return 1 }
    $h = Read-MapIni $ini
    foreach ($k in 'name', 'type', 'count') { if (-not $h.ContainsKey($k)) { Say "  ОШИБКА: в [map] нет строки $k=" Red; $err++ } }
    $type = "$($h['type'])".ToLower()
    if ($type -ne 'land' -and $type -ne 'sea') { Say "  ОШИБКА: type должен быть land или sea" Red; $err++ }
    $n = 0; [void][int]::TryParse("$($h['count'])", [ref]$n)
    $attrs = @{}
    for ($i = 1; $i -le $n; $i++) {
        $v = $h["a$i"]
        if (-not $v) { Say "  ОШИБКА: count=$n, но нет строки a$i" Red; $err++; continue }
        $j = $v.IndexOf('=')
        if ($j -le 0 -or $j -ge $v.Length - 1) { Say "  ОШИБКА: a$i без 'путь=значение': $v" Red; $err++; continue }
        $attrs[$v.Substring(0, $j).ToLower()] = $v.Substring($j + 1)
    }
    $extra = @($h.Keys | Where-Object { $_ -match '^a(\d+)$' -and [int]$Matches[1] -gt $n })
    if ($extra.Count) { Say "  ВНИМАНИЕ: строки $($extra -join ', ') больше count=$n, игра их не прочтёт" Yellow; $warn++ }

    # the models the map names: in its folder, or a path to the models of the game (..\..\models\...)
    $gms = @(Get-ChildItem $dir -Filter '*.gm' -File | ForEach-Object FullName)
    if ($type -eq 'land') { $mk = $attrs.Keys | Where-Object { $_ -like 'models.*' -and $_ -notlike '*.tech' -and $_ -notlike '*.level' } }
    else { $mk = $attrs.Keys | Where-Object { $_ -in 'model', 'refl_model', 'locators' } }
    if (-not $mk) { Say "  ОШИБКА: в map.ini не названа ни одна модель" Red; $err++ }
    foreach ($k in $mk) {
        $m = $attrs[$k]
        $e = if ($k -like '*characterspatch') { '.ptc' } else { '.gm' }
        $p = [IO.Path]::GetFullPath((Join-Path $dir ($m + $e)))
        if (-not (Test-Path -LiteralPath $p)) { Say "  ОШИБКА: $k=$m - нет файла $m$e" Red; $err++; continue }
        if ($e -eq '.gm' -and -not $p.StartsWith($dir, [StringComparison]::OrdinalIgnoreCase)) { $gms += $p }
    }
    if ($type -eq 'land' -and -not ($attrs.Keys | Where-Object { $_ -like 'models.*.locators' })) { Say "  ОШИБКА: нет models.always.locators (точки появления)" Red; $err++ }
    if ($type -eq 'sea' -and -not $attrs.ContainsKey('locators')) { Say "  ОШИБКА: нет locators=" Red; $err++ }

    # the files the server sends
    $files = @(Get-ChildItem $dir -Recurse -File)
    foreach ($f in $files) {
        $rel = $f.FullName.Substring($dir.Length + 1)
        $e = $f.Extension.TrimStart('.').ToLower()
        if ($Ext -notcontains $e) { Say "  ВНИМАНИЕ: $rel не будет отправлен клиентам (тип .$e)" Yellow; $warn++ }
        elseif ($rel -notmatch '^[A-Za-z0-9_\- .\\]{3,100}$') { Say "  ВНИМАНИЕ: $rel не будет отправлен (в имени не латиница или длиннее 100)" Yellow; $warn++ }
        if ($f.Length -gt 64MB) { Say "  ОШИБКА: $rel больше 64 МБ" Red; $err++ }
    }
    if ($files.Count -gt 1000) { Say "  ОШИБКА: файлов $($files.Count), больше 1000" Red; $err++ }

    # the textures of the models: all must be in the folder of the map
    $need = @{}
    foreach ($g in $gms) { foreach ($t in Get-GmTextures $g) { $need[$t.ToLower()] = $t } }
    $miss = 0; $copied = 0
    foreach ($t in ($need.Values | Sort-Object)) {
        $base = [IO.Path]::GetFileName($t)
        if ((Test-Path -LiteralPath (Join-Path $dir ($t + '.tx'))) -or (Test-Path -LiteralPath (Join-Path $dir ($base + '.tx')))) { continue }
        $src = Find-Texture $t
        if (-not $src) { Say "  ОШИБКА: текстуры $t нет ни в папке карты, ни в RESOURCE\Textures" Red; $err++; continue }
        if ($Fix) { Copy-Item -LiteralPath $src (Join-Path $dir ($base + '.tx')); $copied++; Say "  + $($src.Substring($Res.Length + 1))" Green }
        else { $miss++; Say "  нет текстуры $t (есть в $($src.Substring($Res.Length + 1)))" Yellow }
    }
    if ($miss) { Say "  ОШИБКА: не хватает текстур: $miss. Запусти с -Fix, они скопируются в папку карты" Red; $err++ }
    if ($copied) { Say "  скопировано текстур: $copied" Green }
    $mb = [math]::Round((($files | Measure-Object Length -Sum).Sum) / 1MB, 1)
    $res = if ($err) { "ОШИБОК: $err" } else { 'OK' }
    Say "  $type, '$($h['name'])', файлов $($files.Count), $mb МБ, текстур в моделях $($need.Count) - $res" $(if ($err) { 'Red' } elseif ($warn) { 'Yellow' } else { 'Green' })
    return $err
}

$dirs = if ($Map) { @(Join-Path $Dl $Map) } else { @(Get-ChildItem $Dl -Directory | ForEach-Object FullName) }
$bad = 0
foreach ($d in $dirs) {
    if (-not (Test-Path $d)) { if ($New) { New-Item -ItemType Directory $d | Out-Null } else { Say "нет папки $d" Red; $bad++; continue } }
    if ((Check-Map (Resolve-Path $d).Path) -gt 0) { $bad++ }
}
if (-not $dirs) { Say 'в Downloads нет папок карт' Yellow }
exit $bad
