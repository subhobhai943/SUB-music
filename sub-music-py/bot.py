"""Entry point for the Python version of SUB Music."""

from __future__ import annotations

import asyncio
import pathlib

import discord
from discord.ext import commands

import config
from music.player import MusicController

BASE_DIR = pathlib.Path(__file__).parent
COMMANDS_DIR = BASE_DIR / "commands"


class SubMusicBot(commands.Bot):
    """Discord bot implementation for SUB Music."""

    def __init__(self) -> None:
        intents = discord.Intents.default()
        intents.message_content = True
        intents.guilds = True
        intents.voice_states = True
        super().__init__(command_prefix=config.PREFIX, intents=intents, help_command=None)
        self.music_controller = MusicController(self)

    async def setup_hook(self) -> None:
        for command_file in COMMANDS_DIR.glob("*.py"):
            if command_file.name.startswith("__"):
                continue
            await self.load_extension(f"commands.{command_file.stem}")

    async def on_ready(self) -> None:
        if self.user is not None:
            print(f"✅ Logged in as {self.user} (ID: {self.user.id})")
        print(f"🎵 SUB Music is ready. Prefix: {config.PREFIX}")

    async def on_command_error(self, ctx: commands.Context, error: commands.CommandError) -> None:
        if isinstance(error, commands.CommandNotFound):
            return
        if isinstance(error, commands.MissingRequiredArgument):
            await ctx.reply("❌ Missing command argument. Use `s!help` to view usage.")
            return
        await ctx.reply(f"❌ {error}")

    async def close(self) -> None:
        await self.music_controller.shutdown()
        await super().close()


async def main() -> None:
    bot = SubMusicBot()
    async with bot:
        await bot.start(config.DISCORD_TOKEN)


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n[SUB Music] Shutting down...")
