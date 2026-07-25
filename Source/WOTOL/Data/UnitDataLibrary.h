#pragma once

#include "CoreMinimal.h"
#include "Data/WOTOLTypes.h"
#include "Gameplay/Units/UnitDataAsset.h"
#include "UnitDataLibrary.generated.h"

// Bibliothèque statique — contient toutes les stats GDD des 12 unités de démo
// Utilisée par UUnitDataRegistrySubsystem pour peupler les DataAssets en mémoire
// au démarrage du jeu. Liamor n'a rien à saisir manuellement.
UCLASS()
class WOTOL_API UUnitDataLibrary : public UObject
{
	GENERATED_BODY()

public:
	// Retourne les stats complètes d'une unité par son nom canonique
	static UUnitDataAsset* CreateUnitDataAsset(const FName& UnitID, UObject* Outer);

	// Liste de tous les IDs d'unités de démo
	static TArray<FName> GetAllDemoUnitIDs();
};
