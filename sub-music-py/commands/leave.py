"""Leave command for SUB Music."""

from __future__ import annotations

from discord.ext import commands


class Leave(commands.Cog):
    def __init__(self, bot: commands.Bot) -> None:
        self.bot = bot

    @commands.command(name="leave", aliases=["disconnect"])
    async def leave(self, ctx: commands.Context) -> None:
        """Disconnect the bot from voice."""

        if ctx.guild is None:
            return

        left = await self.bot.music_controller.leave(ctx.guild.id)
        if not left:
            await ctx.reply("ℹ️ I am not connected to a voice channel.")
            return

        await ctx.reply("👋 Left the voice channel and cleared the queue.")


async def setup(bot: commands.Bot) -> None:
    await bot.add_cog(Leave(bot))
