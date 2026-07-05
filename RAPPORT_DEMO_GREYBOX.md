# 🎮 WOTOL — Démo greybox : rapport de livraison (Stages 1-2)

Branche : **`feature/demo-greybox-flow`** (partie de la base stable `claude/base-jouable-v1`).
Base figée intacte : aucun fichier existant modifié — **100 % additif**.

---

## 1. Fichiers créés (8 nouveaux, dossier `Source/WOTOL/Gameplay/Demo/`)

| Fichier | Rôle |
|---|---|
| `DemoFlowSubsystem.h/.cpp` | Machine d'états des 41 étapes + déblocages + persistance GameInstance |
| `WOTOLDemoUnit.h/.cpp` | Unité greybox contextualisée (forme/taille/couleur par catégorie) |
| `WOTOLCaptureObject.h/.cpp` | Cristalliseur (Aquiloris) / Abyssalyseur (Noxéens) : PV, dégâts, réparation, capture |
| `WOTOLDemoDirector.h/.cpp` | Assemble les 2 armées selon le roster débloqué et lance la bataille RTS |
| `WOTOLGameMode_Demo.h/.cpp` | Auto-assemble la scène (Director + caméra) pour limiter la config éditeur |

**Fichiers modifiés : AUCUN.**

---

## 2. Comment tester (après compilation)

