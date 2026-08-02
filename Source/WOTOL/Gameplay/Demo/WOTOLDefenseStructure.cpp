#include "WOTOLDefenseStructure.h"
#include "WOTOLGlow.h"
#include "WOTOLBubbleBurst.h"
#include "Gameplay/Units/UnitBase.h"
#include "Core/FactionRegistrySubsystem.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

AWOTOLDefenseStructure::AWOTOLDefenseStructure()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	Head = CreateDefaultSubobject<USceneComponent>(TEXT("Head"));
	Head->SetupAttachment(SceneRoot);
	Head->SetRelativeLocation(FVector(0.f, 0.f, 220.f));
}

void AWOTOLDefenseStructure::BeginPlay()
{
	Super::BeginPlay();
	ApplyLevelStats();
	CurrentHealth = MaxHealth;
	// La faction impose le type (identité visuelle propre).
	DefenseType = (OwnerFaction == EFactionID::Noxeens)
		? EWOTOLDefenseType::NoxeenSentinel : EWOTOLDefenseType::AquilorisTurret;
	BuildVisual();
}

void AWOTOLDefenseStructure::ApplyLevelStats()
{
	StructureLevel = FMath::Clamp(StructureLevel, 1, 3);
	const float Step = static_cast<float>(StructureLevel - 1);
	DamagePerShot *= 1.f + 0.25f * Step;
	Range *= 1.f + 0.12f * Step;
	FireCooldown *= FMath::Pow(0.90f, Step);
	MaxHealth *= 1.f + 0.35f * Step;
}

void AWOTOLDefenseStructure::BuildVisual()
{
	if (!SceneRoot || !Head) return;

	const TCHAR* M_CYL  = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* M_SPH  = TEXT("/Engine/BasicShapes/Sphere.Sphere");
	const TCHAR* M_CONE = TEXT("/Engine/BasicShapes/Cone.Cone");

	auto Add = [&](USceneComponent* Parent, const TCHAR* MeshPath, const FVector& Loc,
		const FVector& Scale, const FRotator& Rot, const FLinearColor& Col, bool bGlow) -> UStaticMeshComponent*
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
		if (!C) return nullptr;
		C->SetupAttachment(Parent);
		C->RegisterComponent();
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, MeshPath)) C->SetStaticMesh(M);
		C->SetRelativeLocationAndRotation(Loc, Rot);
		C->SetRelativeScale3D(Scale);
		UMaterialInstanceDynamic* MID = bGlow ? WOTOLGlow::MakeGlow(this, Col) : WOTOLGlow::MakeMatte(this, Col);
		if (MID) C->SetMaterial(0, MID);
		return C;
	};

	// Amas de pointes (cristal/épine) tout autour du socle, tailles/rotations variées — se
	// rapproche des planches officielles "Tourelle hydrocristalline"/"Œil bioluminal" fournies
	// par Liamor le 01/08/2026 (un amas dense de pointes irrégulières, pas un seul cône nu).
	// Nombre ET taille du cluster augmentent avec StructureLevel (1->3), comme les 4 étapes de
	// croissance illustrées sur les planches. Seed fixe par acteur (déterministe à l'écran,
	// jamais recalculé au hasard entre deux frames) mais VARIÉ par instance (a sinon toutes les
	// tourelles/sentinelles d'un même niveau étaient rigoureusement identiques -> alignement au
	// cordeau visible, incohérent avec des cristaux/épines naturels).
	auto AddSpikeCluster = [&](const FLinearColor& SpikeCol, float BaseRadius, int32 Count)
	{
		FRandomStream Rng(GetUniqueID() * 977 + 13);
		for (int32 i = 0; i < Count; ++i)
		{
			const float Angle = (360.f / static_cast<float>(Count)) * static_cast<float>(i)
				+ Rng.FRandRange(-12.f, 12.f);
			const float Dist = Rng.FRandRange(BaseRadius * 0.55f, BaseRadius);
			const FVector Pos(FMath::Cos(FMath::DegreesToRadians(Angle)) * Dist,
				FMath::Sin(FMath::DegreesToRadians(Angle)) * Dist, 6.f);
			const float H2 = Rng.FRandRange(70.f, 150.f) * (0.75f + 0.25f * static_cast<float>(StructureLevel));
			const float Wd = Rng.FRandRange(0.22f, 0.34f);
			const float Tilt = Rng.FRandRange(-8.f, 8.f);
			Add(SceneRoot, M_CONE, Pos, FVector(Wd, Wd, H2 / 100.f),
				FRotator(Tilt, Rng.FRandRange(0.f, 360.f), Tilt), SpikeCol, true);
		}
	};

	if (OwnerFaction == EFactionID::Noxeens)
	{
		// OEIL BIOLUMINAL (Noxéens) : pilier sombre + orbe vert bioluminescent qui vise, entouré
		// d'épines sombres (planche "Entraves abyssales"/"Œil bioluminal").
		const FLinearColor Dark(0.07f, 0.10f, 0.11f, 1.f);
		const FLinearColor Green(0.28f, 1.6f, 0.55f, 1.f);
		Add(SceneRoot, M_CYL, FVector(0, 0, 10.f),  FVector(1.6f, 1.6f, 0.2f), FRotator::ZeroRotator, Dark, false); // socle
		AddSpikeCluster(Dark, 130.f, 6 + StructureLevel * 2);
		Add(SceneRoot, M_CYL, FVector(0, 0, 100.f), FVector(0.7f, 0.7f, 2.0f), FRotator::ZeroRotator, Dark, false); // fût
		Add(Head, M_SPH, FVector::ZeroVector, FVector(0.9f, 0.9f, 0.9f), FRotator::ZeroRotator, Green, true);        // orbe
		Add(Head, M_CONE, FVector(60.f, 0, 0), FVector(0.3f, 0.3f, 0.9f), FRotator(90.f, 0, 0), Green, true);        // canon
	}
	else
	{
		// TOURELLE HYDROCRISTALLINE (Aquiloris) : socle cristal entouré de pointes bleues, tête
		// sphérique + canon (planche "Tourelle hydrocristalline"/"Rempart cristallin").
		const FLinearColor Steel(0.20f, 0.24f, 0.30f, 1.f);
		const FLinearColor Blue(0.35f, 0.85f, 3.0f, 1.f);
		Add(SceneRoot, M_CYL, FVector(0, 0, 10.f),  FVector(1.7f, 1.7f, 0.25f), FRotator::ZeroRotator, Steel, false); // socle
		AddSpikeCluster(Blue, 140.f, 6 + StructureLevel * 2);
		Add(SceneRoot, M_CYL, FVector(0, 0, 110.f), FVector(0.8f, 0.8f, 2.0f),  FRotator::ZeroRotator, Steel, false); // fût
		Add(SceneRoot, M_CONE, FVector(0, 0, 190.f),FVector(0.5f, 0.5f, 0.6f),  FRotator::ZeroRotator, Blue, true);   // cristal
		Add(Head, M_SPH, FVector::ZeroVector, FVector(0.85f, 0.85f, 0.7f), FRotator::ZeroRotator, Steel, false);      // tête
		Add(Head, M_CONE, FVector(70.f, 0, 0), FVector(0.28f, 0.28f, 1.0f), FRotator(90.f, 0, 0), Blue, true);        // canon
	}
}

