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
- Noms affichés : Akis / Aquiloryons / Aquilans / Akisfères / Aquilombre.
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

### 5. Vue cité — rendu & interaction ✅ 🧪 (commit `2ffba03`)
- HUD `DrawCityView` : fond `Cite_Aquiloris.png`, compteur de cristaux, cartes de bâtiments
  cliquables (Académie→Aquiloryons, Dôme→Aquilans, Champ de tir→Akisfères, Nexus→Aquilombre,
  Cœur-Éclat→Léviaphénix), bouton « Partir en expédition ».
- PlayerController : clics des cartes → `ProduceUnit`, bouton départ → lance la phase suivante.
- Director : au spawn de l'armée joueur, `DrainReserve` pour ajouter les unités produites.

### 6. Écran de chargement + transitions ✅ 🧪 (commit `35dcc65`, écran de base)
- HUD `DrawLoading` (fond animé + logo + barre/anneau + astuce). Écran `Loading` intercalé
  entre phases lourdes (cité↔bataille, monde↔bataille).

### 7. Monde ouvert / nage libre 3D ✅ 🧪
- Pion nageur ZQSD/WASD + montée/descente, caméra 3e personne, silhouette greybox visible.
- **Connecté à la démo jouée dans la même carte procédurale** : possession caméra RTS ↔ héros,
  sans `OpenLevel` et sans perdre les ressources/progression du `GameInstance`.
- Après le bouton explicite « Lancer la partie » : chargement/lore → exploration → approche à
  5–10 m du Kraken → chargement → placement de l'armée → bataille tutorielle.
- Après le rapport du Kraken : retour réel en nage libre avant la séquence Cristalliseur.

### 8. Séquence Cristalliseur → Cœur-Éclat → œuf ✅ 🧪
- Après victoire créature : fenêtre d'objectif → carte du Cristalliseur dans l'inventaire →
  sélection → cible circulaire lumineuse dans le monde 3D → clic valide → dépense atomique →
  construction et acquisition du territoire → Cœur-Éclat → œuf de Léviaphénix.
- Le bâtiment posé est conservé jusqu'à la défense ; aucun deuxième Cristalliseur n'est généré.

### 9. Défense du Cristalliseur (phases 10-12) ✅ 🧪
- Bâtiment avec PV/bonus/réparation (`WOTOLCaptureObject` existe déjà — à étendre).
- Destruction = **échec immédiat** (fenêtre rouge) → zone neutre. Pas de bâtiment ennemi posé.

### 10. Câblage du flux 13 phases dans le Director ✅ 🧪
- `bEnableFullFlowV08` est désormais **activé par défaut** ; l'ancienne boucle reste seulement
  comme repli de diagnostic.
- Fenêtres plein écran corrigées : une modale d'objectif peut maintenant s'afficher sur
  Exploration, Interlude, Cité et Chargement sans être masquée par un `return` du HUD.
- Rapport Kraken : récompenses séparées (cristaux, matériaux abyssaux, biomasse, nourriture),
  puis coût atomique du Cristalliseur. Montants provisoires et éditables dans le Director.
- Cité : bâtiment Akisfères/Nox Blast à construire sur l'une des 3 parcelles libres, coûts
  cristal + matériau abyssal retirés seulement au clic de placement.
- Objectif obligatoire : produire 10 unités à distance. Progression visible, plafond d'armée
  à 35 et réservation automatique des places/cristaux nécessaires pour éviter tout soft-lock.
- La 10e production déclenche une alerte noxéenne modale. Après validation, le bouton de cité
  devient « Défendre le Cristalliseur » et mène à la préparation de la bataille rivale.
- Économie greybox provisoire : récompense Kraken 2200 cristaux / 100 matériaux, bâtiment
  distance 300 / 25, unité distance 140. Ces valeurs restent éditables, pas validées comme balance.

### 10b. Équilibrage adaptatif des pertes — phases 1 et 2 ✅ 🧪
- Le Director mesure toutes les 0,75 s : faction, difficulté, effectifs réels, pertes joueur,
  effectif/état de santé ennemi et temps écoulé.
- Il applique un correcteur séparé aux dégâts infligés/reçus par l'ennemi. Les statistiques,
  bonus de faction, axes, bâtiments et avantages de territoire restent intacts et continuent
  d'influencer le résultat ; la boucle fermée compense leur effet observé pendant la partie.
- Un unique ennemi « ancre » (Kraken ou chef rival) conserve 6 % de PV tant que le minimum de
  pertes n'est pas atteint. Au maximum de la plage, les survivants sont protégés à 1 PV et
  l'ennemi devient très vulnérable : une partie terminée reste dans la plage demandée.
- Cibles de référence :

| Phase | Facile | Normal | Difficile |
|---|---:|---:|---:|
| Kraken — 16 unités | 2–3 pertes | 5 pertes | 10 pertes, 6 survivants |
| Défense — 35 unités | 7–8 pertes | 14–16 pertes (cible 15) | 15–20 pertes (cible 18) |

- Si l'effectif réel change, chaque borne est recalculée par
  `arrondi(effectif réel × pertes de référence / effectif de référence)`.
- Correction du compteur cité : la phase 2 possède 25 unités de base (1 + 16 + 8), puis les
  10 unités à distance produites remplissent exactement le plafond `35/35`.

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
Akis / Aquiloryons / Aquilans / Akisfères / Aquilombre, factions démo = Aquiloris + Noxéens.
Les maquettes servent pour le **style visuel**, pas pour les noms.

---

## Conception : voir `Docs/JEUX_DE_REFERENCE.md`
Chaque boucle WOTOL est mappée à un jeu éprouvé (Total War, Homeworld, XCOM, Company of Heroes,
Warcraft III) avec les patterns concrets à copier. Synthèse : *« campagne Total War sous-marine,
en volume Homeworld »*.

## Reste à faire (nécessite un PC pour compiler/valider)
- Compiler UE 5.8 et tester l'enchaînement réellement branché : menu → lancement manuel → nage →
  proximité Kraken → placement/bataille → rapport/récompenses → retour nage → placement spatial
  Cristalliseur → récompenses → cité → bâtiment distance → 10 unités → alerte → défense.
- Faire au minimum 3 simulations par faction et difficulté sur les phases 1/2 ; vérifier les
  plages de pertes ci-dessus, la durée, et l'absence de blocage à 6 %/1 PV.
- **HUD** : jauge verticale SURFACE/MID/SOL (Homeworld), panneau héros + capacités, restyle fenêtre d'objectif.
- **Module 11** : intégrer logo/emblèmes/icônes de rôles (assets de référence).
- **Module 12** : build autonome (packaging Windows) — côté éditeur.

## Ordre de compilation conseillé au retour du PC
1. Pull `feature/v0.8-jeu-complet`, recompiler, **vérifier que ça build** (tous les modules).
2. Valider visuellement : **vert Noxéen**, **échelle Noxedrake**, **fenêtre d'objectif**, **vue cité**, **nage**.
3. Basculer `bEnableFullFlowV08 = true` et tester la boucle 13 phases complète.
4. On restyle le HUD d'après les maquettes + on package.