1. Récupérer la branche `feature/demo-greybox-flow`, compiler.
2. Ouvrir un niveau avec un **sol** + poser un **`NavMeshBoundsVolume`** au-dessus (touche **P** = nav verte ; sans ça l'IA ne bouge pas).
3. **World Settings → GameMode Override = `WOTOLGameMode_Demo`**.
4. **Play** → 2 armées de formes contextualisées (chef + 10 infanterie + 5 montées) s'affrontent automatiquement, pilotées par tout le code RTS existant.

Faction joueur : vient du menu si lancé depuis le flow ; sinon **Aquiloris par défaut** (réglable sur le Director : `DefaultPlayerFaction`).

---

## 3. Ce qui est démontré dès maintenant

- ✅ Greybox contextualisé : formes par catégorie, **tailles réelles du GDD**, **couleurs de faction** (bleu/vert)
- ✅ Roster progressif : distance **verrouillée** tant que la 1ère victoire n'est pas marquée (`DemoFlowSubsystem`)
- ✅ Combat temps réel (réutilise `RTSBattleManager`, IA, moral, verticalité existants)
- ✅ Type de bataille paramétré : CreatureEncounter (1 créature massive) / RivalDefense (escouade rivale)
- ✅ Objet de capture fonctionnel (PV / dégâts / réparation / Grade 1)
- ✅ Machine d'états complète prête à piloter les 41 étapes

---

## 4. Valeurs prototype (clairement temporaires)

- `WOTOLCaptureObject.MaxHealth = 600` → **PrototypeDefault** (exemple du doc : 450/600). À remplacer par la vraie valeur du tableur quand dispo.
- Tailles d'unités (`WOTOLDemoUnit::GetUnitHeightMeters`) = valeurs du document de démo (mètres).
- Créature = réutilise les **stats du mythique rival** (PV existants ~2000) agrandie ×2.5 — aucune stat inventée.

---

## 5. Reste à faire (Stages 3-7, prochaines sessions)

3. Déploiement 3D (volume, légende formes, cadenas, timer, tuto verticalité) + barres de vie monde (HealthBar widget)
4. Bataille créature dédiée (boss bar, IA créature spécifique) + écran victoire
5. Capture → découverte mythique juvénile → retour cité → déblocage distance
6. Alerte rivale → 2e bataille → réparation objet → fin de démo
7. Menus (faction jouable only, héros, difficulté) + écrans victoire/fin

⚠️ **Contraintes éditeur incontournables** (impossibles à faire en code pur) :
- les 4 `.umap` et les widgets UMG doivent être créés dans l'éditeur ;
- le `NavMeshBoundsVolume` (déplacement IA) ;
- choix retenu : **1 niveau + construction par code** pour minimiser ça.

---

## 5bis. ⭐ MISE À JOUR — Démo AUTO-JOUABLE (Stage 3)

La démo s'enchaîne maintenant **toute seule** (idéal projection réunion), sans intervention :

1. **Bataille créature** : ton armée (chef + 10 infanterie + 5 montées) vs 1 créature massive
2. **Victoire détectée** (camp anéanti) → distance **débloquée**, mythique **découvert**, **objet de capture posé** (Cristalliseur/Abyssalyseur), message « Zone capturée — Grade 1 »
3. **Bataille de défense rivale** : ton armée (cette fois **AVEC la distance**) vs escouade rivale
4. **Victoire** → objet de capture **endommagé puis réparé** → **« Fin de démo »**

+ **Décor contextualisé** par code : sol fond marin, **arche centrale** repère, plateaux verticaux, **zones de déploiement colorées** par faction. Messages narratifs affichés à l'écran.

Fichiers ajoutés : `WOTOLGreyboxEnvironment.h/.cpp`. Director enrichi (détection fin de bataille via FactionRegistry + IsAlive, enchaînement des phases, objet de capture, messages).

**Test = 3 étapes** : niveau avec sol + `NavMeshBoundsVolume` → GameMode Override `WOTOLGameMode_Demo` → Play. Tout se déroule seul.

---

## 5ter. ⭐⭐ MISE À JOUR — Boucle jouable complète (Stages 3-7)

La démo est maintenant une **vraie boucle jouable** (plus seulement une projection auto) :

### Flux d'écrans (HUD 100 % C++ Canvas, aucun UMG)
`Menu principal` → `Choix de faction` → `Préparation (placement)` → `Bataille` →
`Résumé de bataille` → (boucle) `Rejouer / Changer de faction / Quitter`.

- **Faction réellement jouable** : choisir Noxéens fait jouer les Noxéens (mêmes mécaniques),
  l'IA pilote les Aquiloris en face — et inversement. Source fiable = `DemoFlowSubsystem::SelectedFaction`.
- **Préparation (2 phases)** : on place ses unités dans **son premier tiers** (barrière colorée
  + clamp), puis bouton **« Lancer la bataille »**. Boutons **Monter / Descendre** (couche verticale).
- **Écran de résumé** : à la fin de chaque bataille, **pertes détaillées par type d'unité** des
  **deux camps** (perdus / total / survivants). Phase 1 → bouton *Continuer*; fin de démo →
  *Rejouer / Changer de faction / Quitter*, avec **remise à zéro propre** du HUD.

### Combat & ressenti
- **Auto-combat** : les unités engagent d'office l'ennemi le plus proche (comme l'IA) ; clic droit
  = ordre (déplacer en **conservant la formation** / attaquer, calage sur la **couche** de la cible).
- **Parade / Esquive** (matrice v3) → « Pare » / « Esquive » flottants, combats plus longs.
- **Projectiles visibles** (boules) pour la distance ; **chiffres de dégâts** + éclats de bulles.
- **Sélection** : boîte de drag, `Ctrl` = tout sélectionner, **double-clic** = zoom rapproché
  sur l'unité. Barre de commandement = cartes de groupe avec **PV total / restant**.

### Boss « Kraken »
- **Design unique** (céphalopode + 2 grands fouets **articulés**), **identique quelle que soit la
  faction** jouée (avant : on affrontait le mythique adverse). Silhouette forcée via `bIsBoss`
  posé **avant** `FinishSpawningActor`.
- **Coup de fouet** périodique (~2 s) : **repousse + blesse** les unités devant lui.
- **Coriace** : PV ×11, **défense 55 % / parade 45 %** → vrai pilier de la démo.

### Caméra & contrôles
- Clavier **AZERTY** : **Z** avancer, **S** reculer, **Q** gauche, **D** droite, **E** monter,
  **Espace** descendre (+ flèches et WASD conservés). Clic droit maintenu = rotation ; molette = zoom.

### Rendu / lisibilité
- Décor sous-marin (brouillard, lumière tamisée, récif, poissons/requins/algues).
- **Contraste** sur tous les noms (ombre noire) : alliés, ennemis, bâtiments.
- **Objet de capture** (Cristalliseur / Abyssalyseur) : étiquette **petite, posée au socle**
  (racine non-scalée) — plus de texte géant flottant.
- **Objectifs** dans une **fenêtre fixe en haut à gauche**, jamais cachée par la barre du Kraken.

### Performance
- Phase 2 dimensionnée **~26 vs 26** unités entièrement riggées (stabilité portable).

Fichiers ajoutés depuis : `WOTOLDemoHUD.*`, `WOTOLPlayerController_Battle.*`, `WOTOLBattleCamera.*`,
`WOTOLProjectileTracer.*`, `WOTOLDamageNumber.*`, `WOTOLBubbleBurst.*`, `WOTOLAmbientFish.*`,
`UnitSelectionManager` + rig articulé procédural dans `WOTOLDemoUnit`.

### Reste hors-scope (assets fournis par Liamor)
- **Audio** : je câble les sons si Liamor fournit les fichiers (je ne génère pas d'audio).
- Remplacement des greybox par les meshes Meshy + Blueprints/UMG visuels (côté éditeur).

---

## 5quater. 💡 EN RÉSERVE (à ne PAS implémenter tout de suite)

Idées validées par Liamor à garder pour plus tard :

- **Phase 3 — « La Revanche »** : après une **ellipse temporelle** (racontée hors-champ via un
  écran de transition), une 3e bataille de **grande ampleur** entre **Aquiloris et Noxéens**
  (les 2 factions jouables), avec le **roster COMPLET** de chaque faction — donc les **2 unités
  manquantes** : le **Mythique jouable** + l'**unité Spéciale** (Aquilombres / Noxéons).
  But : montée en complexité progressive (Phase 1 = 1 seul ennemi = le Kraken → Phase 2 = escouade
  rivale + siège du bâtiment → Phase 3 = affrontement total roster complet). Test de lisibilité RTS.
- Transition narrative : présentation du mythique adopté (voir écran d'interlude actuel) qui,
  en phase 3, deviendrait **jouable**.

### Compétences des unités MANQUANTES (à implémenter en phase 3) — dictées par Liamor
**AQUILORIS**
- **Aquilombres (Spéciale)** — Compétence (CD) : **se téléporte derrière les lignes ennemies**,
  assène un **coup critique dans le dos**, puis **revient à sa position initiale** (arrière).
- **Léviaphénix (Mythique)** — c'est un **BUFFER** : sa compétence **renforce les attaques, les
  compétences et la défense** des autres unités de la faction (aura). Pas de grosse attaque ;
  au corps-à-corps il donne des **coups de nageoire / de queue** quand on l'attaque de près.

**NOXÉENS**
- **Noxéons (Spéciale)** — fait **jaillir de la bioluminescence du fond marin** (zone) pour
  **augmenter les capacités énergétiques** des unités Noxéennes proches (buff Noxeblast /
  Noxeflare / Noxedrake / Noxar…). Support de zone.
- **Noxedrake (Mythique, = le Kraken jouable)** — grosse compétence : **gros rayon VERT** (faisceau
  continu, couleur de faction) ; au corps-à-corps, **coups de mâchoire** (mord les ennemis proches).

### Corrections de couleur (déjà appliquées aux unités de phase 2)
- **Rayon de Noxar** = **VERT** (couleur de faction), pas violet.
- **Flash d'éblouissement de Noxeflare** = **VIOLET**.

⚠️ Statut : **idée en réserve, non codée** (sauf les corrections de couleur ci-dessus).
À réévaluer quand la phase 2 sera validée en jeu.

---

## 6. Garanties

- Base figée `claude/base-jouable-v1` **non touchée**.
- Tout le code démo respecte le `UE5_CPP_PLAYBOOK.md` (anti-erreurs de compil).
- Travail isolé sur `feature/demo-greybox-flow`.
- ⚠️ Non compilé localement (pas d'Unreal côté assistant) → corriger les éventuelles erreurs au 1er build en collant le **texte** du log.
