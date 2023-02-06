from datetime import datetime


class Session:
    def __init__(self, start: datetime, end: datetime):
        self.start = start
        self.end = end
        self.duration = (end - start).total_seconds()
