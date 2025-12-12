# MCP Jira - Configuration

Ce serveur MCP expose l'API Jira Cloud aux agents Claude.

## Configuration des Credentials

### Option 1 : Variables d'environnement (recommandé pour CI/CD)

```bash
export JIRA_URL="https://your-company.atlassian.net"
export JIRA_EMAIL="your-email@company.com"
export JIRA_API_TOKEN="your-api-token"
```

### Option 2 : Via env dans settings.local.json (recommandé pour dev local)

Ajouter dans `.claude/settings.local.json` :

```json
{
  "env": {
    "JIRA_URL": "https://your-company.atlassian.net",
    "JIRA_EMAIL": "your-email@company.com",
    "JIRA_API_TOKEN": "your-api-token"
  }
}
```

> **Note** : `.claude/settings.local.json` est gitignored, vos credentials ne seront pas commités.

## Obtenir un API Token Jira

1. Aller sur https://id.atlassian.com/manage-profile/security/api-tokens
2. Cliquer sur "Create API token"
3. Donner un nom (ex: "Claude Code MCP")
4. Copier le token généré

## Outils disponibles

| Outil | Description |
|-------|-------------|
| `get_issue` | Récupère un ticket par sa clé (PROJ-123) |
| `search_issues` | Recherche avec JQL |
| `get_issue_from_text` | Extrait et récupère les tickets d'un texte |
| `get_project_info` | Infos sur un projet |

## Utilisation dans /analyze

Le contexte Jira est automatiquement extrait du commit message si un pattern de ticket est détecté (ex: `[PROJ-123] Fix bug`).

Ce contexte est passé aux agents pour enrichir l'analyse :
- **ANALYZER** : Vérifie que l'impact correspond au scope du ticket
- **REVIEWER** : Vérifie les acceptance criteria
- **RISK** : Ajuste selon la criticité

## Dépannage

### Le serveur ne démarre pas

Vérifier les logs :
```bash
JIRA_LOG_LEVEL=DEBUG python -m mcp.jira.server
```

### Erreur 401 (Unauthorized)

- Vérifier que l'email et le token sont corrects
- Vérifier que l'utilisateur a accès au projet Jira

### Erreur 403 (Forbidden)

- L'utilisateur n'a pas les permissions sur le ticket/projet
- Vérifier les permissions Jira

### Ticket non trouvé

- Vérifier que la clé est correcte (sensible à la casse)
- Vérifier que le projet existe
