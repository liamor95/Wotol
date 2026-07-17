#include "WOTOLRewardActor.h"
#include "WOTOLGlow.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

AWOTOLRewardActor::AWOTOLRewardActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	Spinner = CreateDefaultSubobject<USceneComponent>(TEXT("Spinner"));
	Spinner->SetupAttachment(SceneRoot);
}

void AWOTOLRewardActor::BeginPlay()
{
	Super::BeginPlay();
	BaseZ = GetActorLocation().Z;
	BuildVisual();
}

void AWOTOLRewardActor::BuildVisual()
{
	if (!Spinner) return;

	const TCHAR* M_SPH  = TEXT("/Engine/BasicShapes/Sphere.Sphere");
	const TCHAR* M_CONE = TEXT("/Engine/BasicShapes/Cone.Cone");
	const TCHAR* M_CYL  = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");

	auto AddPiece = [&](const TCHAR* MeshPath, const FVector& Loc, const FVector& Scale,
		const FRotator& Rot, const FLinearColor& Emissive, bool bGlow) -> UStaticMeshComponent*
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
		if (!C) return nullptr;
		C->SetupAttachment(Spinner);
		C->RegisterComponent();
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, MeshPath)) C->SetStaticMesh(M);
		C->SetRelativeLocationAndRotation(Loc, Rot);
		C->SetRelativeScale3D(Scale);
		UMaterialInstanceDynamic* MID = bGlow ? WOTOLGlow::MakeGlow(this, Emissive)
		                                      : WOTOLGlow::MakeMatte(this, Emissive);
		if (MID) C->SetMaterial(0, MID);
		return C;
	};

	if (RewardType == EWOTOLRewardType::HeartShard)
	{
		// CŒUR-ÉCLAT : cristal bleu rayonnant (2 cônes joints en losange) + halo.
		const FLinearColor Blue(0.35f, 0.85f, 3.0f, 1.f); // HDR bleu vif (bloom)
		AddPiece(M_CONE, FVector(0, 0, 55.f),  FVector(0.7f, 0.7f, 1.1f), FRotator(0, 0, 0),    Blue, true);
		AddPiece(M_CONE, FVector(0, 0, -55.f), FVector(0.7f, 0.7f, 1.1f), FRotator(180, 0, 0),  Blue, true);
		AddPiece(M_SPH,  FVector(0, 0, 0.f),   FVector(0.35f, 0.35f, 0.35f), FRotator::ZeroRotator, Blue, true);
	}
	else
	{
		// ŒUF DE LÉVIAPHÉNIX : œuf ovoïde doré-cyan posé dans un petit nid sombre.
		const FLinearColor Nest(0.06f, 0.08f, 0.10f, 1.f);
		const FLinearColor Egg (2.2f, 1.6f, 0.5f, 1.f);   // HDR doré chaud
		AddPiece(M_CYL, FVector(0, 0, -70.f), FVector(1.6f, 1.6f, 0.5f), FRotator::ZeroRotator, Nest, false); // nid
		AddPiece(M_SPH, FVector(0, 0, 0.f),   FVector(0.9f, 0.9f, 1.3f), FRotator::ZeroRotator, Egg, true);    // œuf
	}
}

void AWOTOLRewardActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bCollected) return;

	// Rotation lente + flottaison douce (attire l'œil, sensation aquatique).
	if (Spinner) Spinner->AddLocalRotation(FRotator(0.f, 45.f * DeltaSeconds, 0.f));
	BobPhase += DeltaSeconds;
	FVector L = GetActorLocation();
	L.Z = BaseZ + FMath::Sin(BobPhase * 1.6f) * 12.f;
	SetActorLocation(L);

	// Récupération de PROXIMITÉ : le héros joueur s'approche à portée.
	if (bAutoCollectByProximity)
	{
		if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
		{
			if (APawn* Hero = PC->GetPawn())
			{
				if (FVector::Dist(Hero->GetActorLocation(), GetActorLocation()) <= CollectRadius)
				{
					Collect();
				}
			}
		}
	}
}

void AWOTOLRewardActor::Collect()
{
	if (bCollected) return;
	bCollected = true;
	OnRewardCollected.Broadcast(RewardType);
	// Petit délai avant destruction visuelle (laisse le temps à un effet BP éventuel).
	SetActorEnableCollision(false);
	Destroy();
}
