# WOTOL — Analyse des mécaniques des grands jeux de stratégie (recherche communauté)

**But** : recenser les mécaniques PHARES des plus gros jeux de stratégie récents, ce que les
**communautés** en disent (ce qui marche, ce qui est adoré, ce qui est détesté), puis évaluer
pour CHAQUE mécanique : est-ce **légitime** pour WOTOL ? est-ce que **ça vaut le coup** ?
comment l'**adapter différemment** (jamais copier tel quel) à notre jeu sous-marin en volume ?

> ⚠️ **Aucune de ces mécaniques n'est décidée ni implémentée.** Ce document est une base de
> discussion. Chaque proposition attend TA validation avant tout code. (Recherche : juillet 2026.)

---

## PARTIE 1 — Ce que les communautés ADORENT (transversal)

D'après les discussions joueurs (Reddit, Steam, forums Paradox), reviews 2024-2025 :

1. **Factions ASYMÉTRIQUES** > équilibrage miroir. Les joueurs préfèrent la variété (mécaniques,
   playstyles, forces/faiblesses distinctes) à des factions identiques bien équilibrées.
2. **Progression qui fait sens** (déblocages, montée en puissance ressentie, objectifs long terme).
3. **Décisions macro plutôt que micro** : penser la stratégie, pas cliquer vite.
4. **Identité forte par faction** (AoM : chaque panthéon = un vrai style ; Total War : mécaniques
   de campagne uniques par race).
5. **Le système à 3 niveaux d'unités** d'Age of Mythology (troupe / héros / créature mythique) est
   cité comme un modèle : lisible, stratégique, iconique.

## PARTIE 1bis — Ce que les communautés DÉTESTENT (à ÉVITER absolument)

1. **La micro-gestion pénible** (« click faster to win ») : tue la stratégie réelle. Beaucoup
   cherchent des STR « sans micro ».
2. **Le snowball / scalabilité** : celui qui a la plus grosse boule en milieu de partie gagne tout ;
   en fin de partie les choix deviennent sans intérêt (Stellaris cité en repoussoir).
