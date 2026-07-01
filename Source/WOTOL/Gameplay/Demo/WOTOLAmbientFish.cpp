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
	if (UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone")))
	{
		Body->SetStaticMesh(M);
		Tail->SetStaticMesh(M);
	}
	// Corps allongé (cône couché) + petite queue
	Body->SetRelativeScale3D(FVector(S * 0.35f, S * 0.35f, S));
	Body->SetRelativeRotation(FRotator(90.f, 0.f, 0.f)); // pointe vers l'avant (+X)
	Tail->SetRelativeScale3D(FVector(0.6f, 0.6f, 0.5f));
	Tail->SetRelativeLocation(FVector(0.f, 0.f, -S * 60.f)); // à l'arrière du corps

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
