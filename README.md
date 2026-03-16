# SUB Music (C + Discord Gateway + Lavalink)

SUB Music is a lightweight Discord music bot written in C.
It **does not use Concord** and avoids implementing Discord voice/audio protocols directly.

Instead:
- The bot handles Discord Gateway events and commands.
- Lavalink handles YouTube extraction (`yt-dlp`) and voice streaming.

## Architecture

```text
Discord Gateway (WebSocket)
      |
      v
 src/discord_gateway.c
      |
      v
 src/commands.c  ---> src/queue.c
      |
      v
 src/lavalink_client.c (REST via libcurl)
      |
      v
 Lavalink v4 server
```

### Responsibilities

- `discord_gateway.c`
  - Connects to `wss://gateway.discord.gg`
  - Sends IDENTIFY + heartbeat
  - Receives dispatch events (`READY`, `MESSAGE_CREATE`, `VOICE_STATE_UPDATE`)
  - Sends voice state updates (join/leave channel)

- `commands.c`
  - Parses `s!` commands
  - Supports:
    - `s!p <song or link>`
    - `s!skip`
    - `s!stop`
    - `s!leave`
    - `s!help`
  - Calls Lavalink REST API and queue manager

- `lavalink_client.c`
  - Uses `GET /v4/loadtracks?identifier=ytsearch:QUERY`
  - Uses `PATCH /v4/sessions/{sessionId}/players/{guildId}`

- `queue.c`
  - In-memory per-guild queue

## Project layout

```text
sub-music/
├── src/
│   ├── main.c
│   ├── discord_gateway.c
│   ├── discord_gateway.h
│   ├── commands.c
│   ├── commands.h
│   ├── lavalink_client.c
│   ├── lavalink_client.h
│   ├── queue.c
│   ├── queue.h
│
├── config/
│   └── config.json
│
├── scripts/
│   └── run.sh
│
├── Makefile
└── README.md
```

## Dependencies

Install development packages (names may vary by distro):

- `libwebsockets`
- `libcurl`
- `jansson`
- `pkg-config`
- `make`
- `gcc`

## Build

```bash
make
```

Output binary:

```bash
./submusic
```

## Configuration

Edit `config/config.json`:

```json
{
  "discord": {
    "token": "YOUR_DISCORD_BOT_TOKEN",
    "intents": "513"
  },
  "lavalink": {
    "host": "127.0.0.1",
    "port": 2333,
    "password": "youshallnotpass"
  }
}
```

- `intents=513` enables guild messages + message content. If you need voice-state tracking reliability, include voice-state intent as needed in your bot/app settings and intent bitmask.

## Run Lavalink

Use a Lavalink v4 setup (Java 17+), with a matching `application.yml` password and `yt-dlp` support.

Typical steps:

1. Download Lavalink v4 jar
2. Provide `application.yml` with:
   - `server.port: 2333`
   - `lavalink.server.password: youshallnotpass`
3. Start:
   ```bash
   java -jar Lavalink.jar
   ```

Then run SUB Music:

```bash
scripts/run.sh
```

## Notes

- Audio playback/streaming is fully handled by Lavalink.
- This implementation focuses on a simplified architecture and keeps bot logic lightweight in C.
