#!/bin/bash
# GPK_MP: установка выделенного сервера на Ubuntu / Debian (VPS без видеокарты) одной командой.
#   tar -xzf GPK_MP_Dedicated_Linux.tar.gz
#   sudo bash GPK_MP_Dedicated/dedicated/linux/install.sh Deathmatch            # один сервер
#   sudo bash GPK_MP_Dedicated/dedicated/linux/install.sh Deathmatch LandDuel   # несколько (у каждого свой порт)
#   sudo bash /opt/gpk_mp/dedicated/linux/install.sh TDM       # уже установлен: включить ещё один сервер
#   sudo bash install.sh gpk_server.tgz Deathmatch             # из архива (dedicated\make_linux_server.ps1)
# Распакованная папка переезжает в /opt/gpk_mp (места на VPS мало).
# Что делает: Wine (32 бит) + Xvfb + 32-битный OpenGL, пользователь gpk, игра в /opt/gpk_mp, службы systemd
# gpk-dedicated@<имя> (перезапуск при падении / когда все вышли, автозапуск после перезагрузки), порты в ufw.
set -e
[ "$(id -u)" = "0" ] || { echo "Запустите через sudo"; exit 1; }
DST="${GPK_DST:-/opt/gpk_mp}"
# откуда игра: архив .tgz первым аргументом, "" = уже стоит в /opt/gpk_mp, иначе - папка, в которой лежит этот скрипт
ARC=""; FROM=""
if [ $# -gt 0 ] && { [ -z "$1" ] || [ -f "$1" ]; }; then ARC="$1"; shift; fi
if [ -z "$ARC" ]; then
  HERE="$(cd "$(dirname "$0")/../.." && pwd)"
  if [ -f "$HERE/engine.exe" ] && [ "$HERE" != "$DST" ]; then FROM="$HERE"; fi
fi
[ $# -gt 0 ] || set -- Deathmatch

echo "== Пакеты: Wine 32 бит, Xvfb, 32-битный OpenGL (mesa)"
dpkg --add-architecture i386
apt-get update -q
DEBIAN_FRONTEND=noninteractive apt-get install -y -q --no-install-recommends \
  wine wine32 xvfb xauth libgl1:i386 libgl1-mesa-dri:i386 libglx-mesa0:i386

echo "== Пользователь gpk"
id gpk >/dev/null 2>&1 || useradd -m -s /bin/bash gpk

# новая игра в $DST; прошлые логи (dedicated/run) и конфиги серверов (presets/*.ini, ваши правки) остаются
install_tree() {
  local SRC="$1" TMP="$2"
  echo "== Игра -> $DST"
  for s in $(systemctl list-units --plain --no-legend 'gpk-dedicated@*' 2>/dev/null | awk '{print $1}'); do systemctl stop "$s" || true; done
  if [ -d "$DST" ]; then
    rm -rf "$DST.old"; mv "$DST" "$DST.old"
  fi
  mv "$SRC" "$DST"
  if [ -d "$DST.old" ]; then
    if [ -d "$DST.old/dedicated/run" ]; then rm -rf "$DST/dedicated/run"; mv "$DST.old/dedicated/run" "$DST/dedicated/run"; fi
    if [ -d "$DST.old/dedicated/presets" ]; then cp -f "$DST.old/dedicated/presets/"*.ini "$DST/dedicated/presets/" 2>/dev/null || true; fi
    rm -rf "$DST.old"
  fi
  [ -n "$TMP" ] && rm -rf "$TMP"
  return 0
}
if [ -n "$ARC" ]; then
  TMP="$(mktemp -d "$(dirname "$DST")/gpk_unpack.XXXX")"
  tar -xzf "$ARC" -C "$TMP"
  TOP="$(ls -1 "$TMP" | head -1)"
  [ -f "$TMP/$TOP/engine.exe" ] || { echo "В архиве нет <папка>/engine.exe"; rm -rf "$TMP"; exit 1; }
  install_tree "$TMP/$TOP" "$TMP"
elif [ -n "$FROM" ]; then
  install_tree "$FROM" ""
fi
[ -f "$DST/engine.exe" ] || { echo "Нет $DST/engine.exe: запустите install.sh из распакованной папки сервера или укажите архив"; exit 1; }
# архив из Windows: без флага запуска и, может быть, с CRLF у скриптов
sed -i 's/\r$//' "$DST"/dedicated/linux/*.sh "$DST"/dedicated/linux/*.service
chmod +x "$DST"/dedicated/linux/*.sh
chown -R gpk:gpk "$DST"

echo "== Службы"
cp "$DST/dedicated/linux/gpk-dedicated@.service" /etc/systemd/system/
systemctl daemon-reload
for N in "$@"; do
  CFG="$DST/dedicated/presets/$N.ini"
  [ -f "$CFG" ] || { echo "Нет конфига $CFG - пропущено"; continue; }
  PORT="$(grep -iE '^[[:space:]]*Port[[:space:]]*=' "$CFG" | head -1 | sed 's/.*=[[:space:]]*//; s/[[:space:]\r]*$//')"
  if command -v ufw >/dev/null 2>&1 && ufw status | grep -q "Status: active" && [ -n "$PORT" ]; then
    ufw allow "$PORT/udp" >/dev/null && echo "   ufw: открыт $PORT/udp"
  fi
  systemctl enable "gpk-dedicated@$N" >/dev/null 2>&1
  systemctl restart "gpk-dedicated@$N"
  echo "   gpk-dedicated@$N запущен (порт ${PORT:-?}/udp)"
done

echo
echo "Готово. Первый запуск Wine создаёт свою папку (~1 минута)."
echo "  журнал:     journalctl -u gpk-dedicated@$1 -f"
echo "  лог игры:   tail -f $DST/dedicated/run/$1/system.log"
echo "  стоп/старт: systemctl stop|start|restart gpk-dedicated@$1"
echo "  память:     free -m"
echo "Если у хостера есть свой файрвол в панели - откройте там порты серверов (UDP)."
