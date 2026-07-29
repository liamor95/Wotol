#include "WOTOLZoneTelegraph.h"
#include "WOTOLGlow.h"
#include "Gameplay/Units/UnitBase.h"
#include "Core/FactionRegistrySubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

AWOTOLZoneTelegraph::AWOTOLZoneTelegraph()
{
	PrimaryActorTick.bCanEverTick = true;
	Disc = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Disc"));
	RootComponent = Disc;
	Disc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Disc->SetCanEverAffectNavigation(false);
}

void AWOTOLZoneTelegraph::BeginPlay()
{
	Super::BeginPlay();

	if (UStaticMesh* Cyl = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")))
	{
		Disc->SetStaticMesh(Cyl);
	}
	// Cylindre moteur : rayon de base 50 UE -> mis à l'échelle du rayon voulu, très aplati
	// (disque fin posé au sol, pas un cylindre plein).
	const float S = FMath::Max(20.f, Radius) / 50.f;
	Disc->SetRelativeScale3D(FVector(S, S, 0.03f));
	Disc->SetRelativeLocation(FVector(0.f, 0.f, 4.f));

	if (UMaterialInstanceDynamic* MID = WOTOLGlow::MakeHalo(this, Color, 0.35f))
	{
		Disc->SetMaterial(0, MID);
	}

	SetLifeSpan(Duration + 0.1f);
}

void AWOTOLZoneTelegraph::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Elapsed += DeltaSeconds;

	// Pulsation douce (aspect "hologramme actif").
	const float Pulse = 0.95f + 0.05f * FMath::Sin(Elapsed * 8.f);
	const float S = FMath::Max(20.f, Radius) / 50.f * Pulse;
	Disc->SetRelativeScale3D(FVector(S, S, 0.03f));

	if (!bAppliesBlindToEnemies) return;

	UWorld* World = GetWorld();
	UFactionRegistrySubsystem* Reg = World ? World->GetSubsystem<UFactionRegistrySubsystem>() : nullptr;
	if (!Reg) return;

	const EFactionID EnemyFac = (CasterFaction == EFactionID::Aquiloris)
		? EFactionID::Noxeens : EFactionID::Aquiloris;
	const FVector Center = GetActorLocation();
	const float R2 = Radius * Radius;
	const float Now = World->GetTimeSeconds();

	for (AUnitBase* Enemy : Reg->GetUnitsForFaction(EnemyFac))
	{
		if (!Enemy || !Enemy->IsAlive()) continue;
		if (FVector::DistSquared2D(Enemy->GetActorLocation(), Center) > R2) continue;
		Enemy->BlindedUntil = Now + 0.4f; // rafraîchi en continu tant que dans le voile
	}
}
