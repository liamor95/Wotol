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

	// Palette fond marin
	const FLinearColor FloorColor(0.04f, 0.08f, 0.09f, 1.f); // fond marin sombre
	const FLinearColor RockColor (0.09f, 0.12f, 0.12f, 1.f); // roches du fond
	const FLinearColor SandColor (0.20f, 0.18f, 0.12f, 1.f); // bancs de sable

	const FRotator NoRot = FRotator::ZeroRotator;
	const TCHAR* Cube  = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* Plane = TEXT("/Engine/BasicShapes/Plane.Plane");

	// SOL agrandi (260m) = seul élément BLOQUANT (les unités tiennent dessus)
	SpawnBlock(Plane, Center, FVector(260.f, 260.f, 1.f), FloorColor, NoRot, true);

	// Bancs de sable (planes plats légèrement au-dessus du sol, décor)
	SpawnBlock(Plane, Center + FVector(-1600.f, 800.f, 3.f),  FVector(40.f, 30.f, 1.f), SandColor, NoRot, false);
	SpawnBlock(Plane, Center + FVector(1700.f, -900.f, 3.f),  FVector(45.f, 28.f, 1.f), SandColor, NoRot, false);
	SpawnBlock(Plane, Center + FVector(0.f, 1800.f, 3.f),     FVector(50.f, 25.f, 1.f), SandColor, NoRot, false);

	// Arche centrale (repère), décalée hors du couloir de combat
	SpawnBlock(Cube, Center + FVector(0.f, -900.f, 450.f), FVector(1.8f, 1.8f, 9.f), RockColor, NoRot, false);
	SpawnBlock(Cube, Center + FVector(0.f, 900.f, 450.f),  FVector(1.8f, 1.8f, 9.f), RockColor, NoRot, false);
	SpawnBlock(Cube, Center + FVector(0.f, 0.f, 920.f),    FVector(1.8f, 19.f, 1.f), RockColor, NoRot, false);

	// Reliefs rocheux variés répartis sur toute la map (verticalité décorative)
	const float P[][4] = {
		{-2200.f, -1600.f, 300.f, 6.f}, { 2200.f,  1600.f, 300.f, 6.f},
		{-2400.f,  1400.f, 180.f, 4.5f},{ 2400.f, -1400.f, 180.f, 4.5f},
		{-1300.f, -2200.f, 220.f, 5.f}, { 1300.f,  2200.f, 220.f, 5.f},
		{ -700.f,  1700.f, 130.f, 3.5f},{  700.f, -1700.f, 130.f, 3.5f},
		{-3000.f,     0.f, 360.f, 7.f}, { 3000.f,     0.f, 360.f, 7.f},
	};
	for (const float* R : P)
	{
		SpawnBlock(Cube, Center + FVector(R[0], R[1], R[2]),
			FVector(R[3], R[3], R[2] / 100.f), RockColor, NoRot, false);
	}

	(void)Rival; // (les zones de déploiement colorées arriveront avec l'écran de déploiement)
}
