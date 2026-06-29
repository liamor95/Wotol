#include "WOTOLDemoUnit.h"
#include "Gameplay/Units/UnitDataAsset.h"
#include "Gameplay/AI/AIAdaptiveController.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Data/WOTOLTypes.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

AWOTOLDemoUnit::AWOTOLDemoUnit()
{
	PrimaryActorTick.bCanEverTick = true;

	ShapeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShapeMesh"));
	ShapeMesh->SetupAttachment(RootComponent);
	ShapeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Étiquette flottante nom + PV (au-dessus de la tête)
	NameTag = CreateDefaultSubobject<UTextRenderComponent>(TEXT("NameTag"));
	NameTag->SetupAttachment(RootComponent);
	NameTag->SetRelativeLocation(FVector(0.f, 0.f, 140.f));
	NameTag->SetHorizontalAlignment(EHTA_Center);
	NameTag->SetWorldSize(40.f);
	NameTag->SetText(FText::GetEmpty());

	// L'IA RTS possède automatiquement l'unité au spawn
	AIControllerClass = AAIAdaptiveController::StaticClass();
	AutoPossessAI     = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AWOTOLDemoUnit::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!NameTag) return;

	const FString DisplayName = (UnitData && !UnitData->DisplayName.IsEmpty())
		? UnitData->DisplayName.ToString()
		: GetName();
	const int32 HpPct = FMath::RoundToInt(GetHealthPercent() * 100.f);

	NameTag->SetText(FText::FromString(FString::Printf(TEXT("%s\n%d%%"), *DisplayName, HpPct)));
	NameTag->SetTextRenderColor(FFactionColors::Get(GetFaction()).ToFColor(true));

	// L'étiquette fait toujours face à la caméra du joueur
	if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		if (PC->PlayerCameraManager)
		{
			const FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
			FRotator Face = (CamLoc - NameTag->GetComponentLocation()).Rotation();
			Face.Pitch = 0.f; Face.Roll = 0.f;
			Face.Yaw += 180.f; // le texte se lit de face
			NameTag->SetWorldRotation(Face);
		}
	}
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
		case EUnitRole::Chef:        // sphère = chef/commandant (silhouette unique)
			MeshPath = TEXT("/Engine/BasicShapes/Sphere.Sphere");
			Scale = FVector(0.9f, 0.9f, HeightU / 100.f);
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
	// Forme centrée sur la capsule
	ShapeMesh->SetRelativeLocation(FVector::ZeroVector);

	// COLLISION : la capsule épouse la taille réelle de la forme (les meshes
	// primitifs font 100 UE -> demi-extent = Scale * 50). Les unités ne se
	// rentrent plus dedans ni dans la créature géante.
	const float CapR = FMath::Max(20.f, FMath::Max(Scale.X, Scale.Y) * 50.f);
	const float CapH = FMath::Max(20.f, Scale.Z * 50.f);
	GetCapsuleComponent()->SetCapsuleSize(CapR, CapH);
	// Remonte l'étiquette au-dessus de la forme
	if (NameTag) NameTag->SetRelativeLocation(FVector(0.f, 0.f, CapH + 40.f));

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
