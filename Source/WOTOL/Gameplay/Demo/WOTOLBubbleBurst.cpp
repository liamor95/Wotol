#include "WOTOLBubbleBurst.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

int32 AWOTOLBubbleBurst::LiveCount = 0;
int32 AWOTOLBubbleBurst::MaxLive   = 130; // plafond global d'eclats simultanes

AWOTOLBubbleBurst::AWOTOLBubbleBurst()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
}

void AWOTOLBubbleBurst::EndPlay(const EEndPlayReason::Type Reason)
{
	--LiveCount;
	Super::EndPlay(Reason);
}

void AWOTOLBubbleBurst::Burst(UWorld* World, const FVector& Loc, const FLinearColor& Color, int32 Count)
{
	if (!World) return;
	// PLAFOND : au-dela, on n'ajoute plus d'eclats (evite l'accumulation qui fait ramer la
	// phase 3). Les effets restent presents, simplement bornes pendant les pics.
	if (LiveCount >= MaxLive) return;

	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AWOTOLBubbleBurst* FX = World->SpawnActor<AWOTOLBubbleBurst>(
		AWOTOLBubbleBurst::StaticClass(), Loc, FRotator::ZeroRotator, P);
	if (!FX) return;
	++LiveCount;

	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	// Un SEUL materiau partage pour tout l'eclat (au lieu d'un MID par bulle) -> moins de coût.
	UMaterialInstanceDynamic* SharedMID = BaseMat ? UMaterialInstanceDynamic::Create(BaseMat, FX) : nullptr;
	if (SharedMID) SharedMID->SetVectorParameterValue(TEXT("Color"), Color);

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

		if (SharedMID) B->SetMaterial(0, SharedMID);
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
