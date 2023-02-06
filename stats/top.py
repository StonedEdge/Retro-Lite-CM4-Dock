from typing import List, Dict, Callable, Any
import datetime

from stats.gamestats import GameStats
from stats.title import get_title


class TopList:
    def __init__(self, stats: Dict[str, List[GameStats]]):
        self._stats = stats

    def _get_top_times_played(self) -> List[GameStats]:
        return self._get_sorted(lambda x: x.times_played)

    def _get_top_total_time(self) -> List[GameStats]:
        return self._get_sorted(lambda x: x.total_time)

    def _get_top_average(self) -> List[GameStats]:
        return self._get_sorted(lambda x: x.average)

    def _get_top_median(self) -> List[GameStats]:
        return self._get_sorted(lambda x: x.median)

    def _get_sorted(self, key: Callable[[GameStats], Any]) -> List[GameStats]:
        stats = [x for y in list(self._stats.values()) for x in y]
        return sorted(stats, key=key, reverse=True)

    @staticmethod
    def _trim_microseconds(delta: float) -> str:
        td = datetime.timedelta(delta)
        return str(td - datetime.timedelta(microseconds=td.microseconds))

    def _get_top(self, criteria: str) -> List[GameStats]:
        if criteria == "total" or criteria is None:
            return self._get_top_total_time()
        elif criteria == "times":
            return self._get_top_times_played()
        elif criteria == "average":
            return self._get_top_average()
        elif criteria == "median":
            return self._get_top_median()
        return [];

    def print_bar_chart(self, criteria: str, bar_length: int, list_length: int = -1):
        top_list = self._get_top(criteria)
        f = None
        if criteria == "total" or criteria is None:

            def g(x):
                r = x.get_total_time_played()
                return r, str(datetime.timedelta(seconds=r))

            f = g
        elif criteria == "times":

            def g(x):
                r = x.get_times_played()
                return r, str(r)

            f = g
        elif criteria == "average":

            def g(x):
                r = x.get_average_session_time()
                return r, self._trim_microseconds(datetime.timedelta(seconds=r))

            f = g
        elif criteria == "median":

            def g(x):
                r = x.get_median_session_time()
                return r, self._trim_microseconds(datetime.timedelta(seconds=r))

            f = g

        max_value = max(f(x)[0] for x in top_list[:list_length])
        increment = max_value / bar_length
        longest_label_length = max(
            len(get_title(g.get_game(), g.get_system())) for g in top_list[:list_length]
        )
        longest_value_length = max(len(f(g)[1]) for g in top_list[:list_length])

        for g in top_list[:list_length]:
            value, value_string = f(g)
            bar_chunks, remainder = divmod(int(value * 8 / increment), 8)
            bar = "█" * bar_chunks
            title = get_title(g.get_game(), g.get_system())
            if remainder > 0:
                bar += chr(ord("█") + (8 - remainder))
            bar = bar or "▏"
            print(
                f"{title.rjust(longest_label_length)} ▏ "
                f"{value_string.rjust(longest_value_length)} {bar}"
            )

    def get_list_entries_raw(
        self, criteria: str, length: int = 0
    ) -> List[Dict[str, any]]:
        result = []
        top_list = self._get_top(criteria)
        if length == 0:
            length = len(top_list)
        for g in top_list[:length]:
            stats_dict = {
                "title": get_title(g.get_game(), g.get_system()),
                "system": g.get_system(),
                "times": g.get_times_played(),
                "total": g.get_total_time_played(),
                "average": g.get_average_session_time(),
                "median": g.get_median_session_time(),
            }
            result.append(stats_dict)
        return result

    def get_list_entries(self, criteria: str, length: int = 0) -> List[Dict[str, any]]:
        entries = self.get_list_entries_raw(criteria, length)
        for e in entries:
            e["total"] = datetime.timedelta(seconds=e['total'])
            e["average"] = datetime.timedelta(seconds=e["average"])
            e["median"] = datetime.timedelta(seconds=e["median"])
        return entries

    def print_list_entries(self, criteria: str, length: int = -1):
        game_list = self.get_list_entries(criteria, length)

        for i, g in enumerate(game_list, start=1):
            print(
                i,
                f"{g['title']} for {g['system']}, ",
                f"played {g['times']} times, ",
                f"time played: {g['total']}, ",
                f"avg: {g['average']}, ",
                f"median: {g['median']}",
            )
