"""
Permission checks for R-Type Admin Bot commands.
"""

import discord
from discord import app_commands

from config import config


def is_admin_channel():
    """Check if command is used in admin channel."""
    async def predicate(interaction: discord.Interaction) -> bool:
        if config.admin_channel_id and interaction.channel_id != config.admin_channel_id:
            await interaction.response.send_message(
                "This command can only be used in the admin channel.",
                ephemeral=True
            )
            return False
        return True
    return app_commands.check(predicate)


def has_admin_role():
    """Check if user has admin role."""
    async def predicate(interaction: discord.Interaction) -> bool:
        if config.admin_role_id:
            member = interaction.user
            if isinstance(member, discord.Member):
                if not any(role.id == config.admin_role_id for role in member.roles):
                    await interaction.response.send_message(
                        "You don't have permission to use admin commands.",
                        ephemeral=True
                    )
                    return False
        return True
    return app_commands.check(predicate)
