#include "WOTOLProjectileTracer.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

AWOTOLProjectileTracer::AWOTOLProjectileTracer()
{
	PrimaryActorTick.bCanEverTick = true;

	Ball = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ball"));
	RootComponent = Ball;
	Ball->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Ball->SetCanEverAffectNavigation(false);

	// Halo plus large autour du cœur = boule bien plus repérable à l'écran.
	Halo = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Halo"));
	Halo->SetupAttachment(Ball);
	Halo->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Halo->SetCanEverAffectNavigation(false);
}

void AWOTOLProjectileTracer::Fire(UWorld* World, const FVector& From, const FVector& To,
	const FLinearColor& Color, float Size)
{
	if (!World) return;
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AWOTOLProjectileTracer* T = World->SpawnActor<AWOTOLProjectileTracer>(
		AWOTOLProjectileTracer::StaticClass(), From, FRotator::ZeroRotator, P);
	if (!T) return;

	T->Target = To;
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	// Cœur vif (couleur saturée, un peu éclaircie pour "briller")
	const float S = 0.55f * Size; // nettement plus gros qu'avant (0.28)
	if (Sphere) T->Ball->SetStaticMesh(Sphere);
	T->Ball->SetRelativeScale3D(FVector(S, S, S));
	if (BaseMat)
	{
		if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, T))
		{
			const FLinearColor Core(FMath::Min(1.f, Color.R + 0.3f),
				FMath::Min(1.f, Color.G + 0.3f), FMath::Min(1.f, Color.B + 0.3f), 1.f);
			MID->SetVectorParameterValue(TEXT("Color"), Core);
			T->Ball->SetMaterial(0, MID);
		}
	}
	// Halo (env. 1.9× le cœur, couleur de faction pleine) : lisible de loin.
	if (Sphere) T->Halo->SetStaticMesh(Sphere);
	T->Halo->SetRelativeScale3D(FVector(1.9f, 1.9f, 1.9f)); // relatif au cœur
	if (BaseMat)
	{
		if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, T))
		{
			MID->SetVectorParameterValue(TEXT("Color"), Color);
			T->Halo->SetMaterial(0, MID);
		}
	}
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
