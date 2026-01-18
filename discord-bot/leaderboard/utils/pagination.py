"""Discord pagination utilities with buttons."""

import discord
from discord.ui import View, Button
from typing import Callable, Optional


class PaginatedView(View):
    """A view with pagination buttons for navigating through pages."""

    def __init__(
        self,
        pages: list[discord.Embed],
        author_id: Optional[int] = None,
        timeout: float = 120.0,
    ):
        """Initialize paginated view.

        Args:
            pages: List of embeds to paginate
            author_id: User ID who can interact (None = anyone)
            timeout: View timeout in seconds
        """
        super().__init__(timeout=timeout)
        self.pages = pages
        self.current_page = 0
        self.author_id = author_id

        # Update button states
        self._update_buttons()

    def _update_buttons(self):
        """Update button states based on current page."""
        self.first_button.disabled = self.current_page == 0
        self.prev_button.disabled = self.current_page == 0
        self.next_button.disabled = self.current_page >= len(self.pages) - 1
        self.last_button.disabled = self.current_page >= len(self.pages) - 1

        # Update page indicator
        self.page_indicator.label = f"{self.current_page + 1}/{len(self.pages)}"

    async def interaction_check(self, interaction: discord.Interaction) -> bool:
        """Check if the user can interact with this view."""
        if self.author_id is None:
            return True
        if interaction.user.id != self.author_id:
            await interaction.response.send_message(
                "❌ Seul l'auteur de la commande peut utiliser ces boutons.",
                ephemeral=True,
            )
            return False
        return True

    @discord.ui.button(label="⏮️", style=discord.ButtonStyle.secondary)
    async def first_button(self, interaction: discord.Interaction, button: Button):
        """Go to first page."""
        self.current_page = 0
        self._update_buttons()
        await interaction.response.edit_message(
            embed=self.pages[self.current_page], view=self
        )

    @discord.ui.button(label="◀️", style=discord.ButtonStyle.primary)
    async def prev_button(self, interaction: discord.Interaction, button: Button):
        """Go to previous page."""
        self.current_page = max(0, self.current_page - 1)
        self._update_buttons()
        await interaction.response.edit_message(
            embed=self.pages[self.current_page], view=self
        )

    @discord.ui.button(label="1/1", style=discord.ButtonStyle.secondary, disabled=True)
    async def page_indicator(self, interaction: discord.Interaction, button: Button):
        """Page indicator (non-interactive)."""
        pass

    @discord.ui.button(label="▶️", style=discord.ButtonStyle.primary)
    async def next_button(self, interaction: discord.Interaction, button: Button):
        """Go to next page."""
        self.current_page = min(len(self.pages) - 1, self.current_page + 1)
        self._update_buttons()
        await interaction.response.edit_message(
            embed=self.pages[self.current_page], view=self
        )

    @discord.ui.button(label="⏭️", style=discord.ButtonStyle.secondary)
    async def last_button(self, interaction: discord.Interaction, button: Button):
        """Go to last page."""
        self.current_page = len(self.pages) - 1
        self._update_buttons()
        await interaction.response.edit_message(
            embed=self.pages[self.current_page], view=self
        )

    async def on_timeout(self):
        """Disable all buttons on timeout."""
        for item in self.children:
            if isinstance(item, Button):
                item.disabled = True


def paginate_embed_fields(
    base_embed: discord.Embed,
    items: list[str],
    items_per_page: int = 10,
    field_name: str = "Classement",
) -> list[discord.Embed]:
    """Create multiple embeds from a list of items.

    Args:
        base_embed: Base embed to clone for each page
        items: List of formatted strings to display
        items_per_page: Number of items per page
        field_name: Name for the field containing items

    Returns:
        List of embeds, one per page
    """
    if not items:
        embed = base_embed.copy()
        embed.add_field(name=field_name, value="Aucune donnée", inline=False)
        return [embed]

    pages = []
    total_pages = (len(items) + items_per_page - 1) // items_per_page

    for page_num in range(total_pages):
        start = page_num * items_per_page
        end = start + items_per_page
        page_items = items[start:end]

        embed = base_embed.copy()
        embed.add_field(name=field_name, value="\n".join(page_items), inline=False)

        if total_pages > 1:
            embed.set_footer(text=f"Page {page_num + 1}/{total_pages} • R-Type Leaderboard")
        else:
            embed.set_footer(text="R-Type Leaderboard")

        pages.append(embed)

    return pages


def paginate_leaderboard(
    entries: list[dict],
    format_entry: Callable[[int, dict], str],
    base_embed: discord.Embed,
    items_per_page: int = 10,
) -> list[discord.Embed]:
    """Create paginated leaderboard embeds.

    Args:
        entries: List of leaderboard entries
        format_entry: Function that takes (rank, entry) and returns formatted string
        base_embed: Base embed to clone for each page
        items_per_page: Number of entries per page

    Returns:
        List of embeds, one per page
    """
    items = [format_entry(i, entry) for i, entry in enumerate(entries)]
    return paginate_embed_fields(base_embed, items, items_per_page)
