BIBLIOTHEQUE AUDIO — WOTOL (demo)
==================================

Depose ici tes fichiers son (importe-les en .uasset via le Content Browser :
glisser-deposer un .wav/.mp3 dans le dossier -> UE cree un asset SoundWave).

Ensuite, pour que le son soit JOUE en jeu, il faut l'ASSIGNER a la bonne
propriete dans le Blueprint du DemoDirector (BP base sur AWOTOLDemoDirector) :

  MUSIQUE (dossier Music/) — 3 pistes pilotees par l'ECRAN, en boucle. Elles ne
  redemarrent PAS quand on passe d'un ecran a un autre partageant la meme piste.
  Nomme tes fichiers EXACTEMENT ainsi (ou assigne-les sur le DemoDirector) et ca
  se declenche tout seul, AUCUN Blueprint :

    - MenuMusic     -> Menu principal + Choix de faction/difficulte
    - BattleMusic   -> Placement des unites + Bataille (boucle jusqu'a la fin)
    - SummaryMusic  -> Resume de bataille + ecran de transition

  Enchainement automatique par phase :
    Menu/Faction (MenuMusic) -> Placement+Combat (BattleMusic) -> Resume+
    Transition (SummaryMusic) -> Placement+Combat phase 2 (BattleMusic) -> ...
    Retour menu principal -> MenuMusic (la boucle repart).

  SFX/    effets ponctuels :
    - UI/         clics de boutons, navigation menus
    - Combat/     coups, impacts, morts
    - Abilities/  competences (rayons, ondes de choc, souffle...)
    - Buildings/  batiments (Cristalliseur, etc.)

  Ambience/  boucles d'ambiance sous-marine (fond, courant, bulles)
  Voice/     voix / cris des commandants (optionnel)

NB : les noms de fichiers sont libres ; c'est l'ASSIGNATION dans le Blueprint
qui compte (sauf conventions futures). Garde des noms clairs (ex :
Music_Battle_Loop, SFX_Aquis_Shockwave).
