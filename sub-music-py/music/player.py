"""Guild music player management for SUB Music."""

from __future__ import annotations

import asyncio
from dataclasses import dataclass, field
from typing import Dict, Optional

import discord
from discord.ext import commands

from music.queue_manager import MusicQueue, Track
from utils.ytdl_source import YTDLSource


@dataclass(slots=True)
class GuildMusicState:
    """Playback state for a single guild."""

    guild_id: int
    queue: MusicQueue = field(default_factory=MusicQueue)
    voice_client: Optional[discord.VoiceClient] = None
    text_channel: Optional[discord.abc.Messageable] = None
    current_track: Optional[Track] = None
    lock: asyncio.Lock = field(default_factory=asyncio.Lock)

    def is_connected(self) -> bool:
        return self.voice_client is not None and self.voice_client.is_connected()

    def is_playing(self) -> bool:
        return self.voice_client is not None and self.voice_client.is_playing()

    def clear(self) -> None:
        self.queue.clear()
        self.current_track = None


class MusicController:
    """Coordinates queues and voice playback across guilds."""

    def __init__(self, bot: commands.Bot) -> None:
        self.bot = bot
        self.guild_states: Dict[int, GuildMusicState] = {}

    def get_state(self, guild_id: int) -> GuildMusicState:
        if guild_id not in self.guild_states:
            self.guild_states[guild_id] = GuildMusicState(guild_id=guild_id)
        return self.guild_states[guild_id]

    async def join_voice_channel(self, ctx: commands.Context) -> GuildMusicState:
        if ctx.guild is None:
            raise commands.CommandError("This command can only be used in a server.")

        voice_channel = getattr(ctx.author.voice, "channel", None)
        if voice_channel is None:
            raise commands.CommandError("Join a voice channel first.")

        state = self.get_state(ctx.guild.id)

        if state.voice_client and state.voice_client.is_connected():
            if state.voice_client.channel != voice_channel:
                await state.voice_client.move_to(voice_channel)
        else:
            state.voice_client = await voice_channel.connect(self_deaf=True)

        state.text_channel = ctx.channel
        return state

    async def enqueue(self, ctx: commands.Context, query: str) -> tuple[GuildMusicState, Track, bool]:
        state = await self.join_voice_channel(ctx)
        track = await YTDLSource.create_track(query, str(ctx.author))
        state.queue.enqueue(track)
        should_start = state.current_track is None and not state.is_playing()

        if should_start:
            await self.play_next(ctx.guild.id)

        return state, track, should_start

    async def play_next(self, guild_id: int) -> None:
        state = self.get_state(guild_id)

        async with state.lock:
            if not state.is_connected():
                state.clear()
                return

            if state.voice_client is not None and state.voice_client.is_playing():
                return

            next_track = state.queue.dequeue()
            if next_track is None:
                state.current_track = None
                return

            state.current_track = next_track

            try:
                audio_source = YTDLSource.create_audio_source(next_track)
            except Exception as exc:  # noqa: BLE001
                await self._notify_text_channel(state, f"❌ Failed to prepare **{next_track.title}**: {exc}")
                state.current_track = None
                await self.play_next(guild_id)
                return

            def after_playback(error: Optional[Exception]) -> None:
                if error:
                    self.bot.loop.call_soon_threadsafe(
                        asyncio.create_task,
                        self._notify_text_channel(state, f"❌ Playback error: {error}"),
                    )
                future = asyncio.run_coroutine_threadsafe(self._advance_queue(guild_id), self.bot.loop)
                try:
                    future.result()
                except Exception as callback_error:  # noqa: BLE001
                    print(f"[SUB Music] Queue advance failed for guild {guild_id}: {callback_error}")

            state.voice_client.play(audio_source, after=after_playback)
            await self._notify_text_channel(
                state,
                f"🎶 Now playing: **{next_track.title}**\n🔗 {next_track.webpage_url}",
            )

    async def _advance_queue(self, guild_id: int) -> None:
        state = self.get_state(guild_id)
        state.current_track = None
        await self.play_next(guild_id)

    async def skip(self, guild_id: int) -> bool:
        state = self.get_state(guild_id)
        if state.voice_client is None or not state.voice_client.is_playing():
            return False
        state.voice_client.stop()
        return True

    async def stop(self, guild_id: int) -> bool:
        state = self.get_state(guild_id)
        had_activity = state.current_track is not None or not state.queue.is_empty()
        state.clear()
        if state.voice_client and state.voice_client.is_playing():
            state.voice_client.stop()
        return had_activity

    async def leave(self, guild_id: int) -> bool:
        state = self.get_state(guild_id)
        if state.voice_client is None:
            return False

        state.clear()
        if state.voice_client.is_playing():
            state.voice_client.stop()
        await state.voice_client.disconnect()
        state.voice_client = None
        state.text_channel = None
        return True

    def get_queue_view(self, guild_id: int) -> tuple[Optional[Track], list[Track]]:
        state = self.get_state(guild_id)
        return state.current_track, state.queue.to_list()

    async def shutdown(self) -> None:
        for guild_id in list(self.guild_states):
            try:
                await self.leave(guild_id)
            except Exception:  # noqa: BLE001
                pass

    async def _notify_text_channel(self, state: GuildMusicState, message: str) -> None:
        if state.text_channel is not None:
            await state.text_channel.send(message)
