# TODO WOTOL — notes a appliquer au PROCHAIN changement

`claude/wotol-demo-finale` contient maintenant la fusion de mon travail (bug Cristalliseur
duplique + tourelles) ET du travail fait en parallele sur `agent/connect-exploration-flow`
(nage 3D connectee, cite complete, alerte noxeenne, equilibrage adaptatif des pertes). Mes
deux corrections de bug sont supersedees par leur refonte (tourelles achetees par le joueur,
reconstruites depuis l'etat de territoire persistant a chaque BeginPreparation).

Priorite au prochain retour PC : compiler `claude/wotol-demo-finale` et jouer la boucle
13 phases complete de bout en bout (menu -> lancement manuel -> nage -> Kraken -> rapport ->
placement Cristalliseur en 3D -> cite -> batiment distance -> alerte -> defense -> phase 3).
Rien de tout cela n'a ete compile/vu tourner cote assistant (pas d'editeur Unreal ici).

Fait cette session (19/07/2026, en plus de la fusion agent/connect-exploration-flow) :
- Minimap schematique (coin haut-droit, sous pause/reglages) : AWOTOLDemoHUD::DrawMinimap.
  Absente jusqu'ici alors que c'est un standard du genre (Total War, Company of Heroes,
  Homeworld) et que 3 des maquettes de Liamor la montrent. Bornes calculees dynamiquement
  depuis les unites vivantes + le batiment, points colores par faction, repere camera.
  Clic-pour-recentrer AJOUTE cette session (etait note comme amelioration possible) :
  bornes du monde extraites en fonction partagee GetMinimapWorldFrame (dessin ET clic
  restent forcement synchronises, meme principe que BuildRosterGroups) + MinimapScreenToWorld
  (inverse la projection) -> clic sur la minimap = BattleCamera->FocusOn() du point vise.
  Uniquement en bataille (Screen::Playing), la ou une minimap sert vraiment a naviguer.
- Ecran REGLAGES enfin reel (avant : juste Reprendre/Recommencer/Quitter, le bouton
  "Options" de l'ancien menu principal (WOTOLGameMode_MainMenu, architecture UMG abandonnee)
  etait un stub vide) :
  - Volume musique REEL (barre cliquable, s'applique immediatement au morceau en cours via
    AWOTOLDemoDirector::SetMusicVolume + UAudioComponent::SetVolumeMultiplier).
  - Plein ecran / fenetre (UGameUserSettings, aucun asset requis).
  - Pas de reglage bruitages separe : aucun systeme de son global (Sound Class/Mix) n'existe
    encore pour les SFX, un curseur ferait semblant de marcher pour rien. A ajouter le jour
    ou de vrais SFX arrivent.
- Placement du Cristalliseur : aperçu HOLOGRAPHIQUE reprenant la vraie silhouette du bâtiment
  (AWOTOLCaptureObject::SetGhostPreviewMode, coquille translucide pulsante WOTOLGlow::MakeHalo),
  en plus de l'anneau existant. Puis animation de CONSTRUCTION (montée en échelle ~2,5 s,
  BeginConstruction) avant de révéler les récompenses.
- Nouvelle étape "seq_defense_prompt" : après construction, message explicite invitant à
  installer une tourelle/sentinelle (vue Territoire) avant la contre-attaque.
- Héros d'exploration (WOTOLHeroCharacter) : sprint maintenu (Alt gauche), ruée courte avec
  recharge (touche C, impulsion + coup de FOV), inclinaison de caméra en virage (banking),
  léger camera lag — sensation "action" distincte du pilotage RTS des batailles tactiques.
  Mappings ajoutés dans Config/DefaultInput.ini (Sprint=LeftAlt, Dash=C).
- Zone d'exploration enrichie (WOTOLGreyboxEnvironment::BuildArena, phase 1 uniquement) :
  ajout d'une arche traversable (SpawnArch) et d'une ruine à gradins + colonnade
  (SpawnZiggurat/SpawnColonnade) décalées de part et d'autre du trajet héros->Kraken —
  ces fonctions existaient déjà dans le fichier mais n'étaient jamais appelées. Le trajet
  ExplorationHeroOffset/ExplorationKrakenOffset a aussi reçu un décalage Y pour ne plus
  être une ligne droite sur X. Rien de bloquant (traversable), donc pas de risque pour la
  navigation de la bataille qui réutilise ensuite la même arène.

