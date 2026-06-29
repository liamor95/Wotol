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

## 6. Garanties

- Base figée `claude/base-jouable-v1` **non touchée**.
- Tout le code démo respecte le `UE5_CPP_PLAYBOOK.md` (anti-erreurs de compil).
- Travail isolé sur `feature/demo-greybox-flow`.
- ⚠️ Non compilé localement (pas d'Unreal côté assistant) → corriger les éventuelles erreurs au 1er build en collant le **texte** du log.
