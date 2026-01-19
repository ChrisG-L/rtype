"""Cogs for R-Type Admin Bot."""

from .admin import AdminCog
from .users import UsersCog
from .moderation import ModerationCog
from .messages import MessagesCog

__all__ = ["AdminCog", "UsersCog", "ModerationCog", "MessagesCog"]


async def setup_cogs(bot):
    """Load all cogs."""
    await bot.add_cog(AdminCog(bot))
    await bot.add_cog(UsersCog(bot))
    await bot.add_cog(ModerationCog(bot))
    await bot.add_cog(MessagesCog(bot))
