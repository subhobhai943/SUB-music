module.exports = {
  name: "p",
  aliases: ["play"],
  description: "Play a YouTube URL or search query.",

  async execute({ message, args, musicManager }) {
    if (!message.guild) {
      await message.reply("❌ This command can only be used in a server.");
      return;
    }

    const voiceChannel = message.member?.voice?.channel;
    if (!voiceChannel) {
      await message.reply("❌ Join a voice channel first.");
      return;
    }

    const query = args.join(" ").trim();
    if (!query) {
      await message.reply("Usage: `s!p <youtube link or search query>`");
      return;
    }

    const state = musicManager.getGuildState(message.guild.id);

    try {
      await state.connect(voiceChannel);
    } catch (error) {
      console.error("[SUB Music] Voice join failed:", error);
      await message.reply("❌ Could not join your voice channel.");
      return;
    }

    let track;
    try {
      track = await musicManager.resolveTrack(query);
    } catch (error) {
      console.error("[SUB Music] Track resolve failed:", error);
      await message.reply("❌ Failed to fetch this YouTube track.");
      return;
    }

    if (!track) {
      await message.reply("❌ No YouTube results found.");
      return;
    }

    track.requestedBy = message.author.tag;

    const result = await state.enqueueAndMaybePlay(track, message.channel);

    if (result.started) {
      await message.reply(`▶️ Starting playback: **${track.title}**`);
    } else {
      const pending = state.getQueueView().pending.length;
      await message.reply(`✅ Added to queue: **${track.title}** (Position: ${pending})`);
    }
  }
};
