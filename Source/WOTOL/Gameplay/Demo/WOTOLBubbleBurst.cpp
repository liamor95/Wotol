#include "WOTOLBubbleBurst.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

AWOTOLBubbleBurst::AWOTOLBubbleBurst()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
}

void AWOTOLBubbleBurst::Burst(UWorld* World, const FVector& Loc, const FLinearColor& Color, int32 Count)
{
	if (!World) return;

	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AWOTOLBubbleBurst* FX = World->SpawnActor<AWOTOLBubbleBurst>(
		AWOTOLBubbleBurst::StaticClass(), Loc, FRotator::ZeroRotator, P);
	if (!FX) return;

	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	for (int32 i = 0; i < Count; ++i)
	{
		UStaticMeshComponent* B = NewObject<UStaticMeshComponent>(FX);
		if (!B) continue;
		B->SetupAttachment(FX->Root);
		B->RegisterComponent();
		B->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (Sphere) B->SetStaticMesh(Sphere);

		const float S = FMath::FRandRange(0.10f, 0.28f);
		const FVector Scale(S, S, S);
		B->SetRelativeScale3D(Scale);
		B->SetRelativeLocation(FVector(
			FMath::FRandRange(-25.f, 25.f), FMath::FRandRange(-25.f, 25.f), FMath::FRandRange(-10.f, 20.f)));

		if (BaseMat)
		{
			if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, FX))
			{
				MID->SetVectorParameterValue(TEXT("Color"), Color);
				B->SetMaterial(0, MID);
			}
		}
		FX->Bubbles.Add(B);
		FX->BaseScales.Add(Scale);
		FX->Vels.Add(FVector(
			FMath::FRandRange(-40.f, 40.f), FMath::FRandRange(-40.f, 40.f), FMath::FRandRange(80.f, 180.f)));
	}
}

void AWOTOLBubbleBurst::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Life += DeltaSeconds;
	const float T = FMath::Clamp(Life / MaxLife, 0.f, 1.f);

	for (int32 i = 0; i < Bubbles.Num(); ++i)
	{
		if (!Bubbles[i]) continue;
		Bubbles[i]->AddRelativeLocation(Vels[i] * DeltaSeconds);
		Bubbles[i]->SetRelativeScale3D(BaseScales[i] * FMath::Max(0.02f, 1.f - T)); // rétrécit
	}

	if (Life >= MaxLife)
	{
		Destroy();
	}
}
