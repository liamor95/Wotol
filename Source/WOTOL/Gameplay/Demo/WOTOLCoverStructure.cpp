#include "WOTOLCoverStructure.h"
#include "Gameplay/Units/UnitBase.h"
#include "Core/FactionRegistrySubsystem.h"
#include "WOTOLBubbleBurst.h"
#include "WOTOLDamageNumber.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

AWOTOLCoverStructure::AWOTOLCoverStructure()
{
	PrimaryActorTick.bCanEverTick = true;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	HealthTag = CreateDefaultSubobject<UTextRenderComponent>(TEXT("HealthTag"));
	HealthTag->SetupAttachment(SceneRoot);
	HealthTag->SetHorizontalAlignment(EHTA_Center);
	HealthTag->SetWorldSize(34.f);
	HealthTag->SetRelativeLocation(FVector(0.f, 0.f, 40.f));
	HealthTag->SetTextRenderColor(FColor(230, 210, 150, 255));
	HealthTag->SetVisibility(false);
}

void AWOTOLCoverStructure::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = MaxHealth;
	BuildVisual();
}

void AWOTOLCoverStructure::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bDestroyed || !HealthTag) return;

	// Étiquette de PV : visible seulement si destructible ET endommagé.
	const bool bShow = !bIndestructible && CurrentHealth < MaxHealth - 1.f;
	HealthTag->SetVisibility(bShow);
	if (bShow)
	{
		HealthTag->SetText(FText::FromString(FString::Printf(TEXT("Ruine  %d / %d"),
			FMath::RoundToInt(CurrentHealth), FMath::RoundToInt(MaxHealth))));
		if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
			if (PC->PlayerCameraManager)
			{
				FRotator F = (PC->PlayerCameraManager->GetCameraLocation() - HealthTag->GetComponentLocation()).Rotation();
				F.Pitch = 0.f; F.Roll = 0.f;
				HealthTag->SetWorldRotation(F);
			}
	}
}

// Ajoute une pièce (mesh primitif) avec collision bloquante (unités + tirs).
static UStaticMeshComponent* AddCoverPiece(AActor* Owner, USceneComponent* Root,
	const TCHAR* MeshPath, const FVector& Loc, const FVector& Scale, const FRotator& Rot,
	const FLinearColor& Color)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Owner);
	if (!C) return nullptr;
	C->SetupAttachment(Root);
	C->RegisterComponent();
	if (UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, MeshPath)) C->SetStaticMesh(M);
	C->SetRelativeLocationAndRotation(Loc, Rot);
	C->SetRelativeScale3D(Scale);
	// Bloque tout : unités (WorldStatic/Pawn) ET les tracés de tir (couverture).
	C->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	C->SetCollisionObjectType(ECC_WorldStatic);
	C->SetCollisionResponseToAllChannels(ECR_Block);
	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
		if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, Owner))
		{
			MID->SetVectorParameterValue(TEXT("Color"), Color);
			C->SetMaterial(0, MID);
		}
	return C;
}

