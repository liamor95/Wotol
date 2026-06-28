# WOTOL — Contexte projet pour Claude Code

Source de vérité unique. Se référer ici avant tout code. Ne jamais inventer de données non présentes ici.

---

## 1. Projet

WOTOL (War of the Ocean's Legacy) — RTS tactique sous-marin, UE5, association 3DÉCORS.
Marque INPI n° 26 5230895. Cible : PC Windows 10/11, Steam, PEGI 12+.

**SCOPE DÉMO ABSOLU (Couche 1 uniquement) :**
- 1 bataille complète jouable
- 2 factions jouables : **AQUILORIS** et **NOXÉENS** uniquement
- Verticalité 4 couches opérationnelle
- IA ennemie basique fonctionnelle
- HUD de bataille essentiel
- Commandants avec capacités
- Système de moral basique

**HORS DÉMO (ne pas implémenter) :** Carte stratégique, Thalassidras, Muréniens, Pirates Abyssaux.

---

## 2. Stack technique — NE PAS DÉVIER

- **Moteur : UE 5.7.4** — ne pas migrer
- **Hybride C++/Blueprint** : Claude Code écrit le C++ (subsystems, logique, data), Liamor fait les Blueprints visuels, UMG, animations
- Éclairage : Lumen (bioluminescence), Géométrie : Nanite, Audio : MetaSounds
- **Bloqueur actif VRAM** : assets Meshy haute densité → plan : poly count audit → Low Poly Meshy → LOD chains → BC7/BC5 textures. Nanite stable skeletal meshes depuis UE5.5.

---

## 3. Architecture C++ existante

### Subsystèmes déjà codés (ne pas recréer)
| Classe | Rôle |
|---|---|
| `UPlayerProfileSubsystem` | Profilage comportemental joueur |
| `USaveGameSubsystem` | Save/Load via UWOTOLSaveGame |
| `UFactionRegistrySubsystem` | Liste vivante des unités actives (remplace GetAllActorsOfClass) |
| `UTacticalPhaseManager` | Timer centralisé unique pour les tours |
| `UTerritoryStateManager` | Grades de territoire, capture |
| `ABattleStateObserver` | Conditions victoire/défaite |
| `UAIAdaptiveController` | IA adaptative par faction |
| `UHexGridManager` | Grille hex axiale (déploiement) |
| `UResourceManager` | 8 ressources avec cap sur EnergieOceanique |
| `UUnitDeploymentManager` | Phase déploiement hex |
| `UBattleTimerManager` | Compte à rebours bataille |
| `UObjectiveManager` | Objectifs + popup zone neutre |
| `UMenuFlowSubsystem` | Navigation menus (8 étapes) |
| `UHeroExperienceComponent` | XP/niveau héros |

### Bugs corrigés (ne pas réintroduire)
- `GetAllActorsOfClass` → passer par `UFactionRegistrySubsystem`
- Timers par unité → `UTacticalPhaseManager`
- Stats hardcodées → `S_UnitData` / `UUnitDataAsset`

---

## 4. Architecture Blueprint (fait par Liamor dans l'éditeur)

| Blueprint | Rôle |
|---|---|
| `BP_GameInstance` | Persistance inter-niveaux, faction, ressources globales |
| `BP_BattleManager` | **COMPOSANT LE PLUS CRITIQUE** — orchestrateur bataille, tours, résolution, conditions victoire |
| `BP_UnitBase` | Classe mère TOUTES les unités — stats via S_UnitData UNIQUEMENT |
| `BP_PlayerUnit` | Hérite BP_UnitBase — input joueur, sélection, ordres |
| `BP_EnemyUnit` | Hérite BP_UnitBase — IA décision par menace/couche/commandant |
| `BP_UnitController` | Traduction input → actions unité |
| `BP_Zone` | Zone capturable : propriétaire, ressources, liens adjacents |
| `WBP_BattleHUD` | Portrait commandant + roster + minimap verticale + timer |
| `WBP_FactionSelect` | Écran sélection faction |
| `BP_StrategyMapManager` | **COUCHE 2 — NE PAS IMPLÉMENTER** |

---

## 5. Enums canoniques

```
E_Faction : Thalassidras / Noxéens / Aquiloris / Muréniens / PiratesAbyssaux
E_ZoneOwner : Neutral / Player / Enemy
E_UnitType : Chef / Mythique / Speciale / Montee / Distance / Infanterie
E_UnitState : Idle / Moving / Attacking / Ability / Routing / Dead
E_VerticalLevel :
  Epipelagique (0-200m)    — lumière max, vitesse +20%, couverture réduite
  Mesopelagique (200-1000m) — crépuscule, embuscades, avantage Noxéens
  Bathypelagique (1000-4000m) — obscurité, unités lourdes, attaque ascendante bonus
  Hadal (4000m+)           — élite/Titans uniquement, attaque ascendante ×3
```

---

## 6. Mécaniques de jeu

### Verticalité 4 couches
- Déplacement vertical coûte des actions et expose à des malus temporaires
- **Attaque ascendante** (depuis couche inférieure) = bonus dégâts
- **Attaque descendante** = bonus précision et portée
- Hadal : attaque ascendante ×3 dégâts, cooldown long

### Fog of War 3D
- Épipélagique : bonne visibilité horizontale, mauvaise vers le bas
- Hadal : invisible sauf si ennemi a détecteurs dans couches intermédiaires
- Noxéens : avantage visibilité naturel dans zones sombres

### Moral (jauge DISTINCTE des PV)
- Triggers baisse : pertes massives, mort du Commandant, certaines capacités ennemies
- 0 moral → état ROUTING → incontrôlable jusqu'à intervention du chef
- Mort Commandant = malus moral massif sur toute l'armée → risque déroute généralisée

### Conditions victoire bataille
1. Destruction totale forces ennemies
2. Fuite adversaire (déroute généralisée)
3. Mort du Commandant ennemi (optionnel)
4. Objectifs spécifiques (tenir zone, protéger structure)
5. Retraite tactique du joueur

### Commandant/Général
- Capacités actives + aura passive
- Présence inspire les troupes (bonus moral passif)
- Stats plus élevées que les unités ordinaires

### Terrain dynamique
- Courants marins : modifient déplacements et formations
- Volcans sous-marins : zones de danger, obstacle ou arme
- Zones bioluminescentes : avantage Noxéens, visibilité accrue
- Certains éléments activables tactiquement

### Synergies de faction
- **Aquiloris** : bonus coordination si unités se soutiennent mutuellement
- **Noxéens** : puissance amplifiée via zones bioluminescentes des Noxéons
- **Thalassidras** : bonus défensifs en zone favorable (Couche 2)
- **Muréniens** : bonus vitesse/esquive en mouvement permanent (Couche 2)
- **Pirates Abyssaux** : cooldowns réduits après élimination ennemie (Couche 2)

---

## 7. Stats complètes des unités — DÉMO

### AQUILORIS (ressource : Cristaux d'énergie)

| Unité | Type | PV | ATK/s | DEF% | Vit | Portée | T.Att | Zone | CD | Coût | Diff | Syn |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Aquis (Chef) | Chef | 1400 | 120 | 15 | 1.0 | 1 | 1.1s | Cône léger | 12s | 220 | 3 | 5 |
| Léviaphénix | Mythique | 2000 | 130 | 15 | 1.2 | 3 | 1.6s | Aura large | 20s | 400 | 4 | 5 |
| Aquiloryons | Infanterie | 1700 | 80 | 25 | 0.8 | 1 | 1.3s | Mono | 10s | 130 | 2 | 4 |
| Aquilances | Montée | 1200 | 140 | 10 | 1.3 | 3 | 0.9s | Charge ligne | 14s | 180 | 3 | 3 |
| Aquisphères | Distance | 1000 | 130 | 5 | 0.9 | 5 | 1.2s | Petite zone | 8s | 140 | 2 | 4 |
| Aquilombres | Spéciale | 900 | 170 | 5 | 1.1 | 1 | 0.8s | Mono | 12s | 160 | 4 | 3 |

**Détails des compétences Aquiloris :**

**Aquis** — Lame Photonique (Cône)
- Base : Onde de choc directionnelle
- Axe 1 (DPS) : Onde dégâts importants sur plusieurs unités
- Axe 2 (Support) : Onde circulaire projette ennemis proches
- Passif : bonus coordination + réduction recharge alliés proches
- Synergie : Aquilombres

**Léviaphénix** — Résonance Technologique (Aura)
- Base : Amplifie stats toutes unités alliées proches
- Axe 1 : Rayonnement Stabilisateur — amplification poussée, zone élargie
- Axe 2 : Rayonnement Vital — fait revenir quelques unités tombées
- Passif : amplifie dégâts, réduit recharges, augmente défense boucliers
- Synergie : Aquisphères

**Aquiloryons** — Mur de Cristal (Mono)
- Base : Formation rempart, absorption dégâts frontaux
- Axe 1 : Mur amplifié — protection collective accrue
- Axe 2 : Double Lames — sacrifie défense pour dégâts
- Passif : bonus coordination, renforce unités adjacentes
- Synergie : Aquilances

**Aquilances** — Percée Ondulatoire (Charge ligne)
- Base : Charge frontale concentrant énergie de la lance
- Axe 1 : Percée amplifiée — dégâts augmentés, renverse unités légères
- Axe 2 : Rempart Synthétique — formation hauteur, empêche attaques descendantes
- Passif : résistance frontale, saignement au contact
- Synergie : Aquiloryons

**Aquisphères** — Hydrolaser (Petite zone)
- Base : Tir sphère laser précise
- Axe 1 (Hydrosniper) : longue portée mono-cible, dégâts élevés
- Axe 2 (Hydropompe) : tir zone, dégâts réduits mais AoE
- Passif : bonne précision naturelle
- Synergie : Léviaphénix

**Aquilombres** — Ombres Glissées (Mono)
- Base : Mode furtif — coup critique dans le dos de la cible
- Axe 1 : dégâts critiques augmentés + retour furtif auto
- Axe 2 : Ombres Projetées — ombre massive devant lignes ennemies, réduit visibilité/précision
- Passif : Invisibles si immobiles
- Synergie : Chef de faction (Aquis)

---

### NOXÉENS (ressource : Biolumens)

| Unité | Type | PV | ATK/s | DEF% | Vit | Portée | T.Att | Zone | CD | Coût | Diff | Syn |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Noxar (Chef) | Chef | 1300 | 150 | 10 | 1.0 | 1 | 1.0s | Cône moyen | 12s | 230 | 4 | 4 |
| Noxedrake | Mythique | 2000 | 180 | 12 | 1.0 | 2 | 1.2s | Souffle/Zone | 18s | 420 | 5 | 3 |
| Noxeflare | Infanterie | 1000 | 170 | 8 | 1.1 | 1 | 0.8s | Mono | 10s | 125 | 2 | 2 |
| Noxeblast | Distance | 900 | 140 | 5 | 1.0 | 5 | 1.1s | Mono | 8s | 140 | 3 | 3 |
| Noxéons | Spéciale | 1100 | 90 | 8 | 0.9 | 3 | 1.3s | Zone | 10s | 160 | 4 | 5 |
| Noxebeast | Montée | 1700 | 120 | 20 | 1.2 | 1 | 1.1s | Petite zone | 14s | 190 | 3 | 3 |

**Détails des compétences Noxéens :**

**Noxar** — Fracture Abyssale (tirs laser DISCONTINUS)
- Type attaque : MÊLÉE
- Base : Tirs laser courts discontinus — harcèlement + ignore partiellement l'armure
- Axe 1 (Domination Laser) : rayons traversants, dégâts exponentiels sur cible isolée, explosion finale, recharge réduite sur élimination
- Axe 2 (Surcharge Bioluminescente) : halo amplif vitesse d'attaque + dégâts énergétiques + résistance peur/contrôle alliés
- Passif (Cœur Abyssal Instable) : chaque élimination proche = charge de Surcharge. Catalyse recharge Noxedrake.
- Synergie : Noxedrake (accélère sa recharge)

**Noxedrake** — Souffle d'Extinction (rayon CONTINU — ≠ Noxar qui fait des tirs discontinus)
- Base : Laser continu géant depuis la gueule — traverse les unités — dégâts massifs/s — applique Combustion Luminale (DoT)
- Axe 1 (Dévastation Totale) : souffle plus large, explosion terminale fin canal, recharge réduite sur élimination
- Axe 2 (Dominion Radieux) : marquage cumulatif (plus exposé = plus dégâts), légère auto-régénération sur dégâts infligés
- Passif (Instinct des Profondeurs) : en infligeant dégâts continus : vitesse augmente, résistance contrôle s'améliore, scaling progressif
- Synergie : Noxar (accélère sa recharge)
- ⚠️ PV faibles pour un mythique, vulnérable pendant canalisation, priorité ennemie absolue

**Noxeflare** — Éblouissement Abyssal (Mono)
- Type : MÊLÉE. Corps violets, yeux lumineux violets multiples.
- Attaque base : Griffes Crépusculaires (bonus sur ennemis aveuglés)
- Base : Flash violet bioluminescent frontal — réduction précision ennemie — désorientation courte
- Axe 1 (Voie du Voile Profond) : zone élargie, durée augmentée, ralentissement ajouté, chance d'interruption
- Axe 2 (Voie de la Frappe Aveugle) : bonus dégâts massifs sur aveuglés, recharge réduite sur élimination
- Passif (Aura d'Incertitude) : ennemis proches subissent légère baisse précision passive permanente
- Synergie clé : Noxeblast (combo aveuglement + exécution)

**Noxeblast** — Décharge Abyssale (Mono)
- Design : humanoïde abyssal sombre, yeux bleus lumineux, 2 tentacules max dans le dos, projectile depuis les PAUMES (pas d'armes)
- Base : Tir énergie concentrée, mono-cible, longue portée, dégâts purs
- Axe 1 (Rayon Perforant) : tirs traversants qui percent plusieurs unités alignées
- Axe 2 (Explosion Bioluminescente) : explose à l'impact — aveugle unités proches — désorganise formations
- Passif (Yeux des Abysses) : bonus dégâts significatif sur cible affectée par désorientation/aveuglement
- Synergie clé : Noxeflare (Noxeflare aveugle, Noxeblast exécute)

**Noxéons** — Émergence Luminale (Zone)
- Design : organismes bioluminescents massifs, couleur verte
- Attaque base : Tentacules bioluminescents
- Base : Zone bioluminescente au sol — bonus dégâts + vitesse aux Noxéens dans la zone
- Axe 1 (Réacteur de Guerre) : bonus zone fortement augmentés, cooldowns réduits, charge Noxedrake accélérée
- Axe 2 (Ancrage Abyssal) : zone plus large, résistance accrue alliés, régénération continue, réduction contrôles
- Passif (Réseau Luminescent) : chaque Noxéon actif augmente légèrement la production énergétique globale. Cumulatif.
- Synergie : Noxar et Noxedrake
- ⚠️ Faible mobilité, totalement dépendant de la protection

**Noxebeast** — Fracasse-Fosse (Petite zone)
- Base : Charge destructrice frontale — repousse/renverse unités légères — interrompt compétences ennemies
- Axe 1 (Bastion Brutal) : réduction massive dégâts après charge — provocation courte — zone instable au sol (ralentit)
- Axe 2 (Défoncement) : charge plus rapide, dégâts augmentés, perfore formations, renverse unités lourdes
- Passif (Carapace Pressurisée) : plus il subit dégâts consécutifs, plus résistance augmente. Immunité brève contrôle à haut seuil.
- Synergie : Noxedrake (ouvre les formations pour le souffle)
- ⚠️ VULNÉRABLE 2-3 secondes APRÈS la charge

---

## 8. Bâtiments Aquiloris (validés)

| Bâtiment | Rôle |
|---|---|
| Cristalliseur | Capture zone, génération cristaux, montée grade territoires |
| Académie Aquiloryon | Recrutement/amélioration Aquiloryons |
| Dôme des Aquilances | Recrutement/amélioration Aquilances |
| Champ de Tir des Aquisphères | Recrutement/amélioration Aquisphères |
| Nexus des Ombres | Recrutement/amélioration Aquilombres |
| Cœur-Éclat du Léviaphénix | Gestion/amélioration mythique |
| Sanctuaire Cristallin | Stockage et protection ressources |
| Atelier des Courants | Recherches et améliorations globales |
| Puits des Courants Cristallins | Production/stockage mana et énergie magique |
| Aquilore | Cité principale, commandement, chef Aquis |

---

## 9. Structs de données — RÈGLE ABSOLUE

- `S_UnitData` : toutes les stats numériques des unités — **JAMAIS hardcodées ailleurs**
- `S_ZoneData` : toutes les données de zones

---

## 10. Règles à ne jamais enfreindre

- Pas de `GetAllActorsOfClass` → `UFactionRegistrySubsystem`
- Pas de `FTimerHandle` par unité → `UTacticalPhaseManager`
- Pas de stats hardcodées → `UPrimaryDataAsset`
- Convention nommage UE5 : `U` (UObject/Subsystem), `A` (Actor), `F` (struct), `E` (enum)
- Tout point d'entrée Blueprint : `UFUNCTION(BlueprintCallable)` ou `BlueprintImplementableEvent`
- **Scope démo : Aquiloris + Noxéens UNIQUEMENT**
- **Ne jamais implémenter la carte stratégique (Couche 2)**

---

## 11. Journal technique Notion

À la fin de chaque session de travail, tu mets à jour la page Notion 🛠️ Journal technique WOTOL avec ce que tu as fait, les décisions prises, et le prochain blocage. ID de la page : 38872c33-8b59-81d6-9686-cbd8653395ed

---

## 12. Build & compilation — RÈGLES ANTI-BUG (vécues, ne JAMAIS réintroduire)

Ces problèmes ont déjà bloqué l'ouverture du projet. Tout code généré doit respecter
ces règles pour compiler du premier coup dans UE 5.7.4.

### Environnement
- **Moteur installé : Unreal Engine 5.7.4** → `WOTOL.uproject` doit avoir `"EngineAssociation": "5.7"`
- **Visual Studio 2022** avec la charge de travail **« Game development with C++ »** OBLIGATOIRE
- Fichiers `Target.cs` obligatoires dans `Source/` : `WOTOL.Target.cs` + `WOTOLEditor.Target.cs`
- Dans les `Target.cs` : `DefaultBuildSettings = BuildSettingsVersion.Latest` (JAMAIS `V5` →
  provoque le conflit `UndefinedIdentifierWarningLevel: Off != Error` avec l'éditeur partagé)

### Encodage des fichiers
- `WOTOL.uproject` doit rester **UTF-8 sans BOM**. **NE JAMAIS l'ouvrir/enregistrer dans le Bloc-notes**
  (ça le convertit en UTF-16 → erreur `JsonException: '0xFF' is an invalid start of a value`)
- Tous les fichiers source `.h`/`.cpp` en UTF-8

### Conventions de nommage UE (erreurs UHT bloquantes)
- Classe dérivée d'un **Acteur** (AActor, ACharacter, **AAIController**, APawn, AController…) →
  préfixe **`A`** obligatoire. Ex : `AAIAdaptiveController` (PAS `UAIAdaptiveController`)
- Classe dérivée de **UObject** (subsystem, component, data asset, widget) → préfixe **`U`**
- Erreur typique : `Class 'UXxx' has an invalid Unreal prefix, expecting 'AXxx'`

### UFUNCTION — interdits UHT
- **Jamais** retourner un **pointeur de struct** (`const FMaStruct*`) depuis une `UFUNCTION`.
  → Garder la fonction en C++ pur (sans macro UFUNCTION), et fournir une version Blueprint
  qui renvoie une **copie + booléen** via paramètre de sortie (`bool GetX(FName, FMaStruct& Out)`)
- **Jamais** nommer un paramètre de `UFUNCTION` comme un membre hérité d'`AActor` :
  `Instigator`, `Owner`, `Role`, `Owner` masquent les membres → erreur `shadowing is not allowed`.
  Utiliser `InstigatorUnit`, `OwnerActor`, etc.

### Includes
- Tout header qui utilise un type de `WOTOLTypes.h` (`EFactionID`, `EResourceType`, `FUnitStats`…)
  doit faire `#include "Data/WOTOLTypes.h"` (les forward declarations ne suffisent pas pour les enums/structs utilisés par valeur)
- `WOTOL.Build.cs` DOIT contenir `PublicIncludePaths.Add(ModuleDirectory);` sinon les includes
  relatifs à la racine du module (`"Data/WOTOLTypes.h"`, `"Gameplay/Units/UnitBase.h"`) échouent →
  erreur `fatal error C1083: Cannot open include file`

### Méthode de récupération du code recommandée (Liamor)
- Utiliser **GitHub Desktop** (clone + bouton « Pull ») plutôt que « Download ZIP » à répétition :
  évite la corruption d'encodage et permet la compilation **incrémentale** (rapide).
- Workflow : Pull origin → ouvrir `WOTOL.uproject` → recompiler. Jamais éditer les fichiers à la main.
