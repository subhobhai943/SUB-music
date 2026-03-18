"""Queue command for SUB Music."""

from __future__ import annotations

from discord.ext import commands


class Queue(commands.Cog):
    def __init__(self, bot: commands.Bot) -> None:
        self.bot = bot

    @commands.command(name="queue", aliases=["q"])
    async def queue(self, ctx: commands.Context) -> None:
        """Display the current queue."""

        if ctx.guild is None:
            return

        current, pending = self.bot.music_controller.get_queue_view(ctx.guild.id)
        if current is None and not pending:
            await ctx.reply("ℹ️ The queue is empty.")
            return

        lines: list[str] = []
        if current is not None:
            lines.append(f"🎵 **Now Playing:** {current.title}")

        if pending:
            lines.append("\n📜 **Up Next:**")
            for index, track in enumerate(pending[:10], start=1):
                lines.append(f"{index}. {track.title} — requested by {track.requested_by}")
            if len(pending) > 10:
                lines.append(f"...and {len(pending) - 10} more")

        await ctx.reply("\n".join(lines))


async def setup(bot: commands.Bot) -> None:
    await bot.add_cog(Queue(bot))