void AWOTOLCoverStructure::BuildVisual()
{
	const TCHAR* M_CUBE = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* M_CYL  = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* M_CONE = TEXT("/Engine/BasicShapes/Cone.Cone");

	// Pierre Éthérienne : gris-bleu patiné + lisérés cyan (technologie ancienne).
	const FLinearColor Stone(0.32f, 0.36f, 0.42f, 1.f);
	const FLinearColor Dark (0.18f, 0.21f, 0.26f, 1.f);
	const FLinearColor Glow (0.25f, 0.75f, 0.95f, 1.f);

	auto Add = [&](const TCHAR* Mesh, const FVector& L, const FVector& S, const FRotator& R, const FLinearColor& Col)
	{
		if (UStaticMeshComponent* C = AddCoverPiece(this, SceneRoot, Mesh, L, S, R, Col)) Parts.Add(C);
	};

	if (Variant == 1) // pan de mur en ruine
	{
		Add(M_CUBE, FVector(0, 0, 140), FVector(0.6f, 4.5f, 3.0f), FRotator::ZeroRotator, Stone);
		Add(M_CUBE, FVector(0, -160, 360), FVector(0.6f, 1.6f, 1.4f), FRotator(0, 0, 8.f), Stone); // créneau
		Add(M_CUBE, FVector(0, 40, 40), FVector(0.9f, 5.2f, 0.6f), FRotator::ZeroRotator, Dark);   // socle
		Add(M_CONE, FVector(0, 90, 300), FVector(0.15f, 0.15f, 1.2f), FRotator(0, 0, 0), Glow);    // conduit lumineux
	}
	else if (Variant == 2) // arche brisée
	{
		Add(M_CYL, FVector(0, -160, 220), FVector(0.5f, 0.5f, 4.4f), FRotator::ZeroRotator, Stone);
		Add(M_CYL, FVector(0,  160, 160), FVector(0.5f, 0.5f, 3.2f), FRotator::ZeroRotator, Stone);
		Add(M_CUBE, FVector(0, -20, 440), FVector(0.5f, 2.4f, 0.5f), FRotator(0, 0, 12.f), Stone);  // linteau penché
		Add(M_CONE, FVector(0, -160, 470), FVector(0.3f, 0.3f, 0.8f), FRotator::ZeroRotator, Glow);
	}
	else // grand pilier (défaut)
	{
		Add(M_CYL, FVector(0, 0, 40), FVector(2.4f, 2.4f, 0.4f), FRotator::ZeroRotator, Dark);      // base large
		Add(M_CYL, FVector(0, 0, 300), FVector(1.5f, 1.5f, 5.2f), FRotator::ZeroRotator, Stone);    // fût
		Add(M_CUBE, FVector(0, 0, 560), FVector(1.9f, 1.9f, 0.5f), FRotator(0, 45.f, 6.f), Stone);  // chapiteau brisé
		Add(M_CONE, FVector(20, 0, 640), FVector(0.5f, 0.5f, 1.6f), FRotator(14.f, 0, 0), Glow);    // cristal au sommet
	}

	HealthTag->SetRelativeLocation(FVector(0.f, 0.f, (Variant == 0) ? 760.f : 520.f));
}

void AWOTOLCoverStructure::TakeCoverDamage(float Amount, AUnitBase* /*InstigatorUnit*/)
{
	if (bDestroyed || bIndestructible || Amount <= 0.f) return;
	CurrentHealth -= Amount;
	if (CurrentHealth <= 0.f) Collapse();
}

void AWOTOLCoverStructure::Collapse()
{
	if (bDestroyed) return;
	bDestroyed = true;

	// DÉBRIS : dégâts de zone à TOUTES les unités proches (amis comme ennemis) — c'est
	// une masse de gravats qui tombe. L'IA peut donc effondrer un pilier au-dessus des ennemis.
	UWorld* W = GetWorld();
	const FVector Origin = GetActorLocation();
	if (W)
	{
		if (UFactionRegistrySubsystem* Reg = W->GetSubsystem<UFactionRegistrySubsystem>())
		{
			const EFactionID Facs[2] = { EFactionID::Aquiloris, EFactionID::Noxeens };
			for (EFactionID F : Facs)
				for (AUnitBase* U : Reg->GetUnitsForFaction(F))
				{
					if (!U || !U->IsAlive()) continue;
					if (FVector::Dist(U->GetActorLocation(), Origin) <= DebrisRadius)
					{
						U->TakeDamageFromUnit(DebrisDamage, nullptr);
						U->LaunchCharacter((U->GetActorLocation() - Origin).GetSafeNormal2D() * 700.f + FVector(0, 0, 250.f), true, true);
					}
				}
		}
		AWOTOLBubbleBurst::Burst(W, Origin + FVector(0, 0, 120.f), FLinearColor(0.6f, 0.62f, 0.68f, 1.f), 40);
		AWOTOLDamageNumber::SpawnText(W, Origin + FVector(0, 0, 300.f), TEXT("EFFONDREMENT"),
			FLinearColor(0.85f, 0.8f, 0.6f, 1.f));
	}

	// Retire la collision et l'affiche (débris au sol : on laisse un socle bas).
	for (UStaticMeshComponent* C : Parts)
	{
		if (!C) continue;
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetRelativeScale3D(C->GetRelativeScale3D() * FVector(1.1f, 1.1f, 0.12f)); // s'écrase
		C->AddRelativeLocation(FVector(0, 0, -C->GetRelativeLocation().Z * 0.85f));
	}
	if (HealthTag) HealthTag->SetVisibility(false);
}
