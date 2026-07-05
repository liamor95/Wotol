#include "WOTOLCurrentField.h"
#include "OceanCurrentSubsystem.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

AWOTOLCurrentField::AWOTOLCurrentField()
{
	PrimaryActorTick.bCanEverTick = true;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;
}

void AWOTOLCurrentField::BeginPlay()
{
	Super::BeginPlay();

	UStaticMesh* Cyl = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UMaterialInterface* Base_ = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	const int32 N = 20;
	for (int32 i = 0; i < N; ++i)
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
		if (!C) continue;
		C->SetupAttachment(SceneRoot);
		C->RegisterComponent();
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCanEverAffectNavigation(false);
		if (Cyl) C->SetStaticMesh(Cyl);
		// Traînée fine et allongée (couchée le long de +X, orientée ensuite vers le courant).
		C->SetRelativeScale3D(FVector(0.06f, 0.06f, 2.2f));
		const FVector P(
			FMath::FRandRange(-Span, Span),
			FMath::FRandRange(-Span, Span),
			FMath::FRandRange(ZLow, ZHigh));
		C->SetRelativeLocation(P);
		if (Base_)
			if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base_, this))
			{
				MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.4f, 0.75f, 1.f, 1.f));
				C->SetMaterial(0, MID);
			}
		Streaks.Add(C);
		Base.Add(P);
	}
}

void AWOTOLCurrentField::Tick(float Dt)
{
	Super::Tick(Dt);
	UWorld* W = GetWorld();
	UOceanCurrentSubsystem* Cur = W ? W->GetSubsystem<UOceanCurrentSubsystem>() : nullptr;
	const bool bActive = Cur && Cur->IsActive();

	for (int32 i = 0; i < Streaks.Num(); ++i)
	{
		UStaticMeshComponent* C = Streaks[i];
		if (!C) continue;
		if (!bActive) { C->SetVisibility(false); continue; }
		C->SetVisibility(true);

		const FVector Dir = Cur->GetDirection();
		FVector P = C->GetRelativeLocation();
		// Dérive (vitesse ∝ intensité + facteur de couche : plus haut = plus vite).
		const float F = Cur->GetFactorAt(P.Z);
		P += Dir * (Cur->GetStrength() * (0.7f + F) * Dt);
		// Bouclage dans le volume (l'axe de dérive ré-enroule).
		if (FVector::DotProduct(P, Dir) > Span * 1.2f) P -= Dir * (Span * 2.2f);
		C->SetRelativeLocation(P);
		// Oriente la traînée le long du courant.
		C->SetWorldRotation((Dir.Rotation() + FRotator(90.f, 0.f, 0.f)).Quaternion());
	}
}
