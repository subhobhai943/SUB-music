"""Help command for SUB Music."""

from __future__ import annotations

from discord.ext import commands


class Help(commands.Cog):
    def __init__(self, bot: commands.Bot) -> None:
        self.bot = bot

    @commands.command(name="help")
    async def help_command(self, ctx: commands.Context) -> None:
        """Display available commands."""

        prefix = self.bot.command_prefix
        lines = [
            "📘 **SUB Music Commands**",
            f"- `{prefix}p <youtube link or search query>` — Play a YouTube URL or search query.",
            f"- `{prefix}skip` — Skip the current track.",
            f"- `{prefix}queue` — Display the current queue.",
            f"- `{prefix}stop` — Stop playback and clear the queue.",
            f"- `{prefix}leave` — Disconnect from the voice channel.",
            f"- `{prefix}help` — Show this help message.",
        ]
        await ctx.reply("\n".join(lines))


async def setup(bot: commands.Bot) -> None:
    await bot.add_cog(Help(bot))
