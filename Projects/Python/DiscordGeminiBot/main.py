import json
import os

import discord
import requests
from discord import app_commands
from discord.ext import commands
from dotenv import load_dotenv

# Load environment variables from .env
load_dotenv()
TOKEN = os.getenv("DISCORD_TOKEN")
GEMINI_API_KEY = os.getenv("GEMINI_API_KEY")

# Setup Discord bot intents
intents = discord.Intents.default()
intents.message_content = True

# Create bot instance
bot = commands.Bot(command_prefix="/", intents=intents)
tree = bot.tree  # Access the application command tree

# File to store conversation history
HISTORY_FILE = "ChatHist.json"

def normalize_message(message):
    """Normalize a message to match the required format."""
    role = message.get("role", "user")
    parts = [{"text": part.get("text", "")} for part in message.get("parts", [])]
    return {"role": role, "parts": parts}

def load_chat_history():
    """Load conversation history from file."""
    if os.path.exists(HISTORY_FILE):
        try:
            with open(HISTORY_FILE, "r") as f:
                raw_history = json.load(f)
        except json.JSONDecodeError:
            raw_history = {}
    else:
        raw_history = {}

    return {user_id: [normalize_message(msg) for msg in messages] for user_id, messages in raw_history.items()}

def save_chat_history(history):
    """Save conversation history to file."""
    with open(HISTORY_FILE, "w") as f:
        json.dump(history, f, indent=4)

def generate_content(prompt, user_id, max_tokens=1000):
    """Generate a response from Gemini API using conversation history."""
    chat_hist = load_chat_history()
    key = str(user_id)
    user_history = chat_hist.get(key, [])

    # Append the new user message
    user_history.append({"role": "user", "parts": [{"text": prompt}]})

    # Keep only the last 6 messages
    user_history = user_history[-6:]
    chat_hist[key] = user_history

    url = f"https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash:generateContent?key={GEMINI_API_KEY}"
    headers = {"Content-Type": "application/json"}
    payload = {
        "contents": user_history,
        "generationConfig": {"maxOutputTokens": max_tokens}
    }

    response = requests.post(url, headers=headers, json=payload)

    if response.status_code == 200:
        res_json = response.json()
        if "candidates" in res_json:
            model_text = res_json["candidates"][0]["content"]["parts"][0]["text"]
            user_history.append({"role": "model", "parts": [{"text": model_text}]})
            chat_hist[key] = user_history
            save_chat_history(chat_hist)
            return model_text

    save_chat_history(chat_hist)
    return f"Error {response.status_code}: {response.text}"

@bot.event
async def on_ready():
    """Event when bot is ready."""
    await tree.sync()  # Sync slash commands
    print(f"Bot is live! {bot.user.name} is ready.")

@tree.command(name="chat", description="Chat with the AI bot.")
async def chat(interaction: discord.Interaction, prompt: str):
    """Handles the /chat command."""
    response_text = generate_content(prompt, interaction.user.id, max_tokens=1000)
    
    try:
        await interaction.response.send_message(response_text)
    except discord.errors.NotFound:
        await interaction.followup.send(response_text)

@tree.command(name="c", description="Alias for /chat command.")
async def c(interaction: discord.Interaction, prompt: str):
    """Handles the /c command."""
    response_text = generate_content(prompt, interaction.user.id, max_tokens=1000)

    try:
        await interaction.response.send_message(response_text)
    except discord.errors.NotFound:
        await interaction.followup.send(response_text)

bot.run(TOKEN)
