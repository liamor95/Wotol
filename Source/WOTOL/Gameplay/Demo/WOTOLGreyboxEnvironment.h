#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/WOTOLTypes.h"
#include "WOTOLGreyboxEnvironment.generated.h"

class AStaticMeshActor;

// ─────────────────────────────────────────────────────────────────────────────
// DÉCOR GREYBOX CONTEXTUALISÉ (section 3) — construit par code une arène lisible :
//   - un sol (fond marin) ;
//   - une arche/ruine centrale servant de repère ;
//   - deux zones de déploiement colorées (joueur vs rival).
// Formes primitives Unreal uniquement, aucun asset externe. Présentable en réunion.
// ─────────────────────────────────────────────────────────────────────────────
UCLASS()
class WOTOL_API AWOTOLGreyboxEnvironment : public AActor
{
	GENERATED_BODY()

public:
	AWOTOLGreyboxEnvironment();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Greybox")
	EFactionID PlayerFaction = EFactionID::Aquiloris;

	// Demi-écart entre les deux zones de déploiement
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Greybox")
	float ArmySeparation = 2500.f;

	UFUNCTION(BlueprintCallable, Category = "Greybox")
	void BuildArena();

protected:
	virtual void BeginPlay() override;

private:
	AStaticMeshActor* SpawnBlock(const TCHAR* MeshPath, const FVector& Loc,
		const FVector& Scale, const FLinearColor& Color);
};
