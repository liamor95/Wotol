#include "WOTOLDemoUnit.h"
#include "Gameplay/Units/UnitDataAsset.h"
#include "Gameplay/AI/AIAdaptiveController.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Data/WOTOLTypes.h"

AWOTOLDemoUnit::AWOTOLDemoUnit()
{
	ShapeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShapeMesh"));
	ShapeMesh->SetupAttachment(RootComponent);
	ShapeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// L'IA RTS possède automatiquement l'unité au spawn
	AIControllerClass = AAIAdaptiveController::StaticClass();
	AutoPossessAI     = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AWOTOLDemoUnit::BeginPlay()
{
	Super::BeginPlay();   // initialise UnitData -> stats, faction, rôle
	BuildGreyboxShape();
}

// Tailles réelles approximatives (mètres) — valeurs du GDD/document de démo
float AWOTOLDemoUnit::GetUnitHeightMeters(FName UnitID)
{
	// Aquiloris
	if (UnitID == TEXT("Aquis"))       return 1.80f;
	if (UnitID == TEXT("Aquiloryons")) return 1.75f;
	if (UnitID == TEXT("Aquilances"))  return 2.00f;
	if (UnitID == TEXT("Aquipheres"))  return 1.70f;
	if (UnitID == TEXT("Aquilombres")) return 1.55f;
	if (UnitID == TEXT("Leviaphenix")) return 4.00f;
	// Noxéens
	if (UnitID == TEXT("Noxar"))       return 1.50f;
	if (UnitID == TEXT("Noxeflare"))   return 1.70f;
	if (UnitID == TEXT("Noxebeast"))   return 2.50f;
	if (UnitID == TEXT("Noxeblast"))   return 1.60f;
	if (UnitID == TEXT("Noxeons"))     return 1.80f;
	if (UnitID == TEXT("Noxedrake"))   return 6.50f;
	return 1.75f; // défaut prototype
}

void AWOTOLDemoUnit::BuildGreyboxShape()
{
	if (!ShapeMesh) return;

	const EUnitRole UnitRole = UnitData ? UnitData->Role : EUnitRole::Infanterie;
	const FName UnitID       = UnitData ? UnitData->GetFName() : NAME_None;
	const float HeightM  = GetUnitHeightMeters(UnitID);
	const float HeightU  = HeightM * 100.f; // mètres -> UE units (cm)

	// Forme primitive selon la catégorie (rôle)
	const TCHAR* MeshPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	FVector Scale(0.5f, 0.5f, 1.f);
	switch (UnitRole)
	{
		case EUnitRole::Chef:        // cylindre haut (silhouette de commandant)
			MeshPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
			Scale = FVector(0.55f, 0.55f, HeightU / 100.f);
			break;
		case EUnitRole::Infanterie:  // cylindre simple
			MeshPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
			Scale = FVector(0.5f, 0.5f, HeightU / 100.f);
			break;
		case EUnitRole::Montee:      // bloc massif et large (monture)
			MeshPath = TEXT("/Engine/BasicShapes/Cube.Cube");
			Scale = FVector(1.8f, 1.0f, HeightU / 100.f);
			break;
		case EUnitRole::Distance:    // cône orienté (direction de tir visible)
			MeshPath = TEXT("/Engine/BasicShapes/Cone.Cone");
			Scale = FVector(0.7f, 0.7f, HeightU / 100.f);
			break;
		case EUnitRole::Speciale:    // cylindre fin/atypique
			MeshPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
			Scale = FVector(0.35f, 0.35f, HeightU / 100.f);
			break;
		case EUnitRole::Mythique:    // grand bloc imposant (teaser)
			MeshPath = TEXT("/Engine/BasicShapes/Cube.Cube");
			Scale = FVector(2.2f, 2.2f, HeightU / 100.f);
			break;
	}

	if (UStaticMesh* LoadedMesh = LoadObject<UStaticMesh>(nullptr, MeshPath))
	{
		ShapeMesh->SetStaticMesh(LoadedMesh);
	}
	ShapeMesh->SetRelativeScale3D(Scale);
	// Pose la base de la forme au niveau des pieds de la capsule
	ShapeMesh->SetRelativeLocation(FVector(0.f, 0.f, -88.f));

	// Couleur officielle de la faction (FFactionColors = source de vérité)
	if (UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, this))
		{
			MID->SetVectorParameterValue(TEXT("Color"), FFactionColors::Get(GetFaction()));
			ShapeMesh->SetMaterial(0, MID);
		}
	}
}
