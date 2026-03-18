"""Stop command for SUB Music."""

from __future__ import annotations

from discord.ext import commands


class Stop(commands.Cog):
    def __init__(self, bot: commands.Bot) -> None:
        self.bot = bot

    @commands.command(name="stop")
    async def stop(self, ctx: commands.Context) -> None:
        """Stop playback and clear the queue."""

        if ctx.guild is None:
            return

        await self.bot.music_controller.stop(ctx.guild.id)
        await ctx.reply("⏹️ Playback stopped and queue cleared.")


async def setup(bot: commands.Bot) -> None:
    await bot.add_cog(Stop(bot))
