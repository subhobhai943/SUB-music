# SUB Music (Node.js Edition)

SUB Music is a Discord music bot that plays YouTube audio directly in voice channels.

This version uses:
- `discord.js`
- `@discordjs/voice`
- `play-dl`
- FFmpeg installed on your machine

No Lavalink and no external audio server are required.

## Project Structure

```text
sub-music-js/
├── index.js
├── config.js
├── commands/
│   ├── play.js
│   ├── skip.js
│   ├── queue.js
│   ├── stop.js
│   ├── leave.js
│   └── help.js
├── music/
│   ├── musicManager.js
│   └── queue.js
├── package.json
└── README.md
```

## Commands

- `s!p <youtube link or search query>`
- `s!skip`
- `s!queue`
- `s!stop`
- `s!leave`
- `s!help`

## Setup

1. Install Node.js 18+.
2. Install FFmpeg and ensure `ffmpeg` is available in your PATH.
3. Install dependencies:

```bash
npm install
```

4. Edit `config.js`:

```js
module.exports = {
  token: "YOUR_DISCORD_BOT_TOKEN",
  prefix: "s!"
};
```

5. Start bot:

```bash
node index.js
```

## Notes

- The bot supports direct YouTube links and search queries.
- Queue is maintained per guild.
- Playback continues automatically with next queued track.
- Use Discord Developer Portal to enable **Message Content Intent** and **Server Members/Voice intents as needed**.