3. **Fin de partie = enfer de micro-gestion** (trop d'unités/planètes à gérer).
4. **Contrôles 3D lourds** : Homeworld 3 — la 3D est ADORÉE comme concept, mais sélectionner un point
   dans le vide (distance + direction + niveau Z) est jugé « ungainly », pathfinding capricieux,
   sélection de groupes hétérogènes qui rate. **Leçon n°1 pour WOTOL.**

---

## PARTIE 2 — Mécaniques phares par jeu

### Age of Mythology: Retold — *(le plus proche de WOTOL)*
- **3 niveaux d'unités** : troupe humaine (pierre-feuille-ciseau infanterie>cavalerie>archers), **héros**
  (uniques, forts contre les mythiques), **unités mythiques** (créatures surpuissantes, vulnérables aux héros).
- **Pouvoirs divins** (God Powers) : désormais **multi-usage avec cooldown**, réutilisation coûte de la **faveur**.
- **Panthéons asymétriques** : à chaque « âge », choix entre 2 dieux mineurs → techs / mythiques / pouvoirs différents.
- Communauté : adore la **lisibilité** du triangle + le **choix identitaire** des dieux.

### Total War: Warhammer III
- **Roster d'unités en régiments** + formations + **cartes d'unités** ; on lit des blocs, pas des soldats.
- **Seigneurs qui BUFFENT l'armée** (Skarsnik, Khazrak) appréciés > « doomstack » solo (débat, mais tendance).
- **Mécaniques de campagne uniques par faction** (Convois nains du Chaos, Caravanes de Cathay, Sanctuaires
  d'Oxyotl avec téléportation) : très aimées car donnent une **identité**.

### Company of Heroes 3
- **Système de couverture** dynamique (façon XCOM mais temps réel) : chaque obstacle protège ; la
  couverture se **crée et se détruit** en combat (bâtiments rasés, chars épaves réutilisées).
- **Capture de territoire = ressources** (carburant, munitions) + points de victoire ; couper les
  approvisionnements ennemis. Placement des unités crucial.
- Cité comme « meilleur RTS de la décennie » pour cover + destruction.

### Homeworld 3 *(LE cas 3D — à étudier de près)*
- **Mouvement full 3D** avec **terrain spatial** (structures géantes, astéroïdes) → embuscades,
  chokepoints, attaquer par en-dessous, se cacher derrière le terrain. Concept ADORÉ.
- **MAIS** contrôles 3D critiqués (sélection Z pénible), pathfinding et formations bancals.
- **Leçon WOTOL** : garder la verticalité **SIMPLE et lisible** (SURFACE/MID/SOL, pas 6 degrés de
  liberté à la souris). Le terrain (arches, reliefs, failles) DOIT servir tactiquement (couverture,
  contournement par le haut/bas) — c'est là que la 3D devient un plaisir, pas une corvée.

### They Are Billions *(défense de base — pertinent pour la forteresse)*
- Mélange **RTS + tower-defense + gestion**. **Défenses en COUCHES** aux chokepoints (murs + nids de
  snipers ciblant les priorités + AoE). Les ennemis prennent le **chemin le plus court** vers le centre
  → on **canalise** (funneling) les assaillants.
- Spectacle des hordes fauchées par une défense bien pensée = satisfaction.
- **Piège** : ça peut devenir **répétitif et punitif**. À doser.

### XCOM 2 *(boucle base ↔ mission)*
- **Couche stratégique entre missions** = le vrai cœur : recherche, allocation de ressources,
  préparation de l'escouade. Chaque mission compte, chaque choix de base impacte le tactique.
- Escouade **persistante**, investissement émotionnel.

### Crusader Kings 3 *(narratif émergent)*
- Histoires **générées par le joueur** via la simulation (personnages avec opinions, rancunes, complots).
  Adoré pour l'imprévisibilité. → **Peu prioritaire** pour une démo de bataille, mais piste de saveur
  pour les commandants (personnalité, rivalités) plus tard.

### Autres (2025) : EU5 (populations + réalisme militaire), Tempest Rising (C&C moderne, factions
asymétriques, base-building), Stronghold Crusader DE (défense de château, « facile à prendre en main,
dur à maîtriser »), Anno 117 (réseaux d'échange profonds).

---

## PARTIE 3 — Grille d'applicabilité à WOTOL (propositions, À DÉCIDER)

Légende décision : 🟢 fort intérêt · 🟡 à discuter · 🔴 déconseillé pour la démo

| # | Mécanique (jeu) | Accueil communauté | Légitime pour WOTOL ? | Adaptation proposée (≠ copie) | Reco |
|---|---|---|---|---|---|
| 1 | **Verticalité SIMPLE + terrain tactique** (Homeworld, en corrigeant ses défauts) | 3D adorée, contrôles détestés | OUI, c'est notre pilier | Garder SURFACE/MID/SOL clair ; faire que arches/reliefs/failles servent (couverture, contournement haut/bas, embuscade Noxéenne). JAMAIS de sélection Z à la souris. | 🟢 |
| 2 | **Triangle troupe / héros / mythique** (AoM) | Modèle cité en exemple | OUI, on l'a déjà (types + chef + mythique) | Formaliser les **contres** : héros forts vs mythiques, etc., version sous-marine | 🟢 |
| 3 | **Pierre-feuille-ciseau des types** (AoM/AoE) | Base saine, lisible | OUI | Infanterie/Montée/Distance/Spéciale avec contres clairs, adaptés aux couches (ascendant/descendant) | 🟢 |
| 4 | **Capacités de commandant type « pouvoir divin »** (cooldown + coût) (AoM) | Très aimé | OUI (on a déjà capacités + aura) | Capacités de chef **multi-usage à cooldown**, coût en ressource de faction ; identité par faction | 🟡 |
| 5 | **Capture de zone = ressources/bonus** (CoH3/Northgard) | Cœur du genre | OUI (Cristalliseur existe) | Zone capturée → production + bonus atk/def ; couper les approvisionnements ennemis | 🟢 |
| 6 | **Défense en couches + canalisation** (They Are Billions) | Satisfaisant mais peut lasser | OUI pour la forteresse (tu l'as demandé) | Tourelles/sentinelles par faction aux chokepoints ; doser pour éviter la répétition | 🟡 |
| 7 | **Boucle base↔mission avec recherche** (XCOM) | Très apprécié | OUI (cité en cours) | Cité : produire/améliorer/rechercher entre batailles ; chaque expédition compte | 🟢 |
| 8 | **Factions vraiment asymétriques** (transversal) | Préféré à l'équilibrage miroir | OUI | Aquiloris (coordination/cristal-tech) vs Noxéens (abysses/bioluminescence) déjà distincts — accentuer | 🟢 |
| 9 | **Système de couverture dynamique** (CoH3) | Adoré | Partiellement | Version légère : reliefs/épaves = couverture, sans destruction complexe (perf 8 Go RAM) | 🟡 |
| 10 | **Mécanique de campagne unique par faction** (Total War) | Donne l'identité | Plus tard | Un « truc » signature par faction (ex. Noxéens : réseau bioluminescent qui buff en zone) | 🟡 |
| 11 | **Narratif émergent / personnalités** (CK3) | Adoré mais lourd | Hors démo | Éventuellement : rivalités de commandants, en saveur | 🔴 (démo) |
| 12 | **Destruction lourde du décor** (CoH3) | Spectaculaire | Coûteux | Reporté (contrainte VRAM/perf) | 🔴 (démo) |

## PARTIE 4 — Principes de conception tirés des « détestés » (garde-fous)

- **Éviter la micro-hell** : on commande des **groupes** (déjà notre direction Total War), pas des soldats
  individuels ; peu de clics mais **des clics qui comptent**.
- **Éviter le snowball** : la progression (niveaux de bâtiment/grade) doit **enrichir** sans rendre la
  victoire automatique ; garder des contres et des enjeux jusqu'au bout.
- **Éviter la 3D pénible** : verticalité = 3 couches lisibles + terrain, **jamais** un contrôle Z à la souris.
- **Facile à prendre en main, dur à maîtriser** (Stronghold) : la démo doit être **claire pour tout le monde**.

---

## Sources
- [Best Strategy Games of 2025 — TheGamer](https://www.thegamer.com/best-strategy-games-2025-list/)
- [15 Best Strategy Games of 2025 — Strategy & Wargaming](https://strategyandwargaming.com/2025/12/05/the-15-best-strategy-games-of-2025/)
- [Total War: Warhammer III — discussions Steam](https://steamcommunity.com/app/1142710/discussions/0/591758690592272553/)
- [Every Age of Empires Game, Ranked — Game Rant](https://gamerant.com/age-of-empires-ranked-best-worst/)
- [Age of Mythology: Retold — All Gods & Powers — Game Rant](https://gamerant.com/age-of-mythology-retold-all-gods-powers-explained/)
- [Age of Mythology: Retold — Wikipedia](https://en.wikipedia.org/wiki/Age_of_Mythology:_Retold)
- [Homeworld 3 review — PC Gamer](https://www.pcgamer.com/games/rts/homeworld-3-review/)
- [Homeworld 3 review — PCGamesN](https://www.pcgamesn.com/homeworld-3/review)
- [Company of Heroes 3 review — Inverse](https://www.inverse.com/gaming/company-of-heroes-3-review)
- [Company of Heroes 3 — Fandom Wiki](https://companyofheroes.fandom.com/wiki/Company_of_Heroes_3)
- [They Are Billions — Best Base Defense — NerdBurglars](https://nerdburglars.net/gameguides/building-the-best-base-defense/)
- [Best Base Defense Games — Game Rant](https://gamerant.com/best-base-defense-games/)
- [XCOM-like games with management layers — Turn Based Lovers](https://turnbasedlovers.com/lists/best-modern-xcom-like-strategy-games-with-management-layers/)
- [Crusader Kings III emergent narrative — Paste Magazine](https://www.pastemagazine.com/games/crusader-kings-iii/crusader-kings-iii-emergent-narrative)
- [Best Strategy Games Without Micromanagement — Game Rant](https://gamerant.com/best-strategy-games-no-micromanaging/)
- [Late-game micromanagement-hell (essai) — Forums Paradox](https://forum.paradoxplaza.com/forum/threads/essay-on-game-design-ideas-to-fix-poor-game-progression-and-lategame-micromanagement-hell.1603103/)
