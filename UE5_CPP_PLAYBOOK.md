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

### C3 — Nom qui masque un membre d'AActor (paramètre OU variable membre)
- **Symptôme** : `... cannot be defined ... already defined in scope 'AActor' (shadowing is not allowed)`.
- **Cause** : un **paramètre de UFUNCTION** OU une **UPROPERTY membre** porte le nom d'un membre hérité.
  Noms piégeux : `Instigator`, `Owner`, `Role`, **`OnDestroyed`** (délégué déjà sur AActor),
  `InputComponent`, `Children`, `Tags`…
- **Correction** : renommer (`InstigatorUnit`, `OwnerActor`, `UnitRole`, `OnCaptureDestroyed`…).
- ⚠️ Vérifier en particulier les **délégués** UPROPERTY : `OnDestroyed` existe déjà sur AActor.

### C4 — Enum/struct utilisé sans include
- **Symptôme** : type non déclaré (`EFactionID` undeclared, etc.).
- **Cause** : forward declaration insuffisante pour un enum/struct utilisé par valeur.
- **Correction** : `#include` du header qui définit le type (ex. `"Data/WOTOLTypes.h"`).

### C5 — Accès à un membre sur un type incomplet (forward-declared)
- **Symptôme** : `error C2027: use of undefined type 'UXxx'` en accédant à `Ptr->Membre`.
- **Cause** : la classe n'est que forward-declared (`class UXxx;`), le `.cpp` doit voir sa définition.
- **Correction** : `#include` du header de la classe dans le `.cpp` (ex. `#include "UnitDataAsset.h"`
  dès qu'on fait `GetUnitData()->Stats.X`). Pareil pour `GetCharacterMovement()->X`
  → `#include "GameFramework/CharacterMovementComponent.h"`.

### C6 — Lambda sur un délégué DYNAMIC
- **Symptôme** : `error C2039: 'AddWeakLambda' (ou 'AddLambda') is not a member of 'FOnXxx'`.
- **Cause** : un `DECLARE_DYNAMIC_MULTICAST_DELEGATE` n'accepte PAS de lambda — uniquement `AddDynamic`.
- **Correction** : créer une `UFUNCTION() void Handler(...)` et faire `Delegate.AddDynamic(this, &Class::Handler)`.
  (Les lambdas ne marchent que sur les délégués NON-dynamiques : `DECLARE_MULTICAST_DELEGATE`.)

### C7bis — Variable LOCALE qui masque un membre de classe (C4458)
- **Symptôme** : `error C4458: declaration of 'X' hides class member` (bloquant : -WarningsAsErrors).
- **Cause** : une variable locale porte le nom d'un membre hérité. Pièges classiques sur
  AActor/ACharacter : **`Role`** (AActor::Role), **`Mesh`** (ACharacter::Mesh), `Owner`, `Children`.
- **Correction** : renommer la locale (`UnitRole`, `LoadedMesh`…). Vrai même pour des
  variables temporaires anodines (`UStaticMesh* Mesh`, `const EUnitRole Role`).

### C7 — Valeur d'enum inexistante
- **Symptôme** : `error C2065: 'Ground': undeclared identifier` sur `EMonEnum::Ground`.
- **Cause** : la valeur n'existe pas dans l'enum (ex. `EVerticalLayer` n'a pas `Ground`, mais `Epipelagique`).
- **Correction** : toujours vérifier les valeurs réelles de l'enum dans `WOTOLTypes.h` avant usage.

### ⚡ Astuce diagnostic (gain de temps majeur)
- Quand la compilation échoue, **copier-coller le TEXTE du log** (fichier `Saved/Logs/WOTOL.log`)
  plutôt qu'une photo : le texte montre TOUTES les erreurs d'un coup → correction groupée en 1 passe
  au lieu d'itérer une erreur à la fois (chaque recompilation coûtant plusieurs minutes).

### C8 — Propriété pilotant `BeginPlay` posée trop tard (spawn différé)
- **Symptôme** : la construction visuelle/logique faite dans `BeginPlay` ignore un flag qu'on
  pensait avoir réglé (ex. le boss se construit avec la mauvaise silhouette).
- **Cause** : avec un spawn normal, `BeginPlay` s'exécute AVANT qu'on ait réglé la propriété
  depuis l'appelant. Ex. `Unit = Spawn(...); Unit->bIsBoss = true;` → trop tard.
- **Correction** : utiliser `SpawnActorDeferred<>()` → régler les propriétés → `FinishSpawningActor()`.
  `BeginPlay` se déclenche à `FinishSpawningActor`, donc toutes les propriétés posées avant sont vues.
- **Prévention** : toute propriété lue par `BeginPlay`/`OnConstruction` doit être posée entre le
  `SpawnActorDeferred` et le `FinishSpawningActor`.

---

## F. Greybox procédural (kitbash de primitives, animation par code)

### F1 — Texte (`UTextRenderComponent`) attaché à un mesh mis à l'échelle
- **Symptôme** : l'étiquette de nom/PV est **démesurée** et **projetée très haut** ; si elle
  pivote vers la caméra elle apparaît **cisaillée/déformée**.
- **Cause** : le texte hérite de l'échelle du parent. Une échelle **non-uniforme** (ex. `(3,3,4)`)
  multiplie la taille ET le décalage Z, et introduit du **shear** dès qu'on applique une rotation
  monde (le scale non-uniforme et la rotation ne commutent pas).
- **Correction** : donner à l'acteur une **racine `USceneComponent` NON mise à l'échelle**, puis
  attacher le mesh (agrandi) ET les textes **séparément** à cette racine. Le texte garde alors sa
  vraie taille et son vrai offset, sans déformation.
- **Prévention** : ne jamais attacher un `UTextRenderComponent` directement sous un mesh scalé.

### F2 — Verticalité = décalage VISUEL, pas physique
- **Contexte** : forcer la position Z réelle (capsule) d'un `ACharacter` en hauteur **casse** le
  suivi de chemin navmesh (l'unité devient passive) et la portée d'attaque.
- **Solution retenue** : un conteneur `VisualRoot` (`USceneComponent`) porte tout le visuel ; on
  interpole son offset Z (`CurLayer`). Le **corps physique reste au sol** → déplacement + attaque 2D
  fiables à toute « hauteur ».
- **Corollaires** :
  - **Clic sur une unité en hauteur** : mettre un `USphereComponent` « ClickProxy » sur `VisualRoot`
    (QueryOnly, bloque `ECC_Pawn`) ; la capsule au sol peut ignorer `ECC_Pawn`.
  - **Marqueur d'équipe au sol** : l'attacher à `VisualRoot` pour qu'il **monte avec la couche**
    (sinon il reste collé au sol et empêche de superposer les unités).
  - **Ne pas se bloquer entre couches** : `Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore)`
    → une unité montée n'est plus bloquée par un obstacle au sol.

### F3 — Animation procédurale sans squelette
- Articulations = `USceneComponent` pivots enfants ; « os » = `UStaticMeshComponent` suspendus.
  On anime en faisant tourner les pivots au Tick (`RInterpTo`). Chaîne (épaule→coude→main,
  ou fouet à 3 segments) = pivots imbriqués. Le déroulé « coup de fouet » = rotation propagée
  avec retard croissant par segment.

### F4 — Muter un `UPrimaryDataAsset` partagé
- **Symptôme potentiel** : booster les stats d'une unité modifie TOUTES les unités du même type.
- **Cause** : `Registry->GetUnitData(Id)` renvoie l'**instance partagée** de l'asset.
- **Règle** : n'accepter la mutation directe que pour une unité **unique** (ex. le boss Kraken,
  seul de son type dans la démo). Sinon, prévoir un override par-instance.

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
