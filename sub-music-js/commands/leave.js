module.exports = {
  name: "leave",
  aliases: ["disconnect"],
  description: "Leave the voice channel.",

  async execute({ message, musicManager }) {
    if (!message.guild) return;

    musicManager.removeGuildState(message.guild.id);
    await message.reply("👋 Left the voice channel and cleared the queue.");
  }
};
