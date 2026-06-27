# CHAUVE QUI PEUT — Game Design Document

**Version** : 2.0  
**Statut** : En développement actif  
**Type** : Survival post-apocalyptique humoristique solo/local  
**Plateforme cible** : PC (Windows / Linux / Mac)  
**Moteur envisagé** : Godot 4 ou Unity (à confirmer)  

---

## AVERTISSEMENT ÉDITORIAL

CHAUVE QUI PEUT est un projet **indépendant de WOTOL**. Ces deux univers ne se croisent pas, ne partagent pas de lore, de personnages, de mécaniques, d'assets, de code ou de direction artistique. Ce document ne concerne que CHAUVE QUI PEUT.

---

## TABLE DES MATIÈRES

1. Vision globale
2. Concept central
3. La Forteresse des Chevelus
4. Les PNJ IA de la Forteresse
5. La boucle de gameplay depuis la Forteresse
6. L'exploration des zones extérieures
7. La quête principale : le Remède
8. Les objectifs secondaires
9. L'expansion du territoire
10. La neutralisation des Chauves
11. La persistance des Chauves neutralisés
12. Les degrés de Chauvité
13. Les catégories de Chauves
14. Le système de contamination
15. Le système de bannissement
16. La bascule de camp
17. Phase 2 : le joueur Banni
18. La récupération de mèches de cheveux
19. L'implantation visible des mèches
20. Le patchwork capillaire
21. La crédibilité capillaire
22. Les réactions des PNJ aux cheveux volés
23. Infiltration et retour à la Forteresse
24. Les deux grandes phases du jeu
25. Mécaniques inspirées de Project Zomboid
26. Mécaniques propres à CHAUVE QUI PEUT
27. Direction artistique recommandée
28. Option Semi-Réaliste vs Option Stylisée
29. Version prototype faisable
30. Risques de conception
31. Ce qu'il faut absolument éviter

---

## 1. VISION GLOBALE

### Pitch en une phrase

Un survivant au sommet de sa forme dans une forteresse de chevelus doit explorer un monde post-apocalyptique contaminé par un virus qui rend chauve — et qui, en le rendant lui-même chauve, le transforme progressivement en ennemi de ses propres alliés.

### Positionnement

CHAUVE QUI PEUT se positionne comme un survival systémique inspiré dans son esprit par **Project Zomboid** — pas une copie, mais une reprise de l'approche : monde persistant, gestion des besoins, exploration progressive, construction de base, dangers permanents — appliquée à un univers entièrement original, post-apocalyptique et humoristique.

Ce qui distingue CHAUVE QUI PEUT de tous les autres survivals :

- Le joueur commence fort, pas faible. L'enjeu est de **conserver**, pas de **progresser**.
- La menace principale n'est pas la mort, c'est la **transformation**.
- Les ennemis ne sont pas des monstres, ce sont des **humains malades**.
- L'exclusion sociale est un mécanisme de jeu central, pas une cinématique.
- La chevelure est une ressource stratégique, une monnaie sociale, et une armure.

### Ton

Humoristique, absurde, grinçant. Le jeu rit de la panique sociale face à la calvitie tout en construisant un vrai système de survie sous-jacent. Le décalage entre le sérieux du danger et le ridicule du problème (des cheveux) est la signature émotionnelle du jeu.

---

## 2. CONCEPT CENTRAL

### Le virus folliculaire

Dans ce monde, une contamination inconnue s'est répandue. Ce n'est pas un virus bactériologique classique. C'est un **pathogène folliculaire** — un agent qui s'attaque aux follicules pileux et, via un mécanisme encore inexpliqué, corrode progressivement les fonctions cognitives, motrices et sociales de ses victimes.

**Plus une personne perd ses cheveux, plus elle perd :**
- ses compétences techniques ;
- sa coordination motrice ;
- sa mémoire à court terme ;
- son jugement ;
- son empathie ;
- son contrôle des impulsions ;
- son humanité au sens large.

Les Chauves complets ne sont pas des morts-vivants. Ce sont des humains régressés, désocialisés, agressifs et instinctifs — mais ils étaient des êtres humains. Cette nuance est essentielle au jeu, à son ton et à sa mécanique de neutralisation.

### L'inversion RPG

CHAUVE QUI PEUT inverse la courbe classique du RPG et du survival.

| Survival classique | CHAUVE QUI PEUT |
|---|---|
| Le joueur commence faible | Le joueur commence fort |
| Le joueur monte en puissance | Le joueur essaie de ne pas perdre sa puissance |
| Les ennemis sont des obstacles | Les ennemis étaient des gens |
| La mort est la fin du jeu | La calvitie mène à l'exclusion sociale, puis à la transformation |
| La base se construit | La base existe déjà |

Ce renversement est le socle de tout le game design. Chaque décision de gameplay doit être calibrée autour de cette idée : **le joueur régresse, et cette régression a des conséquences systémiques**.

---

## 3. LA FORTERESSE DES CHEVELUS

### Présentation

La Forteresse est le point de départ du jeu. C'est une grande installation sécurisée, occupée exclusivement par des survivants qui ont conservé leurs cheveux — les **Chevelus**. Elle est à la fois :

- la **base principale** du joueur ;
- son **refuge** contre les Chauves ;
- son **hub social** avec les PNJ ;
- sa **zone de craft** et d'artisanat ;
- sa **zone de soin** (blessures, contamination) ;
- sa **zone de stockage** (ressources, objets, armes) ;
- le **point de départ** de toutes les expéditions ;
- le **symbole culturel** de la résistance chevelue.

### Structure de la Forteresse au démarrage

La Forteresse est déjà construite et fonctionnelle au lancement de la partie. Elle comprend :

| Zone | Description |
|---|---|
| **Grand mur d'enceinte** | Murs renforcés, barricades, postes de guet |
| **Portail principal** | Entrée sécurisée avec gardes IA, code ou badge |
| **Place centrale** | Hub de rencontre, point de distribution, zone sociale |
| **Atelier** | Fabrication d'objets, réparation, craft avancé |
| **Infirmerie** | Soins des blessures, traitement anti-contamination |
| **Station capillaire** | Soins des cheveux, produits anti-chute, traitements folliculaires |
| **Entrepôt principal** | Stockage de nourriture, eau, matériaux, objets |
| **Armurerie** | Armes, munitions, équipements de protection |
| **Dortoirs** | Repos, sommeil, récupération de fatigue |
| **Cuisine/Cantine** | Nourriture, eau potable, besoins physiologiques |
| **Poste de commandement** | Décisions stratégiques, carte de territoire, organisation |

### Position sociale du joueur au départ

Le joueur commence avec :
- un accès complet à toutes les zones de la Forteresse ;
- un niveau de compétences élevé dans plusieurs domaines ;
- des armes et équipements de qualité ;
- la confiance des PNJ IA dirigeants ;
- un statut reconnu (explorateur senior, éclaireur principal, ou autre) ;
- accès aux missions d'exploration ;
- accès aux ateliers et aux soins.

Ce n'est pas un personnage générique. Il a une position réelle dans la communauté des Chevelus.

### Le joueur ne peut pas rester sans rien faire

La Forteresse est confortable, mais elle ne peut pas fonctionner indéfiniment sans apport extérieur. Des mécaniques de pression poussent le joueur à sortir régulièrement :

- Les **ressources se consomment** (nourriture, eau, médicaments, matériaux) ;
- Des **quêtes urgentes** apparaissent régulièrement ;
- La **progression de la quête principale** exige des expéditions ;
- Des **survivants signalent des zones critiques** à explorer ;
- Des **Chauves commencent à approcher** les murs — la pression augmente ;
- Des **pannes** surviennent (générateur, murs, réserves) — il faut des pièces extérieures ;
- Des **événements aléatoires** obligent à agir (groupe de Chauves en approche, signal de détresse, etc.).

---

## 4. LES PNJ IA DE LA FORTERESSE

### Principe