- Systeme de competences (ability) rendu REELLEMENT fonctionnel pour la premiere fois :
  - Bug corrige : AUnitBase::InitFromDataAsset() peuplait AbilityComp->AbilityClasses APRES
    que Super::BeginPlay() ait deja declenche AbilityComponent::BeginPlay() (qui instancie
    les abilities) -> AUCUNE ability n'etait jamais reellement creee, meme quand le DataAsset
    en assignait une. Corrige via UAbilityComponent::RebuildAbilitiesFromClasses() (rappelable
    explicitement apres avoir rempli AbilityClasses).
  - Repli greybox : UAbilityBase_Generic (nouvelle classe concrete minimale, aucune logique
    propre) instanciee automatiquement pour toute unite dont le GDD documente une competence
    (AbilityName non vide) mais qui n'a encore aucune classe UAbilityBase assignee — ce qui
    est le cas de toutes les unites actuellement. Configuree avec les valeurs DEJA presentes
    dans UnitDataLibrary.cpp (nom, cooldown) ; seul Damage est derive de AttackDPS x2.5
    (placeholder de demo, clairement commente comme tel).
  - Touche R (AWOTOLPlayerController_Battle::ActivateSelectionAbility) : active la competence
    de chaque unite selectionnee. CIBLAGE : priorite a l'ennemi vivant sous le CURSEUR
    (focus fire manuel, demande par Liamor - ex. tir laser qui depasse l'ennemi le plus
    proche pour abattre une cible plus importante derriere) ; repli automatique sur l'ennemi
    le plus proche PAR UNITE si rien sous le curseur ou hors de portee pour cette unite.
    HUD : petit panneau "Pret (R)" / "Recharge : Xs" pour l'unite primaire selectionnee
    (DrawAbilityStatus).
  - AVANT cette session : le joueur n'avait AUCUN moyen d'activer une competence (seule l'IA
    ennemie utilisait ActivateAbility, via AIAdaptiveController) — trouve en comparant avec les
    demos historiques de jeux de reference (XCOM, Company of Heroes, Warcraft III : toutes
    exposent explicitement le kit de competences au joueur, cf. recherche web session du
    19/07/2026).

Encore a faire (releve pendant cette session, pas encore code) :
- Cinematique/texte d'intro anime + lore de faction au clic : PARTIELLEMENT deja en place
  (DrawFactionSelect affiche une phrase de lore par faction selectionnee, et
  BeginOpeningExploration ouvre deja une fenetre d'objectif de lore par faction avant
  l'exploration) — pas une vraie cinematique animee, mais pas un vrai manque non plus.
  A ne retravailler que si Liamor confirme vouloir plus que le texte actuel.
- La zone d'exploration reste dans la MEME arene que la bataille (pas de vraie zone dediee
  de 70 m²) : les nouveaux reperes aident a la sensation d'exploration mais n'ajoutent pas
  un espace physiquement plus grand a parcourir. A revisiter si le rythme parait encore trop
  court une fois teste en jeu.
- Le repli generique donne une ability FONCTIONNELLE (degats) mais pas fidele au design
  (cone/aura/zone du GDD) : Axe 1/Axe 2, formes de zone, feedback visuel de competence
  restent a faire (le "onglet Competences" est deja note comme travail futur ailleurs
  dans les docs).
- VERIFIE cette session : les batiments de defense (WOTOLDefenseStructure) n'utilisent PAS
  le systeme d'ability partage — ils ont leur propre logique de tir independante (FireTimer/
  DamagePerShot/Range/FindTarget, cible auto la faction adverse la plus proche a portee via
  UFactionRegistrySubsystem). C'est un choix legitime pour des tourelles automatiques (pas
  des unites avec un kit de competences) — code relu, fonctionnel, aucun bug trouve. Ferme,
  pas un manque.

Fait cette session (suite, meme jour) :
- Ecran de PERSONNALISATION DU HEROS enfin implemente (etait absent : FHeroLoadout existait
  comme simple struct de donnees dans WOTOLTypes.h mais rien ne l'alimentait ni ne l'affichait ;
  UHeroLoadoutDataAsset + AWOTOLHeroCharacter::SetLoadout() existaient aussi mais n'etaient
  JAMAIS appeles nulle part). Nouvel ecran EDemoScreen::HeroCustomization, insere entre le
  choix de faction/difficulte et le lancement reel (StartDemoAfterSelection) :
  - Choix d'HERITAGE (4 : Thalassi/Givrelere/Abysseen/Gardien) et de SPECIALITE (4 :
    Thalassi/Guerrier/Mage/Inquisiteur), chacun avec une courte description ; portrait
    cyclable (1/5, halo teinte par la faction, pas de vrais portraits illustres en attendant
    du contenu artistique). Stocke dans UDemoFlowSubsystem::HeroLoadout (nouveau, coherent
    avec SelectedFaction/Difficulty qui vivent deja la — PAS dans WOTOLGameInstance::
    SessionConfig, qui appartient a l'ancienne architecture UMG abandonnee et n'est lu nulle
    part dans le flux actif).
  - Bouton Retour (re-choix de faction) et bouton Confirmer (lance la demo comme avant).
  - N'affecte PAS encore visuellement le héros en jeu (aucun mesh/asset par heritage) : pure
    collecte du choix pour l'instant. A relier plus tard a un vrai UHeroLoadoutDataAsset par
    combinaison heritage/specialite si Liamor veut un impact mecanique/visuel reel.

