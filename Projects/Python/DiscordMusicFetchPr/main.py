import os

import discord
import yt_dlp
from discord.ext import commands
from dotenv import load_dotenv

load_dotenv()
TOKEN = os.getenv("DISCORD_BOT_TOKEN")

intents = discord.Intents.default()
intents.message_content = True

bot = commands.Bot(command_prefix="/", intents=intents, case_insensitive=True)
tree = bot.tree

@bot.event
async def on_ready():
    await tree.sync()  # Sync commands to make them visible in the slash command menu
    print(f"Logged in as {bot.user}")

@tree.command(name="join", description="Bot joins your voice channel.")
async def join(interaction: discord.Interaction):
    await interaction.response.defer()  # Acknowledge request instantly
    
    if interaction.user.voice:
        await interaction.user.voice.channel.connect()
        await interaction.followup.send(f"Joined {interaction.user.voice.channel.name}")
    else:
        await interaction.followup.send("You must be in a voice channel!")

@tree.command(name="play", description="Play music from a YouTube URL.")
async def play(interaction: discord.Interaction, youtube_url: str):
    await interaction.response.defer()  # Notify Discord that we're processing

    if not interaction.user.voice:
        await interaction.followup.send("You must be in a voice channel!")
        return

    vc = interaction.guild.voice_client or await interaction.user.voice.channel.connect()

    try:
        ydl_opts = {
            "format": "bestaudio",
            "quiet": True
        }
        with yt_dlp.YoutubeDL(ydl_opts) as ydl:
            info = ydl.extract_info(youtube_url, download=False)
            audio_url = info.get("url")
            title = info.get("title", "Unknown title")

        if vc.is_playing():
            vc.stop()

        vc.play(discord.FFmpegPCMAudio(audio_url, before_options="-reconnect 1 -reconnect_streamed 1 -reconnect_delay_max 5"))
        await interaction.followup.send(f"**Now playing:** {title}")  # Respond after processing
    except Exception as e:
        await interaction.followup.send(f"Error: {e}")

@tree.command(name="pause", description="Pause the music.")
async def pause(interaction: discord.Interaction):
    await interaction.response.defer()
    
    vc = interaction.guild.voice_client
    if vc and vc.is_playing():
        vc.pause()
        await interaction.followup.send("Paused playback.")
    else:
        await interaction.followup.send("No track is currently playing.")

@tree.command(name="resume", description="Resume playback.")
async def resume(interaction: discord.Interaction):
    await interaction.response.defer()
    
    vc = interaction.guild.voice_client
    if vc and vc.is_paused():
        vc.resume()
        await interaction.followup.send("Resumed playback.")
    else:
        await interaction.followup.send("No track is paused.")

@tree.command(name="stop", description="Stop playback.")
async def stop(interaction: discord.Interaction):
    await interaction.response.defer()
    
    vc = interaction.guild.voice_client
    if vc:
        vc.stop()
        await interaction.followup.send("Stopped playback.")
    else:
        await interaction.followup.send("Bot is not in a voice channel!")

@tree.command(name="leave", description="Disconnect the bot from the voice channel.")
async def leave(interaction: discord.Interaction):
    await interaction.response.defer()
    
    vc = interaction.guild.voice_client
    if vc:
        await vc.disconnect()
        await interaction.followup.send("Disconnected from the voice channel.")
    else:
        await interaction.followup.send("Bot is not in a voice channel!")

bot.run(TOKEN)

