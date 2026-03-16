# SUB Music (C Discord Music Bot)

SUB Music is a Discord music bot written in C with command prefix `s!`.

It supports:
- `s!p <youtube_link_or_song_name>`
- `s!skip`
- `s!stop`
- `s!leave`
- `s!help`

## Features

- Joins the requester's voice channel
- Plays YouTube links directly
- Searches YouTube when a text query is provided (`ytsearch1:`)
- Queue support (up to 100 songs)
- Skip, stop, and leave controls
- Modular C project layout for scaling

## Project Structure

```text
sub-music-bot/
│
├── src/
│   ├── main.c
│   ├── commands.c
│   ├── commands.h
│   ├── music_player.c
│   ├── music_player.h
│   ├── youtube.c
│   ├── youtube.h
│   └── voice.c
│
├── include/
├── config/
│   └── config.json
├── scripts/
│   └── run.sh
├── Makefile
└── README.md
```

## Dependencies

Install runtime/build dependencies on Linux:

```bash
sudo apt update
sudo apt install -y ffmpeg yt-dlp libsodium-dev libopus-dev libcurl4-openssl-dev
```

Install Concord library from source:

```bash
git clone https://github.com/Cogmasters/concord.git
cd concord
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
sudo cmake --install build
```

## Configure

Edit `config/config.json`:

```json
{
  "token": "YOUR_DISCORD_BOT_TOKEN",
  "prefix": "s!"
}
```

## Build

```bash
make
```

## Run

```bash
./submusic
```

or:

```bash
./scripts/run.sh
```

## Commands

- `s!p <song or link>` → Play music
- `s!skip` → Skip current song
- `s!stop` → Stop playback and clear queue
- `s!leave` → Leave voice channel
- `s!help` → Show command help

## Audio Pipeline

Playback flow:

1. Resolve YouTube URL/title with `yt-dlp` (`--get-url --get-title`).
2. Stream best audio and decode to PCM with FFmpeg:
   - `yt-dlp -f bestaudio -o - ... | ffmpeg -i pipe:0 -f s16le -ar 48000 -ac 2 pipe:1`
3. Encode PCM frames to Opus.
4. Send Opus frames through Discord voice transport layer.

## Notes

- The `voice_send_opus_frame()` hook in `src/voice.c` is where you bind Concord's voice packet send call for your installed Concord version.
- The rest of the bot (commands, queueing, yt-dlp/ffmpeg pipeline, Opus encoding, threading) is implemented and ready.
- Logging is printed to stdout for ready state and now playing events.
