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
	if (UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")))
	{
		T->Ball->SetStaticMesh(M);
	}
	const float S = 0.28f * Size;
	T->Ball->SetRelativeScale3D(FVector(S, S, S));
	if (UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, T))
		{
			MID->SetVectorParameterValue(TEXT("Color"), Color);
			T->Ball->SetMaterial(0, MID);
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
