"""
Permission checks for R-Type Admin Bot commands.
"""

import discord
from discord import app_commands

from config import config


def is_admin_channel():
    """Check if command is used in admin channel.

    Fails closed: denies access if admin_channel_id is not configured.
    """
    async def predicate(interaction: discord.Interaction) -> bool:
        # Fail closed: deny if not configured
        if not config.admin_channel_id:
            await interaction.response.send_message(
                "⚠️ Admin channel not configured. Contact bot administrator.",
                ephemeral=True
            )
            return False
        if interaction.channel_id != config.admin_channel_id:
            await interaction.response.send_message(
                "This command can only be used in the admin channel.",
                ephemeral=True
            )
            return False
        return True
    return app_commands.check(predicate)


def has_admin_role():
    """Check if user has admin role.

    Fails closed: denies access if admin_role_id is not configured.
    """
    async def predicate(interaction: discord.Interaction) -> bool:
        # Fail closed: deny if not configured
        if not config.admin_role_id:
            await interaction.response.send_message(
                "⚠️ Admin role not configured. Contact bot administrator.",
                ephemeral=True
            )
            return False
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
