# WOTOL — Roadmap v0.8 (« jeu complet »)

Branche : `feature/v0.8-jeu-complet` (partie du commit gelé démo v0.7.9 `d8283d6`).
Objectif : passer de la démo-bataille figée à la **boucle 13 phases** du document maître
(`Docs/DOCUMENT_MAITRE_WOTOL.md`), avec cité, monde ouvert, séquence Cristalliseur/Cœur-Éclat/œuf,
défense, et écrans de transition/chargement.

## Légende
✅ fait & poussé · 🔨 en cours · ⏳ à faire · 🧪 à compiler/tester (pas de PC actuellement)

---

## Modules

### 1. Nettoyage de canon ✅ 🧪
- Noxéens en **vert** (Noxeflare, Noxeblast, effets, éblouissement) — plus de violet.
- Noms affichés : Akis / Akilorions / Aquilans / Akisfères / Aquilombre.
- Tailles : Léviaphénix 3,8 m · Noxedrake 6,75 m.
- Commit `fb31233`.

### 2. Fenêtre d'objectif modale ✅ 🧪
- Subsystem : `OpenObjectiveWindow` / `ConfirmObjectiveWindow` / delegate `OnObjectiveConfirmed`.
- HUD : `DrawObjectiveWindow` (voile + panneau titre/corps/bouton, bleu=objectif / rouge=échec).
- PlayerController : clic prioritaire « Continuer » + gel de l'action tant qu'ouverte.
- Commit `d2a0b20`.

### 3. Docs & visuels de référence ✅
- `Docs/DOCUMENT_MAITRE_WOTOL.md` (source de vérité canon).
- `Content/UI/Reference/` : logo, cité Aquiloris, cartes, emblèmes, icônes de rôles.
- Commit `131e6c5`.

### 4. Économie de la cité ✅ 🧪
- Cristaux, `ProduceUnit`/`CanProduce`/`GetProductionCost`, réserve `ReserveUnits`, `DrainReserve`.
- Écrans `EDemoScreen::City/WorldMap/Loading` (enum).
- Commit `48c77d5`.

### 5. Vue cité — rendu & interaction ⏳
- HUD `DrawCityView` : fond `Cite_Aquiloris.png`, compteur de cristaux, cartes de bâtiments
  cliquables (Académie→Akilorions, Dôme→Aquilans, Champ de tir→Akisfères, Nexus→Aquilombre,
  Cœur-Éclat→Léviaphénix), bouton « Partir en expédition ».
- PlayerController : clics des cartes → `ProduceUnit`, bouton départ → lance la phase suivante.
- Director : au spawn de l'armée joueur, `DrainReserve` pour ajouter les unités produites.

### 6. Écran de chargement + transitions ⏳
- HUD `DrawLoading` (fond animé + logo + barre/anneau + astuce). Écran `Loading` intercalé
  entre phases lourdes (cité↔bataille, monde↔bataille).

### 7. Monde ouvert / nage libre 3D ⏳
- Pion nageur ZQSD + montée/descente, caméra 3e personne, zone neutre ~70 m².
- Transition automatique à ~5–10 m de la créature → bataille (fenêtre d'objectif).

### 8. Séquence Cristalliseur → Cœur-Éclat → œuf ⏳
- Après victoire créature : fenêtre « Placez le Cristalliseur » → pose → Cœur-Éclat apparaît →
  fenêtre « Récupérez le Cœur-Éclat » → œuf de Léviaphénix (récompense).

### 9. Défense du Cristalliseur (phases 10-12) ⏳
- Bâtiment avec PV/bonus/réparation (`WOTOLCaptureObject` existe déjà — à étendre).
- Destruction = **échec immédiat** (fenêtre rouge) → zone neutre. Pas de bâtiment ennemi posé.

### 10. Câblage du flux 13 phases dans le Director ⏳
- Remplacer l'enchaînement 3-batailles par la machine 13 phases pilotée par les fenêtres
  d'objectif (`OnObjectiveConfirmed` → `Director` agit → phase suivante).

### 11. Intégrations UI d'assets ⏳
- Logo (menu), emblèmes (sélection faction), icônes de rôles (marqueurs HUD), écran carte.

### 12. Build autonome ⏳
- Cible packagée (exécutable Windows indépendant). À faire côté éditeur/packaging.

---

## Maquettes UI de référence (`Content/UI/Reference/Maquettes/`)

Fournies par Liamor — cibles visuelles pour styliser les écrans C++.

| Maquette | Écran cible | Enseignements de style |
|---|---|---|
| `UI_ReglageJoueur` | sélection faction/difficulté | panneau encadré (coins ornés bleus), Facile/Normal/Difficile, modes Ironman/Aléatoire |
| `UI_PersonnalisationHero` | perso héros (futur) | prénom + héritage + spécialité + portrait |
| `UI_ResumePartie` | récap avant lancement | portrait + résumé + « Lancer la partie » |
| `UI_Chargement_Faction` / `_Intro` | **écran de chargement** | titre faction + **texte de lore** + **barre de progression** + « Astuce » |
| `UI_FenetreObjectif` | **fenêtre d'objectif** | panneau encadré, sous-titre, **icônes de récompense**, bouton ACCEPTER |
| `UI_EscouadeSelection` | **cité / escouade** | cartes d'unités (portrait + catégorie + compteur x/y), « 5 max », CONFIRMER |
| `UI_Tutoriel_Verticalite_01..03` | **tuto phase 1** | panneau latéral + flèches verticales (placement en hauteur, déplacement entre niveaux) |
| `UI_HUD_JaugeVerticale` | HUD bataille | **jauge verticale SURFACE / MID / SOL** à gauche |
| `UI_HUD_Complet` / `_8Ressources` | HUD bataille | barre haute (menu G / timer C / pause·vitesse·réglages D), **8 ressources**, panneau héros (PV+ressource+niveau+capacités), roster groupé (effectifs), minimap |
| `UI_HUD_Bataille_01/02` | HUD bataille | ressources G, objectif D, roue de capacités, minimap |
| `UI_Bataille_Creature` | bataille créature | échelle créature vs armée, ambiance |

### Écarts de nommage dans les maquettes (⚠️ ne PAS suivre)
Les maquettes affichent d'anciens noms (Aquistance, Aquiloryon, Aquilance, « Thalassidras (A) »
comme faction jouée, héros « Aquilian/Aquilance Roi des Profondeurs »). Le **canon du code** prévaut :
Akis / Akilorions / Aquilans / Akisfères / Aquilombre, factions démo = Aquiloris + Noxéens.
Les maquettes servent pour le **style visuel**, pas pour les noms.

---

## Ordre de compilation conseillé au retour du PC
1. Pull `feature/v0.8-jeu-complet`, recompiler, vérifier que **ça build** (modules 1-4).
2. Valider visuellement : **vert Noxéen** + **échelle Noxedrake** + **fenêtre d'objectif**.
3. On enchaîne modules 5→10 (je les prépare d'ici là).