Les survivants de la Forteresse sont contrôlés par l'IA. Ils ne sont pas des décors. Ce sont des agents actifs qui participent à la vie quotidienne de la base.

Chaque PNJ a :
- un **rôle** (voir ci-dessous) ;
- un **niveau de compétences** propre ;
- une **chevelure** (type, longueur, couleur) visible dans le jeu ;
- une **relation** avec le joueur (confiance, méfiance, gratitude, hostilité) ;
- une **routine quotidienne** (travail, patrouille, repos, social) ;
- des **dialogues contextuels** selon la situation et le niveau de cheveux du joueur.

### Rôles des PNJ IA

| Rôle | Comportement IA |
|---|---|
| **Garde** | Patrouille les murs et les accès, alerte en cas d'intrusion |
| **Ingénieur** | Répare les structures endommagées, améliore les installations |
| **Médecin** | Soigne les blessures, diagnostique la contamination, gère l'infirmerie |
| **Coiffeur capillologue** | Gère la station capillaire, administre les traitements anti-chute |
| **Artisan** | Craft d'objets, fabrication d'armes, réparation d'équipements |
| **Cuisinier** | Gère les ressources alimentaires, prépare nourriture et eau |
| **Éclaireur** | Surveille les zones extérieures, rapporte les mouvements de Chauves |
| **Stratège** | Planifie les expéditions, gère la carte, organise les défenses |
| **Agriculteur** | Gère les potagers, cultures, ressources renouvelables |
| **Trafiquant** | Commerce de ressources rares, échanges, économie interne |
| **Dirigeant** | Prend les grandes décisions, annonce les bannissements |
| **Mécano** | Entretient les véhicules, le générateur, les machines |

### Comportement IA des PNJ

Les PNJ IA doivent être crédibles, pas omniscients.

- Ils **réagissent à l'état du joueur** : si ses cheveux diminuent, leur comportement change progressivement (distance sociale, méfiance, refus d'accès) ;
- Ils **prennent des initiatives autonomes** : réparer un mur sans qu'on le demande, alerter en cas d'anomalie, stocker des ressources ;
- Ils **peuvent échouer** dans leurs tâches si les ressources manquent ou si un danger les en empêche ;
- Ils **mémorisent les actions du joueur** : si le joueur les a aidés, leur relation est meilleure ; si le joueur a fait des erreurs, ils peuvent s'en souvenir ;
- Ils **discutent entre eux** et peuvent influencer l'opinion collective de la Forteresse sur le joueur ;
- Ils **réagissent visuellement aux mèches récupérées** (voir section 22).

### L'Assemblée des Chevelus

Les dirigeants de la Forteresse constituent un conseil informel. Ce sont eux qui décident du bannissement. Ils ont une autorité collective sur les accès et les droits. Le joueur peut influencer ce conseil via :
- ses actions en expédition (succès/échec) ;
- ses ressources rapportées ;
- ses relations avec les membres ;
- son niveau de cheveux (le plus décisif).

---

## 5. LA BOUCLE DE GAMEPLAY DEPUIS LA FORTERESSE

### Cycle quotidien (Phase 1 : Chevelu)

```
MATIN
├── Vérifier état du joueur : cheveux, santé, fatigue, besoins
├── Vérifier état de la Forteresse : ressources, murs, PNJ
├── Consulter les missions disponibles (principale + secondaires)
└── Préparer l'expédition : équipement, soins préventifs, plan de route

JOURNÉE (EXPÉDITION)
├── Sortir de la Forteresse (portail)
├── Explorer la zone cible
├── Looter les conteneurs et bâtiments
├── Neutraliser ou éviter les Chauves
├── Gérer les imprévus (embuscades, zones inconnues, dangers)
└── Revenir à la Forteresse avant la nuit ou avant épuisement

SOIR / RETOUR
├── Rentrer dans la Forteresse (contrôle capillaire à l'entrée)
├── Déposer les ressources à l'entrepôt
├── Soins si nécessaire (blessures, contamination)
├── Traitement capillaire si nécessaire
├── Craft ou amélioration d'équipement
├── Interactions sociales avec les PNJ
└── Repos / sommeil (récupération de fatigue)
```

### Tension permanente

Chaque sortie présente un risque de contamination. Le joueur doit :
- **planifier** son équipement protecteur avant de sortir ;
- **gérer** l'exposition pendant l'exploration ;
- **traiter** rapidement tout contact contaminant après le retour ;
- **ne pas dépasser** son niveau de résistance actuel.

Plus il explore des zones dangereuses, plus le risque monte. La tentation de rester dans une zone un peu plus longtemps pour trouver ce médicament rare peut coûter des cheveux.

---

## 6. L'EXPLORATION DES ZONES EXTÉRIEURES

### Architecture de la carte

