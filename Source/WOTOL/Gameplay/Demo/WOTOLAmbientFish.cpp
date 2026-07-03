#include "WOTOLAmbientFish.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

AWOTOLAmbientFish::AWOTOLAmbientFish()
{
	PrimaryActorTick.bCanEverTick = true;

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	RootComponent = Body;
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetCanEverAffectNavigation(false);

	Tail = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tail"));
	Tail->SetupAttachment(Body);
	Tail->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AWOTOLAmbientFish::Configure(const FVector& InCenter, float InRadius, float InSpeed,
	float InPhase, float InHeightAmp, float InBaseZ, const FLinearColor& Color, float SizeM)
{
	CenterPoint = InCenter; Radius = InRadius; Speed = InSpeed;
	Phase = InPhase; HeightAmp = InHeightAmp; BaseZ = InBaseZ;

	const float S = SizeM;
	// Corps = sphère ALLONGÉE le long de X (sens de nage) -> poisson horizontal
	if (UStaticMesh* MB = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")))
	{
		Body->SetStaticMesh(MB);
	}
	if (UStaticMesh* MT = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone")))
	{
		Tail->SetStaticMesh(MT);
	}
	Body->SetRelativeScale3D(FVector(S * 1.10f, S * 0.45f, S * 0.42f));
	Body->SetRelativeRotation(FRotator::ZeroRotator);
	// Nageoire caudale (cône pointant vers l'arrière -X)
	Tail->SetRelativeScale3D(FVector(0.26f, 0.26f, S * 0.5f));
	Tail->SetRelativeLocation(FVector(-S * 48.f, 0.f, 0.f));
	Tail->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));

	UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (BaseMat)
	{
		if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, this))
		{
			MID->SetVectorParameterValue(TEXT("Color"), Color);
			Body->SetMaterial(0, MID);
			Tail->SetMaterial(0, MID);
		}
	}
}

void AWOTOLAmbientFish::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Angle += Speed * DeltaSeconds;

	const float C = FMath::Cos(Angle + Phase);
	const float Sn = FMath::Sin(Angle + Phase);
	const FVector Pos = CenterPoint + FVector(C * Radius, Sn * Radius,
		BaseZ + FMath::Sin((Angle + Phase) * 2.f) * HeightAmp);
	SetActorLocation(Pos);

	// Oriente le poisson dans le sens de la nage (tangente au cercle)
	const FVector Tangent(-Sn, C, 0.f);
	SetActorRotation(Tangent.Rotation());
}
