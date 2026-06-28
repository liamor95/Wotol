# 🛠️ WOTOL — Rapport de session technique — 29/06/2026

> **À partager sur Notion + Drive.** Base de données « ce qui marche » pour WOTOL
> et pour les futurs projets de jeu du même type (RTS tactique UE5).

---

## 1. 🎉 RÉSULTAT MAJEUR : le jeu compile ET se lance

- Le module C++ **WOTOL** compile **54/54 fichiers** sous **Unreal Engine 5.7.4**.
- L'**éditeur Unreal s'ouvre** avec tous les systèmes C++ chargés et visibles.
- Plateforme vérifiée : Windows 11, i5-10300H, VS2022 (Game dev C++).

C'est le franchissement du plus gros obstacle : **la base technique tourne.**

---

## 2. 🗂️ SAUVEGARDES — État des branches Git (TRÈS IMPORTANT)

| Branche GitHub | Rôle | Règle |
|---|---|---|
| `claude/base-jouable-v1` | 🔒 **POINT FIGÉ** — base qui compile et se lance | **NE JAMAIS MODIFIER** |
| `claude/peaceful-tesla-s9qphq` | Branche de travail principale | Référence de dev |
| `claude/greybox-prototype` | 🧪 Prototype de test (formes simples) | Expérimental |

**Commit de référence de la base figée : `f5f4cc1`.**
→ En cas de problème, on revient toujours à `claude/base-jouable-v1` = zéro perte.

---

## 3. ⚙️ SYSTÈMES C++ EN PLACE (architecture)

Tout est codé en C++ (logique), branchable en Blueprint (visuel) :

**Combat / unités**
- `AUnitBase` — classe mère unités (PV, faction, attaque, mort)
- `UUnitMoraleComponent` — jauge de moral distincte des PV, état Routing
- `UVerticalLayerComponent` — 4 couches océaniques + multiplicateurs d'attaque verticale
- `UAbilityComponent` / `UAbilityBase` — compétences
- `UFormationComponent` — 5 formations (ligne, coin, carré, lâche, colonne)

**IA (RTS temps réel, PAS tour par tour)**
- `AAIAdaptiveController` — ordres RTS (Move, AttackMove, AttackTarget, Hold, Ability, ChangeLayer, Retreat)
- `UUnitAIStateComponent` — machine d'états (Idle/Seeking/Attacking/Retreating…) active en continu
- Adaptation au profil comportemental du joueur

**Bataille / monde**
- `URTSBattleManager` — orchestrateur temps réel (déploiement → combat → résolution)
- `UFactionRegistrySubsystem` — liste vivante des unités (remplace GetAllActorsOfClass)
- `UTerritoryStateManager` — capture de zones, grades 1-4
- `UFactionSynergySubsystem` — synergies (Aquiloris coordination / Noxéens zones bioluminescentes)
- `UBuildingManagerSubsystem` — construction, production, destruction
- `UResourceManager` — 8 ressources
- `AWOTOLBattleCamera` — **caméra libre 3D** (Total War / Bannerlord)
- `AWOTOLPlayerController_Battle` — sélection (clic/boîte), ordres
- `AWOTOLUnitSpawner` — apparition des unités

**Données (jamais de stats en dur)**
- `UUnitDataAsset` (FUnitStats) — stats unités
- `UUnitDataLibrary` + `UUnitDataRegistrySubsystem` — **les 12 unités (6 Aquiloris + 6 Noxéens) avec toutes leurs stats du GDD, chargées automatiquement au démarrage**
- `UBuildingDataAsset`, `UBattleConfigDataAsset`, `FFactionColors` (couleurs officielles centralisées)

**Boucle de démo**
- `AWOTOLGameMode_Battle` → bataille → `AWOTOLGameMode_Hub` (cité post-bataille)

---

## 4. 🧪 PROTOTYPE GREYBOX (test sans modèles 3D)

But : vérifier que la logique fonctionne **sans aucun mesh Meshy**, avec des formes simples.

- `AWOTOLGreyboxUnit` — Aquiloris = **cube bleu**, Noxéens = **cône vert**
- `AWOTOLGreyboxArena` — posé dans un niveau + Play → 2 armées s'affrontent toutes seules

**Test côté éditeur (~5 min) :** niveau avec sol → poser un `NavMeshBoundsVolume` →
glisser `WOTOLGreyboxArena` → Play.

⚠️ Greybox pas encore compilé/vérifié (à tester) — il est isolé sur sa branche,
la base figée n'est pas affectée.

---

## 5. 🐞 BASE DE DONNÉES DES BUGS RÉSOLUS (réutilisable tous projets UE5/C++)

Documentée en détail dans `UE5_CPP_PLAYBOOK.md` et `CLAUDE.md` §12.

1. **`.uproject` corrompu** (`JsonException 0xFF`) → ne jamais l'ouvrir dans le Bloc-notes (UTF-8 obligatoire)
2. **Version moteur** → `EngineAssociation` = version installée (5.7)
3. **`Target.cs` manquants** → créer les 2 fichiers Target
4. **`BuildSettingsVersion.V5`** en conflit → utiliser `Latest`
5. **Chemin d'include** (`C1083`) → `PublicIncludePaths.Add(ModuleDirectory)` dans Build.cs
6. **Préfixe de classe** (AIController = `A`, pas `U`)
7. **UFUNCTION renvoyant un pointeur de struct** → copie + booléen
8. **Paramètre masquant AActor** (`Instigator`) → renommer
9. **Type incomplet** (`C2027`) → `#include` du header complet
10. **Lambda sur délégué dynamique** → `UFUNCTION` + `AddDynamic`
11. **Valeur d'enum inexistante** → vérifier l'enum réel

**⚡ Astuce clé** : coller le TEXTE du log (pas une photo) → toutes les erreurs vues d'un coup.

**Process recommandé** : GitHub Desktop + bouton « Pull » (au lieu de re-télécharger le ZIP).

---

## 6. ➡️ PROCHAINES ÉTAPES (dans l'ordre, sans précipitation)

1. **Tester le greybox** (formes qui se battent) — corriger si besoin
2. **Backup/point figé** une fois le greybox validé
3. **Revue de cohérence des stats** (à faire ensemble) :
   - quelle unité peut aller à quelle couche verticale
   - vitesses, portées, équilibrage
   - règles de déplacement vertical (coût en actions, malus)
4. Remplacer progressivement les formes par les **modèles Meshy** (sans toucher au code)
5. HUD, menus (UMG), niveaux, audio

---

## 7. 🎯 OBJECTIF COURT TERME

**Réunion association 3DÉCORS (~3 semaines)** : montrer une **bataille jouable** (même en
cubes/cônes) → démontrer que le moteur de jeu fonctionne et donner de l'envergure au projet.

---

*Tout est conservé et versionné sur GitHub. Chaque avancée validée est enregistrée
dans cette base de connaissances réutilisable pour les futurs projets.*
