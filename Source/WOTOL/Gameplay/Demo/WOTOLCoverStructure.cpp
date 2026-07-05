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

	if (bFalling) { TickFall(DeltaSeconds); return; }
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
	// Longueur qui balaiera le sol en tombant (≈ hauteur de la structure).
	PillarLen = (Variant == 0) ? 780.f : (Variant == 2) ? 560.f : 460.f;
}

void AWOTOLCoverStructure::TakeCoverDamage(float Amount, AUnitBase* /*InstigatorUnit*/)
{
	if (bDestroyed || bIndestructible || Amount <= 0.f) return;
	CurrentHealth -= Amount;
	if (CurrentHealth <= 0.f) Collapse();
}

void AWOTOLCoverStructure::Collapse()
{
	if (bDestroyed || bFalling) return;
	UWorld* W = GetWorld();
	const FVector Origin = GetActorLocation();

	// DIRECTION DE CHUTE : vers l'unité vivante la plus proche (le pilier "tombe sur" les
	// unités) ; à défaut, direction aléatoire. -> il écrasera ce qui est sur son passage.
	FVector Dir = FVector(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), 0.f).GetSafeNormal();
	if (W)
	{
		if (UFactionRegistrySubsystem* Reg = W->GetSubsystem<UFactionRegistrySubsystem>())
		{
			float Best = TNumericLimits<float>::Max();
			const EFactionID Facs[2] = { EFactionID::Aquiloris, EFactionID::Noxeens };
			for (EFactionID F : Facs)
				for (AUnitBase* U : Reg->GetUnitsForFaction(F))
				{
					if (!U || !U->IsAlive()) continue;
					FVector To = U->GetActorLocation() - Origin; To.Z = 0.f;
					const float D = To.Size();
					if (D > 60.f && D < Best) { Best = D; Dir = To.GetSafeNormal(); }
				}
		}
	}
	FallDir = Dir.IsNearlyZero() ? FVector(1, 0, 0) : Dir;

	// Démarre la BASCULE (animée dans TickFall). La collision est retirée pendant la chute.
	bFalling = true;
	FallElapsed = 0.f;
	AlreadyHit.Reset();
	for (UStaticMeshComponent* C : Parts)
		if (C) C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (HealthTag) HealthTag->SetVisibility(false);

	if (W)
		AWOTOLDamageNumber::SpawnText(W, Origin + FVector(0, 0, PillarLen), TEXT("Il s'effondre !"),
			FLinearColor(0.9f, 0.85f, 0.6f, 1.f));
}

void AWOTOLCoverStructure::TickFall(float Dt)
{
	UWorld* W = GetWorld();
	if (!W || !SceneRoot) return;

	FallElapsed += Dt;
	const float Alpha = FMath::Clamp(FallElapsed / FallDuration, 0.f, 1.f);
	// Accélération de chute (ease-in) : lent au début, s'abat vite à la fin.
	const float Eased = Alpha * Alpha;
	const float Angle = Eased * (PI * 0.5f); // 0 -> 90° (à plat)

	// Bascule autour de la base : axe horizontal perpendiculaire à la direction de chute.
	const FVector Axis = FVector::CrossProduct(FVector::UpVector, FallDir).GetSafeNormal();
	SceneRoot->SetWorldRotation(FQuat(Axis, Angle));

	// La "pointe" (haut du pilier) qui s'abat : position courante = base + haut pivoté.
	const FVector Origin = GetActorLocation();
	const FVector Tip = Origin + FQuat(Axis, Angle).RotateVector(FVector(0, 0, PillarLen));

	// Écrase les unités sur le passage de la pointe (une seule fois chacune).
	if (UFactionRegistrySubsystem* Reg = W->GetSubsystem<UFactionRegistrySubsystem>())
	{
		const EFactionID Facs[2] = { EFactionID::Aquiloris, EFactionID::Noxeens };
		for (EFactionID F : Facs)
			for (AUnitBase* U : Reg->GetUnitsForFaction(F))
			{
				if (!U || !U->IsAlive()) continue;
				if (AlreadyHit.Contains(U)) continue;
				if (FVector::Dist(U->GetActorLocation(), Tip) <= DebrisRadius)
				{
					AlreadyHit.Add(U);
					U->TakeDamageFromUnit(DebrisDamage, nullptr);
					U->LaunchCharacter(FallDir * 650.f + FVector(0, 0, 260.f), true, true);
					AWOTOLBubbleBurst::Burst(W, U->GetActorLocation() + FVector(0, 0, 40.f),
						FLinearColor(0.6f, 0.62f, 0.68f, 1.f), 10);
				}
			}
	}

	if (Alpha >= 1.f)
	{
		// Fin de la chute : gravats au sol (visuel aplati), plus de mise à jour.
		bFalling = false;
		bDestroyed = true;
		AWOTOLBubbleBurst::Burst(W, Tip + FVector(0, 0, 20.f), FLinearColor(0.62f, 0.64f, 0.7f, 1.f), 30);
		for (UStaticMeshComponent* C : Parts)
			if (C) { FVector S = C->GetRelativeScale3D(); C->SetRelativeScale3D(FVector(S.X, S.Y, S.Z * 0.4f)); }
	}
}
