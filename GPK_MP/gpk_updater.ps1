# GPK_MP: обновление мода до последнего релиза GitHub (кнопка "Обновить" в главном меню).
# Пофайлово: берёт дерево папки GPK_MP репозитория на теге последнего релиза, сравнивает git-хеши
# с файлами в папке игры и качает только отличающиеся. Заменённые файлы сохраняются в _update_backup.
# Работает в PowerShell 2.0 (Windows 7 из коробки) и новее.
# Запуск вручную: powershell -ExecutionPolicy Bypass -File gpk_updater.ps1 [-DryRun] [-NoRestart] [-Tag 21.11.26]
param(
    [string]$GameDir = '',
    [string]$Tag = '',
    [switch]$DryRun,
    [switch]$NoRestart,
    [string]$Only = ''                   # для проверки: только пути с этим началом
)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
$Repo = 'SYNth3515/GPK_MP'
$Sub = 'GPK_MP/'                         # папка мода внутри репозитория
# настройки игрока: не перезаписываются, если уже есть
$Keep = @('options', 'engine.ini', 'gpk_update.ini', 'unins000.dat', 'unins000.exe')
$TextExt = @('.c', '.h', '.ini', '.txt', '.bat', '.ps1', '.md')   # в репозитории LF, в игре CRLF

# PowerShell 2.0 has no $PSScriptRoot / $PSCommandPath
$ScriptPath = $MyInvocation.MyCommand.Path
if (-not $GameDir) { $GameDir = Split-Path -Parent $ScriptPath }
$GameDir = [IO.Path]::GetFullPath($GameDir)
$LogFile = Join-Path $GameDir 'gpk_updater.log'
try { $Host.UI.RawUI.WindowTitle = 'GPK_MP - обновление' } catch {}

function Say([string]$s, [string]$c = 'Gray') {
    Write-Host $s -ForegroundColor $c
    try { Add-Content -Path $LogFile -Value ((Get-Date -Format 'HH:mm:ss') + ' ' + $s) -Encoding UTF8 } catch {}
}

function Fail([string]$s) {
    Say ('ОШИБКА: ' + $s) 'Red'
    Say 'Обновление не завершено. Подробности в gpk_updater.log. Можно скачать релиз вручную:' 'Yellow'
    Say ('https://github.com/' + $Repo + '/releases/latest') 'Yellow'
    if (-not $DryRun) { Read-Host 'Нажмите Enter, чтобы закрыть окно' | Out-Null }
    exit 1
}

# git blob sha1 of the bytes
$Sha1 = [Security.Cryptography.SHA1]::Create()
function BlobSha([byte[]]$b) {
    $h = [Text.Encoding]::ASCII.GetBytes('blob ' + $b.Length + [char]0)
    $all = New-Object byte[] ($h.Length + $b.Length)
    [Buffer]::BlockCopy($h, 0, $all, 0, $h.Length)
    [Buffer]::BlockCopy($b, 0, $all, $h.Length, $b.Length)
    return (($Sha1.ComputeHash($all) | ForEach-Object { $_.ToString('x2') }) -join '')
}

# like git: a NUL in the first 8000 bytes = binary
function IsText([byte[]]$b) {
    return ([Array]::IndexOf($b, [byte]0, 0, [Math]::Min($b.Length, 8000)) -lt 0)
}

# Latin1 keeps every byte as one char: fast line end changes on strings
$L1 = [Text.Encoding]::GetEncoding(28591)
function Crlf2Lf([byte[]]$b) { return $L1.GetBytes($L1.GetString($b).Replace("`r`n", "`n")) }
function Lf2Crlf([byte[]]$b) { return $L1.GetBytes($L1.GetString($b).Replace("`r`n", "`n").Replace("`n", "`r`n")) }

Say ''
Say '=== GPK_MP: обновление ===' 'Cyan'
Say ('Папка игры: ' + $GameDir)

