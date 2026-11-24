#!/bin/bash
# GPK_MP: dedicated server on Linux (Wine + a virtual screen Xvfb, no video card needed).
#   gpk-dedicated.sh Deathmatch            # a preset of dedicated/presets
#   gpk-dedicated.sh /path/to/my.ini       # an own config (a copy of any preset)
# The server runs in dedicated/run/<name> (symlinks to the game + its own engine.ini, dedicated.ini, logs).
# This script runs the game ONCE; systemd (gpk-dedicated@.service) starts it again after a crash / when all players left.
# Without systemd: GPK_LOOP=1 gpk-dedicated.sh Deathmatch
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
DED="$(dirname "$HERE")"
GAME="${GPK_GAME:-$(dirname "$DED")}"
CFG="${1:-Deathmatch}"
[ -f "$CFG" ] || CFG="$DED/presets/$CFG.ini"
[ -f "$CFG" ] || { echo "no config: $1"; exit 1; }
[ -f "$GAME/engine.exe" ] || { echo "no engine.exe in $GAME"; exit 1; }
NAME="$(basename "$CFG" .ini)"
RUN="$DED/run/$NAME"
mkdir -p "$RUN/logs" "$RUN/SAVE"

for d in PROGRAM RESOURCE modules; do
  [ -e "$RUN/$d" ] || ln -s "$GAME/$d" "$RUN/$d"
done
for f in "$GAME"/*.exe "$GAME"/*.dll "$GAME"/*.dat; do
  b="$(basename "$f")"
  case "$b" in unins*|config.exe) continue;; esac
  [ -e "$RUN/$b" ] || ln -s "$f" "$RUN/$b"
done

# engine.ini of the server: the one of the game, a small window, few frames
sed -e 's/^\s*full_screen\s*=.*/full_screen = 0/' -e 's/^\s*screen_x\s*=.*/screen_x = 640/' \
    -e 's/^\s*screen_y\s*=.*/screen_y = 480/' -e 's/^\s*max_fps\s*=.*/max_fps = 0/' \
    -e 's/^\s*show_fps\s*=.*/show_fps = 0/' -e 's/^\s*PostProcess\s*=.*/PostProcess = 0/' \
    "$GAME/engine.ini" > "$RUN/engine.ini"
# dedicated.ini = the config + every missing / empty key as "default" (the game crashes on a missing key)
KEYS="Enabled Name Port Mode Teams Password AdminPassword MaxPlayers Island Land Weather Respawn RespawnTime TimeLimit GameSpeed LateJoin StartPosition Credit MaxShipClass BotSkill NoBoarding PassivePerks ActivePerks FastBoarding PistolReload Internet Lan P2P MaxFps MinPlayers StartDelay RoundEndDelay QuitWhenEmpty RotateMap Render"
awk -v keys="$KEYS" '
  BEGIN { n = split(keys, K, " ") }
  { sub(/\r$/, "") }
  /^[ \t]*\[Server\]/ { sec = 1 }
  /^[ \t]*[A-Za-z0-9_]+[ \t]*=/ {
      k = $0; sub(/[ \t]*=.*/, "", k); sub(/^[ \t]*/, "", k)
      v = $0; sub(/^[^=]*=[ \t]*/, "", v); sub(/[ \t]+$/, "", v)
      if (k == "Enabled") v = "1"
      if (v == "") v = "default"
      have[tolower(k)] = 1; $0 = k " = " v
  }
  { L[++m] = $0 }
  END {
      if (!sec) printf "[Server]\r\n"
      for (i = 1; i <= m; i++) printf "%s\r\n", L[i]
      for (i = 1; i <= n; i++) if (!(tolower(K[i]) in have)) printf "%s = %s\r\n", K[i], (K[i] == "Enabled" ? "1" : "default")
  }' "$CFG" > "$RUN/dedicated.ini"

export WINEDEBUG="${WINEDEBUG:--all}"
export WINEPREFIX="${WINEPREFIX:-$HOME/.wine-gpk}"
export WINEARCH=win32
# no Mono / Gecko (the game needs neither): their install dialog would hang a headless first start
export WINEDLLOVERRIDES="${WINEDLLOVERRIDES:-mscoree,mshtml=}"
cd "$RUN" || exit 1
# no sound card (a VPS): FMOD fails and writes an error to system.log every frame -> an empty ALSA device
if [ ! -e /proc/asound/card0 ] && [ ! -f "$HOME/.asoundrc" ]; then
  printf 'pcm.!default {\n  type null\n}\n' > "$HOME/.asoundrc"
fi
# a crash must end the process (systemd restarts it): without this Wine shows its crash dialog on Xvfb and waits forever
if [ ! -f "$WINEPREFIX/.gpk_nocrashdlg" ]; then
  xvfb-run -a wine reg add 'HKCU\Software\Wine\WineDbg' /v ShowCrashDialog /t REG_DWORD /d 0 /f >/dev/null 2>&1 && touch "$WINEPREFIX/.gpk_nocrashdlg"
fi

run_once() {
  echo "$(date '+%F %T') $NAME: start"
  xvfb-run -a -s "-screen 0 640x480x24" wine engine.exe
  code=$?
  stamp="$(date '+%Y-%m-%d_%H-%M-%S')"
  for l in compile.log system.log error.log; do [ -f "$l" ] && mv -f "$l" "logs/${stamp}_$l"; done
  ls -1t logs | tail -n +61 | while read -r x; do rm -f "logs/$x"; done
  echo "$(date '+%F %T') $NAME: the game has closed (code $code)"
  return $code
}

if [ "${GPK_LOOP:-0}" = "1" ]; then
  while true; do run_once; sleep 5; done
else
  run_once
fi
