module.exports = {
  name: "skip",
  aliases: [],
  description: "Skip the currently playing track.",

  async execute({ message, musicManager }) {
    if (!message.guild) return;
    const state = musicManager.getGuildState(message.guild.id);

    if (!state.skip()) {
      await message.reply("ℹ️ Nothing is currently playing.");
      return;
    }

    await message.reply("⏭️ Skipped the current track.");
  }
};
