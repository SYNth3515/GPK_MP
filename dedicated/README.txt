ВЫДЕЛЕННЫЙ СЕРВЕР GPK_MP
========================

Выделенный сервер - это игра без игрока: без заставок и меню она сразу создаёт сервер по файлу конфига и сама ведёт
раунды. Игроки заходят к нему как к обычному серверу (вкладки Интернет / LAN / прямое подключение).

Как он работает
- Лобби: когда в нём набралось MinPlayers игроков, идёт отсчёт StartDelay секунд (если все нажали «Готов» - 5 секунд).
  Админ (пароль AdminPassword) может начать сразу кнопкой старта.
- Раунд кончился (победитель или время вышло): через RoundEndDelay секунд все возвращаются в лобби, следующий раунд -
  на новой случайной карте (Island / Land = random, RotateMap = 1).
- Все игроки вышли (QuitWhenEmpty = 1) или игра упала: она закрывается и запускается снова.

Конфиги
- dedicated\presets\*.ini - готовые: Deathmatch (порт 29020), TDM (29021), Convoy (29022), Fort (29023),
  LandDuel (29024). Все параметры описаны прямо в файле.
- Свой сервер: скопируйте любой пресет, поменяйте Name, Port и что нужно, запустите с -Config путь\к\файлу.ini.
- У каждого сервера свой порт. Порт (UDP) нужно открыть / пробросить на роутере, либо P2P = 1.

Windows
  powershell -ExecutionPolicy Bypass -File dedicated\start_server.ps1 -Config Deathmatch
  powershell -ExecutionPolicy Bypass -File dedicated\start_all.ps1        (все пресеты, каждый в своём окне)
Сервер работает в папке dedicated\run\<имя> (там его engine.ini, dedicated.ini и logs). Окно игры сервера - чёрное,
маленькое, его не нужно трогать. Остановка - Ctrl+C в окне PowerShell.

Linux (Ubuntu / Debian, VPS без видеокарты): игра идёт через Wine с виртуальным экраном Xvfb
  1) На Windows, в папке игры с модом, соберите облегчённую копию игры (~0.45 ГБ вместо ~9 ГБ, без локаций,
     текстур и звуков - серверу они не нужны):
       powershell -ExecutionPolicy Bypass -File dedicated\make_linux_server.ps1
     Рядом с папкой игры появится архив gpk_server.tgz.
  2) Залейте на VPS в /root два файла: gpk_server.tgz и dedicated\linux\install.sh (WinSCP, MobaXterm, FileZilla).
  3) На VPS:
       sudo bash install.sh gpk_server.tgz Deathmatch              (один сервер)
       sudo bash install.sh gpk_server.tgz Deathmatch LandDuel     (несколько)
       sudo bash install.sh "" TDM                                  (игра уже стоит: включить ещё один сервер)
     Скрипт ставит Wine (32 бит), Xvfb и 32-битный OpenGL, создаёт пользователя gpk, кладёт игру в /opt/gpk_mp,
     запускает службы gpk-dedicated@<имя> (перезапуск при падении и когда все вышли, автозапуск после
     перезагрузки) и открывает порты в ufw. Свой конфиг: положите его в /opt/gpk_mp/dedicated/presets/<имя>.ini
     и запустите  sudo bash install.sh "" <имя>
  Управление:
    journalctl -u gpk-dedicated@Deathmatch -f                    (что происходит)
    tail -f /opt/gpk_mp/dedicated/run/Deathmatch/system.log      (лог игры; прошлые запуски - в logs)
    sudo systemctl stop|start|restart gpk-dedicated@Deathmatch
    sudo systemctl disable --now gpk-dedicated@Deathmatch        (выключить совсем)
  Если у хостера есть свой файрвол в панели - откройте там порты серверов (UDP).
  Без systemd: GPK_LOOP=1 bash /opt/gpk_mp/dedicated/linux/gpk-dedicated.sh Deathmatch

Память: один сервер под Wine - примерно 150-300 МБ (Deathmatch с двумя игроками ~130 МБ на Windows).
Нужно ~1.5 ГБ диска (Wine + 32-битные библиотеки ~1 ГБ, игра ~0.45 ГБ). На VPS с 1 ГБ памяти - один-два сервера,
лучше со swap 1 ГБ.
