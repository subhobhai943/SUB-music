module.exports = {
  name: "stop",
  aliases: [],
  description: "Stop playback and clear queue.",

  async execute({ message, musicManager }) {
    if (!message.guild) return;
    const state = musicManager.getGuildState(message.guild.id);

    state.stop();
    await message.reply("⏹️ Playback stopped and queue cleared.");
  }
};
