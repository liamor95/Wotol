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

	// L'acteur est posé au SOL (Z fourni par le Kraken via trace). Le nuage est décrit en
	// LOCAL au-dessus, à FogTopLocal, et redescend en local -> pas de "bloc" qui tombe.
	FogTopLocal = FMath::Max(120.f, FloatHeight - GetActorLocation().Z);

	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	// Nuage VOLUMÉTRIQUE : ~40 bulles réparties dans un ellipsoïde (large en XY, haut en Z)
	// -> aspect fumée/encre, pas des disques plats.
	const int32 N = 42;
	const float CloudR = Radius * 0.60f; // rayon XY du nuage (~5 m² d'emprise visuelle)
	const float CloudH = 340.f;          // hauteur du nuage (volume)
	for (int32 i = 0; i < N; ++i)
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
		if (!C) continue;
		C->SetupAttachment(SceneRoot);
		C->RegisterComponent();
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCanEverAffectNavigation(false);
		if (Sphere) C->SetStaticMesh(Sphere);

		// Position dans l'ellipsoïde (distribution vers le centre pour un noyau dense).
		const float ang = InkRnd(i) * 6.283f;
		const float rr  = FMath::Pow(InkRnd(i + 7), 0.5f) * CloudR;
		const float hx  = FMath::Cos(ang) * rr;
		const float hy  = FMath::Sin(ang) * rr;
		const float hz  = FogTopLocal + (InkRnd(i + 13) - 0.4f) * CloudH;
		FWOTOLInkBubble B;
		B.Mesh   = C;
		B.Home   = FVector(hx, hy, hz);
		// Position finale au sol : étalée un peu plus large, aplatie (flaque).
		const float gr = rr * 1.25f + 40.f;
		B.Ground = FVector(FMath::Cos(ang) * gr, FMath::Sin(ang) * gr, 6.f + InkRnd(i + 31) * 6.f);
		B.Size   = 26.f + InkRnd(i + 19) * 34.f;
		B.Phase  = InkRnd(i + 23) * 6.283f;
		B.Drip   = InkRnd(i + 29); // écoulement échelonné (gouttes)

		const float s = B.Size / 50.f;
		C->SetRelativeScale3D(FVector(s, s, s));
		C->SetRelativeLocation(B.Home);

		if (Base)
			if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, this))
			{
				// Encre violet-noir bioluminescente, légèrement variable par bulle.
				const float v = 0.02f + InkRnd(i + 37) * 0.06f;
				MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.06f + v, 0.02f, 0.12f + v, 1.f));
				C->SetMaterial(0, MID);
			}
		Bubbles.Add(B);
	}
}

void AWOTOLInkZone::Tick(float Dt)
{
	Super::Tick(Dt);
	Elapsed += Dt;
	UWorld* W = GetWorld();
	if (!W) return;
	const float Now = W->GetTimeSeconds();

	for (int32 i = 0; i < Bubbles.Num(); ++i)
	{
		FWOTOLInkBubble& B = Bubbles[i];
		if (!B.Mesh) continue;

		// Ondulation permanente (fumée sous l'eau) : mouvement lent en tourbillon.
		const FVector Swirl(
			FMath::Sin(Now * 1.6f + B.Phase) * 22.f,
			FMath::Cos(Now * 1.3f + B.Phase * 1.3f) * 22.f,
			FMath::Sin(Now * 1.1f + B.Phase) * 16.f);

		// Fraction d'ÉCOULEMENT propre à la bulle (gouttes échelonnées via Drip).
		float fall = 0.f;
		if (Elapsed > FogTime)
		{
			const float local = (Elapsed - FogTime) / DescendTime; // 0..1 global
			// chaque bulle démarre à Drip*0.5 et met ~0.5 à couler -> écoulement progressif.
			fall = FMath::Clamp((local - B.Drip * 0.55f) / 0.55f, 0.f, 1.f);
			fall = fall * fall * (3.f - 2.f * fall); // lissage (smoothstep)
		}

		// Interpolation nuage -> sol, avec un léger "étirement" de goutte pendant la chute.
		FVector Pos = FMath::Lerp(B.Home, B.Ground, fall) + Swirl * (1.f - fall * 0.7f);
		B.Mesh->SetRelativeLocation(Pos);

		// Forme : sphère en l'air ; s'aplatit en galette une fois au sol (flaque).
		const float flat = FMath::Lerp(1.f, 0.28f, fall);
		const float widen = FMath::Lerp(1.f, 1.5f, fall);
		const float s = B.Size / 50.f;
		B.Mesh->SetRelativeScale3D(FVector(s * widen, s * widen, s * flat));
	}

	// Dissipation : fondu (rétrécissement) sur la dernière seconde.
	const float Remain = Lifetime - Elapsed;
	if (Remain < 1.f)
		SetActorScale3D(FVector(FMath::Max(0.06f, Remain)));

	// ── EFFET sur les unités DANS la zone (toutes factions sauf le Kraken) ──
	// Ralenti + précision ~0 (aveuglement), rafraîchis en continu ; léger poison ~1/s.
	DotAccum += Dt;
	const bool bDot = (DotAccum >= 1.f);
	if (bDot) DotAccum = 0.f;

	if (UFactionRegistrySubsystem* Reg = W->GetSubsystem<UFactionRegistrySubsystem>())
	{
		const FVector Cn = GetActorLocation();
		const float R2 = Radius * Radius;
		for (uint8 f = 1; f <= (uint8)EFactionID::PiratesAbyssaux; ++f)
			for (AUnitBase* U : Reg->GetUnitsForFaction((EFactionID)f))
			{
				if (!U || !U->IsAlive()) continue;
				if (AWOTOLDemoUnit* DU = Cast<AWOTOLDemoUnit>(U))
					if (DU->bCreatureBrain) continue;
				if (FVector::DistSquared2D(U->GetActorLocation(), Cn) > R2) continue;
				U->SlowUntil    = Now + 0.35f;
				U->BlindedUntil = Now + 0.35f;
				if (bDot) U->TakeDamageFromUnit(12.f, Caster.Get());
			}
	}

	if (Elapsed >= Lifetime) Destroy();
}