# 1. игра должна быть закрыта (dll и exe заняты, пока она работает)
$exe = Join-Path $GameDir 'engine.exe'
if (-not $DryRun) {
    Say 'Жду закрытия игры...'
    for ($i = 0; $i -lt 60; $i++) {
        $p = @(Get-Process -Name engine -ErrorAction SilentlyContinue | Where-Object { try { $_.Path -eq $exe } catch { $true } })
        if ($p.Count -eq 0) { break }
        if ($i -eq 30) { Say 'Игра не закрылась сама, закрываю её' 'Yellow'; $p | Stop-Process -Force -ErrorAction SilentlyContinue }
        Start-Sleep -Milliseconds 500
    }
    Start-Sleep -Milliseconds 500
}

# 2. запись в папку игры (Program Files требует прав администратора: один запрос UAC)
if (-not $DryRun) {
    $probe = Join-Path $GameDir ('gpk_upd_probe_' + $PID + '.tmp')
    try { [IO.File]::WriteAllText($probe, 'x'); Remove-Item $probe -Force }
    catch {
        $admin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
        if ($admin) { Fail ('нет доступа на запись в папку игры: ' + $_.Exception.Message) }
        Say 'Нужны права администратора для записи в папку игры, запрашиваю...' 'Yellow'
        $a = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', ('"' + $ScriptPath + '"'), '-GameDir', ('"' + $GameDir + '"'))
        if ($Tag) { $a += @('-Tag', $Tag) }
        if ($NoRestart) { $a += '-NoRestart' }
        try { Start-Process -FilePath 'powershell.exe' -ArgumentList $a -Verb RunAs | Out-Null; exit 0 }
        catch { Fail 'права администратора не выданы' }
    }
}

# GitHub wants TLS 1.2: .NET 4.5+ has it; .NET 3.5 of PowerShell 2.0 only with the update KB3154518 of Windows 7
try { [Net.ServicePointManager]::SecurityProtocol = [Enum]::ToObject([Net.SecurityProtocolType], 3072) } catch { Say ('TLS 1.2: ' + $_.Exception.Message) 'Yellow' }

Add-Type -AssemblyName System.Web.Extensions
$js = New-Object Web.Script.Serialization.JavaScriptSerializer
$js.MaxJsonLength = 268435456
$wc = New-Object Net.WebClient
$wc.Encoding = [Text.Encoding]::UTF8        # the JSON of GitHub (else the paths in Cyrillic break)
$wc.Headers.Add('User-Agent', 'GPK_MP-updater')

# 3. последний релиз и дерево репозитория на его теге
try {
    if (-not $Tag) {
        $wc.Headers.Set('User-Agent', 'GPK_MP-updater')
        $rel = $js.DeserializeObject($wc.DownloadString('https://api.github.com/repos/' + $Repo + '/releases/latest'))
        $Tag = [string]$rel['tag_name']
        Say ('Последний релиз: ' + $rel['name'] + ' (тег ' + $Tag + ')') 'Green'
    }
    $wc.Headers.Set('User-Agent', 'GPK_MP-updater')
    $treeJson = $wc.DownloadString('https://api.github.com/repos/' + $Repo + '/git/trees/' + [Uri]::EscapeDataString($Tag) + '?recursive=1')
} catch {
    $m = $_.Exception.Message
    if ($PSVersionTable.PSVersion.Major -lt 3) { $m = $m + ' (Windows 7: установите Windows Management Framework 5.1 или обновление KB3154518 для TLS 1.2)' }
    Fail ('GitHub недоступен: ' + $m)
}

$tree = $js.DeserializeObject($treeJson)
if ($tree['truncated']) { Fail 'дерево репозитория получено не полностью' }

$files = @()
foreach ($e in $tree['tree']) {
    if ($e['type'] -ne 'blob') { continue }
    $path = [string]$e['path']
    if (-not $path.StartsWith($Sub)) { continue }
    if ($Only -and -not $path.Substring($Sub.Length).StartsWith($Only)) { continue }
    $files += , @($path.Substring($Sub.Length), [string]$e['sha'], [long]$e['size'])
}
if ($files.Count -lt 100 -and -not $Only) { Fail ('в релизе слишком мало файлов: ' + $files.Count) }
Say ('Файлов мода в релизе: ' + $files.Count)

