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

Fait cette session (26/07/2026, retours de Liamor sur l'illustration 3D de la cite) :
- Correction de nom : le chef Aquiloris s'appelle **Aquis** (pas "Akis") ; renomme partout
  (code, batiment Aquisferes/unite distance, commentaires, doc install).
- Controle de vitesse de jeu (x1/x1.5/x2) dans l'ecran Reglages, sous le bouton plein ecran :
  AWOTOLPlayerController_Battle::SetGameSpeed + UGameplayStatics::SetGlobalTimeDilation.
- Materialisation VISUELLE de la croissance de la cite avant la phase 3 : jusqu'ici la
  transition (UnlockAll) enchainait directement sur la grande bataille sans jamais repasser
  par la cite -> le joueur ne voyait jamais ses nouveaux batiments Speciale/Mythique, seulement
  le texte de l'interlude. Nouveau retour obligatoire en cite (bReadyForGrandBattleDeparture),
  badge "NOUVEAU !" sur les cartes fraichement debloquees, bouton "EMBARQUER - GRANDE BATAILLE".
- Armee de phase 3 desormais COMPOSEE PAR LE JOUEUR au lieu d'une liste scriptee fixe
  (34/22/30/12 Noxeens, 20/12/18/8 Aquiloris, qui ignorait le recrutement en cite) : base fixe
  reduite (~18 troupes incl. chef+mythique) + recrutement libre jusqu'au plafond par faction
  (60 Aquiloris / 100 Noxeens), reutilisant le systeme ProduceUnit/CanProduce existant. Le
  Mythique reste non-recrutable en serie (creature unique, deja ajoutee automatiquement).
- Fiche technique des batiments : le niveau (attaque+defense, une seule jauge dans ce systeme)
  et la voie tactique choisie (axe de competence) s'affichent maintenant ensemble.
- Confirme via grep : le cout ressource des defenses et l'objectif "10 unites a distance"
  ETAIENT DEJA implementes avant ce message (DemoFlowSubsystem::DefenseInstallCrystalCost/
  CanInstallNextDefense, IsRangedBuildingConstructed/IsDefenseMissionReady) - pas des ajouts
  manquants, juste pas encore vus en jeu faute de compilation.
- Confirme : Mode Ironman / Mode Aleatoire n'ont JAMAIS ete implementes comme fonctionnalites
  (voir entree du 19/07/2026 plus bas) - seulement des champs de donnees INERTES
  (bIronmanMode/bRandomMode) herites de la structure de sauvegarde, jamais lus par le jeu,
  aucune UI ne les expose. Confirmation demandee par Liamor, aucune correction necessaire.
- GRADES INDIVIDUELS par unite (meme categorie = plusieurs stades melanges, pas tous au meme
  niveau) : demande de Liamor pour eviter qu'un bloc entier d'infanterie ne tombe d'un coup
  lors d'un echange de tirs. `AWOTOLDemoDirector::RollUnitGradeFactor(CenterLevel)` tire un
  grade (1/2/3, +15%/grade) autour d'un centre, cote JOUEUR (centre = niveau reel du batiment)
  ET cote RIVAL (centre = niveau de base 1, la rivale n'ayant pas de batiments a ameliorer dans
  cette demo) - sans deplacer la moyenne du groupe, donc sans casser l'equilibrage adaptatif.
  `GrandBattleCasualtyPacingSeconds` ajuste 600s -> 480s (8 min, duree cible demandee pour la
  grande bataille). Le reste de la demande (equilibrage adaptatif reel-temps selon pertes/
  composition, difficulte differenciee y compris au Kraken, pertes minimum garanties meme en
  Facile) etait deja en place (`ConfigureAdaptiveCasualtyTargets`, `EnemyDiffKHP`/`DMG`,
  documente avec les ratios de force R≈1.93/1.40/1.11) - verifie mais pas retouche.

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

- Astuces de l'ecran de CHARGEMENT completees : les 4 astuces existantes ne parlaient QUE de
  tactique de combat, jamais des fonctions du HUD ajoutees cette session (competence R,
  ecran Commandes). Ajoute 2 astuces d'interface au tirage (6 au lieu de 4) — moment ideal
  pour enseigner ca, entre deux phases, pendant un temps mort deja utilise pour du texte.

- COMPARAISON AUX VRAIES MAQUETTES : premiere fois que je regarde reellement
  Content/UI/Reference/Maquettes/ au lieu de deviner. Bonnes surprises : mes libelles
  Heritage (Thalassi/Givrelere/Abysseen/Gardien) et Specialite (Thalassi/Guerrier/Mage/
  Inquisiteur) correspondent EXACTEMENT a UI_PersonnalisationHero.png, sans l'avoir jamais
  vue. La jauge Surface/Mid/Sol existe deja (matche UI_HUD_Complet.png).
  - AJOUTE : ECRAN RECAPITULATIF avant lancement (EDemoScreen::PreGameSummary), conforme a
    UI_ResumePartie.png — la maquette revele un flux en 3 ecrans (Reglage joueur -> Perso
    heros -> RECAPITULATIF avec bouton "LANCER LA PARTIE") alors que je n'en avais code
    que 2 (Confirmer sur l'ecran heros lancait direct). Le bouton "CONFIRMER LE HEROS" ouvre
    maintenant ce recap (nom/faction/difficulte/heritage/specialite/portrait), avec Retour
    (vers personnalisation) et Lancer la partie (StartDemoAfterSelection, comme avant).
  - PAS FAIT (decision explicite de Liamor, question posee) : champ de saisie libre pour le
    nom du heros (la maquette en a un ; necessiterait de capturer le clavier caractere par
    caractere en Canvas HUD sans UMG — plus risque a coder sans compilateur) ; Mode Ironman /
    Mode Aleatoire (presents dans les maquettes Reglage du Joueur/Resume, mais semblent etre
    des fonctions du JEU COMPLET — permadeath, carte procedurale — hors scope d'une demo a
    arene unique) ; controle de vitesse de jeu (icone avance-rapide vue dans UI_HUD_Complet.png,
    absente du code). Ces trois points restent a rediscuter si besoin, pas ecartes definitivement.

- THEME D'INTERFACE DYNAMIQUE PAR FACTION — decision Liamor validee le 22/07/2026 (trouvee en
  auditant Drive + Notion sur demande explicite). "Apres le choix de faction, tous les ecrans
  suivants doivent charger l'identite visuelle de la faction" ; architecture demandee :
  structure centralisee type "Faction UI Theme", jamais de couleurs codees en dur par ecran.
  - Trouve en auditant le code : `FFactionColors::Get()` (WOTOLTypes.h) est DEJA la "source
    de verite unique" documentee dans le fichier lui-meme ("Ne jamais definir ces couleurs
    ailleurs") — mais PAS RESPECTEE : au moins 8 fonctions du HUD (FactionSelect,
    HeroCustomization, PreGameSummary, DrawBuildingBar, CityView, TerritoryView, SkillsView,
    LoadingScreen) redefinissaient CHACUNE leur propre bleu/vert legerement different au lieu
    d'appeler cette fonction -> vraie incoherence de teinte d'un ecran a l'autre, exactement
    ce que la decision de Liamor pointe.
  - FAIT : ajoute `FFactionColors::GetSecondary()` (teinte claire/douce, palette secondaire
    demandee) ; remplace TOUTES les couleurs codees en dur par faction dans WOTOLDemoHUD.cpp
    par des appels a `FFactionColors::Get/GetSecondary()`. Les libelles TEXTE (noms de
    batiments/unites par faction) restent inchanges, seules les COULEURS etaient dupliquees.
  - FAIT : nouvelle fonction `DrawFactionAmbientTint()` — lavis translucide (opacite 0.07)
    dans la teinte de la faction, appele apres le fond sous-marin sur les ecrans qui suivent
    le choix de faction en pur Canvas 2D (personnalisation heros, recapitulatif, competences,
    chargement, resume de bataille, transition entre deux batailles). PAS applique a la Cite/
    Territoire : ce sont des scenes 3D reelles (camera isometrique/bataille), deja teintees par
    faction via les materiaux 3D existants (WOTOLCityEnvironment/WOTOLCityBuildingProp
    utilisent deja FFactionColors) — un lavis 2D par-dessus aurait ete redondant et aurait pu
    ternir la lisibilite du gameplay (barres de vie, boutons).
  - PAS TOUCHE (deliberement) : le fond sous-marin lui-meme (`DrawUnderwaterBackground`,
    degrade bleu partage par ~10 ecrans) n'a pas ete redessine par faction — la consigne de
    Liamor dit explicitement "ne pas inventer les visuels definitifs avant reception des
    ressources" ; recolorer tout le degrade aurait ete inventer une identite visuelle, pas
    juste centraliser une couleur deja decidee. Le lavis ci-dessus est le compromis : donne
    une ambiance par faction sans redessiner le fond.
  - Scope volontairement pas touche (hors periemetre "couleurs deja codees") : materiaux,
    motifs, cadres, fonds d'ecran de chargement dedies par faction — necessitent les assets
    de Liamor, pas inventes ici.

