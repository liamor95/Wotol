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

	if (OwnerFaction == EFactionID::Noxeens)
	{
		// SENTINELLE Noxéenne : pilier sombre + orbe vert bioluminescent qui vise.
		const FLinearColor Dark(0.07f, 0.10f, 0.11f, 1.f);
		const FLinearColor Green(0.28f, 1.6f, 0.55f, 1.f);
		Add(SceneRoot, M_CYL, FVector(0, 0, 100.f), FVector(0.7f, 0.7f, 2.0f), FRotator::ZeroRotator, Dark, false); // fût
		Add(SceneRoot, M_CYL, FVector(0, 0, 10.f),  FVector(1.6f, 1.6f, 0.2f), FRotator::ZeroRotator, Dark, false); // socle
		Add(Head, M_SPH, FVector::ZeroVector, FVector(0.9f, 0.9f, 0.9f), FRotator::ZeroRotator, Green, true);        // orbe
		Add(Head, M_CONE, FVector(60.f, 0, 0), FVector(0.3f, 0.3f, 0.9f), FRotator(90.f, 0, 0), Green, true);        // canon
	}
	else
	{
		// TOURELLE Aquiloris : socle cristal + tête sphérique + canon (bleu énergie).
		const FLinearColor Steel(0.20f, 0.24f, 0.30f, 1.f);
		const FLinearColor Blue(0.35f, 0.85f, 3.0f, 1.f);
		Add(SceneRoot, M_CYL, FVector(0, 0, 10.f),  FVector(1.7f, 1.7f, 0.25f), FRotator::ZeroRotator, Steel, false); // socle
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
