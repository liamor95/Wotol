# WOTOL — Jeux de référence & patterns à copier

Objectif : ne pas réinventer la roue. Chaque boucle de WOTOL existe déjà, éprouvée, dans un
jeu connu. On **copie le pattern qui marche** et on l'adapte à l'univers sous-marin.
Principe directeur (Liamor) : **interface claire et simple, compréhensible par tout le monde.**

---

## 1. Bataille rangée temps réel (le cœur du jeu)

**Références : Total War (Warhammer), Company of Heroes.**

| Pattern éprouvé | Application WOTOL | État code |
|---|---|---|
| Unités en **groupes/régiments** (une carte = un groupe, pas 1 soldat) | Marqueurs groupés (icône+effectif+barre) | ✅ fait |
| **Roster** d'unités en bas d'écran, clic = sélection du groupe | Barre de commandement | ✅ fait |
| **Formations** (blocs, ligne, cohésion) | Blocs de 5, `SetFormation` | ✅ fait |
| **Moral** : un groupe qui craque fuit (rout) | Jauge de moral → Routing | ✅ existe (à polir) |
| **Sélection à la boîte** + double-clic = focus caméra | Box-select + suivi caméra | ✅ fait |
| **Objectif de bataille** affiché en clair (haut d'écran) | Bandeau objectif | ✅ fait |
| Barre de **PV du boss** en bas/haut | `DrawBossBar` | ✅ fait |

> À retenir de Total War : **lisibilité de la masse**. On ne suit jamais 1 unité, on lit des
> blocs colorés + bannières + barres de groupe. C'est déjà notre direction.

---

## 2. Combat en VOLUME (verticalité 3D)

**Référence n°1 : Homeworld** (le seul grand RTS en vraie 3D volumétrique).

| Pattern Homeworld | Application WOTOL |
|---|---|
| **Bandes de couches** + indicateur d'altitude à l'écran | Jauge **SURFACE / MID / SOL** (maquette fournie) — mes boutons monter/descendre deviennent cette jauge |
| Déplacement en 3D mais **contrôle simplifié** (plan + hauteur explicite) | ZQSD au plan + monter/descendre dédiés (module 7) ✅ |
| Caméra qui **suit en 3D** sans perdre le joueur | `FollowGroup` caméra ✅ |
| **Bonus tactique** selon l'axe d'attaque | Attaque ascendante = +dégâts (règle GDD) ✅ |

> À faire : transformer les 2 boutons couche en **jauge verticale visuelle** (clarté).

---

## 3. Cité / production entre les batailles

**Références : campagne Total War (colonie), XCOM (base), Northgard.**

| Pattern éprouvé | Application WOTOL | État |
|---|---|---|
| Écran **base/cité** distinct de la bataille | `EDemoScreen::City` + `DrawCityView` | ✅ fait |
| **Bâtiments cliquables** qui produisent des unités | Cartes de production | ✅ fait |
| Dépenser une **ressource** pour produire | Cristaux + `ProduceUnit` | ✅ fait |
| Les unités produites **renforcent l'armée** de la mission suivante | Réserve → `DrainReserve` dans l'armée | ✅ fait |
| Bouton **« Partir en mission »** clair | « Partir en expédition » → défense | ✅ fait |

> À retenir de XCOM : entre deux missions on **prépare**, on n'est jamais jeté sans choix.
> D'où : cité → produire → partir, avec un bouton explicite (jamais d'enchaînement auto).

---

## 4. Enchaînement des missions (flux narratif)

**Références : XCOM, Darkest Dungeon, Warcraft III (campagne).**

| Pattern | Application WOTOL | État |
|---|---|---|
| **Briefing/objectif** avant chaque étape, validé au clic | Fenêtre d'objectif modale + « Continuer » | ✅ fait |
| Messages **victoire / échec** explicites | Fenêtre bleue/rouge + résumé | ✅ fait |
| Récompense montrée (butin, XP) | Fenêtre avec récompenses (à styliser) | 🔨 |
| Pas d'auto-chain qui perd le joueur | Machine à étapes `OnObjectiveConfirmed` | ✅ (module 10 en cours) |

---

## 5. Territoire & bâtiment défendable

**Références : Company of Heroes / Northgard (points de capture), Dawn of War.**

| Pattern | Application WOTOL | État |
|---|---|---|
| **Capturer** un point donne ressources + bonus | Cristalliseur : capture zone + bonus atk/def | ✅ existe |
| Bâtiment avec **PV**, réparable | `WOTOLCaptureObject` (PV, `Repair`) | ✅ |
| **Défendre** sous assaut = tension | Siège + défaite si détruit | ✅ (module 9) |
| Perte du point = conséquence claire | Destruction = **défaite immédiate**, zone neutre | ✅ |

---

## 6. Héros & créature mythique

**Références : Warcraft III (héros + capacités + niveaux), Monster Hunter (boss à patterns).**

| Pattern | Application WOTOL | État |
|---|---|---|
| Héros avec **barre de capacités** + niveau | Panneau héros (maquette) | ⏳ à faire au HUD |
| **Aura/présence** qui booste les troupes | Commandant = bonus moral (GDD) | ✅ existe |
| Boss à **patterns télégraphiés + phase de rage** | IA créature (attaque zone, changement de couche, rage) | 🔨 à enrichir |
| Boss **punit l'inaction** | Le Kraken attaque activement | ✅ (à polir) |

---

## 7. Ce qu'on NE copie pas (garde-fous)

- Pas de **base-building temps réel** façon StarCraft pendant la bataille (trop complexe pour
  la démo) : la production reste à la **cité**, entre les batailles (façon Total War campagne).
- Pas de **micro-gestion d'inventaire** façon Darkest Dungeon (hors scope démo).
- Pas de **tour par tour** : WOTOL est temps réel (comme Total War en phase bataille).

---

## Synthèse : la boucle WOTOL = « campagne Total War sous-marine, en volume Homeworld »

**Cité (XCOM/Total War) → briefing (XCOM) → exploration nage (Homeworld) → bataille rangée en
volume (Total War + Homeworld) → capture/défense de territoire (Company of Heroes) → retour cité.**

Chaque brique est un pattern connu, lisible, déjà validé par des millions de joueurs. On les
assemble proprement pour WOTOL, avec une UI simple.
