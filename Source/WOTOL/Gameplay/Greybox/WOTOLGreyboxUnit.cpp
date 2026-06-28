#include "WOTOLGreyboxUnit.h"
#include "Gameplay/AI/AIAdaptiveController.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Data/WOTOLTypes.h"

AWOTOLGreyboxUnit::AWOTOLGreyboxUnit()
{
	// Forme visuelle attachée à la capsule, sans collision (purement cosmétique)
	ShapeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShapeMesh"));
	ShapeMesh->SetupAttachment(RootComponent);
	ShapeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShapeMesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.6f));
	ShapeMesh->SetRelativeLocation(FVector(0.f, 0.f, -90.f));

	// L'IA RTS possède automatiquement l'unité au spawn
	AIControllerClass = AAIAdaptiveController::StaticClass();
	AutoPossessAI     = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AWOTOLGreyboxUnit::BeginPlay()
{
	// AUnitBase::BeginPlay() initialise les stats depuis UnitData et fixe la faction
	Super::BeginPlay();

	const EFactionID F = GetFaction();

	// Forme selon la faction : Noxéens = cône, autres = cube
	const TCHAR* MeshPath = (F == EFactionID::Noxeens)
		? TEXT("/Engine/BasicShapes/Cone.Cone")
		: TEXT("/Engine/BasicShapes/Cube.Cube");

	if (UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, MeshPath))
	{
		ShapeMesh->SetStaticMesh(Mesh);
	}

	// Couleur officielle de la faction (FFactionColors = source de vérité unique)
	if (UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, this))
		{
			MID->SetVectorParameterValue(TEXT("Color"), FFactionColors::Get(F));
			ShapeMesh->SetMaterial(0, MID);
		}
	}
}
