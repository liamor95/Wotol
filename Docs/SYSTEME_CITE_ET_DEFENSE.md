# WOTOL — Système de CITÉ, PROGRESSION et DÉFENSE

Direction validée par Liamor (session du 17/07/2026). **Étend** le document maître.
Ce n'est **pas** un simple recrutement façon Bannerlord : c'est un vrai système de gestion de
faction/cité, avec progression des bâtiments, arbres de compétences, et **défense de forteresse
jouable**. Mise en place **progressive** (après stabilisation de la boucle démo 0.8).

Jeux de référence (patterns STR éprouvés) : **Age of Empires / Age of Mythology** (bâtiment →
âge/niveau → améliore les unités), **Stronghold** (défense de château en temps réel),
**They Are Billions** (défenses en couches contre des vagues), **Total War** (batailles de siège =
défense de forteresse jouable). *(V Rising a un système proche château + défenses + raids, cité à
titre indicatif — non demandé par Liamor.)*

---

## 1. UN BÂTIMENT PAR TYPE D'UNITÉ (progression indépendante)

Chaque type d'unité a **son propre bâtiment**, amélioré **indépendamment** des autres.

| Faction | Type d'unité | Bâtiment producteur |
|---|---|---|
| Aquiloris | Infanterie (Aquiloryons) | Académie Aquiloryon |
| Aquiloris | Montée (Aquilans) | Dôme des Aquilans |
| Aquiloris | Distance (Aquisfères) | Champ de Tir des Aquisfères |
| Aquiloris | Spéciale (Aquilombre) | Nexus des Ombres |
| Aquiloris | Mythique (Léviaphénix) | Cœur-Éclat du Léviaphénix |
| Noxéens | (équivalents) | Nid / Fosse / Antre / Sanctuaire / Couvain |

**Règle clé : le NIVEAU du bâtiment = le NIVEAU des unités qu'il produit.**
- Améliorer le bâtiment (coût en ressources) → les unités de ce type sortent à un niveau plus élevé
  (plus de PV / dégâts), **sans toucher** aux autres types (progression indépendante).