AUnitBase* AWOTOLDefenseStructure::FindTarget() const
{
	UWorld* W = GetWorld();
	if (!W) return nullptr;
	UFactionRegistrySubsystem* Reg = W->GetSubsystem<UFactionRegistrySubsystem>();
	if (!Reg) return nullptr;

	// Cible = faction adverse (démo : Aquiloris <-> Noxéens).
	const EFactionID Target = (OwnerFaction == EFactionID::Noxeens)
		? EFactionID::Aquiloris : EFactionID::Noxeens;

	AUnitBase* Best = nullptr;
	float BestDist = Range;
	const FVector Origin = GetActorLocation();
	for (AUnitBase* U : Reg->GetUnitsForFaction(Target))
	{
		if (!U || !U->IsAlive()) continue;
		const float D = FVector::Dist(U->GetActorLocation(), Origin);
		if (D <= BestDist) { BestDist = D; Best = U; }
	}
	return Best;
}

void AWOTOLDefenseStructure::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (CurrentHealth <= 0.f) return;

	AUnitBase* Foe = FindTarget();
	if (!Foe) return;

	// La tête vise la cible (yaw seulement, lisible).
	if (Head)
	{
		FRotator Face = (Foe->GetActorLocation() - Head->GetComponentLocation()).Rotation();
		Head->SetWorldRotation(FRotator(0.f, Face.Yaw, 0.f));
	}

	FireTimer -= DeltaSeconds;
	if (FireTimer > 0.f) return;
	FireTimer = FireCooldown;

	// Tir : dégâts directs + petit éclat lumineux au point d'impact (greybox, pas de projectile).
	Foe->TakeDamageFromUnit(DamagePerShot, nullptr);
	const FLinearColor Shot = (OwnerFaction == EFactionID::Noxeens)
		? FLinearColor(0.3f, 1.6f, 0.6f, 1.f) : FLinearColor(0.4f, 0.85f, 2.0f, 1.f);
	const FVector Impact = (Foe->GetFloatingTextAnchor()
		? Foe->GetFloatingTextAnchor()->GetComponentLocation() : Foe->GetActorLocation());
	AWOTOLBubbleBurst::Burst(GetWorld(), Impact, Shot, 6);
}

void AWOTOLDefenseStructure::ApplyDamage(float Amount)
{
	if (CurrentHealth <= 0.f) return;
	CurrentHealth = FMath::Max(0.f, CurrentHealth - Amount);
	if (CurrentHealth <= 0.f)
	{
		Destroy();
	}
}
