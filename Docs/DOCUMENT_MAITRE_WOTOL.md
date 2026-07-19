# WOTOL — DOCUMENT MAÎTRE CONSOLIDÉ

**Version de consolidation : 17 juillet 2026**
**Statut : SOURCE DE VÉRITÉ v0.8.** En cas de conflit avec `CLAUDE.md`, **ce document prévaut**
pour tout ce qui touche au canon (noms, couleurs, tailles, boucle de jeu). `CLAUDE.md` reste
la référence pour les règles techniques de build (UE 5.8, conventions UHT, anti-bugs).

---

## ⚠️ CORRECTIONS DE CANON v0.8 (appliquées au code)

| Élément | Ancien (CLAUDE.md) | **Canon v0.8** | État code |
|---|---|---|---|
| Chef Aquiloris | Aquis / Thalior | **Akis** | ✅ nom affiché |
| Infanterie Aquiloris | Aquiloryons | **Akilorions** | ✅ nom affiché |
| Distance Aquiloris | Aquisphères | **Akisfères** | ✅ nom affiché |
| Montée Aquiloris | Aquilances | **Aquilans** | ✅ nom affiché |
| Spéciale Aquiloris | Aquilombres | **Aquilombre** | ✅ nom affiché |
| Couleur Noxéens | violet/bleu/vert | **VERT bioluminescent uniquement** | ✅ appliqué |
| Couleur violette | (Noxéens) | **Muréniens** (hors démo) | ✅ |
| Taille Léviaphénix | 8 m | **3,5–4 m** (code : 3,8 m) | ✅ |
| Taille Noxedrake | ~4,6 m | **6,5–7 m** (code : 6,75 m) | ✅ |

> Les **IDs internes** du code (`TEXT("Aquis")`, `TEXT("Aquiloryons")`, …) restent inchangés :
> ce sont des clés de dispatch des capacités. Seuls les **noms affichés** (`DisplayName`) suivent le canon.

---

## 1. IDENTITÉ DU PROJET

**Nom officiel : WOTOL — War of the Ocean's Legacy** (graphie « Wotal » interdite).

Jeu de **stratégie-action temps réel sous-marin** : gestion/déplacement d'unités, combats **en volume**
(pas seulement horizontal), exploration, conquête/défense de territoires, construction de bâtiments,
créatures mythiques, **5 factions jouables**. Ampleur d'un Total War + contrôle sous-marin 3D.

**Rendu :** photoréaliste, cinématographique, lisible, coloré sans être enfantin, cohérent bio/archi,
clairement sous-marin. **Modèle éco :** pas de pay-to-win, orientation **pack saisonnier unique**.

---

## 2. PILIERS DE GAMEPLAY

- **Combat en volume** — 3 niveaux principaux (proche du fond / hauteur intermédiaire / partie haute).
  Les unités flottent et se déplacent verticalement. Pas un RTS terrestre sous l'eau.
- **Temps réel** — caméra libre : ZQSD, rotation, zoom, montée/descente, angles multiples.
- **Territoires** — 3 tailles (Petit/Moyen/Grand) × 3 stades (implantation / développement / contrôle avancé).
  Les bâtiments donnent des bonus (attaque, défense).
- **Créatures mythiques** — 1 par faction : silhouette identifiable, anatomie crédible, articulations
  fonctionnelles, attaques sous-marines, échelle cohérente.

---

## 3. LES CINQ FACTIONS

### 3.1 Aquiloris — blanc/bleu, énergie & cristaux bleus, sable, architecture élégante, yeux bleus.
Boucliers = **énergie pure** (pas de cadre métallique). Cité : **Aquilor**. Chef démo : **Akis**.
Mythique : **Léviaphénix** (3,5–4 m).

### 3.2 Noxéens — noir, teintes abyssales, **bioluminescence VERTE**, **AUCUN violet**, aucun cristal bleu,
tentacules/organique, architecture sombre dans les failles. Habitat : **faille entre 2 plaques tectoniques**
(galeries, cavernes, tunnels, pouvant rejoindre les Muréniens). Chef : **Noxar** (petit, trapu, **avec jambes**).
Mythique : **Noxedrake** (6,5–7 m).

### 3.3 Thalassidra *(hors démo)* — corail, formes organiques, barrières. Bâton corail orange/jaune,
monture tortue. Chef ~1,70 m. Mythique : **méduse géante** (nom à fixer).

### 3.4 Muréniens *(hors démo)* — **violet/pourpre**, grottes toxiques. Torse humanoïde + **queue de murène**
(pas de jambes). Chef : **Korvass** (griffes/poison violet). Mythique : **Reine Nuxim** (toujours montrée entière).