- Référence : Age of Empires (forge/âge améliore une branche d'unités à la fois).

---

## 2. GRADES & COMPÉTENCES (2ᵉ onglet — plus tard)

Les bâtiments, en montant de niveau, **débloquent** :
- des **grades** (paliers de puissance de l'unité) ;
- l'accès à l'**onglet Compétences / arbre de compétences** (2ᵉ fenêtre de cité).

### Onglet Compétences (arbre)
C'est là qu'on choisit la **VOIE (axe)** de chaque type d'unité. Correspond aux **Axe 1 / Axe 2**
déjà décrits dans le GDD pour chaque unité.

- Exemple : au lieu d'« Aquiloryons » génériques, on obtient **« Aquiloryons — axe offensif »**
  (Double Lames) OU **« Aquiloryons — axe défensif »** (Mur amplifié).
- **Un axe par type d'unité** à la fois (choix stratégique). Change l'identité tactique de tout
  le groupe de ce type.
- S'applique à **toutes** les unités (chaque unité a ses 2 axes dans le GDD §7).

> Statut : l'onglet arbre arrive **plus tard** (Liamor enverra le détail). On prépare le
> modèle de données (axe sélectionné par type d'unité) dès maintenant.

---

## 3. BÂTIMENT RECHERCHE / MILITAIRE / RESSOURCES

Comme dans tout STR, un bâtiment central de **recherche + militaire + ressources** :
- gère la **production de ressources** globale (cristaux Aquiloris / biolumens Noxéens) ;
- débloque les **recherches** (améliorations globales, accès aux défenses) ;
- centralise les **améliorations militaires**.

Référence : Age of Empires (centre-ville + université), V Rising (autel/établi de recherche).
(Aquiloris : « Atelier des Courants » / « Puits des Courants Cristallins » existent déjà au GDD §8.)

---

## 4. DÉFENSE DE FORTERESSE (raids) — JOUABLE EN BATAILLE RÉELLE

**Système essentiel, jamais formalisé avant.** Quand la cité ou une zone conquise est attaquée
(raid ennemi), la **défense se joue en bataille temps réel** (pas un calcul automatique).

- Référence : **Stronghold** / **They Are Billions** (défendre sa base en temps réel),
  **Total War** (bataille de siège), **V Rising** (raids de château).
- La carte de bataille = **la cité/zone elle-même**, avec ses bâtiments et ses défenses posées.
- Objectif : **protéger le bâtiment central** (Cristalliseur/Abyssalyseur) + les structures.
- Perte du bâtiment central = échec (déjà en place, règle §16).

> En démo 0.8 : la « défense du Cristalliseur » (phase 10) est la **première brique** de ce
> système. Le jeu complet l'étend à la cité entière avec de vraies défenses.

---

## 5. STRUCTURES DE DÉFENSE (à débloquer, propres à chaque faction)

Le Cristalliseur seul **ne suffit pas** à défendre une zone. Il faut de **vraies défenses**,
débloquées et améliorées via le bâtiment recherche/militaire, **propres à chaque faction**.

| Faction | Défenses (pistes) |
|---|---|
| **Aquiloris** | Tourelles cristal, **méga-tourelles hydrosphère**, tourelles laser à énergie de cristaux |
| **Noxéens** | **Sentinelles Noxeblast** postées autour de la cité, zones-pièges à Abyssaliseur, tourelles bioluminescentes |
| Thalassidra | (à définir — corail défensif / barrières vivantes) |
| Muréniens | (à définir — pièges toxiques / gardiens de galerie) |
| Pirates Abyssaux | (à définir — défenses mobiles autour du vaisseau-fief) |

Caractéristiques communes (référence tower-defense / They Are Billions) :
- posées à des **emplacements** autour du bâtiment central / points d'accès ;
- ont des **PV**, une **portée**, un **type d'attaque** ; peuvent être **améliorées** ;
- débloquées par **recherche** (progression) ;
- **identité de faction** forte (une tourelle Aquiloris ≠ une sentinelle Noxéenne).

### Implémentation jouable de la démo

- après la victoire de défense, les dégâts du Cristalliseur/Abyssalyseur persistent ;
- la réparation consomme des cristaux et des matériaux abyssaux, proportionnellement aux PV manquants ;
- cinq emplacements spatiaux sont proposés autour du bâtiment ; au grade 1, une seule défense
  autonome peut être construite ;
- les défenses utilisent l'identité visuelle de la faction et leurs statistiques progressent avec
  leur niveau technologique ;
- une garnison facultative par type d'unité est limitée par le grade du bâtiment et retirée du
  plafond de l'armée de campagne ;
- en cas d'échec avec bâtiment survivant, le joueur peut rejouer immédiatement ou retourner en
  cité, améliorer son armée et revenir dans une fenêtre de 20 minutes ; à expiration, la zone
  redevient neutre. Si le bâtiment est détruit, la neutralisation est immédiate.

Les coûts restent des variables éditables de démo, pas un équilibrage économique définitif.

---

## 6. ORDRE DE MISE EN PLACE (ne pas tout faire d'un coup — §26)

1. **Démo 0.8** : cité, défense, réparation, première tourelle et garnison. ✅ jouable en C++.
2. **Niveaux de bâtiment → niveau d'unité** (progression indépendante). ← prochaine brique data.
3. **Bâtiment recherche/militaire/ressources**.
4. **Structures de défense** (tourelles Aquiloris / sentinelles Noxéennes) + emplacements. ✅ première brique.
5. **Défense de forteresse** sur la carte-cité complète (raids jouables).
6. **Onglet Compétences / arbre** (choix d'axe par unité) — quand Liamor envoie le détail.

> Priorité absolue : la **boucle démo 0.8 stable** d'abord (§26 du document maître).
> Ces systèmes s'ajoutent **par-dessus**, sans casser l'existant.
