#include "WOTOLCurrentField.h"
#include "OceanCurrentSubsystem.h"
#include "WOTOLGlow.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
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
	UStaticMesh* Sph = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	// ── TRAÎNÉES : longues stries ÉMISSIVES qui filent dans la bande (frémissement du courant).
	const int32 N = 46;
	for (int32 i = 0; i < N; ++i)
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
		if (!C) continue;
		C->SetupAttachment(SceneRoot);
		C->RegisterComponent();
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCanEverAffectNavigation(false);
		if (Cyl) C->SetStaticMesh(Cyl);
		// Fine et TRÈS allongée = strie de courant.
		C->SetRelativeScale3D(FVector(0.07f, 0.07f, FMath::FRandRange(3.2f, 5.5f)));
		// Émissif cyan -> la bande BRILLE et se distingue nettement (bloom).
		if (UMaterialInstanceDynamic* MID = WOTOLGlow::MakeGlow(this, FLinearColor(0.5f, 1.4f, 2.2f, 1.f)))
			C->SetMaterial(0, MID);
		StreakCoord.Add(FVector(
			FMath::FRandRange(-Span, Span),
			FMath::FRandRange(-HalfWidth, HalfWidth),
			FMath::FRandRange(ZLow, ZHigh)));
		Streaks.Add(C);
	}

	// ── BULLES : NUÉE dense qui file dans la bande -> matérialise le flux et sa vitesse.
	const int32 NB = 320;
	for (int32 i = 0; i < NB; ++i)
	{
		UStaticMeshComponent* B = NewObject<UStaticMeshComponent>(this);
		if (!B) continue;
		B->SetupAttachment(SceneRoot);
		B->RegisterComponent();
		B->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		B->SetCanEverAffectNavigation(false);
		if (Sph) B->SetStaticMesh(Sph);
		const float s = FMath::FRandRange(0.05f, 0.20f);
		B->SetRelativeScale3D(FVector(s, s, s));
		if (UMaterialInstanceDynamic* MID = WOTOLGlow::MakeGlow(this, FLinearColor(0.8f, 1.3f, 1.8f, 1.f)))
			B->SetMaterial(0, MID);
		BubbleCoord.Add(FVector(
			FMath::FRandRange(-Span, Span),
			FMath::FRandRange(-HalfWidth, HalfWidth),
			FMath::FRandRange(ZLow - 200.f, ZHigh)));
		Bubbles.Add(B);
	}
}

void AWOTOLCurrentField::Tick(float Dt)
{
	Super::Tick(Dt);
	UWorld* W = GetWorld();
	UOceanCurrentSubsystem* Cur = W ? W->GetSubsystem<UOceanCurrentSubsystem>() : nullptr;
	const bool bActive = Cur && Cur->IsActive();
	const float T = W ? W->GetTimeSeconds() : 0.f;

	if (!bActive)
	{
		for (UStaticMeshComponent* C : Streaks) if (C) C->SetVisibility(false);
		for (UStaticMeshComponent* B : Bubbles) if (B) B->SetVisibility(false);
		return;
	}

	const FVector Dir  = Cur->GetDirection();
	const FVector Side = FVector::CrossProduct(FVector::UpVector, Dir).GetSafeNormal();
	const float   Str  = Cur->GetStrength();

	// Vitesse de défilement de la bande : NETTEMENT plus rapide que la dérive gameplay pour
	// donner la SENSATION de vitesse (« ça file »).
	auto Wrap = [&](float along) { if (along > Span) along -= 2.f * Span; else if (along < -Span) along += 2.f * Span; return along; };

	for (int32 i = 0; i < Streaks.Num(); ++i)
	{
		UStaticMeshComponent* C = Streaks[i];
		if (!C) continue;
		C->SetVisibility(true);
		FVector& Co = StreakCoord[i];
		const float F = Cur->GetFactorAt(Co.Z);
		Co.X = Wrap(Co.X + Str * (2.6f + 1.4f * F) * Dt); // file vite le long du courant
		// FRÉMISSEMENT : la strie serpente perpendiculairement (onde qui remonte la bande).
		const float Phase = Co.X * 0.004f + T * 3.2f + (float)i * 0.6f;
		const float Wave  = FMath::Sin(Phase) * (90.f + 60.f * F);
		const FVector Pos = Dir * Co.X + Side * (Co.Y + Wave) + FVector(0, 0, Co.Z);
		C->SetRelativeLocation(Pos);
		const float Slope = FMath::Cos(Phase) * 34.f;
		C->SetWorldRotation((Dir.Rotation() + FRotator(90.f, Slope, 0.f)).Quaternion());
	}

	for (int32 i = 0; i < Bubbles.Num(); ++i)
	{
		UStaticMeshComponent* B = Bubbles[i];
		if (!B) continue;
		B->SetVisibility(true);
		FVector& Co = BubbleCoord[i];
		const float F = Cur->GetFactorAt(Co.Z);
		Co.X = Wrap(Co.X + Str * (2.2f + 1.2f * F) * Dt);
		// Léger dandinement (vie de l'eau) sans quitter la bande.
		const float Ph = T * 2.4f + (float)i * 0.7f;
		const float perp = Co.Y + FMath::Sin(Ph) * 40.f;
		const float z    = Co.Z + FMath::Sin(Ph * 1.6f) * 30.f;
		const FVector Pos = Dir * Co.X + Side * perp + FVector(0, 0, z);
		B->SetRelativeLocation(Pos);
	}
}
