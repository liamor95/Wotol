#include "WOTOLProjectileTracer.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/PointLightComponent.h"
#include "WOTOLGlow.h"

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
	const FLinearColor& Color, float Size, bool bBolt)
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
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	const FLinearColor Core(FMath::Min(1.f, Color.R + 0.3f),
		FMath::Min(1.f, Color.G + 0.3f), FMath::Min(1.f, Color.B + 0.3f), 1.f);

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
	// Cœur du projectile ÉMISSIF au MÊME niveau que le rayon de Noxar (×3.2 +0.3) : il
	// RAYONNE fort, on voit la lumière colorée émaner du projectile pendant tout son vol.
	if (UMaterialInstanceDynamic* MID = WOTOLGlow::MakeGlow(T,
			FLinearColor(Core.R * 3.2f + 0.3f, Core.G * 3.2f + 0.3f, Core.B * 3.2f + 0.3f, 1.f)))
		T->Ball->SetMaterial(0, MID);
	// Lampe = couleur SATURÉE du tir -> le halo qui émane est bien coloré (bleu Aquisphères,
	// violet Noxeblast, etc.).
	if (T->Glow) T->Glow->SetLightColor(Core);

	// HALO lumineux autour du cœur (émissif plus doux) -> nimbe coloré bien visible.
	if (Sphere) T->Halo->SetStaticMesh(Sphere);
	T->Halo->SetRelativeScale3D(bBolt ? FVector(1.3f, 1.8f, 1.8f) : FVector(1.7f, 1.7f, 1.7f));
	if (UMaterialInstanceDynamic* MID = WOTOLGlow::MakeGlow(T,
			FLinearColor(Core.R * 1.6f, Core.G * 1.6f, Core.B * 1.6f, 1.f)))
		T->Halo->SetMaterial(0, MID);
}

void AWOTOLProjectileTracer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Life += DeltaSeconds;

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
