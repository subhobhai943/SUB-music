"""Play command for SUB Music."""

from __future__ import annotations

from discord.ext import commands
from utils.ytdl_source import YTDLSource, YTDLSourceError


class Play(commands.Cog):
    """Commands related to starting playback."""

    def __init__(self, bot: commands.Bot) -> None:
        self.bot = bot

    @commands.command(name="p", aliases=["play"])
    async def play(self, ctx: commands.Context, *, query: str | None = None) -> None:
        """Play a YouTube link or search query."""

        if ctx.guild is None:
            await ctx.reply("❌ This command can only be used in a server.")
            return

        if not query:
            await ctx.reply("Usage: `s!p <youtube link or search query>`")
            return

        try:
            state, track, started = await self.bot.music_controller.enqueue(ctx, query)
        except commands.CommandError as exc:
            await ctx.reply(f"❌ {exc}")
            return
        except YTDLSourceError as exc:
            await ctx.reply(f"❌ Could not load that track: {exc}")
            return
        except Exception:
            await ctx.reply("❌ An unexpected error occurred. Please try again.")
            return

        if started:
            await ctx.reply(f"▶️ Starting playback: **{track.title}**")
        else:
            await ctx.reply(f"✅ Added to queue: **{track.title}** (Position: {state.queue.size()})")


async def setup(bot: commands.Bot) -> None:
    await bot.add_cog(Play(bot))
