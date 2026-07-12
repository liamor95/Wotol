BIBLIOTHEQUE AUDIO — WOTOL (demo)
==================================

Depose ici tes fichiers son (importe-les en .uasset via le Content Browser :
glisser-deposer un .wav/.mp3 dans le dossier -> UE cree un asset SoundWave).

Ensuite, pour que le son soit JOUE en jeu, il faut l'ASSIGNER a la bonne
propriete dans le Blueprint du DemoDirector (BP base sur AWOTOLDemoDirector) :

  MUSIQUE (dossier Music/) — proprietes du DemoDirector :
    - PreparationMusic  -> musique pendant le PLACEMENT (boucle)
    - BattleMusic       -> musique pendant le COMBAT (boucle)
    - VictoryMusic      -> jingle de VICTOIRE (one-shot)
    - DefeatMusic       -> jingle de DEFAITE (one-shot)
    (Une musique de menu peut aussi etre posee ici.)

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
