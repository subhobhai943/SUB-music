const fs = require("fs");
const path = require("path");
const { Client, GatewayIntentBits, Partials } = require("discord.js");

const config = require("./config");
const MusicManager = require("./music/musicManager");

const client = new Client({
  intents: [
    GatewayIntentBits.Guilds,
    GatewayIntentBits.GuildMessages,
    GatewayIntentBits.GuildVoiceStates,
    GatewayIntentBits.MessageContent
  ],
  partials: [Partials.Channel]
});

const musicManager = new MusicManager();
const commandMap = new Map();

function loadCommands() {
  const commandsDir = path.join(__dirname, "commands");
  const files = fs.readdirSync(commandsDir).filter((file) => file.endsWith(".js"));

  for (const file of files) {
    const command = require(path.join(commandsDir, file));
    commandMap.set(command.name, command);
    for (const alias of command.aliases || []) {
      commandMap.set(alias, command);
    }
  }
}

client.once("ready", () => {
  console.log(`✅ Logged in as ${client.user.tag}`);
  console.log(`🎵 SUB Music is ready. Prefix: ${config.prefix}`);

});

client.on("messageCreate", async (message) => {
  if (message.author.bot || !message.content.startsWith(config.prefix)) {
    return;
  }

  const input = message.content.slice(config.prefix.length).trim();
  if (!input.length) {
    return;
  }

  const [rawCommand, ...args] = input.split(/\s+/);
  const commandName = rawCommand.toLowerCase();
  const command = commandMap.get(commandName);

  if (!command) {
    return;
  }

  try {
    await command.execute({
      client,
      message,
      args,
      prefix: config.prefix,
      musicManager,
      commandMap
    });
  } catch (error) {
    console.error(`[SUB Music] Command '${commandName}' failed:`, error);
    await message.reply("❌ Something went wrong while processing the command.");
  }
});

client.on("error", (error) => {
  console.error("[SUB Music] Client error:", error);
});

process.on("SIGINT", () => {
  console.log("\n[SUB Music] Shutting down...");
  for (const guildId of musicManager.guildStates.keys()) {
    musicManager.removeGuildState(guildId);
  }
  process.exit(0);
});

loadCommands();
client.login(config.token);
