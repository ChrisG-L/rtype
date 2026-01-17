"""Cogs for R-Type Admin Bot."""

from .admin import AdminCog
from .users import UsersCog
from .moderation import ModerationCog

__all__ = ["AdminCog", "UsersCog", "ModerationCog"]


async def setup_cogs(bot):
    """Load all cogs."""
    await bot.add_cog(AdminCog(bot))
    await bot.add_cog(UsersCog(bot))
    await bot.add_cog(ModerationCog(bot))
