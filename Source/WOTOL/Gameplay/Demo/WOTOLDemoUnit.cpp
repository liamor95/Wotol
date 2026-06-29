#include "WOTOLDemoUnit.h"
#include "Gameplay/Units/UnitDataAsset.h"
#include "Gameplay/Units/UnitAIStateComponent.h"
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
#include "Core/FactionRegistrySubsystem.h"
#include "WOTOLDamageNumber.h"

AWOTOLDemoUnit::AWOTOLDemoUnit()
{
	PrimaryActorTick.bCanEverTick = true;

	// CRITIQUE : composant machine d'états IA (sinon les attaques ne se déclenchent
	// jamais — il était ajouté côté Blueprint, absent des unités 100% C++).
	CreateDefaultSubobject<UUnitAIStateComponent>(TEXT("AIState"));

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

	if (bCreatureBrain)
	{
		CreatureBrainTick(DeltaSeconds);
	}

	if (!NameTag) return;

	// Le boss s'appelle "Kraken" (créature neutre), pas le nom du mythique rival
	const FString DisplayName = bCreatureBrain
		? FString(TEXT("Kraken"))
		: ((UnitData && !UnitData->DisplayName.IsEmpty()) ? UnitData->DisplayName.ToString() : GetName());

	// VRAIES valeurs de PV (ex: "1700 / 2000")
	const int32 MaxHP = UnitData ? UnitData->Stats.MaxHealth : 100;
	const int32 CurHP = FMath::Clamp(FMath::RoundToInt(CurrentHealth), 0, MaxHP);

	NameTag->SetText(FText::FromString(
		FString::Printf(TEXT("%s\n%d / %d"), *DisplayName, CurHP, MaxHP)));

	// Couleur d'étiquette : violet "calamar" pour le kraken, sinon couleur de faction
	const FLinearColor TagColor = bCreatureBrain
		? FLinearColor(0.7f, 0.15f, 0.85f, 1.f)
		: FFactionColors::Get(GetFaction());
	NameTag->SetTextRenderColor(TagColor.ToFColor(true));

	// Recolore la forme du kraken en violet une seule fois (sa couleur n'est pas verte/océan)
	if (bCreatureBrain && ShapeMID && !bCreatureStyled)
	{
		ShapeMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.55f, 0.1f, 0.7f, 1.f));
		bCreatureStyled = true;
	}

	// L'étiquette fait toujours face à la caméra du joueur
	if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		if (PC->PlayerCameraManager)
		{
			const FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
			// Le texte se lit dans le bon sens : son axe +X pointe VERS la caméra
			FRotator Face = (CamLoc - NameTag->GetComponentLocation()).Rotation();
			Face.Pitch = 0.f; Face.Roll = 0.f;
			NameTag->SetWorldRotation(Face);
		}
	}
}

void AWOTOLDemoUnit::BeginPlay()
{
	Super::BeginPlay();   // initialise UnitData -> stats, faction, rôle
	BuildGreyboxShape();
	OnUnitSelected.AddDynamic(this, &AWOTOLDemoUnit::HandleSelected);
	OnHealthChanged.AddDynamic(this, &AWOTOLDemoUnit::HandleHealthChanged);
	LastKnownHealth = UnitData ? static_cast<float>(UnitData->Stats.MaxHealth) : 100.f;
}

void AWOTOLDemoUnit::HandleHealthChanged(float NewHealth, float MaxHealth)
{
	// Chiffre de dégâts flottant rouge (uniquement quand on PERD des PV)
	if (LastKnownHealth >= 0.f && NewHealth < LastKnownHealth)
	{
		const float Dmg = LastKnownHealth - NewHealth;
		const FVector Loc = GetActorLocation() + FVector(0.f, 0.f, 60.f);
		AWOTOLDamageNumber::Spawn(GetWorld(), Loc, Dmg, FLinearColor(1.f, 0.25f, 0.1f, 1.f));
	}
	LastKnownHealth = NewHealth;
}

void AWOTOLDemoUnit::HandleSelected(bool bSel)
{
	if (!ShapeMID) return;
	// Sélectionnée = blanc lumineux ; sinon couleur de faction
	const FLinearColor C = bSel ? FLinearColor(1.f, 1.f, 1.f, 1.f)
	                            : FFactionColors::Get(GetFaction());
	ShapeMID->SetVectorParameterValue(TEXT("Color"), C);
}

void AWOTOLDemoUnit::CreatureBrainTick(float DeltaSeconds)
{
	if (!IsAlive()) return;
	UWorld* W = GetWorld();
	if (!W) return;
	UFactionRegistrySubsystem* Reg = W->GetSubsystem<UFactionRegistrySubsystem>();
	if (!Reg) return;

	// Cherche l'unité ennemie la plus proche
	const EFactionID EnemyFac = (GetFaction() == EFactionID::Aquiloris)
		? EFactionID::Noxeens : EFactionID::Aquiloris;

	AUnitBase* Nearest = nullptr;
	float Best = TNumericLimits<float>::Max();
	for (AUnitBase* U : Reg->GetUnitsForFaction(EnemyFac))
	{
		if (!U || !U->IsAlive()) continue;
		const float D = FVector::DistSquared(GetActorLocation(), U->GetActorLocation());
		if (D < Best) { Best = D; Nearest = U; }
	}
	if (!Nearest) return;

	FVector To = Nearest->GetActorLocation() - GetActorLocation();
	To.Z = 0.f;
	const float Dist = To.Size();

	// Se tourne vers la cible
	if (Dist > 1.f)
	{
		FRotator R = To.Rotation();
		R.Pitch = 0.f; R.Roll = 0.f;
		SetActorRotation(R);
	}

	const float Range = UnitData ? UnitData->Stats.AttackRange * 200.f : 200.f;
	const float Edge  = Dist - GetSimpleCollisionRadius() - Nearest->GetSimpleCollisionRadius();

	if (Edge <= Range)
	{
		PerformAttack(Nearest);   // throttlé par le cooldown interne de l'unité
	}
	else
	{
		AddMovementInput(To.GetSafeNormal(), 1.f); // avance vers la cible
	}
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
		case EUnitRole::Montee:      // bloc compact (monture) — moins large qu'avant
			MeshPath = TEXT("/Engine/BasicShapes/Cube.Cube");
			Scale = FVector(0.9f, 0.6f, HeightU / 100.f);
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
			ShapeMID = MID; // conservé pour le surlignage de sélection
		}
	}
}
