# 🎮 PLAYBOOK UE5 / C++ — Profil Liamor (3DÉCORS)

> **But** : mémoire portable des erreurs déjà rencontrées et de leurs corrections.
> **Comment l'utiliser** : déposer ce fichier à la racine de TOUT futur projet (à côté
> du `CLAUDE.md`). Claude Code le lit au démarrage et code en respectant ces règles,
> pour ne JAMAIS refaire les mêmes erreurs — quel que soit le jeu/projet.
>
> **Règle pour Claude** : à chaque nouvelle erreur de build/process rencontrée, ajouter
> une entrée ici (symptôme exact + cause + correction + prévention). Toujours relire ce
> playbook AVANT d'écrire du code C++ Unreal.

---

## A. Environnement & ouverture de projet

### A1 — `.uproject` corrompu (encodage)
- **Symptôme** : `JsonException: '0xFF' is an invalid start of a value` à l'ouverture.
- **Cause** : le `.uproject` a été ouvert/enregistré dans le **Bloc-notes** → converti en UTF-16 (taille du fichier ~doublée, ex. 354 → 764 octets).
- **Correction** : remplacer le `.uproject` par une version **UTF-8 sans BOM**.
- **Prévention** : ❌ **NE JAMAIS ouvrir un `.uproject` dans le Bloc-notes.** Pour le lancer → double-clic (Unreal). Pour l'éditer → éditeur de code qui préserve l'UTF-8.

### A2 — Version de moteur incompatible
- **Symptôme** : fenêtre « select engine version » ou échec de conversion.
- **Cause** : `"EngineAssociation"` ne correspond pas à la version installée.
- **Correction** : mettre la version installée (ex. `"5.7"`) dans le `.uproject`.
- **Prévention** : vérifier la version dans Epic Games Launcher → Library AVANT.

### A3 — Visual Studio sans le bon module
- **Symptôme** : la compilation échoue immédiatement / compilateur introuvable.
- **Cause** : VS2022 installé sans la charge **« Game development with C++ »**.
- **Prévention** : Visual Studio Installer → Modify → cocher **Game development with C++**.

---

## B. Fichiers de build (`Target.cs` / `Build.cs`)

### B1 — `Target.cs` manquants
- **Symptôme** : « Generate project files » échoue, module non reconnu.
- **Cause** : pas de `XXX.Target.cs` + `XXXEditor.Target.cs` dans `Source/`.
- **Correction** : créer les deux fichiers Target.

### B2 — `BuildSettingsVersion.V5` en conflit
- **Symptôme** : `UndefinedIdentifierWarningLevel: Off != Error ... This is not allowed`. Échec en < 2 s.
- **Cause** : `DefaultBuildSettings = BuildSettingsVersion.V5` diffère des défauts de l'éditeur partagé (UE 5.7).
- **Correction** : `DefaultBuildSettings = BuildSettingsVersion.Latest;` dans les deux Target.cs.

### B3 — Include racine du module introuvable
- **Symptôme** : `fatal error C1083: Cannot open include file: 'Data/WOTOLTypes.h'`.
- **Cause** : la racine du module n'est pas dans le chemin d'inclusion → les includes relatifs (`"Data/..."`, `"Gameplay/..."`) échouent.
- **Correction** : dans `XXX.Build.cs` → `PublicIncludePaths.Add(ModuleDirectory);`
- **Prévention** : toujours mettre cette ligne dès la création d'un module à structure en sous-dossiers.

---

## C. UnrealHeaderTool (UHT) — conventions C++ obligatoires

### C1 — Préfixe de classe invalide
- **Symptôme** : `Class 'UXxx' has an invalid Unreal prefix, expecting 'AXxx'`.
- **Cause** : classe dérivée d'un **Acteur** (AActor, ACharacter, AAIController, APawn, AController…) nommée avec `U`.
- **Correction** : préfixe **`A`** pour tout descendant d'Acteur ; **`U`** pour tout descendant d'UObject (subsystem, component, data asset, widget) ; **`F`** struct ; **`E`** enum.
- ⚠️ Piège fréquent : un **AIController** est un Acteur → `AMonController`, pas `UMonController`.

### C2 — UFUNCTION renvoyant un pointeur de struct
- **Symptôme** : `Inappropriate '*' on variable of type 'FXxx', cannot have an exposed pointer to this type`.
- **Cause** : une `UFUNCTION` renvoie `const FMaStruct*`.
- **Correction** : garder la fonction en **C++ pur** (sans macro UFUNCTION) ; pour le Blueprint, exposer une version `bool GetX(FName Id, FMaStruct& Out)` (copie + booléen).

### C3 — Paramètre qui masque un membre d'AActor
- **Symptôme** : `Function parameter: 'Instigator' ... already defined in scope 'AActor' (shadowing is not allowed)`.
- **Cause** : paramètre de `UFUNCTION` nommé comme un membre hérité (`Instigator`, `Owner`, `Role`).
- **Correction** : renommer (`InstigatorUnit`, `OwnerActor`, `UnitRole`…).

### C4 — Enum/struct utilisé sans include
- **Symptôme** : type non déclaré (`EFactionID` undeclared, etc.).
- **Cause** : forward declaration insuffisante pour un enum/struct utilisé par valeur.
- **Correction** : `#include` du header qui définit le type (ex. `"Data/WOTOLTypes.h"`).

---

## D. Process de travail recommandé (récupération du code)

- **❌ À éviter** : re-télécharger le ZIP GitHub à chaque correction → lent, perd la compilation
  (recompilation complète ~15 min à chaque fois), risque de corruption d'encodage.
- **✅ Recommandé** : **GitHub Desktop** → clone une fois, puis bouton **« Pull »** à chaque
  correction. Compilation **incrémentale** (seuls les fichiers modifiés se recompilent = rapide).
- **Conseil de Claude au démarrage de session** : toujours rappeler à Liamor d'utiliser
  GitHub Desktop + Pull plutôt que Download ZIP.

---

## E. Comment Claude doit conseiller Liamor (style d'accompagnement)

- Liamor n'est **pas développeur** : expliquer en français, étape par étape, en disant
  **où cliquer** et en gardant les termes anglais de l'interface + traduction entre parenthèses.
- Anticiper : avant de faire recompiler (long), **auditer proactivement** le code pour
  trouver plusieurs erreurs d'un coup au lieu d'itérer une par une.
- Quand un blocage connu de ce playbook se présente : le signaler explicitement
  (« on a déjà eu ça, voici la correction directe »).
