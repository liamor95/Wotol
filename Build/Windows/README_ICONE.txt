ICONE WINDOWS DE L'EXECUTABLE — WOTOL
======================================

Unreal Engine utilise AUTOMATIQUEMENT le fichier suivant comme icone du .exe
Windows packagé :

    Build/Windows/Application.ico

A FAIRE (une seule fois) :
  1. Prends ton fichier  WOTOL_launcher_icon.ico  (multi-resolutions : 256..16).
  2. COPIE-le ici et RENOMME-le exactement :   Application.ico
     -> chemin final :  Build/Windows/Application.ico
  3. Repackage le jeu (build COMPLET) pour que l'icone soit compilee dans le .exe.

NB :
  - Ne recree pas l'icone, utilise ton .ico existant.
  - Si l'ancienne icone persiste apres packaging, c'est le CACHE d'icones Windows
    (vide-le : ie4uinit.exe -show, ou redemarre l'explorateur) — l'icone est bien
    dans le .exe si le fichier ci-dessus existait au moment du build.
