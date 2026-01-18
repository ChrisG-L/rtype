"""Utils module for R-Type Discord Bot."""

from .formatters import format_number, format_duration, format_timestamp, format_game_duration
from .embeds import (
    create_leaderboard_embed,
    create_stats_embed,
    create_kills_embed,
    create_online_embed,
    create_achievements_embed,
    create_history_embed,
    create_rank_embed,
    create_compare_embed,
    create_server_stats_embed,
)
from .pagination import PaginatedView, paginate_embed_fields, paginate_leaderboard

__all__ = [
    "format_number",
    "format_duration",
    "format_timestamp",
    "format_game_duration",
    "create_leaderboard_embed",
    "create_stats_embed",
    "create_kills_embed",
    "create_online_embed",
    "create_achievements_embed",
    "create_history_embed",
    "create_rank_embed",
    "create_compare_embed",
    "create_server_stats_embed",
    "PaginatedView",
    "paginate_embed_fields",
    "paginate_leaderboard",
]
