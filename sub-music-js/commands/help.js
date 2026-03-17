module.exports = {
  name: "help",
  aliases: ["commands"],
  description: "Show available commands.",

  async execute({ message, commandMap, prefix }) {
    const unique = new Map();
    for (const command of commandMap.values()) {
      if (!unique.has(command.name)) {
        unique.set(command.name, command);
      }
    }

    const lines = ["📘 **SUB Music Commands**"];
    for (const command of unique.values()) {
      lines.push(`- \`${prefix}${command.name}\` — ${command.description}`);
    }

    await message.reply(lines.join("\n"));
  }
};
