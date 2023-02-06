from typing import Dict, List

from stats.session import Session
from stats.title import get_title


class History:
    def __init__(
        self, sessions: Dict[str, Dict[str, List[Session]]], list_length: int = -1
    ):
        history = []
        for system, g in sessions.items():
            for game, s in g.items():
                for session in s:
                    info = {
                        "system": system,
                        "game": get_title(game, system),
                        "date": session.start,
                    }
                    history.append(info)
        history.sort(key=lambda x: x["date"], reverse=True)
        self._history = history[:list_length]

    def get_history_data(self) -> List[Dict[str, Dict[str, any]]]:
        return self._history

    def print_history(self):
        for session in self._history:
            print(session["date"], session["system"], session["game"])
