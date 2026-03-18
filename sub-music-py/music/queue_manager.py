"""Queue helpers used by SUB Music."""

from __future__ import annotations

from collections import deque
from dataclasses import dataclass
from typing import Deque, List, Optional


@dataclass(slots=True)
class Track:
    """Represents a playable YouTube track."""

    title: str
    webpage_url: str
    stream_url: str
    requested_by: str
    duration: Optional[int] = None


class MusicQueue:
    """Simple FIFO queue for guild music playback."""

    def __init__(self) -> None:
        self._items: Deque[Track] = deque()

    def enqueue(self, track: Track) -> None:
        self._items.append(track)

    def dequeue(self) -> Optional[Track]:
        if not self._items:
            return None
        return self._items.popleft()

    def clear(self) -> None:
        self._items.clear()

    def is_empty(self) -> bool:
        return not self._items

    def size(self) -> int:
        return len(self._items)

    def to_list(self) -> List[Track]:
        return list(self._items)