- CONFIRME PAR LIAMOR (22/07/2026) : le scope "cite/recrutement" fait partie de la demo (etait
  une question ouverte dans Notion — desormais tranchee). Deja largement implemente cote code
  (DrawCityView, production par categorie, amelioration de batiment niveau 1-3, cite 3D
  isometrique) : rien de nouveau a coder suite a cette confirmation, ca valide juste de
  continuer a maintenir/completer ce qui existe deja plutot que de le considerer hors-scope.

- AUDIT DES VISUELS AJOUTES PAR LIAMOR (23/07/2026, Drive/WOTOL/Images-Visuels + Ressources et
  batiments) — deux corrections de fond demandees explicitement apres le recap :
  - RESSOURCE "NOURRITURE" REMPLACEE PAR "ENERGIE OCEANIQUE" (PlayerFood/LastRewardFood/
    CreatureRewardFood/DefenseRewardFood -> PlayerOceanicEnergy/LastRewardOceanicEnergy/
    CreatureRewardOceanicEnergy/DefenseRewardOceanicEnergy dans DemoFlowSubsystem +
    WOTOLDemoDirector + WOTOLDemoHUD). "Nourriture" ne correspondait a AUCUNE des 4
    ressources reelles montrees sur les planches (Cristaux/Biolumens propre-faction,
    Materiaux abyssaux, Biomasse Marine, Energie Oceanique) — Biomasse couvrait deja le role
    "alimentation". Energie Oceanique est en plus la SEULE ressource a capacite de stockage
    LIMITEE d'apres sa planche -> ajoute MaxOceanicEnergy (plafond, EditAnywhere, defaut 200)
    et clamp dans GrantMissionRewards (le surplus au-dela du plafond est perdu).
  - NOMS DE BATIMENTS CORRIGES (CityBuildingLabel, WOTOLDemoHUD.cpp) d'apres les planches
    Noxeens (texte+lore complets sur chaque image) et la table Notion validee pour Aquiloris :
    - CERTAIN (categorie de production explicite sur la planche) : Infanterie "Nid Noxeflare"
      -> "Fosse d'Emergence" ; Montee "Antre Noxebeast" -> "Cavite des Mastodontes" ; Mythique
      "Couvain Noxedrake" -> "Antre du Noxedrake".
    - MEILLEURE ESTIMATION (fonction decrite mais categorie non explicite sur la planche, a
      confirmer par Liamor) : Distance "Fosse Noxeblast" -> "Noeud Bioluminescent" (booste
      portee/precision a distance) ; Speciale "Sanctuaire Noxeon" -> "Faille Abyssale"
      (entites rares/dangereuses, amplifie la puissance).
    - Aquiloris "Dome des montures" -> "Dome des Aquilances" (aligne sur la table Notion
      "Batiments Aquiloris (valides)"), les autres noms Aquiloris etaient deja corrects.
  - PAS FAIT : les visuels Aquiloris (Batiments) n'ont AUCUN texte/nom detectable (renders
    purs, contrairement aux planches Noxeens completes) — aucune correction possible cote
    Aquiloris au-dela de l'alignement Notion ci-dessus. Les planches d'armes/equipements
    ajoutees aujourd'hui (ex. "Epee et bouclier des Aquiloryons") sont du concept art de
    reference, pas encore integrees (pas d'import d'assets binaires dans ce passage — texte
    seul, coherent avec "ne pas inventer les visuels definitifs" deja applique au theme
    d'interface).

## Garde en memoire — PAS pour la demo (Drive/Notion, 22/07/2026)

Explicitement hors scope demo par instruction de Liamor ("le reste, on le garde en memoire") :
- **Pirates Abyssaux — roster corrompu** : escouades corrompues multi-origines (5 unites/
  escouade), cohortes par role, mobilite verticale independante par variante (regle deja
  tranchee dans Notion si jamais code un jour). Chef = **Nekryss** (jamais "Necris").
- **Faction Mureniens** : stats/roster incomplets au GDD.
- Identite de deux personnages du roster Pirates (peau bleue, femme rousse pale) : non definie.
- Benchmark mecaniques RTS (Total War, AoM Retold, Company of Heroes, Homeworld 3...) dans
  Notion "WOTOL — Game Design & Developpement" : explicitement "non valide par defaut", a
  confronter au code avant toute decision — recoupe en partie la recherche genre deja faite
  cette session (minimap, controles) mais reste a relire en detail si besoin un jour.
- Mode Ironman, Mode Aleatoire (carte procedurale) : toujours hors scope demo (arene unique) -
  CONFIRME PAS DEMANDES PAR LIAMOR (juste releves dans ses maquettes de reference) ; les champs
  bIronmanMode/bRandomMode (WOTOLTypes.h/WOTOLSaveGame.h) restent des placeholders INERTES,
  jamais lus par aucune logique de jeu, aucune UI ne les expose (reponse au message du
  26/07/2026 demandant confirmation que je ne les avais pas implementes de mon propre chef).
- Controle de vitesse de jeu (x1/x1.5/x2) : FAIT le 26/07/2026 (WOTOLPlayerController_Battle::
  SetGameSpeed + 3 chips dans l'ecran Reglages, SetGlobalTimeDilation).

## Idees de Liamor pour APRES la demo (meta-progression, hors scope actuel)

Notees telles quelles pour ne rien perdre, mais PAS a implementer a l'aveugle - ce sont de
vrais systemes de conception qui meritent une vraie session de design, pas un ajout ponctuel :

- **Formations de combat entre unites** (image donnee : tactiques foot 4-4-2 / 5-3-1...) :
  des formations/dispositions debloquees progressivement, dependantes des types d'unites
  deja debloquees (une formation utilisant les Aquisferes n'est possible qu'une fois le
  batiment distance construit, etc.).
- **Progression/niveau des unites** avec choix de competences a debloquer par unite au fil
  de la partie (au-dela du simple Axe 1/Axe 2 deja prevu pour les competences de base).
- Les deux doivent evoluer "au meme rythme" que la progression du joueur (batiments,
  experience du heros) - donc lies au systeme de progression de la cite deja en place
  (UDemoFlowSubsystem::BuildingLevels, GetBuildingLevel/UpgradeBuilding).

A rediscuter avec Liamor avant de coder quoi que ce soit ici : portee, nombre de formations
pour la demo, quelles unites/batiments debloquent quoi.

## Structures defensives + confirmation economie (23/07/2026, planches "obstacles/ressources" + reponses Liamor)

Liamor a envoye (via chat direct, hors Drive) 32 planches de concept art supplementaires :
fiches de ressources par faction (Cristaux/Biolumens/Miasmes Toxiques/Debris Technologiques/
Corail Vivant), tourelles+remparts par faction (Aquiloris = Tourelle hydrocristalline + Rempart
cristallin, Noxeens = Oeil bioluminal + Entraves abyssales, Mureniens = Puits de miasmes +
Rempart de galeries toxiques, Pirates Abyssaux = Batterie du coeur obscur + Rempart de coque
obscure, Thalassidra = Pointes rocheuses de corail + Rempart corallien), armes/roster complet
Aquiloris/Noxeens (deja coherent avec l'existant), et tout le roster "corrompu" Pirates Abyssaux
(Aquilombres/Piquiers/Sonariens/Aquisnipe/Noxebeast/Tortues Bastions/Murefrondes/Aquilances
Abyssaux + armes + Vaisseau Abyssal) — ce dernier confirme/enrichit le systeme "roster corrompu"
deja note ci-dessus comme hors scope demo ; aucune implementation faite sur cette partie.

**Economie confirmee (question posee puis tranchee par Liamor) :** le modele actuel est correct
tel quel — 3 ressources communes a toutes les factions (nourriture -> Biomasse Marine,
constructions -> Mineraux Abyssaux, competences/recherche -> Energie Oceanique) + 1 ressource
exclusive par faction (Cristaux Aquiloris / Biolumens Noxeens / etc.). Aucun changement de code :
la refonte du 23/07 (Nourriture -> Energie Oceanique) reste valide. Deja ecrit dans le GDD/Notion
selon Liamor — a garder en tete pour ne plus reposer la question.

**Implemente (bâtiments defensifs Aquiloris/Noxeens, scope demo confirme par Liamor) :**
- `WOTOLDefenseStructure.h/.cpp` : renomme les 2 types de tourelle greybox existants avec les
  noms officiels des planches — "Tourelle Aquiloris" -> **Tourelle hydrocristalline**,
  "Sentinelle Noxeenne" -> **Oeil bioluminal** (DisplayName de l'enum + commentaires). Aucun
  changement de mesh/couleur : le greybox bleu-cristal / vert-orbe correspondait deja aux
  planches.
- `WOTOLDemoHUD.cpp` (`DrawTerritoryView`) : le bouton "PLACER TOURELLE" utilise maintenant le
  nom par faction (PLACER TOURELLE HYDROCRISTALLINE / PLACER OEIL BIOLUMINAL). La ligne
  "Integrite : X %" affiche en plus le nom du rempart perimetrique (Rempart cristallin /
  Entraves abyssales) — texte descriptif uniquement, pas de nouveau champ/mecanique : le
  batiment central (Cristalliseur/Abyssalyseur, deja etabli) reste l'entite mecanique unique,
  le "Rempart" n'etant que son identite visuelle de fortification perimetrique.
- PAS FAIT (hors scope) : structures Mureniens/Pirates Abyssaux/Thalassidra (3 factions hors
  demo) — planches gardees en reference pour plus tard.

## Audit Drive complet (25/07/2026) — liste canonique des 22 batiments + resolution finale

Sur demande explicite de Liamor : audit complet du dossier Drive WOTOL (tous sous-dossiers,
fichiers modifies depuis le 23/07). Deux nouveaux documents Drive trouves, crees le 24/07 :

- **"CLAUDE — REFERENCE ACTIVE WOTOL"** (statut canonique au 24/07/2026, ecrit explicitement
  a mon intention) : liste DEFINITIVE des 22 batiments (11 Aquiloris + 11 Noxeens). Remplace
  toute liste anterieure. Regle absolue citee : "ne jamais inventer ni reutiliser un ancien
  nom de batiment".
- **"ChatGPT & CLAUDE"** (document d'echange, sync ChatGPT du 25/07/2026) : confirme la meme
  liste + resout explicitement le point ouvert Distance/Speciale Noxeens : "Foyer des
  Decharges recrute les Noxeblasts et Faille Abyssale recrute les Noxeons".

**FAIT (code) :**
- `CityBuildingLabel()` (WOTOLDemoHUD.cpp) : "Noeud Bioluminescent" (estimation, desormais
  fausse) -> **"Foyer des Decharges"** (Distance Noxeens, CERTAIN). Speciale Noxeens
  ("Faille Abyssale") etait deja juste par coincidence — confirme. Commentaire de la fonction
  mis a jour (retrait des mentions "MEILLEURE ESTIMATION", tout est desormais CERTAIN pour les
  5 categories de recrutement des deux factions).
- `DrawTerritoryView()` : la section defense (tourelle + rempart, deja nommee la session
  precedente) est maintenant rattachee a son batiment canonique — prefixe "BASTION CRISTALLIN"
  (Aquiloris) / "ENCEINTE NOXEENNE" (Noxeens) ajoute devant la ligne "DEFENSES : x/y".

**PAS FAIT (hors scope demo, documente pour reference future) :**
La liste canonique compte 22 batiments au total par faction (11+11), mais le systeme de cite
de la demo ne modelise que 5 categories de RECRUTEMENT (Infanterie/Distance/Montee/Speciale/
Mythique) + le batiment central (Cristalliseur/Abyssalyseur, deja code) + la defense (Bastion
Cristallin/Enceinte Noxeenne, nom ajoute ci-dessus). Les 6 batiments restants de la liste
canonique — batiment-siege (Noyau Cristalin/Trone des profondeurs) et les 3 batiments de
ressources partagees par faction (Aquiloris: Puits des Courants Cristallins=Energie Oceanique,
Carriere des Profondeurs=Mineraux Abyssaux, Bassin de Vie Cristalline=Biomasse ; Noxeens:
Matrice Luminale=Energie Oceanique, Gisement Abyssal=Mineraux Abyssaux, Fosse Nourriciere=
Biomasse) — n'ont PAS de representation dediee dans la demo actuelle (les ressources se
generent directement via les recompenses de mission, sans batiment de production visible).
Ajouter ces 6 batiments serait une extension de scope non demandee — a rediscuter avec Liamor
si souhaite pour la demo, sinon naturellement dans le jeu complet.

**Autres elements trouves dans l'audit (non actionnables cote code) :**
- Nouveau GDD "WOTOL_GDD_v3_Complet — REFERENCE ACTIVE" (cree 24/07) : GDD complet du jeu fini
  (4 couches oceaniques, lore Nekryss/Aetheriens, twist de l'artefact...) — hors scope demo,
  deja largement coherent avec le dossier maitre Notion existant.
- Une "Demande a Claude" du 18/07/2026 (doc "ChatGPT & CLAUDE") reste sans reponse : confronter
  le benchmark RTS (Total War, AoM Retold, CoH3, Homeworld 3, XCOM2, SupCom...) au GDD/code/etat
  reel de la demo, item par item. Tache de DOCUMENTATION uniquement (pas de code), a faire dans
  une prochaine session si Liamor le souhaite — volontairement pas traitee dans cette passe pour
  rester focalise sur l'audit Drive demande.
- Tache de production visuelle demandee a ChatGPT (image du Kraken 3/4 face, fond neutre) : hors
  perimetre Claude Code, pas d'action de ma part.

## Reponse au benchmark RTS du 18/07/2026 (25/07/2026, demande explicite de Liamor)

Confrontation du benchmark RTS de ChatGPT (8 piliers) au code reel de `claude/wotol-demo-finale`
(pas seulement au GDD). Reponse complete et signee publiee dans le document Drive "ChatGPT &
CLAUDE — mis a jour 25-07-2026 (v2, benchmark RTS)". Aucun code modifie par cette recherche.

**Trouvaille principale : `UFormationComponent` (Source/WOTOL/Gameplay/Units/FormationComponent.h/.cpp)
est du CODE MORT.** Systeme de formations complet (Line/Wedge/DefensiveSquare/Loose/Column,
bonus DEF/vitesse/reduction AoE par formation, exactement l'esprit Total War/SupCom recommande
par le benchmark) mais jamais instancie ni reference nulle part ailleurs dans le depot — verifie
par grep, aucun hit hors du fichier lui-meme. C'est le gain le plus net a faible risque identifie
par cette revue : le brancher (type de formation par defaut par categorie d'unite + un bouton
HUD pour changer) serait un ajout, pas une modification de logique existante.

**Resume par pilier (detail complet dans le doc Drive) :**
- Commandement intelligent : DEJA PRESENT, le plus abouti (attack-move, formation compacte
  calculee au clic, ciblage vertical auto vers la couche de la cible, focus-fire touche R).
- Verticalite lisible : PARTIEL (jauge HUD 3 bandes sur un DesiredZ continu 0-2400, changement
  par 2 boutons +/-600, pas de selection 3D directe ni de trajectoire previsualisee pour les
  troupes — le ghost preview existant ne sert qu'au placement du Cristalliseur).
- Asymetrie encadree : architecture deja bonne (commandes communes + identites differenciees),
  equilibrage reel non verifiable sans playtest.
- Progression par nouveaux verbes : PARTIEL/ABSENT — voir FormationComponent ci-dessus. Le
  systeme Axe1/Axe2 existe dans les donnees mais l'execution reste un repli generique unique
  (deja note ailleurs dans ce fichier).
- Chefs/mythiques controlables : PARTIEL/ABSENT (cooldown seul sur les abilities, pas de cout
  ressource/telegraphe visuel/fenetre d'esquive ; le Kraken n'a pas de phases scriptees, juste
  un equilibrage adaptatif des pertes via ConfigureAdaptiveCasualtyTargets).
- Territoires sans snowball : TerritoryGrade/BuildingLevels s'accumulent sans reset — pattern a
  surveiller pour le jeu complet, non problematique pour la demo (une seule progression lineaire).
- Defense adaptative : ABSENT — SpawnRivalSquad/SpawnEnemyForCreature font apparaitre l'armee
  ennemie depuis UN SEUL point/axe (Origin+Facing), exactement le chokepoint deconseille par le
  benchmark (reference They Are Billions).
- Variete controlee : PARTIEL, meilleur que prevu cote resultat (vrai systeme de seed par
  tentative deja en code : AdaptiveEncounterSeed/EncounterRandom/TacticalVariant/
  AdaptiveEncounterVariance, confirme la note "±5 selon la tentative"), absent cote objectifs
  (normal pour une demo a arene unique, deja confirme hors scope).
- Faisabilite UE5.8 : pas de StateTree utilise ; RVO Avoidance reellement actif
  (bUseRVOAvoidance) ; NavMesh standard + verticalite simulee par un flottant Z separe
  (AdaptLayerTo) — deja l'architecture "prudente" que le benchmark recommandait sans jamais
  l'avoir formulee ainsi (evite une navigation volumetrique complete).

**Perimetre minimal propose (PAS implemente, attend confirmation de Liamor) :**
1. Brancher le FormationComponent existant — seul gain a faible risque identifie.
2. Ne rien changer au Kraken/a la defense mono-axe/aux territoires avant le premier playtest
   UE5.8 — les cibles de pertes documentees sont calibrees sur l'architecture actuelle, un
   changement non mesure serait un double changement.

## Choix Aquis / Aquira a la personnalisation du heros (25/07/2026, demande de Liamor)

Demande explicite : les joueuses/joueurs Aquiloris peuvent choisir d'incarner soit **Aquis**
(chef historique) soit **Aquira** (nouvelle reine, nom trouve avec Liamor via AskUserQuestion),
avec des statistiques/capacites strictement identiques. Celui des deux qui n'est PAS choisi
devient le chef de la faction en narration/PNJ (a qui Aquis devient chef si Aquira est choisie,
et inversement). Les Noxeens ne sont pas concernes : Noxar reste le seul chef jouable.

**Implemente :**
- `FHeroLoadout::bPlayAsAquira` (WOTOLTypes.h) : nouveau champ, Aquiloris uniquement.
- `UDemoFlowSubsystem::SetHeroPlayAsAquira()` + `RefreshHeroName()` (prive) : le nom du heros
  affiche (`HeroLoadout.HeroName`, jusque-la toujours le placeholder generique "Aquilian" qui
  ne correspondait a aucun nom etabli du GDD) se calcule desormais automatiquement : "Noxar"
  pour les Noxeens, "Aquis" ou "Aquira" pour les Aquiloris selon le choix. Rafraichi a la fois
  au choix de faction (`SetSelectedFaction`) et au choix Aquis/Aquira.
- `AWOTOLDemoHUD::DrawHeroCustomization` : nouveau bloc "INCARNATION" (2 boutons AQUIS/AQUIRA)
  au-dessus de HERITAGE, visible uniquement quand `SelectedFaction == Aquiloris`. Texte
  descriptif rappelant qui devient chef de faction en PNJ selon le choix.
- `AWOTOLPlayerController_Battle::HandleUIClick` : gestion du clic sur les 2 nouveaux boutons.
- Le recapitulatif avant lancement (deja existant, `Row(ColL, "HEROS", Loadout.HeroName)`)
  affiche donc maintenant automatiquement "Aquis"/"Aquira"/"Noxar" au lieu du placeholder.

**PAS FAIT (deliberement, pour rester dans le perimetre demande) :**
- Le nom affiche pour l'unite "Chef" recrutable en bataille (UnitDataLibrary.cpp,
  `DisplayName = "Aquis"`) reste fixe a "Aquis" quel que soit le choix — ce DataAsset represente
  l'unite RTS recrutable (stats identiques dans les deux cas), pas le heros d'exploration.
  Si Liamor veut que ce nom suive aussi le choix Aquira, il faudra rendre son affichage
  dynamique partout ou il apparait en jeu (tooltips, fiche technique...) — pas fait ici, hors
  du perimetre explicitement demande (ecran de personnalisation uniquement).
- Aucun changement visuel/mesh : Aquira reutilise le meme portrait cyclable generique
  (halo teinte par la faction) que le reste du systeme de personnalisation, en attendant de
  vrais portraits illustres.

## Personnalisation modulaire du portrait du heros (26/07/2026, question de Liamor)

Question posee : peut-on swapper independamment des traits visuels du portrait (ex. pointes
sur la tete, forme du nez) tout en gardant le reste du design, plutot que de cycler entre
plusieurs portraits complets ? Reponse (pas de code a faire tant que non demande explicitement -
Liamor a lui-meme propose "sinon on laisse tomber") :
- Cycler entre PLUSIEURS PORTRAITS COMPLETS (image entiere par entiere) est deja simple avec le
  systeme existant (`PortraitIndex`, meme pattern de chargement PNG que ce qui a servi cette
  session pour les batiments/fonds) : chaque portrait est juste un fichier de plus dans
  `Content/UI/`, aucun obstacle technique.
- Le vrai swap MODULAIRE (changer le nez SANS toucher au reste) demande que l'art SOURCE soit
  deja prepare en CALQUES separes et alignes (un fichier transparent par trait, tous calibres
  sur le meme cadrage/la meme echelle) - c'est un travail de PRODUCTION D'ART, pas de code : le
  moteur peut superposer des calques PNG (meme systeme WOTOLBuildingArt::GetXxx), mais il ne
  peut pas decouper/isoler un trait a partir d'une seule image finie et deja assemblee.
- Si Liamor veut poursuivre : il faudrait qu'il fournisse (ou fasse fournir) les traits en
  calques separes (tete/nez/pointes/etc., transparents, alignes) plutot que des portraits deja
  finalises - a rediscuter s'il envoie ces images plus tard.

## Silhouettes de biome par faction sur les ecrans 2D (25/07/2026, demande de Liamor)

Demande explicite : les ecrans ne doivent plus paraitre vides/neutres une fois la faction
choisie — il faut un fond qui rappelle l'identite/le biome de la faction, stylise et
harmonieux, sans surcharger. Images envoyees par Liamor comme EXEMPLES d'intention
uniquement (mockups IA tres polis, pas des assets finaux WOTOL — explicitement dit par
Liamor : "c'est pas definitif, c'est des exemples pour imager mes propos").

**Contrainte technique assumee et expliquee :** aucun outil d'import d'image/texture n'est
disponible dans cet environnement (pas d'editeur Unreal), et le HUD est 100% Canvas (pas
d'UMG). Impossible donc de reproduire des illustrations peintes comme les exemples envoyes.
Solution retenue : silhouettes 100% procedurales (formes geometriques via Canvas, memes
primitives que `DrawUnderwaterBackground` deja en place : K2_DrawPolygon/K2_DrawLine/DrawRect),
concentrees sur les bords/coins bas de l'ecran pour ne jamais empieter sur le texte/les
boutons centraux. Reste un habillage STYLISE, a remplacer par de vrais fonds/materiaux une
fois les assets definitifs de Liamor recus et valides (meme logique que le reste du theme
UI par faction deja en place).

**Implemente :** nouvelle fonction statique `DrawFactionBiomeSilhouette()` (WOTOLDemoHUD.cpp),
appelee automatiquement depuis `DrawFactionAmbientTint()` (donc sur les 6 ecrans qui
l'appellent deja : HeroCustomization, PreGameSummary, SkillsView, LoadingScreen, Summary,
Interlude — aucun nouveau site d'appel necessaire) :
- Aquiloris : fleches de cristal dressees en eventail aux deux coins bas + eclats scintillants
  disperses qui pulsent doucement, teintes via FFactionColors (source de verite deja en place).
- Noxeens : amas sombres bioluminescents aux coins bas + spores qui pulsent + tentacules
  filiformes qui ondulent depuis le bas de l'ecran.
- Hors scope demo (Thalassidra/Mureniens/Pirates Abyssaux) : fonction sans effet pour ces
  factions, pas de motif invente pour elles sans plus d'infos de Liamor.

**PAS FAIT (deliberement) :** l'ecran de choix de faction (DrawFactionSelect) n'a pas ete
touche — aucune faction n'y est "choisie" avant le clic final, et il n'existe pas de suivi de
survol (hover) dans le code actuel pour changer le fond en fonction de la carte survolee comme
dans les exemples envoyes (carrousel 5 factions). Les vues Cite/Territoire (scenes 3D reelles)
restent hors scope de cet ajout, deja teintees par des materiaux 3D existants.

## Vrais fonds de biome par faction integres (25/07/2026, images officielles de Liamor)

Liamor a confirme que le mecanisme decouvert pour MainMenuBG.png (PNG charge directement
depuis le disque via FImageUtils::ImportFileAsTexture2D, sans import manuel dans l'editeur)
peut etre reutilise pour d'autres fonds, et a fourni des images officielles (pas des
exemples cette fois) : planches de batiments Noxeens (confirment a nouveau les 11 noms
canoniques deja en code, rien de nouveau a corriger), deux fonds de biome, l'embleme officiel
Noxeens (lettre "N" bioluminescente).

**Implemente :**
- `AWOTOLDemoHUD::GetFactionBackground(EFactionID)` (meme pattern que GetMenuBackground/
  GetTransitionBackground) : charge `Content/UI/BackgroundAquiloris.png` /
  `BackgroundNoxeens.png` depuis le disque, mis en cache.
- `DrawFactionAmbientTint()` : utilise maintenant la vraie image (melangee a 55% d'opacite
  par-dessus le dégradé procedural existant, pour garder la vignette de lisibilite de
  DrawUnderwaterBackground) quand le fichier existe, sinon repli sur les silhouettes
  procedurales ajoutees plus tot dans la session. Memes 6 ecrans concernes (HeroCustomization,
  PreGameSummary, SkillsView, LoadingScreen, Summary, Interlude).
- Fichiers ajoutes : `Content/UI/BackgroundAquiloris.png` (grotte de cristaux bleus, image
  fournie par Liamor pour ce role) et `Content/UI/BackgroundNoxeens.png` (grotte
  bioluminescente teal/meduses, fournie par Liamor).

**Recu mais PAS encore integre (a decider avec Liamor) :**
- `Content/UI/EmblemNoxeens.png` : embleme officiel Noxeens ("N" bioluminescent) copie dans
  le projet, mais pas encore branche a un endroit precis du HUD. L'ecran de choix de faction
  (DrawFactionSelect) utilise actuellement des icones procedurales simples pour les 2
  boutons Aquiloris/Noxeens ; remplacer UNIQUEMENT celle des Noxeens par ce vrai embleme
  creerait une incoherence visuelle (un bouton avec une vraie image, l'autre avec une forme
  generique) tant qu'un embleme Aquiloris equivalent n'est pas fourni. A rediscuter.
- Planches de batiments Noxeens envoyees (Enceinte Noxeenne, Trone des profondeurs, Cavite
  des mastodontes, Fosse nourriciere, Fosse d'Emergence, Foyer des decharges, Faille
  Abyssale, Antre du Noxedrake, Abysalyseur, Oeil bioluminal, Entraves abyssales) :
  confirment les noms deja corrects en code (section "Audit Drive complet" plus haut), pas
  d'image de batiment integree dans le jeu pour l'instant (la demo n'affiche pas de rendu de
  batiment illustre, seulement du texte + de la 3D greybox) — a voir si Liamor veut une fiche
  visuelle par batiment quelque part (ex. vue Territoire/Cite).

## Embleme Aquiloris recu, equilibre resolu (25/07/2026)

Liamor a envoye le lot complet des visuels officiels Aquiloris (batiments confirmant a
nouveau les 11 noms canoniques deja en code, armes, personnages Aquiloryon/Aquis, fonds de
biome) — dont l'embleme officiel Aquiloris (bouclier bleu "A"), symetrique a l'embleme
Noxeens ("N") recu precedemment. Le point laisse ouvert dans l'entree precedente
("remplacer un seul bouton par une vraie image creerait une incoherence") est resolu :
les deux emblemes existent maintenant.

**Implemente :**
- `AWOTOLDemoHUD::GetFactionEmblem(EFactionID)` (meme mecanisme que GetFactionBackground) :
  charge `Content/UI/EmblemAquiloris.png` / `EmblemNoxeens.png`.
- `DrawFactionSelect()` : les deux boutons de faction affichent maintenant les vrais
  emblemes officiels au lieu des icones procedurales (triangle cyan / hexagone vert),
  avec repli automatique sur l'ancien procedural si un fichier venait a manquer.

Fichiers ajoutes : `Content/UI/EmblemAquiloris.png`, `Content/UI/EmblemNoxeens.png`.

**Toujours pas fait :** aucune fiche/icone de batiment ou d'unite individuelle integree
dans le jeu (la demo n'affiche que du texte + de la 3D greybox pour les batiments/unites,
pas de rendu illustre) — a voir si Liamor veut ca quelque part precisement (vue Cite ?
Territoire ? fiche technique au clic ?).

## Illustrations reelles des batiments en 3D (vue Cite) + fiche technique (25/07/2026)

Suite a la question de Liamor sur le detourage/trompe-l'oeil : implemente pour la VUE CITE
uniquement (camera isometrique FIXE, ne tourne jamais — verifie dans le code,
AWOTOLCityCamera::FixedYaw/FixedPitch jamais modifies). PAS applique a la Bataille/
Territoire/Exploration : la camera de bataille (AWOTOLBattleCamera) PEUT orbiter librement
(clic milieu + glisser, verifie dans le code) — un plan 2D y paraitrait plat/faux des qu'on
tourne autour, contrairement a la Cite. Ces vues gardent leur kitbash 3D greybox.

**Preparation des images (Python/Pillow, installe pour cette tache) :**
- 14 planches officielles (batiments Aquiloris/Noxeens envoyees par Liamor) recadrees sur
  l'illustration principale (coin haut-gauche de chaque planche) puis detourees par
  suppression de couleur (le fond gris clair uni des planches est rendu transparent —
  PAS un vrai detourage IA/segmentation, une simple soustraction de fond par similarite de
  couleur, suffisante ici vu la propretee des planches). Sauvegardees dans
  `Content/UI/Buildings/Building<Faction><Categorie>.png` (Infanterie/Distance/Montee/
  Speciale/Mythique) + `Building<Faction>Central.png` (Cristalliseur/Abysalyseur) +
  `Building<Faction>Defense.png` (Tourelle hydrocristalline/Oeil bioluminal).
- 2 fonds de cite grand format : `Content/UI/CityBackdropAquiloris.png` (vraie image de cite
  flottante envoyee par Liamor) et `CityBackdropNoxeens.png` (repli sur BackgroundNoxeens.png
  deja utilise en 2D — AUCUNE image de "cite Noxeens" grand format n'a ete fournie ; a
  remplacer si Liamor en envoie une plus tard).
- Correction en cours de route : le premier mapping de fichiers etait faux pour 2 batiments
  Aquiloris (confusion entre "Tourelle hydrocristalline", "Rempart cristallin" et
  "Cristalliseur", 3 planches visuellement proches) — corrige apres verification directe du
  texte de chaque planche avant de finaliser.

**Nouveau materiau partage (`WOTOLGlow::MakeSprite`, WOTOLGlow.h/.cpp) :** unlit, MASQUE
(alpha de la texture = decoupe nette), DEUX FACES (`TwoSided = true` — filet de securite : le
plan reste visible meme si le calcul d'orientation n'est pas parfaitement exact, jamais de
face invisible), parametre "Texture" + scalaire "Brightness" (verrouille/actif).

**Nouveau chargeur partage (`WOTOLBuildingArt.h/.cpp`)** : meme mecanisme que
`AWOTOLDemoHUD::GetMenuBackground` (PNG charge directement depuis le disque, cache), mais
centralise pour etre reutilisable par le HUD 2D ET les acteurs 3D.

**`AWOTOLCityBuildingProp`** : le corps du batiment (`TierMesh`) est maintenant un PLAN texture
avec l'illustration officielle si le fichier existe (`bUsingRealArt`), oriente pour faire face
a la camera isometrique fixe (rotation calculee via `FRotationMatrix::MakeFromZX` a partir des
angles fixes de la camera, avec le +Z du monde comme reference "haut" pour eviter que l'image
tourne sur elle-meme). Le NIVEAU (1/2/3) fait maintenant grandir la TAILLE du plan (pas la
hauteur, une image plate ne peut pas s'etirer sans se deformer) ; verrouille/actif = parametre
"Brightness" du materiau. REPLI AUTOMATIQUE sur l'ancien kitbash (cylindre emissif, comportement
identique a avant) si l'image officielle est absente pour une combinaison faction/categorie.

**`AWOTOLCityEnvironment`** :
- Fond de cite ajoute (meme technique d'orientation que les batiments), pose loin derriere le
  reste du decor.
- Bulles ambiantes ajoutees (`BuildAmbientBubbles`/`TickAmbientBubbles`) : 18 petites spheres
  emissives qui montent en boucle autour de l'anneau, teintees par la faction — demande
  explicite de Liamor de garder la vue "vivante" (mouvement/fremissement) maintenant que les
  batiments sont des illustrations 2D plutot qu'un kitbash anime par nature. Rafraichies
  uniquement pendant EDemoScreen::City (meme garde que le rafraichissement des batiments).

**Fiche technique du HUD (`DrawCityView`)** : affiche maintenant la MEME illustration que le
plan 3D (via `WOTOLBuildingArt::GetBuildingIcon`) en haut du panneau, avant le texte —
cohérence demandée par Liamor (pas d'écran qui contredit ce qui est affiché en 3D). Repli
silencieux (pas d'image) si le fichier est absent, le texte existant reste inchangé.

**Point d'attention explicite pour le prochain retour PC (non verifiable ici) :**
l'orientation du plan billboard (calcul `FRotationMatrix::MakeFromZX`) n'a jamais pu etre
visualisee (pas d'editeur Unreal disponible cote Claude Code) — a verifier en priorite a
l'ouverture de la vue Cite : si l'image parait tournee dans son propre plan (a l'envers, de
travers), ajuster le vecteur de reference "haut" passe a MakeFromZX plutot que de recalculer
completement l'orientation.
