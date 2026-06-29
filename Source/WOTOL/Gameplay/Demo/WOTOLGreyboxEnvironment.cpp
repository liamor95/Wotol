#include "WOTOLGreyboxEnvironment.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Data/WOTOLTypes.h"

AWOTOLGreyboxEnvironment::AWOTOLGreyboxEnvironment()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AWOTOLGreyboxEnvironment::BeginPlay()
{
	Super::BeginPlay();
	BuildArena();
}

AStaticMeshActor* AWOTOLGreyboxEnvironment::SpawnBlock(
	const TCHAR* MeshPath, const FVector& Loc, const FVector& Scale, const FLinearColor& Color)
{
	UWorld* W = GetWorld();
	if (!W) return nullptr;

	FActorSpawnParameters P;
	P.Owner = this;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AStaticMeshActor* SMA = W->SpawnActor<AStaticMeshActor>(
		AStaticMeshActor::StaticClass(), Loc, FRotator::ZeroRotator, P);
	if (!SMA) return nullptr;

	UStaticMeshComponent* Comp = SMA->GetStaticMeshComponent();
	if (!Comp) return SMA;

	Comp->SetMobility(EComponentMobility::Movable);

	if (UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, MeshPath))
	{
		Comp->SetStaticMesh(Mesh);
	}
	SMA->SetActorScale3D(Scale);

	if (UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, this))
		{
			MID->SetVectorParameterValue(TEXT("Color"), Color);
			Comp->SetMaterial(0, MID);
		}
	}
	return SMA;
}

void AWOTOLGreyboxEnvironment::BuildArena()
{
	const FVector Center = GetActorLocation();
	const EFactionID Rival = (PlayerFaction == EFactionID::Aquiloris)
		? EFactionID::Noxeens : EFactionID::Aquiloris;

	const FLinearColor FloorColor(0.03f, 0.07f, 0.09f, 1.f); // fond marin sombre
	const FLinearColor StoneColor(0.20f, 0.22f, 0.25f, 1.f); // ruines grises
	// Zones de déploiement = teinte TRÈS sombre de la faction (les unités, vives, ressortent)
	const FLinearColor PlayerColor = FFactionColors::Get(PlayerFaction) * 0.22f;
	const FLinearColor RivalColor  = FFactionColors::Get(Rival) * 0.22f;

	// Sol (plane 1m -> 130m)
	SpawnBlock(TEXT("/Engine/BasicShapes/Plane.Plane"),
		Center + FVector(0.f, 0.f, 0.f), FVector(130.f, 130.f, 1.f), FloorColor);

	// Arche centrale : 2 piliers + linteau (repère visuel)
	SpawnBlock(TEXT("/Engine/BasicShapes/Cube.Cube"),
		Center + FVector(0.f, -400.f, 400.f), FVector(1.5f, 1.5f, 8.f), StoneColor);
	SpawnBlock(TEXT("/Engine/BasicShapes/Cube.Cube"),
		Center + FVector(0.f, 400.f, 400.f), FVector(1.5f, 1.5f, 8.f), StoneColor);
	SpawnBlock(TEXT("/Engine/BasicShapes/Cube.Cube"),
		Center + FVector(0.f, 0.f, 820.f), FVector(1.5f, 9.f, 1.f), StoneColor);

	// Quelques plateaux/reliefs (verticalité)
	SpawnBlock(TEXT("/Engine/BasicShapes/Cube.Cube"),
		Center + FVector(-900.f, -900.f, 150.f), FVector(6.f, 6.f, 3.f), StoneColor);
	SpawnBlock(TEXT("/Engine/BasicShapes/Cube.Cube"),
		Center + FVector(900.f, 900.f, 150.f), FVector(6.f, 6.f, 3.f), StoneColor);

	// Zones de déploiement colorées (planes fins au sol)
	SpawnBlock(TEXT("/Engine/BasicShapes/Plane.Plane"),
		Center + FVector(-ArmySeparation * 0.5f, 0.f, 5.f), FVector(35.f, 50.f, 1.f), PlayerColor);
	SpawnBlock(TEXT("/Engine/BasicShapes/Plane.Plane"),
		Center + FVector(ArmySeparation * 0.5f, 0.f, 5.f), FVector(35.f, 50.f, 1.f), RivalColor);
}
