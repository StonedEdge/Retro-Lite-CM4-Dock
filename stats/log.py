import csv
import datetime
from typing import Dict, List, Optional
from collections import namedtuple
from collections import defaultdict

from stats.session import Session


class Log:
    DATE_FORMAT = "%Y-%m-%dT%H:%M:%S%z"

    def __init__(self, path: str):
        self._sessions = defaultdict(lambda: defaultdict(lambda: []))

        with open(path, "r") as f:
            fieldnames = ["date", "type", "system", "emulator", "path", "command"]
            rows = csv.DictReader(f, fieldnames, delimiter="|", skipinitialspace=True)
            SessionStart = namedtuple("SessionStart", "date system game")
            start = None
            for row in rows:
                system = row["system"]
                date = datetime.datetime.strptime(row["date"], self.DATE_FORMAT)
                game = row["path"]
                if game == "" and system == row["emulator"]:
                    game = system
                if row["type"] == "start":
                    # Overwrite previous values if we didn't find an end tag
                    start = SessionStart(date, system, game)

                elif row["type"] == "end":
                    if start is None:
                        # Missing start for this end
                        continue
                    if not start.system == system or not start.game == game:
                        # Start and end mismatch, discard data
                        start = None
                        continue

                    # Start and end matches
                    session = Session(start.date, date)
                    self._sessions[system][game].append(session)
                    start = None
                else:
                    raise ValueError("Bad type")

    def get_sessions(
        self,
        systems: Optional[List[str]] = None,
        exclude_systems: Optional[List[str]] = None,
        skip_shorter_than: Optional[int] = 0,
        lookback: int = 0,
    ) -> Dict[str, Dict[str, List[Session]]]:

        start_date = datetime.datetime.min
        if lookback > 0:
            start_date = datetime.datetime.now()
            start_date = start_date.replace(hour=0, minute=0, second=0, microsecond=0)
            start_date -= datetime.timedelta(days=(lookback - 1))
        start_date = start_date.replace(tzinfo=datetime.timezone.utc)

        result = defaultdict(lambda: defaultdict(lambda: []))
        for system, games in self._sessions.items():
            if systems is not None and system not in systems:
                continue
            if exclude_systems is not None and system in exclude_systems:
                continue
            for game, sessions in games.items():
                for session in sessions:
                    if session.start < start_date:
                        continue
                    if session.duration < skip_shorter_than:
                        continue
                    result[system][game].append(session)
        return result
