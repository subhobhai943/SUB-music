"""Skip command for SUB Music."""

from __future__ import annotations

from discord.ext import commands


class Skip(commands.Cog):
    def __init__(self, bot: commands.Bot) -> None:
        self.bot = bot

    @commands.command(name="skip")
    async def skip(self, ctx: commands.Context) -> None:
        """Skip the current track."""

        if ctx.guild is None:
            return

        skipped = await self.bot.music_controller.skip(ctx.guild.id)
        if not skipped:
            await ctx.reply("ℹ️ Nothing is currently playing.")
            return

        await ctx.reply("⏭️ Skipped the current track.")


async def setup(bot: commands.Bot) -> None:
    await bot.add_cog(Skip(bot))
