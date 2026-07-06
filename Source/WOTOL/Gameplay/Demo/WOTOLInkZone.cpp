#include "WOTOLInkZone.h"
#include "Gameplay/Units/UnitBase.h"
#include "Gameplay/Demo/WOTOLDemoUnit.h"
#include "Core/FactionRegistrySubsystem.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

static float InkRnd(int32 n)
{
	n = (n << 13) ^ n;
	return (float)((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 2147483647.f;
}

AWOTOLInkZone::AWOTOLInkZone()
{
	PrimaryActorTick.bCanEverTick = true;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;
}

void AWOTOLInkZone::BeginPlay()
{
	Super::BeginPlay();

	UStaticMesh* Cyl = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	// Tache IRRÉGULIÈRE : plusieurs disques APLATIS de tailles/positions variées qui se
	// chevauchent -> contour organique de flaque d'huile, pas un cercle propre.
	const int32 N = 9;
	for (int32 i = 0; i < N; ++i)
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
		if (!C) continue;
		C->SetupAttachment(SceneRoot);
		C->RegisterComponent();
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCanEverAffectNavigation(false);
		if (Cyl) C->SetStaticMesh(Cyl);

		const float ang = InkRnd(i) * 6.283f;
		const float dist = InkRnd(i + 20) * Radius * 0.55f;
		const float bx = FMath::Cos(ang) * dist;
		const float by = FMath::Sin(ang) * dist;
		const float br = Radius * (0.35f + InkRnd(i + 40) * 0.5f);
		// Cylindre TRÈS plat (galette) posé au sol.
		C->SetRelativeScale3D(FVector(br / 50.f, br / 50.f, 0.03f));
		C->SetRelativeLocation(FVector(bx, by, 4.f + InkRnd(i + 60) * 3.f));

		if (Base)
			if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, this))
			{
				// Encre violet-noir bioluminescente (cohérent avec les abysses).
				MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.06f, 0.02f, 0.10f, 1.f));
				C->SetMaterial(0, MID);
			}
		Blobs.Add(C);
	}
}

void AWOTOLInkZone::Tick(float Dt)
{
	Super::Tick(Dt);
	Elapsed += Dt;

	UWorld* W = GetWorld();
	if (!W) return;
	const float Now = W->GetTimeSeconds();

	// Dissipation : sur la dernière seconde, on rétrécit la tache (fondu).
	const float Remain = Lifetime - Elapsed;
	if (Remain < 1.f)
	{
		const float s = FMath::Max(0.05f, Remain);
		SetActorScale3D(FVector(s, s, 1.f));
	}

	// EFFET sur les unités DANS la flaque (toutes factions, sauf le Kraken lui-même) :
	// ralenti + précision réduite (aveuglement), rafraîchis en continu ; léger poison ~1/s.
	DotAccum += Dt;
	const bool bApplyDot = (DotAccum >= 1.f);
	if (bApplyDot) DotAccum = 0.f;

	if (UFactionRegistrySubsystem* Reg = W->GetSubsystem<UFactionRegistrySubsystem>())
	{
		const FVector C = GetActorLocation();
		const float R2 = Radius * Radius;
		for (uint8 f = 1; f <= (uint8)EFactionID::PiratesAbyssaux; ++f)
		{
			for (AUnitBase* U : Reg->GetUnitsForFaction((EFactionID)f))
			{
				if (!U || !U->IsAlive()) continue;
				if (AWOTOLDemoUnit* DU = Cast<AWOTOLDemoUnit>(U))
					if (DU->bCreatureBrain) continue; // l'encre n'affecte pas le Kraken
				if (FVector::DistSquared2D(U->GetActorLocation(), C) > R2) continue;

				U->SlowUntil    = Now + 0.35f; // ralenti tant qu'on reste dedans
				U->BlindedUntil = Now + 0.35f; // précision fortement réduite
				if (bApplyDot) U->TakeDamageFromUnit(12.f, Caster.Get()); // poison d'encre léger
			}
		}
	}

	if (Elapsed >= Lifetime) Destroy();
}
