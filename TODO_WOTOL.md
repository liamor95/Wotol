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

Encore a faire (releve pendant cette session, pas encore code) :
- Cinematique/texte d'intro anime + lore de faction au clic (choix faction/difficulte).
- Ecran de personnalisation du heros avant le lancement.
- Vue cite : confirmer le mode isometrique fixe + zoom/fiche technique au clic batiment
  (actuellement DrawCityView existe mais a verifier en jeu contre cette description precise).
- Marqueur de pose du Cristalliseur : actuellement un anneau lumineux generique
  (CreateCrystalliserPlacementMarkers) ; le silhouette exacte du batiment n'est pas reprise.
