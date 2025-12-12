"""
MCP Jira - Serveur MCP pour l'intégration Jira Cloud.

Ce module expose l'API Jira aux agents Claude via le Model Context Protocol.
Conçu pour être léger et focalisé sur les cas d'usage d'analyse de code.

Usage:
    python -m mcp.jira.server

Configuration:
    Les credentials sont lus depuis les variables d'environnement :
    - JIRA_URL : URL de l'instance Jira (ex: https://company.atlassian.net)
    - JIRA_EMAIL : Email de l'utilisateur
    - JIRA_API_TOKEN : Token API (pas le mot de passe)

    Ou depuis .claude/settings.local.json :
    {
        "jira": {
            "url": "https://company.atlassian.net",
            "email": "user@company.com",
            "api_token": "your-token"
        }
    }
"""

__version__ = "1.0.0"
