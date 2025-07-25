import discord
from discord.ext import commands
import yt_dlp
import os
import asyncio
from mega import Mega
from dotenv import load_dotenv

# Load environment variables
load_dotenv()
TOKEN = os.getenv("DISCORD_BOT_TOKEN")
MEGA_EMAIL = os.getenv("MEGA_EMAIL")  # Your MEGA account email
MEGA_PASSWORD = os.getenv("MEGA_PASSWORD")  # Your MEGA account password

intents = discord.Intents.default()
intents.message_content = True

bot = commands.Bot(command_prefix="/", intents=intents, case_insensitive=True)
tree = bot.tree

@bot.event
async def on_ready():
    await tree.sync()
    print(f"Logged in as {bot.user}")

async def download_video(url: str):
    """Downloads a YouTube video with error handling."""
    ydl_opts = {
        "format": "mp4/best",
        "outtmpl": "video.mp4",
        "quiet": True,
        "noplaylist": True,
        "timeout": 60,
        "retries": 3,
        "force-ipv4": True,
    }
    try:
        await asyncio.to_thread(lambda: yt_dlp.YoutubeDL(ydl_opts).download([url]))
    except Exception as e:
        raise RuntimeError(f"Failed to download video: {e}")

async def upload_to_mega(file_path: str):
    """Uploads video to MEGA and retrieves download link."""
    try:
        mega = Mega()
        m = mega.login(MEGA_EMAIL, MEGA_PASSWORD)
        uploaded_file = m.upload(file_path)
        file_link = m.get_link(uploaded_file)

        if not file_link:
            raise RuntimeError("MEGA upload failed: No download link received.")
        
        return file_link

    except Exception as e:
        raise RuntimeError(f"MEGA upload failed: {e}")

@tree.command(name="download", description="Download and upload a YouTube video to MEGA.")
async def download(interaction: discord.Interaction, youtube_url: str):
    await interaction.response.defer()

    try:
        await interaction.followup.send("Downloading video...", ephemeral=True)
        await download_video(youtube_url)

        await interaction.followup.send("Uploading to MEGA...", ephemeral=True)
        mega_link = await upload_to_mega("video.mp4")

        os.remove("video.mp4")  # Clean up local file after upload

        # Send a clean download embed
        embed = discord.Embed(
            title="Video Download",
            description="Click the button below to download:",
            color=discord.Color.blue()
        )
        embed.add_field(name="Download Link", value=f"[Click Here]({mega_link})", inline=False)

        await interaction.followup.send(embed=embed)

    except RuntimeError as e:
        await interaction.followup.send(f"Error: {e}")

    except Exception as e:
        await interaction.followup.send(f"Unexpected error: {e}")

bot.run(TOKEN)
