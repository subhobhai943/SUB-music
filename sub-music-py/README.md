# SUB Music (Python Edition)

`sub-music-py` is a Python implementation of **SUB Music**, a Discord music bot that streams YouTube audio directly into Discord voice channels without Lavalink.

It uses:
- `discord.py`
- `yt-dlp`
- system-installed **FFmpeg**

## Project Structure

```text
sub-music-py/
├── bot.py
├── config.py
├── requirements.txt
├── README.md
├── commands/
│   ├── help.py
│   ├── leave.py
│   ├── play.py
│   ├── queue.py
│   ├── skip.py
│   └── stop.py
├── music/
│   ├── player.py
│   └── queue_manager.py
└── utils/
    └── ytdl_source.py
```

## Features

- Plays YouTube links directly in Discord voice channels.
- Supports plain text search queries through `yt-dlp`.
- Uses a separate queue for each guild/server.
- Automatically advances to the next song after the current one ends.
- Includes modular command files for easier maintenance.
- Provides basic error handling for voice, extraction, and queue issues.

## Commands

- `s!p <youtube link or search query>` — Play a YouTube video or search result.
- `s!skip` — Skip the current track.
- `s!queue` — Display the current music queue.
- `s!stop` — Stop playback and clear the queue.
- `s!leave` — Disconnect the bot from the voice channel.
- `s!help` — Display available commands.

## Setup

### 1. Install Python dependencies

```bash
pip install -r requirements.txt
```

> **Note:** Discord voice playback also requires FFmpeg to be installed and available on your `PATH`.

### 2. Configure the bot

Edit `config.py`:

```python
DISCORD_TOKEN = "YOUR_BOT_TOKEN"
PREFIX = "s!"
```

### 3. Invite the bot

Make sure your Discord application has these bot permissions/intents enabled:

- **Message Content Intent**
- Permission to connect and speak in voice channels
- Permission to read/send messages in the target text channels

### 4. Start the bot

```bash
python bot.py
```

## How playback works

When a user runs:

```text
s!p <song name or YouTube link>
```

The bot will:

1. Check that the user is in a voice channel.
2. Join that voice channel.
3. Use `yt-dlp` to resolve the YouTube stream.
4. Stream audio with `discord.py` voice features through FFmpeg.
5. Add the track to the guild-specific queue.
6. Automatically play the next queued track when playback ends.

## Notes

- This project does **not** use Lavalink.
- FFmpeg must be installed separately on the machine running the bot.
- Search behavior uses `yt-dlp`'s YouTube search support and selects the first result.
