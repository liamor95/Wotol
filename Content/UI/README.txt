IMAGE D'ACCUEIL DE LA DÉMO
==========================

Fichier : MainMenuBG.png (1536x1024)

POUR L'AFFICHER DANS LE JEU :
1. Ouvrir WOTOL dans l'éditeur Unreal.
2. Dans le Content Browser, aller dans le dossier "UI" (le créer s'il n'existe pas).
3. Glisser-déposer MainMenuBG.png dedans (ou clic droit > Import).
4. L'asset doit s'appeler exactement "MainMenuBG" et être dans /Game/UI/
   -> chemin final : /Game/UI/MainMenuBG

Le code du menu (WOTOLDemoHUD::DrawMainMenu) charge automatiquement
/Game/UI/MainMenuBG.MainMenuBG et l'affiche en plein écran.
Tant que l'asset n'est pas importé, le menu retombe sur le fond procédural
+ titre "WOTOL" en lave (aucune erreur).

L'image contient déjà le titre + le sous-titre : le code ne redessine donc
pas le titre par-dessus quand l'image est présente.
