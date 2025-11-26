# :material-code-review: Rapport du Soldat Code Review

<div class="report-header" markdown>

!!! info "Code Review Globale"

    | Attribut | Valeur |
    |:---------|:-------|
    | **Date** | 2025-11-26 |
    | **Fichiers Analysés** | 50+ |
    | **Issues Critiques** | 8 |
    | **Suggestions** | 15 |

</div>

---

## :material-bug: Issues Critiques

### 1. Password::verify() - Logic Error

```cpp
// Password.cpp:26
bool Password::verify(std::string password) {
    if (utils::hashPassword(password) == password)  // BUG!
        return true;
    return false;
}
```

!!! danger "Fix Requis"

    ```cpp
    bool Password::verify(const std::string& password) const {
        return utils::hashPassword(password) == _passwordHash;
    }
    ```

### 2. Register Logic Inversée

```cpp
// Register.cpp
if (!playerOptByName.has_value()) {  // INVERSÉ!
    return;
}
```

### 3. Engine Null Pointer

```cpp
// Engine.cpp
_gameLoop = std::make_unique<GameLoop>(_window, ...);
// _window est nullptr!
```

---

## :material-lightbulb: Suggestions d'Amélioration

| # | Fichier | Suggestion | Impact |
|:-:|:--------|:-----------|:------:|
| 1 | Health.hpp | Ajouter `[[nodiscard]]` | :material-arrow-up:{ .status-ok } |
| 2 | GameLoop.cpp | Calculer deltatime correctement | :material-arrow-up:{ .status-warning } |
| 3 | *.hpp | Ajouter docstrings Doxygen | :material-arrow-up:{ .status-ok } |
| 4 | SFMLPlugin | Décommenter/implémenter | :material-arrow-up:{ .status-critical } |

---

<div class="report-footer" markdown>

:material-code-review: **Rapport généré par le Soldat Code Review - Army2077**

</div>

---

[:material-arrow-left: Retour aux Soldats](../index.md){ .md-button }
