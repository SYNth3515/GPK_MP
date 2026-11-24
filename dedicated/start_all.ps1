# GPK_MP: все выделенные серверы из dedicated\presets, каждый в своём окне PowerShell.
#   powershell -ExecutionPolicy Bypass -File dedicated\start_all.ps1
#   powershell -ExecutionPolicy Bypass -File dedicated\start_all.ps1 -Only Deathmatch,LandDuel
param([string[]]$Only = @())
$one = Join-Path $PSScriptRoot 'start_server.ps1'
foreach ($f in (Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot 'presets') -Filter '*.ini')) {
    $n = $f.BaseName
    if ($Only.Count -gt 0 -and $Only -notcontains $n) { continue }
    Start-Process powershell -ArgumentList @('-NoExit', '-ExecutionPolicy', 'Bypass', '-File', "`"$one`"", '-Config', $n)
    Write-Host "запущен $n"
    Start-Sleep -Seconds 3
}