### 3.5 Pirates Abyssaux *(hors démo)* — **énergie rouge**, récupération, exilés multi-origines.
1 membre mécanique max par perso. Cité = **vaisseau-fief mobile** (= élément mythique). **Jamais** un bateau de surface.

---

## 4. CIVILISATION ÆTHÉRIENNE
Ancienne civilisation (pas une faction jouable). Ruines **modernes, monumentales, mystérieuses,
triangulaires**, technologiquement différentes. Visibles dans certaines cartes.

---

## 5. UNITÉS GREYBOX

### Aquiloris
| Catégorie | Nom canon | Taille |
|---|---|---|
| Chef | **Akis** | 1,80 m |
| Infanterie | **Akilorions** (boucliers) | 1,75 m |
| Distance | **Akisfères** (canon) | 1,70 m |
| Montée | **Aquilans** | 2 m (monture ~2,5 m) |
| Spéciale | **Aquilombre** (translucide) | humanoïde |
| Mythique | **Léviaphénix** | 3,5–4 m |

### Noxéens
| Catégorie | Nom | Taille |
|---|---|---|
| Chef | **Noxar** (trapu, jambes) | 1,50 m |
| Infanterie | **Nox Flare** | 1,70 m |
| Distance | **Nox Blast** | 1,60 m |
| Montée | **Nox Beast** (quadrupède 4 membres, massif) | 2,50 m |
| Spéciale | **Noxeon** | 1,80 m |
| Mythique | **Noxedrake** | 6,5–7 m |

> Terme officiel : **unité spéciale**, jamais « Joker ».

---

## 6. CONVENTIONS GREYBOX
Chef = rectangle · Infanterie = triangle · Montée = trapèze · Distance = cylindre ·
Spéciale = pentagone · Mythique = cercle. Couleurs = factions. Respecter les tailles réelles.

---

## 7. BOUCLE DE GAMEPLAY DE LA DÉMO (13 PHASES — validation la plus récente)

1. **Configuration** — choix de faction, difficulté et réglages ; rien ne démarre avant le clic
   explicite du joueur sur **« Lancer la partie »**.
2. **Chargement / introduction** — visuel animé, lore de la faction choisie et contexte : une
   expédition vient de découvrir une nouvelle zone.
3. **Exploration en troisième personne** — nage libre 3D, tutoriel ZQSD/souris/rotation/hauteur.
4. **Approche du Kraken** — à ~5–10 m, chargement puis phase de placement de l'armée.
5. **Bataille tutorielle contre le Kraken** — catégories, ordres, verticalité et créature active.
6. **Rapport de victoire** — pertes/statistiques + récompenses séparées : cristaux, matériaux
   abyssaux, biomasse et nourriture ; le joueur clique pour continuer.
7. **Retour en nage libre / conquête** — nouvel objectif modal ; choisir le Cristalliseur dans
   l'inventaire de bâtiments et le poser sur l'emplacement central signalé. Coût réellement retiré.
8. **Territoire acquis** — terraformation au biome de faction, bonus attaque/défense et capacité
   d'armée ; Cœur-Éclat puis œuf/juvénile mythique récupérés.
9. **Retour cité volontaire** — bouton dédié ; vue isométrique fixe, bâtiments visibles et cliquables.
10. **Développement de la cité** — construire le bâtiment des unités à distance (Akisfères pour
    Aquiloris), payer son coût puis produire 10 unités ; limite d'armée de démo visée : 35.
11. **Alerte noxéenne** — la faction rivale se dirige vers la zone nouvellement conquise ; le joueur
    décide de partir défendre le Cristalliseur.
12. **Défense / défaite** — bâtiment réparable et bonus tant qu'actif ; destruction = échec immédiat,
    territoire neutre, aucun bâtiment noxéen posé en v0.8.
13. **Fin de démo** — retour cité, message de conclusion et sortie propre.

> **Règle transversale :** aucune transition automatique quand elle nuit à la caméra/compréhension/rythme.
> Utiliser fenêtres d'objectif, boutons cliquables, messages victoire/échec, validation manuelle,
> repositionnement contrôlé de la caméra. → Implémenté : **fenêtre d'objectif modale** (subsystem/HUD/PC).

---

## 8. BÂTIMENTS DE TERRITOIRE
- **Aquiloris — Cristalliseur** : purification, sécurisation, production de cristaux, bonus atk/def,
  niveaux, PV, réparation. (Nom fonctionnel, identité lore à affiner.)
- **Noxéens — Abyssaliseur** (provisoire) : **PAS** dans la séquence de défaite v0.8.

---

## 9. IA
- **Unités** : temps réel, choix de cibles, exploite la profondeur, évite obstacles, jamais statique,
  utilise les capacités, respecte les rôles.