# 4. сравнение
Say 'Сравниваю файлы...'
$need = New-Object Collections.ArrayList
$n = 0
foreach ($f in $files) {
    $n++
    if ($n % 250 -eq 0) { Write-Host ('  ' + $n + ' / ' + $files.Count) }
    $rel = $f[0]; $local = Join-Path $GameDir ($rel -replace '/', '\')
    if (Test-Path -LiteralPath $local -PathType Leaf) {
        if ($Keep -contains $rel.ToLower()) { continue }
        $b = [IO.File]::ReadAllBytes($local)
        if ((BlobSha $b) -eq $f[1]) { continue }
        if ((IsText $b) -and ((BlobSha (Crlf2Lf $b)) -eq $f[1])) { continue }   # differs only in the line ends
    }
    [void]$need.Add($f)
}
$total = 0; foreach ($f in $need) { $total += $f[2] }
Say ('Нужно обновить файлов: ' + $need.Count + ' (' + [Math]::Round($total / 1MB, 1) + ' МБ)') 'Green'
if ($DryRun) { foreach ($f in $need) { Say ('  ' + $f[0]) }; exit 0 }

# 5. загрузка: каждый файл проверяется по хешу, старый сохраняется в _update_backup
if ($need.Count -gt 0) {
    $bak = Join-Path $GameDir '_update_backup'
    if (Test-Path $bak) { Remove-Item $bak -Recurse -Force -ErrorAction SilentlyContinue }
    $done = 0; $got = 0
    foreach ($f in $need) {
        $done++
        $rel = $f[0]; $local = Join-Path $GameDir ($rel -replace '/', '\')
        Write-Host ('[' + $done + '/' + $need.Count + '] ' + $rel + '  (' + [Math]::Round($got / 1MB, 1) + ' / ' + [Math]::Round($total / 1MB, 1) + ' МБ)')
        $url = 'https://raw.githubusercontent.com/' + $Repo + '/' + [Uri]::EscapeDataString($Tag) + '/' + ((($Sub + $rel).Split('/') | ForEach-Object { [Uri]::EscapeDataString($_) }) -join '/')
        $b = $null
        for ($try = 1; $try -le 3 -and $b -eq $null; $try++) {
            try {
                $wc.Headers.Set('User-Agent', 'GPK_MP-updater')
                $d = $wc.DownloadData($url)
                if ((BlobSha $d) -ne $f[1]) { throw 'хеш не совпал' }
                $b = $d
            } catch {
                Say ('  попытка ' + $try + ': ' + $_.Exception.Message) 'Yellow'
                Start-Sleep -Seconds 2
            }
        }
        if ($b -eq $null) { Fail ('не удалось скачать ' + $rel) }
        if (($TextExt -contains [IO.Path]::GetExtension($rel).ToLower()) -and (IsText $b)) { $b = Lf2Crlf $b }
        try {
            $dir = Split-Path $local -Parent
            if (-not (Test-Path $dir)) { New-Item -ItemType Directory -Path $dir -Force | Out-Null }
            if (Test-Path -LiteralPath $local) {
                $bf = Join-Path $bak ($rel -replace '/', '\')
                New-Item -ItemType Directory -Path (Split-Path $bf -Parent) -Force | Out-Null
                Copy-Item -LiteralPath $local -Destination $bf -Force
            }
            $tmp = $local + '.gpkupd'
            [IO.File]::WriteAllBytes($tmp, $b)
            Move-Item -LiteralPath $tmp -Destination $local -Force
        } catch { Fail ('не удалось записать ' + $rel + ': ' + $_.Exception.Message) }
        $got += $f[2]
    }
    Say ('Готово: обновлено файлов ' + $need.Count + '. Старые версии в папке _update_backup') 'Green'
} else {
    Say 'Все файлы уже совпадают с релизом' 'Green'
}

# 6. новая проверка версии при следующем запуске, перезапуск игры
Remove-Item (Join-Path $GameDir 'gpk_update.ini') -Force -ErrorAction SilentlyContinue
if (-not $NoRestart) {
    Say 'Запускаю игру...' 'Cyan'
    Start-Process -FilePath $exe -WorkingDirectory $GameDir | Out-Null
}
Start-Sleep -Seconds 3
exit 0
