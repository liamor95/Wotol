#include "WOTOLProjectileTracer.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/PointLightComponent.h"
#include "WOTOLGlow.h"
#include "WOTOLBubbleBurst.h"

AWOTOLProjectileTracer::AWOTOLProjectileTracer()
{
	PrimaryActorTick.bCanEverTick = true;

	Ball = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ball"));
	RootComponent = Ball;
	Ball->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Ball->SetCanEverAffectNavigation(false);

	// Source de lumière RATTACHÉE au projectile (il éclaire ce qu'il traverse).
	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(Ball);
	Glow->SetCastShadows(false);
	// HALO LUMINEUX qui ÉMANE du projectile (même logique que le rayon de Noxar) : on
	// voit la lumière colorée se dégager du projectile pendant tout son vol jusqu'à la cible.
	Glow->SetAttenuationRadius(420.f);
	Glow->SetIntensity(2600.f);

	// Halo plus large autour du cœur = boule bien plus repérable à l'écran.
	Halo = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Halo"));
	Halo->SetupAttachment(Ball);
	Halo->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Halo->SetCanEverAffectNavigation(false);
}

void AWOTOLProjectileTracer::Fire(UWorld* World, const FVector& From, const FVector& To,
	const FLinearColor& Color, float Size, bool bBolt, bool bBubbleTrail)
{
	if (!World) return;
	// Oriente l'acteur vers la cible (utile pour l'ovale allongé).
	const FRotator Aim = (To - From).Rotation();
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AWOTOLProjectileTracer* T = World->SpawnActor<AWOTOLProjectileTracer>(
		AWOTOLProjectileTracer::StaticClass(), From, Aim, P);
	if (!T) return;

	T->Target = To;
	// Phase 3 (faible GPU) : pas de traînée de bulles (sature le plafond -> lag).
	T->bTrail = bBubbleTrail && !WOTOLGlow::bLowGpuVFX;
	T->TrailColor = FLinearColor(Color.R, Color.G, Color.B, 1.f);
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	// Couleur SATURÉE de base (on garde la dominante -> pas de délavage vers le blanc).
	const FLinearColor Sat(Color.R, Color.G, Color.B, 1.f);

	// Taille de VRAI projectile (bien plus petit que les unités). Rayon ~15-25 cm.
	if (Sphere) T->Ball->SetStaticMesh(Sphere);
	if (bBolt)
	{
		// Ovale ALLONGÉ le long de l'axe X (sens de tir) : fin et effilé = trait/projectile.
		T->Ball->SetRelativeScale3D(FVector(0.55f * Size, 0.14f * Size, 0.14f * Size));
	}
	else
	{
		// Petite sphère.
		const float S = 0.22f * Size;
		T->Ball->SetRelativeScale3D(FVector(S, S, S));
	}
	(void)BaseMat;
	// Cœur ÉMISSIF = couleur BRUTE ×3 (comme le rayon de Noxar : Color*3+0.2). On NE
	// relève PAS tous les canaux (ce qui délavait en blanc) -> la DOMINANTE reste visible
	// (cyan Aquisphères, violet Noxeblast) au lieu de paraître blanc.
	if (UMaterialInstanceDynamic* MID = WOTOLGlow::MakeGlow(T,
			FLinearColor(Sat.R * 3.0f + 0.15f, Sat.G * 3.0f + 0.15f, Sat.B * 3.0f + 0.15f, 1.f)))
		T->Ball->SetMaterial(0, MID);
	// Lampe = couleur SATURÉE brute du tir -> halo bien coloré (pas blanc). En MODE FAIBLE GPU
	// (phase 3, dizaines de tirs simultanés) on SUPPRIME la lampe dynamique (le cœur émissif +
	// halo restent -> le projectile brille toujours à l'écran, sans le coût des lampes).
	if (T->Glow)
	{
		if (WOTOLGlow::bLowGpuVFX) { T->Glow->DestroyComponent(); T->Glow = nullptr; }
		else                       { T->Glow->SetLightColor(Sat); }
	}

	// HALO autour du cœur (émissif doux, couleur brute) -> nimbe coloré, pas un voile blanc.
	if (Sphere) T->Halo->SetStaticMesh(Sphere);
	T->Halo->SetRelativeScale3D(bBolt ? FVector(1.3f, 1.8f, 1.8f) : FVector(1.7f, 1.7f, 1.7f));
	if (UMaterialInstanceDynamic* MID = WOTOLGlow::MakeGlow(T,
			FLinearColor(Sat.R * 1.5f, Sat.G * 1.5f, Sat.B * 1.5f, 1.f)))
		T->Halo->SetMaterial(0, MID);
}

void AWOTOLProjectileTracer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Life += DeltaSeconds;

	// TRAÎNÉE DE BULLES : le projectile brasse l'eau -> petit chapelet de bulles qui suit la
	// boule tout le long de son vol (mécanique des fluides).
	if (bTrail)
	{
		TrailAccum += DeltaSeconds;
		if (TrailAccum >= 0.035f)
		{
			TrailAccum = 0.f;
			AWOTOLBubbleBurst::Burst(GetWorld(), GetActorLocation(), TrailColor, 2);
		}
	}

	const FVector Loc = GetActorLocation();
	const FVector ToTarget = Target - Loc;
	const float Step = Speed * DeltaSeconds;
	if (ToTarget.SizeSquared() <= Step * Step || Life > 1.2f)
	{
		Destroy();
		return;
	}
	SetActorLocation(Loc + ToTarget.GetSafeNormal() * Step);
}
