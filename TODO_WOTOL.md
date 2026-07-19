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
  Pas de clic-pour-recentrer pour l'instant (amelioration possible).
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
- Cinematique/texte d'intro anime + lore de faction au clic (choix faction/difficulte).
- Ecran de personnalisation du heros avant le lancement.
- Vue cite : confirmer le mode isometrique fixe + zoom/fiche technique au clic batiment
  (actuellement DrawCityView existe mais a verifier en jeu contre cette description precise).
- La zone d'exploration reste dans la MEME arene que la bataille (pas de vraie zone dediee
  de 70 m²) : les nouveaux reperes aident a la sensation d'exploration mais n'ajoutent pas
  un espace physiquement plus grand a parcourir. A revisiter si le rythme parait encore trop
  court une fois teste en jeu.
- Le repli generique donne une ability FONCTIONNELLE (degats) mais pas fidele au design
  (cone/aura/zone du GDD) : Axe 1/Axe 2, formes de zone, feedback visuel de competence
  restent a faire (le "onglet Competences" est deja note comme travail futur ailleurs
  dans les docs).
- Bâtiments de defense (tourelles) : verifier qu'elles utilisent bien ce meme systeme
  d'ability une fois en jeu, ou si elles ont leur propre logique de tir (WOTOLDefenseStructure).

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
