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
		// Départ en BROUILLARD : galette plus volumineuse (haute) ; elle s'aplatira au sol.
		C->SetRelativeScale3D(FVector(br / 50.f, br / 50.f, 0.4f));
		const FVector Base3(bx, by, InkRnd(i + 60) * 40.f);
		C->SetRelativeLocation(Base3);
		BlobBase.Add(Base3);

		if (Base)
			if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, this))
			{
				// Encre violet-noir bioluminescente (cohérent avec les abysses).
				MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.06f, 0.02f, 0.10f, 1.f));
				C->SetMaterial(0, MID);
			}
		Blobs.Add(C);
	}

	// Démarre à la HAUTEUR du crachat (couche du Kraken) : le brouillard flotte là.
	FVector L = GetActorLocation();
	L.Z = FMath::Max(GroundZ, FloatHeight);
	SetActorLocation(L);
}

void AWOTOLInkZone::Tick(float Dt)
{
	Super::Tick(Dt);
	Elapsed += Dt;

	UWorld* W = GetWorld();
	if (!W) return;
	const float Now = W->GetTimeSeconds();

	// ── PHASES : brouillard flottant (FogTime) -> écoulement vers le sol (DescendTime)
	// -> flaque au sol (le reste). Hauteur de l'acteur interpolée en conséquence. ──
	const bool bAirborne = (Elapsed < FogTime);
	{
		float z;
		if (Elapsed < FogTime)                 z = FMath::Max(GroundZ, FloatHeight);
		else if (Elapsed < FogTime + DescendTime)
		{
			const float t = (Elapsed - FogTime) / DescendTime;
			z = FMath::Lerp(FMath::Max(GroundZ, FloatHeight), GroundZ, t);
		}
		else                                   z = GroundZ;
		FVector L = GetActorLocation(); L.Z = z; SetActorLocation(L);
	}

	// ONDULATION type fumée sous-marine tant que c'est aérien, puis APLATISSEMENT au sol.
	for (int32 i = 0; i < Blobs.Num(); ++i)
	{
		if (!Blobs[i] || !BlobBase.IsValidIndex(i)) continue;
		const FVector B = BlobBase[i];
		if (bAirborne)
		{
			const float w = FMath::Sin(Now * 2.5f + i * 1.3f);
			Blobs[i]->SetRelativeLocation(B + FVector(w * 25.f, FMath::Cos(Now * 2.f + i) * 25.f, w * 20.f));
			const float br = Blobs[i]->GetRelativeScale3D().X;
			Blobs[i]->SetRelativeScale3D(FVector(br, br, 0.4f + 0.15f * w)); // volute qui respire
		}
		else
		{
			// s'aplatit progressivement en galette au sol
			FVector S = Blobs[i]->GetRelativeScale3D();
			S.Z = FMath::FInterpTo(S.Z, 0.03f, Dt, 6.f);
			Blobs[i]->SetRelativeScale3D(S);
			Blobs[i]->SetRelativeLocation(FVector(B.X, B.Y, 4.f));
		}
	}

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