La carte est divisée en **zones** découvertes progressivement. Chaque zone a :
- un **niveau de danger** (densité de Chauves, type d'ennemis, pièges) ;
- un **niveau de contamination ambiante** (zones où l'air lui-même est risqué) ;
- des **lieux remarquables** à fouiller pièce par pièce ;
- des **ressources spécifiques** selon le lieu ;
- une **mémoire d'état** (les Chauves neutralisés persistent, les lieux fouillés sont mémorisés).

### Types de lieux

| Lieu | Ressources typiques | Dangers |
|---|---|---|
| **Pharmacie** | Médicaments, produits capillaires, seringues | Chauves en attente, contamination chimique |
| **Salon de coiffure** | Shampoing, conditionneur, mèches, perruques | Chauves coiffeurs (archétype spécial) |
| **Laboratoire** | Sérums, composants de remède, données scientifiques | Zone fortement contaminée, Chauves mutés |
| **Centre commercial** | Nourriture, vêtements, outils, matériaux | Grande zone, nombreux Chauves, difficile à sécuriser |
| **Pharmacie industrielle** | Gros stocks de médicaments, équipements médicaux | Gardée par des Chauves organisés |
| **Station-service** | Carburant, pièces mécaniques, véhicules | Risque d'incendie, Chauves rampants |
| **Usine** | Matériaux de construction, outils, pièces | Bruyante (attire les Chauves), zones effondrées |
| **Bunker** | Données, équipements militaires, longue durée de vie | Fermé, nécessite clés/codes, Chauves piégés dedans |
| **Hôpital** | Matériel médical, traitements avancés | Très contaminé, Chauves anciens patients |
| **Poste électrique** | Pièces de générateur, câblage | Danger électrique, Chauves attirés par les sons |
| **École** | Documents, cartes, ressources alimentaires | Émotion narrative, Chauves anciens enfants |
| **Poste avancé abandonné** | Équipements de survie, munitions | Piégé par Chauves organisés |

### Navigation et carte

Le joueur dispose d'une **carte partielle** au début. Les zones non explorées sont dans le brouillard. Explorer une zone la révèle sur la carte. Il peut :
- **marquer des points d'intérêt** ;
- **noter des dangers** ;
- **tracer une route** vers un objectif ;
- **partager des informations** avec les PNJ de la Forteresse (ce qui débloque des missions secondaires).

### Importance du bruit

Comme dans Project Zomboid, le bruit est un facteur critique. Les Chauves réagissent aux sons.
- Courir fait du bruit ;
- Briser une vitre attire les Chauves proches ;
- Tirer avec une arme à feu attire tous les Chauves du quartier ;
- Certains Chauves (le Hurleur) créent des bruits qui attirent les autres ;
- Marcher lentement, accroupi, réduit les sons.

---

## 7. LA QUÊTE PRINCIPALE : LE REMÈDE

### Arc narratif général

La grande quête du jeu est de **trouver un remède contre le virus folliculaire**. Cet arc narratif en plusieurs actes structure la progression de la carte et des découvertes.

### Acte 1 — Exploration initiale (Phase Chevelu)

Le joueur commence à explorer les zones proches de la Forteresse. Il cherche :
- des **informations sur l'origine du virus** ;
- des **témoignages de survivants** sur l'évolution de la contamination ;
- des **premiers indices sur un remède potentiel** ;
- des ressources pour maintenir la Forteresse.

Il découvre que le virus n'est pas naturel — ou du moins pas entièrement. Des **données de laboratoire** éparpillées dans la ville suggèrent une origine pharmaceutique, industrielle ou militaire.

### Acte 2 — Complication (Début de contamination possible)

Le joueur s'approche de zones plus dangereuses. Les risques augmentent. Il peut être contaminé. S'il gère correctement sa contamination, il reste dans la Forteresse. S'il échoue, la bascule de camp est déclenchée.

Les indices se précisent : un laboratoire dans une zone reculée, une **formule partielle de sérum**, des données sur un traitement expérimental appelé **Follicure-X**.

### Acte 3 — Zone de rupture (Bannissement potentiel)

Si le joueur est banni, sa façon de chercher le remède change radicalement. Il ne peut plus compter sur l'infrastructure des Chevelus. Il doit soit :
- **s'infiltrer** dans des zones de recherche depuis l'extérieur ;
- **trouver des alliés chauves** qui ont conservé un semblant de lucidité ;
- **découvrir une vérité sur les Chauves** que les Chevelus ne veulent pas entendre.

### Acte 4 — Révélation

Le remède existe. Mais son développement a une implication : le Follicure-X ne **rend pas les cheveux**, il **arrête la régression cognitive**. Les Chauves traités ne redeviennent pas Chevelus. Ils restent chauves, mais retrouvent leur lucidité. Ce qui soulève une question narrative : **qui est l'ennemi, finalement ?**

### Acte 5 — Résolution (fins multiples possibles)

- **Fin A** : Le joueur trouve le remède, revient à la Forteresse, prouve son utilité. Les Chevelus acceptent d'utiliser le Follicure-X sur les Chauves stabilisés. Réconciliation partielle.
- **Fin B** : Le joueur, devenu Chauve traité au Follicure-X, crée une troisième faction : les **Stabilisés** — chauves lucides, rejetés par les deux camps.
- **Fin C** : Le joueur infiltre et détruit la source du virus, mais la Forteresse chevelue est déjà trop intégriste pour accepter une paix. Guerre civile.

---

## 8. LES OBJECTIFS SECONDAIRES

Les objectifs secondaires nourrissent la boucle de gameplay quotidienne et récompensent l'exploration.

### Catégories d'objectifs secondaires

**Ravitaillement**
- Fouiller la pharmacie du quartier ouest
- Récupérer 3 flacons de shampoing anti-contamination
- Rapporter des antibiotiques à l'infirmerie
- Trouver des rations alimentaires pour les 5 prochains jours

**Ressources techniques**
- Récupérer une pièce de générateur dans la station-service
- Trouver des câbles électriques dans l'usine
- Rapporter du carburant pour les véhicules de la Forteresse
- Récupérer des matériaux de construction pour renforcer les murs nord

**Territoire**
- Sécuriser la rue principale du quartier est
- Neutraliser les Chauves du parking et barricader l'accès
- Construire un poste avancé à l'intersection des boulevards
- Explorer et cartographier le quartier industriel

**Social / PNJ**
- Retrouver un survivant signalé au nord
- Escorter un groupe de Chevelus vers la Forteresse
- Récupérer des affaires personnelles d'un PNJ dans son ancien appartement
- Sauver un PNJ capturé par un groupe de Chauves

**Scientifique**
- Trouver un journal de laboratoire dans l'hôpital
- Récupérer un échantillon de Chauve pour analyse
- Trouver un terminal informatique avec des données de recherche
- Prélever de l'ADN d'un Chauve extrême (dangereux)

---

## 9. L'EXPANSION DU TERRITOIRE

### Principe

La Forteresse est un organisme vivant. Elle peut et doit grandir.

### Améliorations internes

| Amélioration | Effet gameplay |
|---|---|
| Renforcer les murs | Résistance aux attaques de Chauves |
| Construire une tour de guet | Détection plus large des menaces |
| Créer une serre | Ressources alimentaires renouvelables |
| Installer un générateur de secours | Électricité stable, atelier et infirmerie disponibles 24h |
| Construire une deuxième infirmerie | Soins plus rapides, traitements capillaires plus efficaces |
| Créer un laboratoire de fortune | Craft de sérums anti-contamination, analyse d'échantillons |
| Installer des projecteurs | Sécurité nocturne améliorée |
| Poser des alarmes périmètre | Alerte automatique en cas d'approche de Chauves |

### Expansion extérieure

Le joueur et les PNJ IA peuvent progressivement sécuriser les zones autour de la Forteresse :

- **Poste avancé** : un bâtiment extérieur sécurisé, stockage intermédiaire, point de repos en expédition ;
- **Route sécurisée** : un couloir barricadé entre la Forteresse et un point d'intérêt, permettant des déplacements plus sûrs ;
- **Zone libérée** : un quartier entier neutralisé et barricadé, intégré au périmètre contrôlé ;
- **Antenne relais** : permet d'entendre des signaux radio, de détecter des survies, d'agrandir la carte visible ;
- **Ferme externe** : production alimentaire à l'extérieur des murs.

### Rôle des PNJ dans l'expansion

Les PNJ IA contribuent activement si les ressources le permettent :
- les Ingénieurs réparent et construisent ;
- les Gardes patrouillent les nouvelles zones ;
- les Éclaireurs cartographient les zones adjacentes ;
- les Artisans fabriquent les matériaux nécessaires.

Sans apport régulier de ressources par le joueur, les PNJ ne peuvent pas construire. Le joueur est le moteur de l'expansion.

---

## 10. LA NEUTRALISATION DES CHAUVES

### Philosophie

CHAUVE QUI PEUT n'est pas un jeu de massacre de zombies. C'est un jeu de **gestion du danger humain**.

Les Chauves sont des humains contaminés. Ils ne sont pas morts. Ils ne sont pas intrinsèquement monstrueux. Ils souffrent d'une maladie qui les a privés de leur raison. Les éliminer définitivement est possible, mais ce n'est pas l'approche encouragée par le jeu.

### Méthodes de neutralisation non létales

| Action | Effet | Durée |
|---|---|---|
| **Assommer** | Chauve K.O. au sol | 5-15 minutes selon résistance |
| **Attacher** | Chauve immobilisé durablement | Permanent si bien attaché |
| **Enfermer** | Chauve bloqué dans une pièce/cage | Permanent si porte solide |
| **Piéger** | Chauve immobilisé par piège | Variable selon piège |
| **Attirer ailleurs** | Détourner l'attention (bruit, appât) | Temporaire |
| **Bloquer une porte** | Chauve bloqué à court terme | Temporaire |
| **Immobiliser** (filets, cordes) | Chauve au sol | Dépend des matériaux |
| **Repousser** | Chauve repoussé, pas neutralisé | Temporaire |
| **Ralentir** | Collant, substance visqueuse | Quelques minutes |

### Méthodes létales

La neutralisation définitive est possible mais doit sembler **pesante** :
- le jeu ne récompense pas les massacres ;
- les PNJ de la Forteresse peuvent réagir négativement à un joueur qui tue systématiquement plutôt que neutraliser ;
- certaines fins narratives sont conditionnées à l'approche du joueur.

### Craft de neutralisation

Le joueur peut fabriquer des outils dédiés :
- **Cordes de contention** ;
- **Cage portable** ;
- **Filet lancé** ;
- **Grenade de gaz soporifique** ;
- **Appât sonore** (attire les Chauves dans une direction) ;
- **Appât olfactif au shampoing** (irresistible pour les Chauves proches) ;
- **Bombe à fumée** (pour s'échapper) ;
- **Colle industrielle** (au sol pour immobiliser).

---

## 11. LA PERSISTANCE DES CHAUVES NEUTRALISÉS

### Monde persistant

Les Chauves neutralisés persistent dans l'état où le joueur les a laissés. La zone reste mémorisée.

### Scénarios de persistance

| Situation | Conséquence si joueur revient |
|---|---|
| Chauve bien attaché dans une pièce | Toujours immobilisé |
| Chauve enfermé derrière porte solide | Toujours bloqué, mais peut se déchaîner si porte faible |
| Chauve assommé seulement | **Déjà réveillé**, potentiellement repositionné |
| Chauve piégé dans un filet | Peut s'être libéré selon la solidité du filet |
| Chauve dans une cage | Toujours dedans, mais agité |
| Chauve détourné | De retour dans sa zone, plus méfiant |

### Implications gameplay

- **Un secteur "nettoyé" hier n'est pas nécessairement sûr aujourd'hui.**
- Les Chauves qui se libèrent se repositionnent aléatoirement dans leur zone de départ.
- Certains Chauves (Éclaireurs, Demi-Conscients) peuvent **défaire les attaches** des autres.
- Des groupes de Chauves nomades peuvent **réoccuper** une zone neutralisée.
- La persistance incite le joueur à préférer **fermer solidement** plutôt que neutraliser à moitié.

---

## 12. LES DEGRÉS DE CHAUVITÉ

La calvitie est un spectre. Elle détermine les capacités de l'ennemi et sa dangerosité.

| Degré | Nom | Perte capillaire | Capacités cognitives | Niveau de danger |
|---|---|---|---|---|
| **0** | Normal | Aucune | Intact | N/A (survivant) |
| **1** | Dégarni léger | Légère (tempes, couronne) | ~80% — encore malin | Très élevé (stratégie) |
| **2** | Dégarni avancé | Importante (couronne large) | ~55% — imprévisible | Élevé |
| **3** | Chauve partiel | Sévère (sauf côtés) | ~30% — instinctif dominant | Moyen-élevé |
| **4** | Chauve complet | Totale | ~10% — quasi-animal | Moyen (prédictible) |
| **5** | Crâne lisse | Totale + peau lisse | ~2% — purement instinctif | Fort physiquement, stupide |

### Comportements selon le degré

**Degré 1 (Dégarni léger)**
- Peut tendre des embuscades ;
- Utilise des objets comme armes ;
- Peut feindre d'être endormi ;
- Communique avec d'autres Chauves ;
- Se souvient du joueur si déjà rencontré.

**Degré 2 (Dégarni avancé)**
- Agressif, moins calculé ;
- Peut encore utiliser des outils simples (barre de fer, clé) ;
- Réagit au mouvement et au son ;
- Oublie le joueur s'il sort du champ de vision.

**Degré 3 (Chauve partiel)**
- Agressivité forte, comportement imprévisible ;
- Incapable d'utiliser des objets ;
- Réagit au bruit, à l'odeur des cheveux, aux vibrations ;
- Grogne, siffle, crie.

**Degré 4 (Chauve complet)**
- Très agressif mais totalement prévisible ;
- Attaque tout ce qui bouge ;
- Réagit fortement à l'odeur de shampoing ou de cheveux sains ;
- Peut être facilement attiré ou détourné.

**Degré 5 (Crâne lisse)**
- Physiquement très fort, crâne durci ;
- Vitesse variable selon l'archétype ;
- Réagit uniquement à l'odeur et au bruit ;
- Peut traverser certaines barricades légères ;
- Dangereux s'il attrape le joueur, mais facile à piéger.

---

## 13. LES CATÉGORIES DE CHAUVES

### Types standard (par degré de calvitie)

Chaque zone contient un mélange de degrés 1 à 5. Les zones proches de la Forteresse ont plus de degrés 1-2. Les zones éloignées et les laboratoires ont des degrés 4-5.

### Archétypes spéciaux

Les archétypes sont des Chauves à comportement unique, plus rares et plus dangereux.

| Archétype | Degré | Comportement spécial | Contremesure |
|---|---|---|---|
| **Le Demi-Conscient** | 1-2 | Très malin, peut libérer d'autres Chauves, s'adapte | Isoler et enfermer prioritairement |
| **Le Hurleur** | 3 | Hurlement qui attire tous les Chauves proches | Neutraliser en silence, sans bruit |
| **Le Rapide** | 3-4 | Vitesse élevée, court sur de longues distances | Piège au sol, obstacles, cage |
| **Le Lourd** | 4-5 | Masse élevée, brise les barricades légères | Barricades lourdes, pièges de masse |
| **Le Rampant** | 4 | Se déplace au sol, passe sous les obstacles | Zones surélevées, grilles de sol |
| **Le Coiffeur** | 2-3 | Attaque directement les cheveux du joueur | Protection capillaire, ne pas le laisser attraper la tête |
| **Le Grimpeur** | 3 | Peut escalader murs et structures basses | Défenses en hauteur, aucune corniche accessible |
| **L'Éclaireur** | 2 | Repère le joueur de loin, alerte le groupe | Approche furtive, neutraliser en premier |
| **Le Chef de Meute** | 2 | Organise les Chauves proches, coordonne les attaques | Priorité absolue de neutralisation |
| **Le Chauve Alpha** | 5 | Boss de zone, physiquement exceptionnel, crâne blindé | Tactique longue, pièges multiples, neutralisation complexe |

### Le Coiffeur — archétype signature

Le Coiffeur mérite une mention spéciale : c'est le Chauve le plus original du bestiaire. Il est obsédé par les cheveux — non pas pour en avoir, mais pour les **arracher**. Il cible directement la tête du joueur et peut infliger des **dégâts à la jauge de cheveux** directement, en plus des dégâts de santé. Ce n'est pas le plus fort physiquement, mais il est psychologiquement dévastateur : une attaque du Coiffeur, même repoussée, peut accélérer la chute de cheveux.

---

## 14. LE SYSTÈME DE CONTAMINATION

### Sources de contamination

| Source | Niveau de contamination | Délai d'effet |
|---|---|---|
| **Morsure** | Fort | Rapide (1-2 heures in-game) |
| **Griffure** | Moyen | Modéré (4-6 heures in-game) |
| **Contact prolongé avec un Chauve** | Faible à moyen | Long (12-24 heures in-game) |
| **Zone à contamination ambiante** | Continu (exposition = accumulation) | Progressif |
| **Produit contaminé consommé** | Moyen à fort | Variable |
| **Stress extrême** | Accélère la contamination existante | Immédiat |
| **Blessure non soignée** | Augmente vulnérabilité | Progressif |
| **Spores de Chauve** (zones spéciales) | Fort | Modéré |

### Jauge de Contamination

La **Jauge de Contamination** est visible sur l'interface. Elle monte de 0% à 100%. À 100%, le processus de chute de cheveux commence activement.

- **0-25%** : aucun symptôme visible, traitements simples suffisent ;
- **25-50%** : légères démangeaisons (signal narratif), médicaments nécessaires ;
- **50-75%** : chute de cheveux visible commence, les PNJ réagissent ;
- **75-100%** : chute de cheveux rapide, compétences commencent à baisser, la Forteresse alerte ;
- **100%** : le processus de bannissement est déclenché.

### Traitements anti-contamination

| Traitement | Efficacité | Disponibilité |
|---|---|---|
| **Shampoing anti-folliculaire** | Réduit contamination de 10-20% | Pharmacies, salons |
| **Sérum capillaire médical** | Réduit contamination de 30-40% | Hôpital, laboratoire |
| **Traitement lourd (infirmerie)** | Réduit contamination de 60-70% | Forteresse seulement |
| **Follicure-X (partiel)** | Stoppe temporairement la chute | Rare, lié à la quête principale |
| **Protection physique** (casque, cagoule) | Réduit l'accumulation ambiante | Craft ou loot |

### Effets de la perte de cheveux sur les compétences

La **Jauge de Cheveux** (0-100% de cheveux restants) conditionne directement les compétences.

| Cheveux restants | Compétences affectées |
|---|---|
| 80-100% | Aucun malus |
| 60-80% | Légère perte de précision, mémoire légèrement impactée |
| 40-60% | Compétences techniques -20%, dialogues limités, réactions plus lentes |
| 20-40% | Compétences techniques -50%, coordination -30%, tension sociale maximale |
| 0-20% | Compétences quasi-nulles, comportement imprévisible, bannissement imminent |

---

## 15. LE SYSTÈME DE BANNISSEMENT

### Déclenchement

Le bannissement est un processus, pas un interrupteur. Il suit des étapes observables.

**Étape 1 — Suspicion**
Un PNJ remarque que le joueur a moins de cheveux qu'à l'habitude. Il le signale discrètement à d'autres.

**Étape 2 — Observation**
La Forteresse met le joueur sous surveillance passive. Les PNJ gardent leurs distances. L'accès à certaines zones peut être temporairement restreint.

**Étape 3 — Interpellation**
Le Médecin ou le Dirigeant interpelle le joueur. Un test capillaire est proposé ou imposé.

**Étape 4 — Diagnostic**
Si le test confirme une contamination avancée, l'Assemblée des Chevelus est convoquée.

**Étape 5 — Jugement**
Le Dirigeant annonce la décision. Le joueur peut tenter d'argumenter, de négocier, de présenter ses contributions passées — mais si la contamination est confirmée, le résultat est inévitable.

**Étape 6 — Bannissement**
Le joueur est expulsé par le portail principal. Les portes se ferment derrière lui. Il n'a plus accès à la Forteresse.

### Dialogue de bannissement

> "Tu as servi cette communauté. Nous te devons cela. Mais nous avons vu ce que la calvitie fait aux gens. Nous ne pouvons pas prendre le risque. Les portes se ferment. Ne reviens pas."

### Ce que le joueur perd au bannissement

- Accès aux zones de la Forteresse (artisanat, soins, stockage) ;
- Statut social et confiance des PNJ ;
- Accès à l'armurerie et à la station capillaire ;
- Certaines missions liées à la quête principale (depuis l'intérieur) ;
- La sécurité de dormir derrière des murs défendus.

### Ce que le joueur conserve

- Ses objets en inventaire au moment de l'expulsion ;
- Sa mémoire de la carte ;
- Ses relations individuelles avec certains PNJ qui peuvent rester ambigus.

---

## 16. LA BASCULE DE CAMP

La bascule de camp est **l'un des moments les plus importants du jeu**.

Elle n'est pas une punition. C'est une **transformation narrative et mécanique**. Le jeu change complètement après le bannissement. Un autre jeu commence.

### Avant la bascule (Phase 1)

Le joueur pense comme un Chevelu :
- "Comment je garde mes cheveux ?"
- "Comment je renforce la Forteresse ?"
- "Comment je neutralise ces Chauves pour qu'ils ne reviennent pas ?"
- "Comment je reviens à la Forteresse avant la nuit ?"

### Après la bascule (Phase 2)

Le joueur pense comme un Banni :
- "Comment je récupère des cheveux ?"
- "Comment je mange ce soir sans les ressources de la Forteresse ?"
- "Est-ce que je peux tromper ces Chevelus avec mes mèches volées ?"
- "Est-ce que les Chauves m'accepteront ou me rejettent aussi ?"
- "Est-ce que je veux vraiment retrouver les Chevelus ?"

La bascule est **émotionnelle**. La Forteresse qu'on a construite et défendue est maintenant un lieu fermé dont on voit les lumières depuis l'extérieur.

---

## 17. PHASE 2 : LE JOUEUR BANNI

### Nouvelles conditions de survie

**Alimentation** : Plus de cantine. Il faut fouiller les extérieurs, manger des conserves trouvées, boire de l'eau potentiellement contaminée.

**Santé** : Plus d'infirmerie. Soins de fortune avec matériaux lootés.

**Protection** : Plus de murs. Il faut trouver un abri, barricader un appartement, créer une base de fortune.

**Ressources** : Plus de stockage collectif. Inventaire personnel uniquement.

### Nouvelles possibilités

**Coexistence avec les Chauves**
- Certains Chauves (degrés 1-2) peuvent reconnaître que le joueur est "comme eux" ;
- Des groupes de Chauves partiellement stabilisés peuvent former des factions ;
- Le joueur peut devenir un intermédiaire entre les deux camps.

**Attaque de la Forteresse**
- Le joueur connaît les points faibles des murs ;
- Il peut attaquer pour récupérer des ressources ;
- Il peut tenter de libérer un PNJ qu'il veut emmener ;
- Cela est risqué et coûteux en relations.

**Marché noir**
- Des traders extérieurs (ni Chevelus ni Chauves complets) échangent des ressources ;
- Certains PNJ de la Forteresse peuvent être corrompus — échanges discrets via une zone neutre.

**Découverte de la vérité**
- En dehors de la Forteresse, le joueur peut trouver des informations que les Chevelus lui cachaient ;
- L'origine du virus peut avoir une dimension que la Forteresse connaissait et a dissimulée.

### Base de fortune extérieure

Le joueur peut établir une base autonome hors Forteresse :
- Appartement barricadé ;
- Cave ou sous-sol sécurisé ;
- Poste avancé transformé en refuge personnel ;
- Maison dans une zone dégagée.

Cette base est moins sûre que la Forteresse, mais elle est **libre**. Elle peut être améliorée avec les mêmes mécaniques que la Forteresse.

---

## 18. LA RÉCUPÉRATION DE MÈCHES DE CHEVEUX

### Principe

Une fois banni, le joueur peut tenter de **récupérer des cheveux** par tous les moyens disponibles. Cette mécanique est absente (ou très limitée) en Phase 1, car la Forteresse impose un code moral et pratique sur la propriété des cheveux.

### Sources de cheveux récupérables

| Source | Qualité | Disponibilité |
|---|---|---|
| **Survivants chevelus** (forcé) | Très bonne | Rare, risquée moralement |
| **PNJ neutralisés de la Forteresse** | Bonne | Risquée (relation dégradée) |
| **Cadavres récents** | Variable | Zones de combat récent |
| **Perruques trouvées** | Moyenne à bonne | Salons de coiffure, magasins |
| **Chauves partiels** (cheveux résiduels) | Faible | Zones denses de Chauves |
| **Stocks de salons de coiffure** | Bonne (mèches stockées) | Salons de coiffure |
| **Fausse implantation (kit médical)** | Variable selon kit | Hôpital, laboratoire |
| **Marché noir** | Variable | Traders extérieurs |
| **Syndicat des Perruquiers** (faction) | Haute | Zone découverte plus tard |

### Mécaniques d'arrachage / prélèvement

- L'arrachage d'un Chevelu non consentant est une **action morale lourde** avec conséquences narratives ;
- Le prélèvement sur un PNJ neutralisé demande du **matériel de prélèvement** (kit de coiffure) ;
- Les mèches trouvées sur des cadavres ne demandent que du **temps** ;
- Les perruques s'équipent directement comme objet de tête ;
- Les kits d'implantation permettent une **fixation plus durable** mais nécessitent des ressources.

---

## 19. L'IMPLANTATION VISIBLE DES MÈCHES

### Système d'implantation

Les mèches récupérées peuvent être :
1. **Posées directement** (sans fixation) — tombent vite, très peu crédibles ;
2. **Fixées avec colle ou attaches** — durée moyenne, crédibilité faible ;
3. **Implantées avec un kit médical** — durée longue, crédibilité variable selon cohérence.

### Visibilité dans le jeu

Chaque mèche implantée apparaît **physiquement sur le modèle du personnage** :
- sa couleur d'origine est conservée (rose reste rose, blonde reste blonde) ;
- sa texture est différente des cheveux naturels s'il en reste ;
- sa position sur le crâne correspond à la zone chauve comblée.

Le joueur voit le résultat dans :
- l'**inventaire / écran du personnage** ;
- le **portrait de statut** dans l'interface ;
- les **dialogues** (les PNJ réagissent à l'apparence) ;
- le **monde** (les Chauves peuvent réagir à l'odeur).

---

## 20. LE PATCHWORK CAPILLAIRE

### Le cas de figure classique

Prenons un exemple concret pour illustrer le système :

**Situation** : Le joueur a une calvitie de type "moine" — dessus du crâne chauve, cheveux bruns sur les côtés.

**Il arrache une mèche rose** à un PNJ féminin blessé. Il l'implante sur le dessus de son crâne.

**Résultat visible** : Crâne avec une mèche rose centrale, cheveux bruns sur les côtés.

**Il trouve ensuite** : une mèche blonde dans un salon, une mèche noire sur un cadavre, une mèche bleue dans un kit de coiffure.

**Résultat final** : Un crâne avec une mèche rose, une mèche blonde, une mèche noire, une mèche bleue — disposées en patchwork sur la calvitie.

Ce **patchwork** est :
- **drôle** (visuellement absurde) ;
- **informatif** (les PNJ peuvent identifier les cheveux) ;
- **stratégique** (la crédibilité dépend de la cohérence) ;
- **émotionnel** (chaque mèche a une histoire).

### Gestion des mèches

Le joueur peut :
- **retirer** une mèche pour la reposer ailleurs ;
- **couper** une mèche pour la mélanger ;
- **teindre** une mèche pour homogénéiser (si teinture disponible) ;
- **combiner** des mèches pour couvrir une zone plus large ;
- **remplacer** une mèche abîmée.

---

## 21. LA CRÉDIBILITÉ CAPILLAIRE

### Définition

La **crédibilité capillaire** est une valeur cachée qui détermine si le joueur peut passer pour un Chevelu aux yeux des PNJ.

### Facteurs de crédibilité

| Facteur | Impact |
|---|---|
| Cheveux naturels abondants | +++ crédibilité |
| Perruque professionnelle bien ajustée | ++ crédibilité |
| Mèches cohérentes (même couleur, même type) | + crédibilité |
| Mèches légèrement disparates | ~ crédibilité neutre |
| Patchwork multicolore visible | - crédibilité |
| Mèches mal fixées (qui bougent) | -- crédibilité |
| Patchwork absurde (5+ couleurs) | --- crédibilité |
| Odeur de Chauve persistante | --- crédibilité (indépendant des cheveux) |

### Usage de la crédibilité

- **Haute crédibilité** : le joueur peut approcher la Forteresse, parler à des Chevelus, entrer dans des zones restreintes ;
- **Crédibilité moyenne** : les PNJ sont méfiants, les gardes font des contrôles ;
- **Crédibilité faible** : les Chevelus s'enfuient ou attaquent, les gardes refusent l'entrée ;
- **Crédibilité nulle** : le joueur est immédiatement identifié comme Banni/Chauve.

Certains PNJ ont une **sensibilité capillaire plus haute** (le Médecin, le Coiffeur-capillologue) — ils détectent mieux les patchworks.

---

## 22. LES RÉACTIONS DES PNJ AUX CHEVEUX VOLÉS

### Réactions générales

Les PNJ ne sont pas muets. Ils réagissent à l'apparence du joueur avec des dialogues contextuels.

**Réactions à un patchwork visible** :
- "Attends... c'est quoi ça sur ton crâne ?"
- "Ces cheveux ne t'appartiennent pas. Je les reconnais."
- "Il a les cheveux de Mathilde. Je suis sûr que c'est les cheveux de Mathilde."
- "Bien essayé. Mais le coiffeur voit ce que les autres ne voient pas."
- "Une mèche rose, une mèche blonde, une mèche noire... Tu t'es dit personne remarquerait ?"
- "C'est quoi ce bricolage sur ton crâne, exactement ?"
- "Honnêtement ? Je préfère que tu sois complètement chauve plutôt que ça."

**Réactions selon la cohérence** :
- *Perruque correcte* : "Hm. Tu as l'air d'aller mieux. Entre."
- *Mèches légèrement incohérentes* : "Il y a quelque chose d'étrange dans tes cheveux aujourd'hui. Je te surveille."
- *Patchwork évident* : "Sors d'ici. Immédiatement."

**Réactions selon le PNJ** :
- Le Coiffeur : "Je connais chaque mèche qui passe dans ce quartier. Celles-là viennent de la fille du café. Ou c'était son salon ? De toute façon, c'est pas les tiennes."
- Le Médecin : "Tests capillaires négatifs... mais l'implantation est artificielle. Protocole de quarantaine."
- Le Garde : "Tes cheveux bougent quand tu tournes la tête. Les cheveux bougent pas comme ça."
- Un enfant PNJ : "Pourquoi t'as plusieurs couleurs de cheveux ? C'est une maladie ?"

---

## 23. INFILTRATION ET RETOUR À LA FORTERESSE

### Options après le bannissement

Le joueur banni a plusieurs façons d'interagir avec la Forteresse.

**Option 1 — Infiltration discrète**
- Crédibilité capillaire élevée requise ;
- Entrer par une faille dans les murs (découverte avant le bannissement) ;
- Corrompre un garde PNJ (si relation existante) ;
- Passer par un tunnel ou une zone non surveillée ;
- Risque : être reconnu malgré les cheveux.

**Option 2 — Négociation**
- Se présenter au portail avec des ressources rares ;
- Proposer des informations sur le remède ;
- Promettre de quitter la Forteresse après avoir pris ce dont on a besoin ;
- Certains PNJ peuvent plaider en faveur du joueur.

**Option 3 — Réhabilitation**
- Trouver un traitement partiel (Follicure-X) qui stoppe la chute ;
- Revenir avec des cheveux stables et une preuve du traitement ;
- Subir un nouveau test capillaire sous l'œil des dirigeants ;
- Si accepté : le joueur est réintégré avec statut diminué.

**Option 4 — Attaque**
- Attaque frontale pour récupérer des ressources spécifiques ;
- Risque élevé : les PNJ IA défendent activement ;
- Conséquences narratives majeures sur les fins disponibles.

**Option 5 — Faction externe**
- Créer ou rejoindre un groupe de Bannis et Demi-Chauves ;
- Constituer une force suffisante pour négocier d'égal à égal avec la Forteresse ;
- Approche longue et indirecte mais débouche sur des fins alternatives.

---

## 24. LES DEUX GRANDES PHASES DU JEU

### PHASE 1 : CAMP DES CHEVELUS

**Identité du joueur** : Explorateur de confiance, défenseur de la Forteresse  
**Enjeu** : Maintenir ses cheveux, trouver le remède, protéger la communauté  
**Gameplay** : Exploration, loot, neutralisation non létale, craft, amélioration de la Forteresse  
**Relation aux Chauves** : Ennemis à neutraliser  
**Relation à la Forteresse** : Maison, sécurité, communauté  

**Métriques clés** :
- Jauge de Cheveux (haute = bien)
- Jauge de Contamination (basse = bien)
- Ressources de la Forteresse (maximiser)
- Confiance des PNJ (maintenir)

### PHASE 2 : CAMP DES BANNIS

**Identité du joueur** : Banni en transition, ni Chevelu ni Chauve  
**Enjeu** : Survivre seul, récupérer des cheveux, trouver le remède autrement  
**Gameplay** : Survie de fortune, vol de mèches, infiltration, nouvelles alliances  
**Relation aux Chauves** : Complexe — menace mais aussi semblables  
**Relation à la Forteresse** : Lieu fermé à récupérer, infiltrer ou attaquer  

**Métriques clés** :
- Crédibilité Capillaire (variable)
- Nombre de mèches récupérées
- Qualité de la base de fortune
- Réseau d'alliances extérieures

### Transition entre phases

La transition Phase 1 → Phase 2 est déclenchée par le bannissement.  
La transition Phase 2 → Phase 1 est possible via réhabilitation (rare, conditionnée).  
Les deux phases peuvent se succéder plusieurs fois si le joueur est réhabilité puis re-contaminé.

---

## 25. MÉCANIQUES INSPIRÉES DE PROJECT ZOMBOID

Ces mécaniques sont **reprises dans leur esprit**, pas copiées visuellement ou systémiquement.

| Mécanique PZ | Adaptation CHAUVE QUI PEUT |
|---|---|
| Vue isométrique | Vue semi-isométrique 3D, caméra légèrement plus haute |
| Gestion d'inventaire | Inventaire avec slots, poids, durabilité des objets |
| Loot pièce par pièce | Fouille de chaque contenant dans chaque pièce |
| Importance du bruit | Les Chauves réagissent aux sons — courrir attire, tirer alerte tout le quartier |
| Blessures localisées | Blessures par zone corporelle (tête, bras, jambes) avec traitements spécifiques |
| Fatigue | Jauge de fatigue — expéditions trop longues sans repos = malus |
| Faim et soif | Besoins alimentaires et hydriques gérés en temps réel |
| Stress | La proximité avec les Chauves, les morts, les nuits seules augmentent le stress (et accélèrent la contamination) |
| Sommeil | Nécessité de dormir pour récupérer — possible en Forteresse ou en base de fortune |
| Maladies | Contamination folliculaire, mais aussi blessures infectées, intoxications alimentaires, etc. |
| Craft | Fabrication d'objets de neutralisation, de protection, de soins |
| Barricades | Barricades de portes et fenêtres, améliorables, différents niveaux de résistance |
| Zones découvertes/non découvertes | Carte avec brouillard de guerre progressif |
| Véhicules | Transport possible, consomme du carburant, attire les Chauves par le bruit |
| Base améliorable | La Forteresse grandit via les expéditions et les ressources |
| Dangers persistants | Les Chauves ne disparaissent pas, ils se repositionnent |
| Mort ou transformation lourde | La contamination totale = bannissement. C'est la "mort sociale" avant la mort physique |
| Monde vivant simulé | Les PNJ IA ont des routines, les Chauves patrouillent, les ressources s'épuisent |
| Préparation avant de sortir | Protections capillaires, soins préventifs, plan de route — sortir non préparé est dangereux |

---

## 26. MÉCANIQUES PROPRES À CHAUVE QUI PEUT

Ces mécaniques sont **uniques au jeu** et constituent son identité.

### Jauge de Cheveux

Une métrique centrale visible en permanence dans l'interface. Elle ne remonte pas seule. Elle peut être maintenue ou réduite selon les soins et les événements.

### Jauge de Contamination

Séparée de la Jauge de Cheveux. La contamination monte selon l'exposition. Des traitements la font baisser. Quand elle atteint 100%, la chute de cheveux s'accélère.

### Odeur des Cheveux

Les Chauves sentent les Chevelus. Plus le joueur a des cheveux sains et traités, plus il dégage une odeur perceptible pour les Chauves proches. Des produits **masquants** peuvent réduire cette odeur. Un joueur chauve ou avec des cheveux non traités est moins détectable olfactivement.

### Système de Bannissement social

Mécanique de réputation capillaire dans la Forteresse, avec étapes progressives vers l'exclusion.

### Bascule de camp narrative

Changement complet de gameplay et de perspective narrative au moment du bannissement. Un seul jeu, deux expériences radicalement différentes.

### Récupération de mèches

Prélèvement, conservation, implantation et gestion de mèches récupérées sur d'autres personnages.

### Patchwork capillaire visible

Rendu 3D temps réel des mèches récupérées sur le modèle du personnage.

### Crédibilité capillaire

Valeur calculée sur la cohérence et la qualité des cheveux du joueur, utilisée pour les interactions sociales avec les Chevelus.

### Neutralisation vs Massacre

Système de récompense implicite de la neutralisation non létale plutôt que de l'élimination.

### Persistance des Chauves neutralisés

Les Chauves ne disparaissent pas à la mort de zone. Ils restent dans leur état (attaché, enfermé, assommé, libéré).

### Degrés de Chauvité ennemis

Cinq niveaux de calvitie qui définissent les capacités cognitives et physiques de chaque ennemi.

---

## 27. DIRECTION ARTISTIQUE RECOMMANDÉE

### Recommandation : Option B — Stylisée semi-caricaturale

Après analyse des contraintes de production et de l'identité du jeu, l'**Option B** est recommandée comme direction principale.

### Pourquoi l'Option B est la bonne direction

1. **Cohérence avec le ton** : l'humour visuel assumé renforce le propos satirique ;
2. **Faisabilité** : une petite équipe peut produire des assets stylisés sans pipeline AAA ;
3. **Lisibilité** : la vue semi-isométrique avec personnages stylisés est plus lisible qu'un rendu photoréaliste ;
4. **Identité** : CHAUVE QUI PEUT doit avoir une **gueule reconnaissable immédiatement** — le style caricatural y contribue ;
5. **Référence viable** : Project Zomboid version 2026, plus propre, plus stylisé, avec grosses têtes et humour visuel.

### Spécifications de la direction artistique

**Caméra**
- Semi-isométrique 3D (pas d'isométrie 2D stricte)
- Angle fixe ou rotation légère contrôlée (pas de caméra libre)
- Hauteur : ni trop proche (pas de TPS), ni trop loin (lisibilité des personnages)
- Les personnages doivent être lisibles à distance

**Personnages**
- Proportions légèrement caricaturales : têtes légèrement plus grandes
- Crânes de Chauves **visuellement saillants** — chauvité immédiatement lisible
- Expressions faciales simples mais présentes
- Différenciation claire entre Chevelus (cheveux visibles, colorés, variés) et Chauves (crânes nus, peau plus pâle ou rougeâtre)
- Les mèches récupérées doivent être **immédiatement visibles** sur le modèle du personnage

**Environnements**
- Décors modulaires (blocs réutilisables, cohérents)
- Style post-apocalyptique : bâtiments abîmés, vitres brisées, graffitis, végétation envahissante
- Pas de destruction photoréaliste — esthétique "cartoon post-apo propre"
- Intérieurs exploitables avec des conteneurs visibles
- Palette de couleurs : gris chauds, rouilles, verts délavés, avec des accents de couleur vifs pour les cheveux et les objets importants

**Interface**
- Claire, lisible, non intrusive
- Jauges visibles en permanence : Cheveux, Contamination, Santé, Faim, Soif, Fatigue, Stress
- Portrait du personnage avec mèches visibles
- Minimap / carte semi-transparente
- Inventaire en grille lisible

**Effets visuels**
- Effets de contamination visibles sur le personnage (particules folliculaires, légers effets visuels)
- Animations de perte de cheveux progressives
- Réactions visuelles des ennemis (Chauve Hurleur crie visuellement, Chauve Rapide sprinte avec animation distinctive)

---

## 28. OPTION SEMI-RÉALISTE VS OPTION STYLISÉE

### Option A — Semi-Réaliste Sombre

**Description** : Plus proche d'un concept art réaliste. Personnages à proportions naturelles. Textures détaillées. Éclairage dramatique. Atmosphère lourde.

**Avantages** :
- Immersion plus forte pour un joueur cherchant une expérience sérieuse
- Cohérence avec d'autres survivals AAA

**Inconvénients** :
- Très coûteux à produire pour une petite équipe
- Rend l'humour du jeu plus difficile à doser (ton schizophrène)
- Risque de ressembler à n'importe quel survival sans identité visuelle forte
- Nécessite un pipeline d'assets lourd (textures haute résolution, shaders complexes)

### Option B — Stylisée Semi-Caricaturale *(Recommandée)*

**Description** : Proportions légèrement exagérées, têtes un peu plus grandes, crânes de Chauves proéminents, couleurs saturation équilibrée. Lisible de loin. Humour visuel intégré dans le design.

**Avantages** :
- Identité visuelle immédiatement reconnaissable
- Humour visuel renforcé par le style
- Faisable pour une petite équipe avec des assets stylisés
- Personnages lisibles à toutes les distances de caméra
- Les mèches récupérées sont facilement visibles et drôles
- Référence claire et atteignable (Project Zomboid modernisé)

**Inconvénients** :
- Risque d'être vu comme "trop léger" par des joueurs cherchant un survival sérieux
- Nécessite un travail de direction artistique précis pour ne pas basculer dans le "cartoon enfantin"

### Terminologie à utiliser

Éviter : "cartoon", "chibi", "dessin animé"  
Utiliser : **"stylisé semi-caricatural survival post-apo"** — c'est le bon descripteur.

---

## 29. VERSION PROTOTYPE FAISABLE

### Objectif du prototype

Produire une **vertical slice** qui montre immédiatement les mécaniques centrales et l'originalité du jeu. Le prototype n'a pas besoin d'être complet. Il doit être **jouable**, **lisible** et **convaincant**.

### Scope du prototype

**Carte**
- 1 petite Forteresse (4-6 zones internes)
- 1 rue extérieure principale
- 1 pharmacie (fouillable pièce par pièce)
- 1 atelier externe
- 5-8 conteneurs à fouiller dans la rue

**PNJ**
- 3-4 PNJ IA dans la Forteresse (Garde, Médecin, Artisan, Dirigeant)
- Dialogues contextuels simples liés à l'état capillaire du joueur

**Ennemis**
- 4 types de Chauves : Dégarni léger, Chauve complet, Hurleur, Coiffeur
- Comportements IA simples mais distincts

**Jauges et systèmes**
- Jauge de Cheveux (visible)
- Jauge de Contamination (visible)
- Jauge de Santé
- Jauge de Fatigue
- 3-4 compétences qui diminuent avec la chute de cheveux

**Forteresse**
- Accès libre en début de partie
- Système de bannissement si contamination > 75%
- Dialogue de bannissement fonctionnel

**Système capillaire**
- Mécanisme simple de récupération de mèches (2-3 sources)
- Affichage d'une mèche récupérée sur le personnage (1 slot visible)
- Crédibilité capillaire basique (avec / sans mèche)

**Missions**
- Mission principale : Trouver un échantillon de sérum dans la pharmacie
- Mission secondaire : Récupérer du shampoing anti-contamination
- Mission secondaire : Neutraliser le Chauve qui bloque la rue principale

**Interface**
- Jauges visibles en permanence
- Portrait du personnage avec cheveux/mèches
- Inventaire simple en grille
- Minimap basique

**Caméra**
- Semi-isométrique 3D
- Rotation contrôlée ou angle fixe selon confort de production

### Ce que le prototype doit démontrer

1. La boucle sortie/retour à la Forteresse est fun
2. La neutralisation (pas le massacre) est satisfaisante
3. La perte de cheveux se voit et impacte le gameplay
4. Le bannissement est un moment fort
5. La récupération de mèches est drôle et stratégique
6. L'IA des Chauves réagit au bruit et à l'odeur
7. La Forteresse est un hub crédible

---

## 30. RISQUES DE CONCEPTION

### Risque 1 — Trop d'ambition

**Problème** : Un GDD trop large conduit à un prototype impossible à finir.  
**Mitigation** : Prioriser strictement les mécaniques du prototype. Tout le reste est Phase 2 de développement.

### Risque 2 — Humour qui tue l'immersion

**Problème** : Si le jeu est trop drôle, le joueur ne ressent pas la tension du survival.  
**Mitigation** : L'humour doit venir du **contexte et des dialogues**, pas des mécaniques. La mécanique de survie doit rester sérieuse même si son sujet est absurde.

### Risque 3 — Le bannissement perçu comme punition

**Problème** : Si le joueur vit le bannissement comme une sanction, il rechargera sa sauvegarde pour l'éviter.  
**Mitigation** : Teaser la Phase 2 avant le bannissement. Montrer que de nouvelles possibilités s'ouvrent. Rendre la Phase 2 intéressante dès le départ.

### Risque 4 — Systèmes interdépendants trop complexes

**Problème** : Contamination + Cheveux + Crédibilité + Compétences + Bannissement = beaucoup de jauges.  
**Mitigation** : Unifier visuellement les métriques. Interface claire et lisible. Le joueur doit comprendre son état en un coup d'œil.

### Risque 5 — IA PNJ trop stupide

**Problème** : Des PNJ qui réagissent de façon mécanique cassent la crédibilité de la Forteresse.  
**Mitigation** : Commencer avec des comportements IA simples mais solides. Pas d'IA complexe pour le prototype — mais des réactions contextuelles crédibles.

### Risque 6 — Patchwork capillaire techniquement difficile

**Problème** : Rendre plusieurs mèches de couleurs différentes sur un modèle en temps réel peut être coûteux en production.  
**Mitigation** : Système de slots visibles prédéfinis sur le crâne (5-7 zones), chaque zone pouvant avoir une couleur indépendante. Texture procédurale simple plutôt que vraie simulation de cheveux.

### Risque 7 — Monde trop statique

**Problème** : Si les Chauves ne bougent jamais et les zones restent identiques, l'exploration perd de l'intérêt.  
**Mitigation** : Micro-événements aléatoires : groupe de Chauves qui migre, ressource qui disparaît, Chauve libéré de ses attaches, etc.

---

## 31. CE QU'IL FAUT ABSOLUMENT ÉVITER

### Mécaniques à proscrire

- **Copier Project Zomboid** visuellement ou mécaniquement sans adaptation à l'univers
- **Un jeu d'action TPS** ou FPS — ce n'est pas CHAUVE QUI PEUT
- **Un jeu trop AAA** — hors budget, hors vision
- **Des Zombies classiques** repeints en chauves — les Chauves sont des humains contaminés, pas des morts-vivants
- **La perte de cheveux comme élément purement cosmétique** — elle doit impacter les compétences, le statut, le gameplay
- **Ignorer la Forteresse** — c'est le cœur du jeu en Phase 1
- **Ignorer le bannissement** — c'est le pivot central du jeu
- **Ignorer la bascule de camp** — c'est l'originalité principale
- **Ignorer la récupération de mèches** — c'est la mécanique signature
- **Oublier que le joueur commence puissant** — la régression est le moteur, pas la progression
- **Faire un système de massacre de Chauves** sans conséquence — le jeu valorise la neutralisation
- **Un prototype trop grand** impossible à terminer

### Ton à proscrire

- **Sérieux sans humour** — le jeu est satirique, pas sombre au sens Cormac McCarthy
- **Humour trop gros** — pas de gags foireux, l'humour vient de la situation, pas de la caricature forcée
- **Moral simpliste** (les Chauves = mauvais, les Chevelus = bons) — la Phase 2 doit nuancer tout ça

### Visuels à proscrire

- Rendu photoréaliste impossible à produire pour une petite équipe
- Profondeur de champ de cinéma permanente (illisible en isométrique)
- Vue TPS action en mouvement (incompatible avec le gameplay systémique)
- Personnages trop petits (illisibles) ou trop grands (perd la vue d'ensemble)
- Interface surchargée

---

## SYNTHÈSE RAPIDE : CHAUVE QUI PEUT EN 10 POINTS

1. **Survival systémique** inspiré Project Zomboid, univers entièrement original
2. **Le joueur commence fort** et essaie de ne pas régresser
3. **La Forteresse** est le hub central — communauté vivante, PNJ IA actifs
4. **Explorer est obligatoire** : ressources, remède, agrandissement du territoire
5. **La contamination folliculaire** fait perdre cheveux + compétences + statut social
6. **Les Chauves se neutralisent**, ils ne se massacrent pas — persistance dans le monde
7. **5 degrés de calvitie** + archétypes spéciaux définissent les ennemis
8. **Le bannissement** est le pivot du jeu — la Phase 2 change tout
9. **Récupérer des mèches** et les implanter visiblement est la mécanique signature
10. **Style stylisé semi-caricatural** — Project Zomboid version 2026, identifiable et faisable

---

*Document rédigé pour usage interne équipe de développement CHAUVE QUI PEUT.*  
*Ne pas mélanger avec le projet WOTOL.*
