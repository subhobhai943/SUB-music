module.exports = {
  name: "queue",
  aliases: ["q"],
  description: "Show the current queue.",

  async execute({ message, musicManager }) {
    if (!message.guild) return;
    const state = musicManager.getGuildState(message.guild.id);
    const view = state.getQueueView();

    if (!view.current && view.pending.length === 0) {
      await message.reply("ℹ️ The queue is empty.");
      return;
    }

    const lines = [];
    if (view.current) {
      lines.push(`🎵 **Now Playing:** ${view.current.title}`);
    }

    if (view.pending.length) {
      lines.push("\n📜 **Up Next:**");
      view.pending.slice(0, 10).forEach((track, index) => {
        lines.push(`${index + 1}. ${track.title}`);
      });

      if (view.pending.length > 10) {
        lines.push(`...and ${view.pending.length - 10} more`);
      }
    }

    await message.reply(lines.join("\n"));
  }
};