- **Créature** : attaque directe, déplacement agressif, attaque de zone, cible isolés, changement de
  niveau vertical, phase de rage. Calibrage : défaite si inaction, victoire si mécaniques comprises.

---

## 10. CARTE DU MONDE
Carte sous-marine interactive : déplacement libre, vue dessus/iso, rotation, vue latérale, profondeur,
territoires distincts. **2 représentations** : (1) surface/reliefs iso ; (2) sous-surface radiographique
(galeries, tunnels, failles, réseaux Noxéens/Muréniens). Territoires **dispersés** (pas 4 coins + centre vide),
plus d'espace Thalassidra↔Noxéens, connexion souterraine Noxéens↔Muréniens. Pirates = pas de cité fixe.

---

## 11. ENVIRONNEMENTS DE BATAILLE
4 cartes retenues (au lieu de 6). Relief, dénivelés, arches (~4–5 m = repère d'échelle), rochers, obstacles,
fonds visibles, éléments animés (poissons, requins nourrices, bulles, algues, particules), structures/ruines.
Le fond ne doit jamais être un vide noir. **À corriger** : filtre trop sombre, plateau trop plat, collisions
rochers, horizon insuffisant, manque de relief/couleurs, environnements vides.

---

## 12. ANIMATION & RIG
Coudes, poignets, genoux, chevilles, colonne, flexion du tronc, articulations de monture, **4 membres
fonctionnels pour la Nox Beast**. Animations tenant compte de la **flottabilité** (pas une marche terrestre).

---

## 13. LOGO & ICÔNE
- **Icône lanceur** : bulle d'eau + **W en cristal bleu** entièrement visible, sans nom autour ; export `.ico` multi-résolution.
- **Logo** : chaque lettre a une identité de faction. 1er O (entre W et T) = **Noxéens** (cercle, bioluminescence
  verte au milieu du contour, tentacules noires). 2e O = **Pirates** (bulle d'eau, rouge). Jamais toutes les lettres identiques.

---

## 14. TECHNOLOGIE
UE5 (5.8, migration 18/07/2026), **C++**, caméra libre, ZQSD, nage en volume, déplacement vertical, batailles temps réel,
transitions monde ouvert ↔ combat, données d'unités structurées. Tableur Excel = référence chiffrée.
**Build autonome** (exécutable indépendant, sans UE sur la machine du joueur).

---

## 15. INDEX DES VISUELS DE RÉFÉRENCE (`Content/UI/Reference/`)

| Fichier | Contenu | Usage prévu |
|---|---|---|
| `Logo_WOTOL.png` | Logo principal (lettres par faction) | Menu principal |
| `Cite_Aquiloris.png` | Cité Aquilor (cristal bleu) | Vue cité (phases 2 & 9) |
| `Faille_Noxeens.png` | Faille Noxéenne (verte) | Ambiance / cité Noxéenne |
| `CarteMonde_Surface.png` | Carte surface iso | Écran carte |
| `CarteMonde_Souterraine.png` | Carte Niveau 3 (galeries) | Écran carte (calque sous-sol) |
| `CarteMonde_UI.png` | UI carte interactive (calques/filtres) | Maquette écran carte |
| `Biomes_Aerien.png` | Vue aérienne des biomes | Ambiance |
| `Icones_Roles.png` | 6 boucliers de rôle | Icônes HUD par catégorie |
| `Embleme_Noxeens.png` | Emblème « N » vert | UI faction Noxéens |
| `Embleme_Aquiloris.png` | Emblème « A » bleu | UI faction Aquiloris |

> Images de **référence/concept** (générées) — pas nécessairement des assets finaux cookés.

---

## 16. CONTRAINTES MATÉRIELLES (à respecter dans le code)

Machine réelle de test : **Lenovo Legion, Intel i5-7300, 8 Go RAM** (→ 16/24 Go envisagé), SSD 1 To,
Windows 10 Home, **NVIDIA GTX** (pilote 472.84). Erreur **« Video memory exhausted »** déjà rencontrée.

**Conséquences imposées au projet :**
- optimisation des textures ; limitation des meshes lourds ; **niveaux de détail (LOD)** ;
- éclairage maîtrisé ; réduction des assets inutiles ;
- **prudence avec Nanite, Lumen et les très hautes résolutions** ;
- **tester sur la machine réellement utilisée**.

> Impact greybox : garder les primitives légères, éviter d'importer les 10 PNG de référence
> (~3 Mo chacun) comme textures runtime pleine résolution ; les downscaler si utilisés en UI.

---

## 17. RÈGLES DE COHÉRENCE NON NÉGOCIABLES (§23 du doc)

1. Sous l'eau : **aucun bateau de surface** comme élément pirate principal.
2. Noxéens = **bioluminescence verte**, pas violette.
3. Violet = **Muréniens**.
4. Muréniens = **queue**, pas de jambes.
5. **Noxar a des jambes**.
6. Aquiloris = blanc/bleu/énergie bleue.
7. **Aquilombre translucide**.
8. **Nuxim** montrée entièrement.
9. Vaisseau pirate = cité mobile + fief mythique.
10. Une unité demandée séparément = **image séparée**.
11. Une correction locale **ne modifie pas** les éléments déjà validés.
12. Tailles d'unités **respectées**.
13. Terme officiel : **« unité spéciale »**.
14. Noms validés : **Leviaphenix**, **Noxedrake**.
15. Les phases **ne s'enchaînent pas** automatiquement sans interface.
16. Destruction du bâtiment défendu = **défaite immédiate** (v0.8).
17. Après défaite défensive : territoire **neutre**.
18. Noxéen **ne pose pas** son bâtiment après cette défaite (v0.8).
19. Monde **coloré et lisible** même dans les abysses.
20. Reliefs/failles/galeries/niveaux verticaux = essentiels au level design.

---

## 18. HIÉRARCHIE DES SOURCES (§25 du doc)

1. La **validation explicite la plus récente de Liamor** prévaut.
2. Décision de gameplay validée > ancien concept.
3. Tableur de données le plus récent > valeurs d'anciens messages.
4. Images validées = références visuelles.
5. Une nouvelle génération n'écrase pas silencieusement une décision antérieure.
6. Toute modif importante : identifiée, vérifiée, justifiée.
7. Anciennes appellations = **historique uniquement**, pas réintroduites en production.

---

## 19. OBJECTIF DE PRODUCTION IMMÉDIAT (§26 du doc) — PRIORITÉ

Transformer la greybox en **démo stable et présentable** (cible **Pictanovo, 1er sept. 2026**) :
boucle complète fonctionnelle, sans blocage, caméra contrôlable, objectifs lisibles, tuto → cité →
exploration → créature → Cristalliseur → œuf → défense → victoire/défaite, **build autonome**,
compat UE 5.8 vérifiée, code relu.

**Ne pas s'éparpiller** dans toutes les factions / la carte mondiale / les assets finaux avant que
la boucle soit stable. La démo doit prouver : combat en volume, lisibilité des unités, caméra
agréable, conquête compréhensible, enjeu de défense, identité visuelle/stratégique distincte.

### État d'implémentation de la boucle cité/défense (19/07/2026)
- Pose du Cristalliseur/Abyssalyseur : inventaire, emplacement 3D signalé, validation spatiale,
  paiement atomique et conservation du même acteur jusqu'à la défense.
- Cité : construction du bâtiment de distance sur 3 parcelles, puis production obligatoire de
  10 Akisfères/Nox Blast ; compteur d'objectif et plafond d'armée de démonstration à 35.
- Protection anti-blocage : les places d'armée et les cristaux nécessaires à l'objectif restent
  réservés tant que les 10 unités ne sont pas produites.
- La 10e unité ouvre l'alerte de contre-attaque noxéenne ; le départ en défense reste un clic
  explicite. Les valeurs de coûts/récompenses utilisées sont des paramètres greybox provisoires.

---

## 20. POINTS ENCORE OUVERTS (§24 — NE PAS VERROUILLER)

Nom lore du Cristalliseur · nom équivalent noxéen · rôle de Thalior vs Akis · nom mythique Thalassidra ·
nom/définition du Kraken · les 4 cartes de bataille · ordre Cœur-Éclat ↔ Cristalliseur · contenu du craft
post-bataille 1 · valeurs des 3 niveaux territoriaux · bonus de chaque bâtiment · graphie « 3DÉCORS » ·
relations juridiques · accès GitHub par outil · version finale du logo · intégration des 5 drapeaux ·
cités des autres factions · plan de production matériel · frontière exacte démo 0.8 / jeu complet.

> **Claude ne doit pas inventer** ces valeurs ; attendre une décision explicite de Liamor.

---

## 21. WORKFLOW COLLABORATIF (§19 du doc)

Mutualisation ChatGPT ↔ Claude Code via **GitHub comme intermédiaire** (les deux IA ne communiquent
pas directement). Claude développe et pousse → ChatGPT relit, signale les incompat UE 5.8, corrige
bugs/oublis → chaque itération vérifiée des deux côtés. Dépôt : `github.com/liamor95/Wotol`.
→ Corollaire code : **commits clairs et bien décrits** (déjà appliqué), pour faciliter la relecture externe.
