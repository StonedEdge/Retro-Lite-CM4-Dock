import statistics
from typing import Dict, List

from stats.session import Session


class GameStats:
    def __init__(
        self,
        game: str,
        system: str,
        times_played: int,
        total_time_played: int,
        average_session: int,
        median_session: int,
    ):
        self.game = game
        self.system = system
        self.times_played = times_played
        self.total_time = total_time_played
        self.average = average_session
        self.median = median_session

    def get_game(self) -> str:
        return self.game

    def get_system(self) -> str:
        return self.system

    def get_times_played(self) -> int:
        return self.times_played

    def get_total_time_played(self) -> int:
        return self.total_time

    def get_average_session_time(self) -> int:
        return self.average

    def get_median_session_time(self) -> int:
        return self.median


def get_stats_from_sessions(
    sessions: Dict[str, Dict[str, List[Session]]]
) -> Dict[str, List[GameStats]]:
    aggregate = {}
    for system_name, system in sessions.items():
        for game_name, game in system.items():
            times_played = len(game)
            total_time = 0
            session_lengths = []
            for session in game:
                session_lengths.append(session.duration)
                total_time += session.duration
            average = 0
            median = 0
            if times_played > 0:
                average = total_time / times_played
                median = statistics.median(session_lengths)

            stats = GameStats(
                game_name, system_name, times_played, total_time, average, median
            )
            if system_name in aggregate:
                aggregate[system_name].append(stats)
            else:
                aggregate[system_name] = [stats]
    return aggregate
