import discord
from discord.ext import commands

intents = discord.Intents.all()  # Enable all intents
bot = commands.Bot(command_prefix="!", intents=intents)

@bot.event
async def on_ready():
    print(f'Logged in as {bot.user}')
    for guild in bot.guilds:
        print(f'\nServer: {guild.name} (ID: {guild.id})')
        print(f'Owner: {guild.owner}')
        print(f'Region: {guild.preferred_locale}')
        print(f'Boost Level: {guild.premium_tier}')
        print(f'Boost Count: {guild.premium_subscription_count}')
        print(f'Created At: {guild.created_at}')
        print(f'Icon URL: {guild.icon.url if guild.icon else "None"}')

        print("\nRoles:")
        for role in guild.roles:
            print(f'- {role.name} (ID: {role.id}) | Permissions: {role.permissions}')

        print("\nUsers:")
        for member in guild.members:
            roles = [role.name for role in member.roles]
            print(f'- {member.name}#{member.discriminator} (ID: {member.id}) | Roles: {", ".join(roles)} | Status: {member.status} | Avatar: {member.avatar.url if member.avatar else "None"}')

        print("\nChannels:")
        for channel in guild.channels:
            print(f'- {channel.name} (ID: {channel.id}) | Type: {channel.type} | NSFW: {channel.is_nsfw()}')

        print("\nEmojis:")
        for emoji in guild.emojis:
            print(f'- {emoji.name} (ID: {emoji.id}) | Animated: {emoji.animated}')

        print("\nAudit Logs:")
        async for entry in guild.audit_logs(limit=10):
            print(f'- Action: {entry.action} | User: {entry.user} | Target: {entry.target} | Reason: {entry.reason}')

bot.run("MTM2ODI2MTQwMjUzNTY2MTczMA.G3t6HI.HXPjnaIHToe820SR0-FK7tAAXGZK2_-Lw3exDI")
