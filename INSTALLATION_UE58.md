# Installer et lancer la démo WOTOL sous Unreal Engine 5.8

Ce guide sert à ouvrir et compiler le projet **depuis les sources** (pas la démo packagée
`WOTOL.exe` — voir `README_WOTOL_DEMO.txt` pour ça) sur un PC qui n'a jamais eu Unreal Engine.
La branche à récupérer est `claude/wotol-demo-finale`, dépôt `liamor95/Wotol`.

---

## 1. Installer Unreal Engine 5.8

Le projet est verrouillé sur la version **5.8** exactement (`WOTOL.uproject` →
`"EngineAssociation": "5.8"`).

1. Installer **Epic Games Launcher** (epicgames.com/store/download) si pas déjà fait.
2. Se connecter, onglet **Unreal Engine** → **Bibliothèque** → bouton **+** à côté de
   "Versions moteur".
3. Choisir **5.8** dans la liste, cliquer **Installer**.
4. Cocher au minimum les composants : **Starter Content** (pas obligatoire, la démo est
   greybox), **Editor symbols for debugging** (optionnel), et s'assurer que le support
   **Windows** est bien inclus.

## 2. Installer Visual Studio (nécessaire pour compiler le C++)

Le projet est en C++ pur (aucun Blueprint) : il faut un compilateur.

1. Installer **Visual Studio 2022 Community** (gratuit, visualstudio.microsoft.com).
2. Dans l'installeur, cocher la charge de travail **« Développement Desktop en C++ »**,
   puis dans les composants optionnels à droite, cocher **« Développement de jeu avec Unreal
   Engine »** (ajoute automatiquement les bons kits Windows SDK).

## 3. Récupérer le projet

1. Installer **Git** si besoin (git-scm.com).
2. Cloner le dépôt et se placer sur la bonne branche :
   ```
   git clone https://github.com/liamor95/Wotol.git
   cd Wotol
   git checkout claude/wotol-demo-finale
   ```
   (Si le dossier existe déjà, faire `git pull origin claude/wotol-demo-finale` à la place.)

## 4. Premier lancement et compilation

1. **Clic droit** sur `WOTOL.uproject` → **« Generate Visual Studio project files »**.
   (Génère les fichiers `.sln`/`.vcxproj`, pas fournis dans le dépôt — normal, ils sont
   propres à chaque machine.)
2. **Double-clic** sur `WOTOL.uproject`.
   - Si Windows demande avec quel programme l'ouvrir, choisir **Unreal Engine 5.8** (ou
     passer par le Launcher : Bibliothèque → 5.8 → Lancer, puis ouvrir le projet depuis
     l'écran d'accueil).
   - Unreal va détecter que les modules C++ ne sont pas compilés et proposer
     **« Would you like to rebuild them now? »** → **Oui**.
   - Première compilation : peut prendre **10 à 30 minutes** selon la machine (tout le
     module `WOTOL` compile pour la première fois, y compris tout ce qui a été ajouté cette
     session — thème d'interface par faction, choix Aquis/Aquira, illustrations 3D de la Cité,
     etc.).
3. **C'est le moment de vérité** : si des erreurs de compilation apparaissent, c'est la
   toute première fois que ce code est réellement testé (aucun compilateur disponible côté
   Claude Code pendant toute la session) — copier le message d'erreur exact, ça permettra de
   corriger vite.

## 5. Créer le niveau de la démo (une seule fois)

La démo est **100 % procédurale** (tout est généré par le code au lancement) : il faut juste
un niveau vide avec un sol, pas de décor à construire à la main.

1. Menu **File → New Level… → Empty Level**.
2. **File → Save Current Level As…** → naviguer dans `Content/Maps/` (créer le dossier
   `Maps` s'il n'existe pas) → nommer le fichier **`L_Demo`** exactement (les réglages du
   projet pointent déjà vers `/Game/Maps/L_Demo`).
3. Ajouter un sol : dans le panneau **Place Actors**, glisser un **Plane** (ou un Cube aplati)
   assez grand (échelle ~50x50x1) à la position (0,0,0).
4. Ajouter un **Nav Mesh Bounds Volume** (panneau Place Actors → onglet **Volumes**) : le
   redimensionner pour qu'il couvre largement la zone de jeu (au moins 20000x20000x2000
   unités, centré à peu près à l'origine — la démo spawn plusieurs scènes décalées les unes
   des autres : exploration/bataille, cité, etc.).
5. Sauvegarder (**Ctrl+S**).

Le **GameMode** (`WOTOLGameMode_Demo`) est déjà réglé par défaut pour tout le projet
(`Config/DefaultEngine.ini` → `GlobalDefaultGameMode`) : rien à faire dans World Settings.

## 6. Lancer la démo

- Bouton **Play** (barre d'outils, ou Alt+P) pour tester directement dans l'éditeur.
- Pour un test plus fidèle à ce que joueront les gens : petite flèche à côté de Play →
  **Standalone Game** (lance une vraie fenêtre séparée, sans les à-côtés de l'éditeur).
- La démo doit s'assembler seule : écran-titre → choix de faction → personnalisation du héros
  (Aquis/Aquira pour Aquiloris) → récapitulatif → nage/exploration → Kraken → cité → etc.

## 7. Points à vérifier en priorité (jamais testés faute de compilateur)

Dans l'ordre où les problèmes seraient les plus visibles :

1. **La compilation réussit** (étape 4) — le plus gros risque, rien n'a été compilé de toute
   la session.
2. **Vue Cité** : les bâtiments doivent apparaître comme de vraies illustrations (pas des
   cylindres bleus/verts). Si une image semble **tournée bizarrement dans son propre plan**
   (à l'envers, de travers), c'est un réglage d'orientation à ajuster dans
   `WOTOLCityBuildingProp.cpp` / `WOTOLCityEnvironment.cpp` (voir commentaire
   `FRotationMatrix::MakeFromZX` — changer le vecteur de référence "haut").
3. Le reste des points **déjà connus et listés dans `TODO_WOTOL.md`** (section "Priorité au
   prochain retour PC" en haut du fichier) : boucle complète des 13 phases, réglages
   d'équilibrage des pertes par difficulté, écran Réglages, etc.

## En cas de blocage

Si une erreur de compilation ou un comportement inattendu apparaît, noter :
- Le message d'erreur COMPLET (copier-coller, pas une capture d'écran tronquée).
- Le fichier et la ligne indiqués par Visual Studio.

… et le transmettre pour correction rapide.
