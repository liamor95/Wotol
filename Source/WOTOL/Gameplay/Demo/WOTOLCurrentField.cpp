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
	const float T = W ? W->GetTimeSeconds() : 0.f;

	for (int32 i = 0; i < Streaks.Num(); ++i)
	{
		UStaticMeshComponent* C = Streaks[i];
		if (!C) continue;
		if (!bActive) { C->SetVisibility(false); continue; }
		C->SetVisibility(true);

		const FVector Dir  = Cur->GetDirection();
		const FVector Side = FVector::CrossProduct(FVector::UpVector, Dir).GetSafeNormal();
		FVector P = C->GetRelativeLocation();
		// Dérive PLUS RAPIDE (montre la vitesse du courant) dans le sens choisi.
		const float F = Cur->GetFactorAt(P.Z);
		P += Dir * (Cur->GetStrength() * (1.4f + F) * Dt);
		// ONDULATION / BOUCLE (comme les traits de vent en dessin animé) : la traînée
		// SERPENTE perpendiculairement au courant selon sa progression + le temps.
		const float Along = FVector::DotProduct(P, Dir);
		const float Phase = Along * 0.004f + T * 3.0f + (float)i * 0.7f;
		const float Wave  = FMath::Sin(Phase) * (70.f + 50.f * F);
		// Remplace la composante latérale par l'onde -> chemin sinueux/looping.
		P -= Side * FVector::DotProduct(P, Side);
		P += Side * Wave;
		// Bouclage dans le volume (l'axe de dérive ré-enroule).
		if (Along > Span * 1.2f) P -= Dir * (Span * 2.2f);
		C->SetRelativeLocation(P);
		// Oriente la traînée le long du courant, inclinée selon la PENTE de l'onde
		// -> effet de virage/boucle plutôt qu'un simple trait droit.
		const float Slope = FMath::Cos(Phase) * 38.f;
		C->SetWorldRotation((Dir.Rotation() + FRotator(90.f, Slope, 0.f)).Quaternion());
	}
}
