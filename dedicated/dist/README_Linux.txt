GPK_MP - ВЫДЕЛЕННЫЙ СЕРВЕР ДЛЯ LINUX (VPS)
==========================================

Выделенный сервер - это игра без игрока: она сама создаёт сервер, ведёт раунды, меняет карты
и перезапускается, если упала или все вышли. На Linux она идёт через Wine, видеокарта не нужна.
Сама игра для него не нужна - всё уже в этом архиве.

Что нужно
- VPS с Ubuntu 20.04+ / Debian 11+ (x86_64), root или sudo.
- 1 ГБ памяти (на 1-2 сервера, лучше со swap 1 ГБ), ~1.5 ГБ свободного диска (Wine ~1 ГБ + сервер ~0.5 ГБ).

Установка
1. Залейте GPK_MP_Dedicated_Linux.tar.gz на VPS в /root (WinSCP, MobaXterm, FileZilla - протокол SFTP).
2. На VPS (SSH):
     cd /root
     tar -xzf GPK_MP_Dedicated_Linux.tar.gz
     sudo bash GPK_MP_Dedicated/dedicated/linux/install.sh Deathmatch
   Несколько серверов: ... install.sh Deathmatch LandDuel   (у каждого свой порт)
   Скрипт ставит Wine, Xvfb и 32-битный OpenGL, создаёт пользователя gpk, переносит сервер в /opt/gpk_mp,
   запускает службу (перезапуск при падении, автозапуск после перезагрузки VPS) и открывает порт в ufw.
   Архив после установки можно удалить:  rm /root/GPK_MP_Dedicated_Linux.tar.gz
3. Если у хостера есть свой файрвол в панели управления - откройте там порт сервера (UDP).

Настройки (обязательно смените AdminPassword: в пресетах стоит 1234, и Name - имя в списке серверов)
  nano /opt/gpk_mp/dedicated/presets/Deathmatch.ini            (сохранить: Ctrl+O, Enter, выход Ctrl+X)
  sudo systemctl restart gpk-dedicated@Deathmatch
  Готовые пресеты: Deathmatch (порт 29020), TDM (29021), Convoy (29022), Fort (29023), LandDuel (29024).
  Свой сервер: cp Deathmatch.ini MyServer.ini, поменяйте Name и Port, затем
     sudo bash /opt/gpk_mp/dedicated/linux/install.sh MyServer

Управление
  journalctl -u gpk-dedicated@Deathmatch -f                 что происходит (выход Ctrl+C)
  tail -f /opt/gpk_mp/dedicated/run/Deathmatch/system.log   лог игры (прошлые запуски - в папке logs)
  sudo systemctl restart gpk-dedicated@Deathmatch           перезапуск
  sudo systemctl disable --now gpk-dedicated@Deathmatch     выключить совсем
  free -m                                                   память

Обновление: залейте новый архив и повторите шаг 2. Логи и ваши настройки в presets сохранятся.

Подробно: GPK_MP_Dedicated/dedicated/README.txt (после установки - /opt/gpk_mp/dedicated/README.txt)
