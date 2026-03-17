const {
  AudioPlayerStatus,
  NoSubscriberBehavior,
  VoiceConnectionStatus,
  createAudioPlayer,
  createAudioResource,
  entersState,
  joinVoiceChannel
} = require("@discordjs/voice");
const play = require("play-dl");
const MusicQueue = require("./queue");

class GuildMusicState {
  constructor(guildId) {
    this.guildId = guildId;
    this.queue = new MusicQueue();
    this.player = createAudioPlayer({
      behaviors: {
        noSubscriber: NoSubscriberBehavior.Pause
      }
    });
    this.connection = null;
    this.textChannel = null;
    this.isPlaying = false;
    this.currentTrack = null;

    this.player.on(AudioPlayerStatus.Idle, async () => {
      this.isPlaying = false;
      this.currentTrack = null;
      await this.playNext();
    });

    this.player.on("error", async (error) => {
      console.error(`[SUB Music] Player error on guild ${guildId}:`, error);
      this.isPlaying = false;
      this.currentTrack = null;
      if (this.textChannel) {
        await this.textChannel.send("❌ Error while playing audio. Skipping to next track.");
      }
      await this.playNext();
    });
  }

  async connect(voiceChannel) {
    if (
      this.connection &&
      this.connection.joinConfig.channelId === voiceChannel.id &&
      this.connection.state.status !== VoiceConnectionStatus.Destroyed
    ) {
      return this.connection;
    }

    if (this.connection) {
      this.connection.destroy();
    }

    this.connection = joinVoiceChannel({
      channelId: voiceChannel.id,
      guildId: voiceChannel.guild.id,
      adapterCreator: voiceChannel.guild.voiceAdapterCreator,
      selfDeaf: true
    });

    this.connection.on("error", (error) => {
      console.error(`[SUB Music] Voice connection error on guild ${this.guildId}:`, error);
    });

    await entersState(this.connection, VoiceConnectionStatus.Ready, 20_000);
    this.connection.subscribe(this.player);
    return this.connection;
  }

  async enqueueAndMaybePlay(track, textChannel) {
    this.textChannel = textChannel;
    this.queue.enqueue(track);

    if (!this.isPlaying) {
      await this.playNext();
      return { started: true };
    }

    return { started: false };
  }

  async playNext() {
    if (!this.connection || this.connection.state.status === VoiceConnectionStatus.Destroyed) {
      this.queue.clear();
      this.isPlaying = false;
      return;
    }

    const nextTrack = this.queue.dequeue();
    if (!nextTrack) {
      this.isPlaying = false;
      return;
    }

    this.currentTrack = nextTrack;

    try {
      // play-dl supports YouTube URLs and search results.
      const stream = await play.stream(nextTrack.url, {
        discordPlayerCompatibility: true,
        quality: 2
      });

      const resource = createAudioResource(stream.stream, {
        inputType: stream.type,
        inlineVolume: true,
        metadata: nextTrack
      });

      if (resource.volume) {
        resource.volume.setVolume(0.7);
      }

      this.isPlaying = true;
      this.player.play(resource);

      if (this.textChannel) {
        await this.textChannel.send(`🎶 Now playing: **${nextTrack.title}**`);
      }
    } catch (error) {
      console.error(`[SUB Music] Could not play track on guild ${this.guildId}:`, error);
      this.isPlaying = false;
      this.currentTrack = null;
      if (this.textChannel) {
        await this.textChannel.send("❌ Failed to play this track. Trying the next one...");
      }
      await this.playNext();
    }
  }

  skip() {
    if (!this.isPlaying) {
      return false;
    }
    this.player.stop(true);
    return true;
  }

  stop() {
    this.queue.clear();
    this.currentTrack = null;
    this.isPlaying = false;
    this.player.stop(true);
  }

  disconnect() {
    this.stop();
    if (this.connection) {
      this.connection.destroy();
      this.connection = null;
    }
  }

  getQueueView() {
    const pending = this.queue.toArray();
    return {
      current: this.currentTrack,
      pending
    };
  }
}

class MusicManager {
  constructor() {
    this.guildStates = new Map();
  }

  getGuildState(guildId) {
    if (!this.guildStates.has(guildId)) {
      this.guildStates.set(guildId, new GuildMusicState(guildId));
    }
    return this.guildStates.get(guildId);
  }

  removeGuildState(guildId) {
    const state = this.guildStates.get(guildId);
    if (state) {
      state.disconnect();
      this.guildStates.delete(guildId);
    }
  }

  async resolveTrack(query) {
    // Accept direct YouTube links.
    if (play.yt_validate(query) === "video") {
      const info = await play.video_basic_info(query);
      return {
        title: info.video_details.title,
        url: info.video_details.url,
        requestedBy: null
      };
    }

    // Otherwise treat as a YouTube search query.
    const results = await play.search(query, { source: { youtube: "video" }, limit: 1 });
    if (!results.length) {
      return null;
    }

    return {
      title: results[0].title,
      url: results[0].url,
      requestedBy: null
    };
  }
}

module.exports = MusicManager;
