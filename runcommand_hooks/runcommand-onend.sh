echo $(date -u +%Y-%m-%dT%H:%M:%S%z)'|'end'|'$1'|'$2'|'$3'|'$4 >> ~/RetroPie/game_stats.log
python3 /home/pi/game_end.py &
