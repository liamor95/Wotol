# WOTOL — Contexte projet pour Claude Code

Ce fichier est lu par Claude Code à chaque session. Objectif : ne jamais repartir de zéro, ne jamais réinventer une mécanique ou une classe déjà actée ci-dessous.

---

## 1. Projet en une phrase

WOTOL (War of the Ocean's Legacy) — RTS tactique narratif sous-marin sur Unreal Engine 5, développé par l'association 3DÉCORS. Périmètre actuel : une démo Steam (vertical slice), pas le jeu complet.

## 2. Stack technique — NE PAS DÉVIER

- **Hybride C++/Blueprint confirmé.**
- **Claude Code écrit le C++** : subsystems, GameMode/GameState, logique IA, Data Assets, classes de base — tout ce qui est performance-critique ou doit survivre à un refactor.
- **Blueprint (fait par Liamor dans l'éditeur, pas par Claude Code)** : event graphs UMG, câblage visuel, animations/VFX, Blueprints de niveau héritant des classes C++, instances de Data Assets remplies par les game designers.
- Si une tâche ressemble à "câbler un widget" ou "animer une transition" → ce n'est PAS du C++, le signaler plutôt que de forcer une solution en dur.

## 3. Périmètre exact de la démo

- **2 factions jouables** : Aquiloris et Noxéens (sur 5 dans le design complet — voir section 5).
- **Boucle de jeu** : sélection de faction avec lore → personnalisation de héros limitée → réglages → exploration troisième personne dans une zone neutre (~100m²) → déclenchement d'un encounter de boss (style Pokémon) → transition vers bataille tactique isométrique → capture de territoire grade 1.
- **Solo uniquement** pour cette V1 — pas de réplication réseau. Le code reste compatible multijoueur futur (logique portée par GameState, pas uniquement GameMode) mais aucun netcode n'est implémenté.
- **Verticalité à 3 paliers pour la démo** (sol, intermédiaire ~5m, haut ~15m). Le design complet vise 4 paliers — non implémenté ici, mais l'enum `EVerticalLayer` doit rester extensible.

## 4. Architecture logicielle (V1)

### 4.1 Arborescence de modules

```
WOTOL/Source/WOTOL/
├── Core/
│   ├── WOTOLGameInstance.h/.cpp
│   ├── WOTOLSaveGame.h/.cpp
│   ├── PlayerProfileSubsystem.h/.cpp        (existant)
│   ├── SaveGameSubsystem.h/.cpp             (nouveau)
│   └── FactionRegistrySubsystem.h/.cpp      (nouveau)
├── Gameplay/
│   ├── Battle/
│   │   ├── WOTOLGameMode_Battle.h/.cpp
│   │   ├── WOTOLGameState_Battle.h/.cpp
│   │   ├── BattleStateObserver.h/.cpp       (existant)
│   │   ├── TerritoryStateManager.h/.cpp     (existant)
│   │   ├── TacticalPhaseManager.h/.cpp      (nouveau)
│   │   └── VerticalLayerComponent.h/.cpp    (nouveau)
│   ├── AI/AIAdaptiveController.h/.cpp       (existant)
│   ├── Units/UnitBase.h/.cpp, UnitDataAsset.h/.cpp
│   ├── Factions/FactionDataAsset.h/.cpp
│   └── Exploration/
│       ├── WOTOLGameMode_Exploration.h/.cpp
│       ├── WOTOLHeroCharacter.h/.cpp
│       └── EncounterTriggerVolume.h/.cpp
├── UI/        (classes de base C++ seulement — le reste en Blueprint UMG)
└── Data/      (structs/enums partagés — FTerritoryGrade, EVerticalLayer...)
```

### 4.2 Subsystems persistants (GameInstance)

| Classe | Statut | Rôle |
|---|---|---|
| `UPlayerProfileSubsystem` | existant | Profilage comportemental temps réel du joueur. Alimente l'IA en entrée de bataille, mis à jour en sortie. |
| `USaveGameSubsystem` | nouveau | `UGameInstanceSubsystem` encapsulant `UWOTOLSaveGame`. Expose `SaveGame()`/`LoadGame()` en `BlueprintCallable`. Sauvegarde auto à : fin de bataille, retour menu, capture validée. |
| `UFactionRegistrySubsystem` | nouveau | `UWorldSubsystem`. Liste vivante des unités actives (`RegisterUnit`/`UnregisterUnit` via BeginPlay/EndPlay). Remplace tout `GetAllActorsOfClass`. |

### 4.3 Sous-systèmes du mode bataille (GameMode/GameState)

| Classe | Statut | Rôle |
|---|---|---|
| `ABattleStateObserver` | existant | Surveille l'état de bataille en temps réel, diffuse `OnBattleStateChanged`/`OnVictoryConditionMet`. Consomme `UFactionRegistrySubsystem`, ne scanne plus le monde. |
| `UTerritoryStateManager` | existant | Gestion des grades de territoire (1 à 4 ; grade 1 pour la démo) et conditions de capture. |
| `UTacticalPhaseManager` | nouveau | Tick centralisé unique (un seul `FTimerHandle`). Diffuse `OnTacticalWindowOpened/Closed(EFactionID)`. Remplace les timers par unité. |
| `UVerticalLayerComponent` | nouveau | Composant sur chaque unité, porte `EVerticalLayer`. Consommé par TerritoryStateManager, AIAdaptiveController, HUD. |
| `UAIAdaptiveController` | existant | IA adaptative par faction. Reçoit le profil joueur, s'abonne au TacticalPhaseManager, interroge le FactionRegistry. Exécution individuelle des unités = Behavior Tree/EQS standard, PAS du code custom par unité. |

### 4.4 Couche données

Toute donnée de faction/unité/héros passe par des `UPrimaryDataAsset` (`UFactionDataAsset`, `UUnitDataAsset`, `UHeroLoadoutDataAsset`) — **jamais en dur dans le code**. Ajouter une faction plus tard = créer une instance de Data Asset, zéro C++ à toucher.

### 4.5 Boucle de session

GameInstance (persistant) → Mode exploration (héros, zone neutre, trigger) → Mode bataille tactique (territoire, IA, verticalité) → Sauvegarde & retour → boucle au GameInstance.

## 5. Lore condensé (pour cohérence si du code touche au design)

- **Aquiloris** — civilisation noble/avancée, capitale Aquilore, ressource Cristaux, chef Aquis, arme Épée cristalline, mythique Léviaphénix.
- **Thalassidras** — peuple du corail, ressource Corail vivant, magie naturelle.
- **Noxéens** — créatures abyssales, ressource Biolumens, bioluminescence, attaques explosives.
- **Muréniens** — prédateurs mi-humains/mi-murènes, ressource non définie, combat rapproché.
- **Pirates Abyssaux** — coalition de récupérateurs, ressource Épaves, chef Necris (antagoniste, dernier Æthérien, récupère des unités perdues en bataille).

## 6. Règles à ne jamais enfreindre

- Pas de `GetAllActorsOfClass` pour lister des unités/factions → passer par `UFactionRegistrySubsystem`.
- Pas de `FTimerHandle` par unité pour la logique de phase/tour → passer par `UTacticalPhaseManager`.
- Pas de donnée de faction/unité codée en dur → `UPrimaryDataAsset`.
- Convention de nommage UE5 standard : `U` préfixe pour UObject/Subsystem, `A` pour Actor, `F` pour struct, `E` pour enum.
- Toute classe destinée à être pilotée depuis Blueprint expose ses points d'entrée en `UFUNCTION(BlueprintCallable)` ou `BlueprintImplementableEvent` — ne pas tout enfermer en C++ pur.

## 7. État du dépôt

Dépôt git initialisé en local, `.gitignore` en place (exclut `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`, `.vs/`). Pas encore de remote GitHub configuré au moment de la rédaction de ce fichier — à vérifier en début de session si ça a changé.

## 8. Points ouverts — à confirmer avant d'aller plus loin sur ces sujets précis

1. Solo confirmé pour la démo, ou multijoueur envisagé plus tôt que prévu ?
2. 3 paliers verticaux pour la démo vs 4 pour le jeu complet — confirmé ?
3. Contenu exact de `UBattleConfigDataAsset` (ce que l'exploration transmet à la bataille).
4. Déclencheurs exacts de sauvegarde automatique.
5. Ressource de la faction Muréniens — non définie dans le design actuel.