- VUE CITE : vraie camera 3D isometrique FIXE + zoom + fiche technique au clic batiment,
  comme demande explicitement par Liamor ("peu importe l'ampleur du chantier, si je te l'ai
  demande tu le fais"). Gros morceau fait cette session, 5 nouveaux fichiers :
  - AWOTOLCityCamera (nouveau, Gameplay/Demo/) : pawn camera ISOMETRIQUE — angle fixe
    (Yaw 45 / Pitch -55, jamais modifiable), projection ORTHOGRAPHIQUE (pas de distorsion
    perspective, vrai look city-builder). Pan ZQSD/WASD/fleches (borne a un rayon autour du
    hub, memes bindings directs par touche que AWOTOLBattleCamera) + zoom molette (OrthoWidth
    interpole, 1100-4200).
  - AWOTOLCityEnvironment (nouveau) : decor greybox (sol + hub decoratif + kitbash) spawn
    UNE SEULE FOIS, loin de l'arene de bataille/exploration (Y=+30000 -> meme niveau reutilise
    pour tout, aucun risque de chevauchement). Spawn un AWOTOLCityBuildingProp par categorie
    productible EN ANNEAU, dans le MEME ORDRE que AWOTOLDemoHUD::CityCardCategory (source de
    verite partagee, pas de liste dupliquee).
  - AWOTOLCityBuildingProp (nouveau) : un batiment par categorie (kitbash, meme technique que
    AWOTOLDefenseStructure — composants crees dynamiquement dans BeginPlay, pas dans le
    constructeur). Hauteur = niveau (1/2/3), teinte terne si verrouille/pas construit vs vive
    si actif, anneau de selection (WOTOLGlow::MakeHalo) visible au clic. Seul le socle a une
    collision (ECC_WorldStatic, QueryOnly) -> cible du raycast caméra.
  - UDemoFlowSubsystem : + FOnDemoScreenChanged (broadcast depuis SetScreen — point d'accroche
    UNIQUE pour la possession de camera, evite de toucher les 5 sites SetScreen(City) disperses
    dans le Director) ; + SelectedCityCategory/bCitySelectionValid/SetSelectedCityCategory/
    HasCitySelection (etat de selection partage HUD <-> PlayerController <-> props 3D).
  - AWOTOLDemoDirector : PossessCityCamera() (retrouve la camera/l'environnement par
    TActorIterator, meme convention que PossessBattleCamera existant) + HandleScreenChanged()
    (bind sur OnDemoScreenChanged en BeginPlay, agit UNIQUEMENT sur l'entree dans City — les
    autres ecrans possedent deja la bonne camera via le code existant).
  - AWOTOLPlayerController_Battle : clic sur un AWOTOLCityBuildingProp (raycast ECC_WorldStatic,
    teste EN DERNIER apres tous les boutons 2D pour qu'un bouton reste toujours prioritaire
    meme s'il chevauche visuellement un batiment) OU sur sa carte 2D -> les deux convergent
    vers SetSelectedCityCategory (selection unifiee 3D/2D).
  - AWOTOLDemoHUD::DrawCityView : ENLEVE le fond plein ecran (image/dégradé) qui aurait
    entierement masque la scene 3D rendue derriere le Canvas — ne reste que les bandeaux de
    chrome haut/bas translucides. Nouvelle FICHE TECHNIQUE (panneau a droite, visible quand
    HasCitySelection()) : nom du batiment/de l'unite, niveau, cout d'amelioration, cout de
    production + reserve, ou etat verrouille/pas-encore-construit. Cartes 2D : cadre blanc
    de surbrillance quand leur categorie est la selection courante.
  - GameMode_Demo : spawn l'environnement + la camera de cite au demarrage (loin de l'arene),
    a cote du decor/camera de bataille existants.
  RISQUE PARTICULIER (a signaler a Liamor) : AUCUNE partie de ce systeme n'a pu etre compilee
  ni vue tourner ici (toujours pas d'editeur Unreal disponible cote assistant) — c'est le plus
  gros morceau de code 3D ajoute en une seule fois cette session. A tester en PRIORITE au
  prochain retour PC : ouvrir l'ecran Cite, verifier que la camera isometrique s'affiche bien
  (pas d'ecran noir/vide), que le pan/zoom repondent, que le clic sur un batiment 3D ouvre la
  fiche technique, et que les cartes 2D du bas restent cliquables normalement (upgrade/produce/
  depart/competences) malgre le retrait du fond plein ecran.

- ECRAN COMMANDES (liste des touches), accessible depuis le menu reglages (bouton
  "Commandes", 4e bouton sous Reprendre/Recommencer/Quitter). Trouve en comparant aux
  conventions du genre RTS/tactique (recherche web session du 19/07/2026) : les RTS ont
  presque tous un menu Reglages -> Controles listant les touches, precisement parce que
  beaucoup sortent sans tutoriel et que les joueurs manquent des raccourcis sinon (ex.
  pause tactique, selection de groupe). WOTOL avait deja Echap/P pour la pause et R pour
  les competences, mais AUCUN endroit pour les decouvrir sans lire le code. Liste groupee
  par contexte (Bataille / Exploration / Vue cite), texte seul (aucun changement de
  gameplay), bouton Retour ancre en bas (MenuButtonRect(0) aurait ete recouvert par la
  liste, plus haute que le menu reglages standard). Pas de systeme de groupes de controle
  (Ctrl+1..9) trouve dans le code — mentionne comme convention standard mais PAS ajoute
  (fonctionnalite gameplay, pas juste un ecran d'info, donc hors scope de ce passage).
  N'a pas non plus ete compile/teste (meme reserve que d'habitude).

- MESSAGE DE FIN DE DEMO explicite sur l'ecran de resume, quand c'est reellement la fin
  (victoire totale phase 3, OU defaite non recuperable) — PAS sur l'echec de defense
  recuperable (bSummaryCanReturnToCity), qui relance la boucle et n'est pas une fin. Avant :
  meme bandeau generique "Resume de la bataille" que les resumes intermediaires (Kraken
  vaincu, rivale repoussee), aucune distinction claire du moment ou la demo se termine
  vraiment. Trouve en comparant aux conventions des demos indépendantes (Steam/itch.io
  marquent presque toujours ce moment). Changement minimal et sans risque : reutilise
  exactement le meme bloc de rendu existant (texte noir gras multi-passes déjà centre
  dynamiquement), seul le CONTENU du texte change selon bSummaryIsFinal && 
  !bSummaryCanReturnToCity — aucune nouvelle geometrie/zone cliquable.
- Verifie par la meme occasion : les numeros de degats flottants (WOTOLDamageNumber) existent
  deja (convention standard du genre, feedback de combat lisible) — pas un manque.

- CONFIRMATION avant Recommencer/Quitter (menu reglages). Avant : les deux boutons
  executaient IMMEDIATEMENT au clic (Recommencer relancait le niveau, Quitter fermait le
  jeu), aucun garde-fou contre un mis-clic — risque reel de perdre toute la progression
  d'une partie en cours. Convention quasi universelle (tous les jeux confirment les actions
  destructives). Nouvelle boite "Oui / Annuler" (PlayerController::PendingConfirmAction,
  0=aucune/1=recommencer/2=quitter — un seul entier, un seul a la fois) qui intercepte le
  clic AVANT tout le reste du menu reglages tant qu'elle est ouverte. Change minimal et
  sans risque : aucune des actions existantes n'a change de comportement, juste un clic de
  plus avant qu'elles s'executent.

- Rappel de commandes camera AJOUTE a la vue cite (bandeau haut, sous ARMEE). Recherche
  web (session du 19/07/2026) : le probleme #1 releve par les playtests de demos indes est
  l'onboarding/comprehension de la boucle dans les 10 premieres minutes. Verifie par
  coherence interne : TOUS les autres ecrans pilotes camera de WOTOL ont deja un rappel
  (panneau "CONTROLES" en preparation de bataille, bandeau bas en exploration) — la vue
  cite (ajoutee cette session) etait la seule exception, sans aucun indice que la camera
  isometrique peut etre deplacee/zoomee. Une seule ligne de texte, pas de nouvelle geometrie
  cliquable.

## Idees de Liamor pour APRES la demo (meta-progression, hors scope actuel)

Notees telles quelles pour ne rien perdre, mais PAS a implementer a l'aveugle - ce sont de
vrais systemes de conception qui meritent une vraie session de design, pas un ajout ponctuel :

- **Formations de combat entre unites** (image donnee : tactiques foot 4-4-2 / 5-3-1...) :
  des formations/dispositions debloquees progressivement, dependantes des types d'unites
  deja debloquees (une formation utilisant les Akisferes n'est possible qu'une fois le
  batiment distance construit, etc.).
- **Progression/niveau des unites** avec choix de competences a debloquer par unite au fil
  de la partie (au-dela du simple Axe 1/Axe 2 deja prevu pour les competences de base).
- Les deux doivent evoluer "au meme rythme" que la progression du joueur (batiments,
  experience du heros) - donc lies au systeme de progression de la cite deja en place
  (UDemoFlowSubsystem::BuildingLevels, GetBuildingLevel/UpgradeBuilding).

A rediscuter avec Liamor avant de coder quoi que ce soit ici : portee, nombre de formations
pour la demo, quelles unites/batiments debloquent quoi.
