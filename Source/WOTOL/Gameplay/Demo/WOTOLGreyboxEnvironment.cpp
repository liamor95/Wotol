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
	const TCHAR* MeshPath, const FVector& Loc, const FVector& Scale, const FLinearColor& Color,
	const FRotator& Rot, bool bBlocking)
{
	UWorld* W = GetWorld();
	if (!W) return nullptr;

	FActorSpawnParameters P;
	P.Owner = this;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AStaticMeshActor* SMA = W->SpawnActor<AStaticMeshActor>(
		AStaticMeshActor::StaticClass(), Loc, Rot, P);
	if (!SMA) return nullptr;

	UStaticMeshComponent* Comp = SMA->GetStaticMeshComponent();
	if (!Comp) return SMA;

	Comp->SetMobility(EComponentMobility::Movable);

	// Décor non bloquant = traversable (les unités ne s'y coincent pas).
	// Le sol reste bloquant pour que les unités tiennent dessus.
	if (!bBlocking)
	{
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetCanEverAffectNavigation(false);
	}

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

	const FRotator NoRot = FRotator::ZeroRotator;

	// SOL = seul élément BLOQUANT (les unités tiennent dessus et s'y déplacent)
	SpawnBlock(TEXT("/Engine/BasicShapes/Plane.Plane"),
		Center + FVector(0.f, 0.f, 0.f), FVector(130.f, 130.f, 1.f), FloorColor, NoRot, true);

	// Tout le reste = DÉCOR traversable (non bloquant) -> les unités ne s'y coincent pas

	// Arche centrale : 2 piliers + linteau (repère visuel) — décalée hors du couloir central
	SpawnBlock(TEXT("/Engine/BasicShapes/Cube.Cube"),
		Center + FVector(0.f, -700.f, 400.f), FVector(1.5f, 1.5f, 8.f), StoneColor, NoRot, false);
	SpawnBlock(TEXT("/Engine/BasicShapes/Cube.Cube"),
		Center + FVector(0.f, 700.f, 400.f), FVector(1.5f, 1.5f, 8.f), StoneColor, NoRot, false);
	SpawnBlock(TEXT("/Engine/BasicShapes/Cube.Cube"),
		Center + FVector(0.f, 0.f, 820.f), FVector(1.5f, 15.f, 1.f), StoneColor, NoRot, false);

	// Reliefs visuels à différentes hauteurs (verticalité décorative)
	SpawnBlock(TEXT("/Engine/BasicShapes/Cube.Cube"),
		Center + FVector(-1100.f, -1100.f, 250.f), FVector(5.f, 5.f, 1.2f), StoneColor, NoRot, false);
	SpawnBlock(TEXT("/Engine/BasicShapes/Cube.Cube"),
		Center + FVector(1100.f, 1100.f, 250.f), FVector(5.f, 5.f, 1.2f), StoneColor, NoRot, false);
	SpawnBlock(TEXT("/Engine/BasicShapes/Cube.Cube"),
		Center + FVector(-1100.f, 1100.f, 120.f), FVector(4.f, 4.f, 0.8f), StoneColor, NoRot, false);
	SpawnBlock(TEXT("/Engine/BasicShapes/Cube.Cube"),
		Center + FVector(1100.f, -1100.f, 120.f), FVector(4.f, 4.f, 0.8f), StoneColor, NoRot, false);

	// Zones de déploiement colorées (planes fins au sol, non bloquantes)
	SpawnBlock(TEXT("/Engine/BasicShapes/Plane.Plane"),
		Center + FVector(-ArmySeparation * 0.5f, 0.f, 5.f), FVector(35.f, 50.f, 1.f), PlayerColor, NoRot, false);
	SpawnBlock(TEXT("/Engine/BasicShapes/Plane.Plane"),
		Center + FVector(ArmySeparation * 0.5f, 0.f, 5.f), FVector(35.f, 50.f, 1.f), RivalColor, NoRot, false);
}
