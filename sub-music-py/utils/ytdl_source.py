"""yt-dlp helpers for resolving YouTube tracks and building FFmpeg sources."""

from __future__ import annotations

import asyncio
from typing import Any, Dict

import discord
from yt_dlp import YoutubeDL

from music.queue_manager import Track

YTDL_OPTIONS: Dict[str, Any] = {
    "format": "bestaudio/best",
    "default_search": "ytsearch",
    "noplaylist": True,
    "quiet": True,
    "no_warnings": True,
    "extract_flat": False,
    "source_address": "0.0.0.0",
}

FFMPEG_OPTIONS: Dict[str, str] = {
    "before_options": "-reconnect 1 -reconnect_streamed 1 -reconnect_delay_max 5",
    "options": "-vn",
}


class YTDLSource:
    """Utility wrapper around yt-dlp extraction logic."""

    _ytdl = YoutubeDL(YTDL_OPTIONS)

    @classmethod
    async def create_track(cls, query: str, requested_by: str) -> Track:
        """Resolve a YouTube link or search query into a playable track."""

        loop = asyncio.get_running_loop()
        data = await loop.run_in_executor(None, lambda: cls._ytdl.extract_info(query, download=False))

        if data is None:
            raise ValueError("No results found.")

        if "entries" in data:
            entries = [entry for entry in data["entries"] if entry]
            if not entries:
                raise ValueError("No results found.")
            data = entries[0]

        stream_url = data.get("url")
        webpage_url = data.get("webpage_url") or data.get("original_url") or query
        title = data.get("title") or "Unknown title"

        if not stream_url:
            raise ValueError("Failed to extract an audio stream for this track.")

        return Track(
            title=title,
            webpage_url=webpage_url,
            stream_url=stream_url,
            requested_by=requested_by,
            duration=data.get("duration"),
        )

    @staticmethod
    def create_audio_source(track: Track) -> discord.FFmpegPCMAudio:
        """Create an FFmpeg audio source for a track."""

        return discord.FFmpegPCMAudio(track.stream_url, **FFMPEG_OPTIONS)
