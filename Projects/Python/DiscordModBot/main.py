import discord
from discord.ext import commands
import os
from dotenv import load_dotenv

# Load environment variables
load_dotenv()
TOKEN = os.getenv("DISCORD_BOT_TOKEN")

intents = discord.Intents.default()
intents.message_content = True
intents.guilds = True
intents.messages = True
intents.members = True  # Required for fetching user objects

bot = commands.Bot(command_prefix="/", intents=intents, case_insensitive=True)
tree = bot.tree

@bot.event
async def on_ready():
    await tree.sync()
    print(f"Logged in as {bot.user}")

@tree.command(name="clearmsg", description="Delete messages at max speed.")
async def clear_messages(interaction: discord.Interaction, user: discord.Member = None, amount: int = 1000):
    """Deletes messages in bulk at maximum speed without delays."""
    await interaction.response.defer(thinking=True)

    try:
        if not interaction.channel.permissions_for(interaction.guild.me).manage_messages:
            await interaction.followup.send("❌ I need the 'Manage Messages' permission!", ephemeral=True)
            return

        deleted_messages = 0
        while deleted_messages < amount:
            batch_size = min(1000, amount - deleted_messages)  # Maximum allowed batch size

            if user:
                msgs_to_delete = [msg async for msg in interaction.channel.history(limit=batch_size) if msg.author.id == user.id]
                if msgs_to_delete:
                    await interaction.channel.delete_messages(msgs_to_delete)  # Bulk delete instantly
            else:
                await interaction.channel.purge(limit=batch_size)  # Bulk delete everything

            deleted_messages += batch_size

        await interaction.followup.send(f"✅ Cleared {deleted_messages} messages at **maximum speed**!", ephemeral=True)

    except discord.Forbidden:
        await interaction.followup.send("❌ I don't have permission to delete messages!", ephemeral=True)

    except discord.HTTPException as e:
        await interaction.followup.send(f"❌ Failed to delete messages: {e}", ephemeral=True)

bot.run(TOKEN)
