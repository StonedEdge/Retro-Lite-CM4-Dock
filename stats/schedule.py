from typing import Dict, List
from datetime import datetime, timedelta

from stats.session import Session


class Schedule:
    def __init__(self, sessions: Dict[str, Dict[str, List[Session]]]):
        self._schedule = {x: {y: 0.0 for y in range(0, 24)} for x in range(0, 7)}
        for system_name, system in sessions.items():
            for game_name, game in system.items():
                for session in game:
                    self._add_session(session.start, session.end)
        local = datetime.now()
        utc = datetime.utcnow()
        diff = (
            int((local - utc).days * 86400 + round((local - utc).seconds, -1)) // 3600
        )
        tmp = {}
        for day, hours in self._schedule.items():
            for hour, val in hours.items():
                tmp.setdefault(day, {})
                tmp[day][(hour + diff) % 24] = val
        self._schedule = tmp

    def _add_session(self, start: datetime, end: datetime):
        weekday = start.weekday()
        bucket = start.hour
        if end.hour == bucket and weekday == end.weekday():
            self._schedule[weekday][bucket] += (end - start).total_seconds()
            return
        bucket_end = (
            start
            - timedelta(
                minutes=start.minute,
                seconds=start.second,
                microseconds=start.microsecond,
            )
            + timedelta(hours=1)
        )
        self._schedule[weekday][bucket] += (bucket_end - start).total_seconds()

        self._add_session(bucket_end, end)

    def get_schedule_data(self) -> Dict[int, Dict[int, float]]:
        return self._schedule

    def print_schedule(self):
        gradient = " ░░░▒▒▒▒▒▒▓▓▓▓▓▓▓▓▓████████████"
        max_value = -1
        for day in self._schedule.values():
            max_value = max(max(day.values()), max_value)

        increment = max_value / (len(gradient) - 1)
        if increment == 0:
            print("Could not find any activity")
            return

        day_names = [
            "Monday",
            "Tuesday",
            "Wednesday",
            "Thursday",
            "Friday",
            "Saturday",
            "Sunday",
        ]
        for weekday, hours in sorted(self._schedule.items()):
            result_num = ""
            result_blk = "  "
            for hour, value in sorted(hours.items()):
                block = int(value // increment)
                result_num += " {} ".format(str(hour).rjust(2))
                result_blk += gradient[block] * 4
            result_num += " 24"
            print(day_names[weekday])
            print(result_num)
            print(result_blk)
