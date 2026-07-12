#include "WOTOLAmbientFish.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "WOTOLBubbleBurst.h"

namespace
{
	const TCHAR* SPH = TEXT("/Engine/BasicShapes/Sphere.Sphere");
	const TCHAR* CON = TEXT("/Engine/BasicShapes/Cone.Cone");
	const TCHAR* CUB = TEXT("/Engine/BasicShapes/Cube.Cube");
}

AWOTOLAmbientFish::AWOTOLAmbientFish()
{
	PrimaryActorTick.bCanEverTick = true;
	Hull = CreateDefaultSubobject<USceneComponent>(TEXT("Hull"));
	RootComponent = Hull;
}

UStaticMeshComponent* AWOTOLAmbientFish::AddMesh(USceneComponent* Parent, const TCHAR* MeshPath,
	const FVector& RelLoc, const FVector& Scale, const FRotator& Rot, const FLinearColor& Color)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
	if (!C) return nullptr;
	C->SetupAttachment(Parent ? Parent : RootComponent.Get());
	C->RegisterComponent();
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetCanEverAffectNavigation(false);
	if (UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, MeshPath)) C->SetStaticMesh(M);
	C->SetRelativeLocation(RelLoc);
	C->SetRelativeScale3D(Scale);
	C->SetRelativeRotation(Rot);
	if (UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
		if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, this))
		{
			MID->SetVectorParameterValue(TEXT("Color"), Color);
			C->SetMaterial(0, MID);
		}
	return C;
}

void AWOTOLAmbientFish::Configure(const FVector& InCenter, float InRadius, float InSpeed,
	float InPhase, float InHeightAmp, float InBaseZ, const FLinearColor& Color, float SizeM,
	EFishSpecies InSpecies)
{
	CenterPoint = InCenter; Radius = InRadius; Speed = InSpeed;
	Phase = InPhase; HeightAmp = InHeightAmp; BaseZ = InBaseZ; Species = InSpecies;

	// Échelle : L = longueur du corps en uu. Les fins/queue sont exprimées en fraction de L.
	const float L = SizeM * 120.f;
	BodyLen = L;
	const FLinearColor Belly = Color * 1.35f; // ventre plus clair
	const FLinearColor Dark  = Color * 0.65f; // nageoires plus sombres

	// Pivot de queue à l'arrière du corps (-X) : la queue s'y accroche et bat.
	TailPivot = NewObject<USceneComponent>(this);
	TailPivot->SetupAttachment(Hull);
	TailPivot->RegisterComponent();
	TailPivot->SetRelativeLocation(FVector(-L * 0.5f, 0.f, 0.f));

	// Helper : nageoire caudale VERTICALE (poissons/requins) = plaque fine en éventail.
	auto CaudalVertical = [&](float w, float h)
	{
		AddMesh(TailPivot, CUB, FVector(-L * 0.12f, 0, 0), FVector(w * 0.4f, 0.03f, h), FRotator(0, 0, 0), Dark);
	};
	// Nageoire caudale HORIZONTALE (mammifères marins) = plaque fine à plat.
	auto CaudalHorizontal = [&](float w, float h)
	{
		AddMesh(TailPivot, CUB, FVector(-L * 0.12f, 0, 0), FVector(w * 0.4f, h, 0.03f), FRotator(0, 0, 0), Dark);
	};

	switch (Species)
	{
	case EFishSpecies::Shark:
	{
		SwimRate = 3.0f;
		// Corps fuselé long.
		Body = AddMesh(Hull, SPH, FVector(0, 0, 0), FVector(L / 100.f * 1.0f, L / 100.f * 0.30f, L / 100.f * 0.34f), FRotator::ZeroRotator, Color);
		AddMesh(Hull, SPH, FVector(0, 0, -L * 0.10f), FVector(L / 100.f * 0.85f, L / 100.f * 0.24f, L / 100.f * 0.14f), FRotator::ZeroRotator, Belly); // ventre clair
		AddMesh(Hull, CON, FVector(L * 0.58f, 0, 0), FVector(L / 100.f * 0.20f, L / 100.f * 0.20f, L / 100.f * 0.44f), FRotator(90, 0, 0), Color); // museau ALLONGÉ
		// Aileron dorsal TRIANGULAIRE haut (signature requin).
		AddMesh(Hull, CON, FVector(-L * 0.02f, 0, L * 0.24f), FVector(L / 100.f * 0.22f, 0.04f, L / 100.f * 0.38f), FRotator(0, 0, 0), Dark);
		// Pectorales plates inclinées.
		AddMesh(Hull, CUB, FVector(L * 0.05f, L * 0.22f, -L * 0.06f), FVector(L / 100.f * 0.16f, L / 100.f * 0.30f, 0.03f), FRotator(0, 0, 20), Dark);
		AddMesh(Hull, CUB, FVector(L * 0.05f, -L * 0.22f, -L * 0.06f), FVector(L / 100.f * 0.16f, L / 100.f * 0.30f, 0.03f), FRotator(0, 0, -20), Dark);
		CaudalVertical(L / 100.f * 0.9f, L / 100.f * 0.5f);
		break;
	}
	case EFishSpecies::Dolphin:
	{
		SwimRate = 4.0f;
		Body = AddMesh(Hull, SPH, FVector(0, 0, 0), FVector(L / 100.f, L / 100.f * 0.32f, L / 100.f * 0.34f), FRotator::ZeroRotator, Color);
		AddMesh(Hull, SPH, FVector(0, 0, -L * 0.10f), FVector(L / 100.f * 0.8f, L / 100.f * 0.24f, L / 100.f * 0.14f), FRotator::ZeroRotator, Belly);
		AddMesh(Hull, CON, FVector(L * 0.52f, 0, 0), FVector(L / 100.f * 0.14f, L / 100.f * 0.14f, L / 100.f * 0.28f), FRotator(90, 0, 0), Color); // rostre fin
		AddMesh(Hull, CON, FVector(0, 0, L * 0.22f), FVector(L / 100.f * 0.18f, 0.04f, L / 100.f * 0.24f), FRotator(-24, 0, 0), Dark); // dorsale courbée
		AddMesh(Hull, CUB, FVector(L * 0.08f, L * 0.20f, -L * 0.05f), FVector(L / 100.f * 0.18f, L / 100.f * 0.26f, 0.03f), FRotator(0, 0, 18), Dark);
		AddMesh(Hull, CUB, FVector(L * 0.08f, -L * 0.20f, -L * 0.05f), FVector(L / 100.f * 0.18f, L / 100.f * 0.26f, 0.03f), FRotator(0, 0, -18), Dark);
		CaudalHorizontal(L / 100.f * 0.8f, L / 100.f * 0.5f);
		break;
	}
	case EFishSpecies::Orca:
	{
		SwimRate = 3.4f;
		Body = AddMesh(Hull, SPH, FVector(0, 0, 0), FVector(L / 100.f, L / 100.f * 0.40f, L / 100.f * 0.42f), FRotator::ZeroRotator, FLinearColor(0.04f, 0.05f, 0.07f, 1.f));
		AddMesh(Hull, SPH, FVector(L * 0.05f, 0, -L * 0.14f), FVector(L / 100.f * 0.75f, L / 100.f * 0.30f, L / 100.f * 0.16f), FRotator::ZeroRotator, FLinearColor(0.95f, 0.95f, 0.98f, 1.f)); // ventre BLANC
		// Aileron dorsal TRÈS HAUT ET DROIT (signature orque).
		AddMesh(Hull, CON, FVector(-L * 0.02f, 0, L * 0.40f), FVector(L / 100.f * 0.18f, 0.04f, L / 100.f * 0.62f), FRotator(0, 0, 0), FLinearColor(0.04f, 0.05f, 0.07f, 1.f));
		AddMesh(Hull, CUB, FVector(L * 0.06f, L * 0.26f, -L * 0.08f), FVector(L / 100.f * 0.20f, L / 100.f * 0.30f, 0.03f), FRotator(0, 0, 20), FLinearColor(0.04f, 0.05f, 0.07f, 1.f));
		AddMesh(Hull, CUB, FVector(L * 0.06f, -L * 0.26f, -L * 0.08f), FVector(L / 100.f * 0.20f, L / 100.f * 0.30f, 0.03f), FRotator(0, 0, -20), FLinearColor(0.04f, 0.05f, 0.07f, 1.f));
		CaudalHorizontal(L / 100.f * 0.95f, L / 100.f * 0.55f);
		break;
	}
	case EFishSpecies::Whale:
	{
		SwimRate = 1.8f;
		Body = AddMesh(Hull, SPH, FVector(0, 0, 0), FVector(L / 100.f, L / 100.f * 0.5f, L / 100.f * 0.5f), FRotator::ZeroRotator, Color);
		AddMesh(Hull, SPH, FVector(L * 0.42f, 0, 0), FVector(L / 100.f * 0.5f, L / 100.f * 0.46f, L / 100.f * 0.42f), FRotator::ZeroRotator, Color); // grosse tête
		AddMesh(Hull, SPH, FVector(0, 0, -L * 0.16f), FVector(L / 100.f * 0.85f, L / 100.f * 0.4f, L / 100.f * 0.2f), FRotator::ZeroRotator, Belly);
		AddMesh(Hull, CON, FVector(-L * 0.28f, 0, L * 0.28f), FVector(L / 100.f * 0.14f, 0.04f, L / 100.f * 0.16f), FRotator(-18, 0, 0), Dark); // petite dorsale
		AddMesh(Hull, CUB, FVector(L * 0.02f, L * 0.34f, -L * 0.06f), FVector(L / 100.f * 0.24f, L / 100.f * 0.34f, 0.03f), FRotator(0, 0, 12), Dark);
		AddMesh(Hull, CUB, FVector(L * 0.02f, -L * 0.34f, -L * 0.06f), FVector(L / 100.f * 0.24f, L / 100.f * 0.34f, 0.03f), FRotator(0, 0, -12), Dark);
		CaudalHorizontal(L / 100.f * 1.1f, L / 100.f * 0.6f);
		break;
	}
	case EFishSpecies::Ray: // RAIE MANTA
	{
		SwimRate = 2.2f;
		// Corps central PLAT, plus large que long (le disque de la manta), sombre dessus.
		Body = AddMesh(Hull, CUB, FVector(0, 0, 0), FVector(L / 100.f * 0.75f, L / 100.f * 0.85f, L / 100.f * 0.09f), FRotator(0, 45, 0), Color);
		AddMesh(Hull, CUB, FVector(0, 0, -L * 0.03f), FVector(L / 100.f * 0.6f, L / 100.f * 0.7f, L / 100.f * 0.06f), FRotator(0, 45, 0), Belly); // ventre clair
		// AILES très larges (envergure ~2x le corps), APLATIES (cône écrasé en Z = aile plate
		// et large qui se distingue à l'horizontale) -> silhouette de manta en vol.
		Wings.Add(AddMesh(Hull, CON, FVector(-L * 0.05f, L * 0.60f, 0),
			FVector(L / 100.f * 1.3f, L / 100.f * 1.5f, 0.05f), FRotator::ZeroRotator, Color));
		Wings.Add(AddMesh(Hull, CON, FVector(-L * 0.05f, -L * 0.60f, 0),
			FVector(L / 100.f * 1.3f, L / 100.f * 1.5f, 0.05f), FRotator::ZeroRotator, Color));
		// CORNES CÉPHALIQUES : les deux petits lobes à l'avant de la bouche (signature manta).
		AddMesh(Hull, CON, FVector(L * 0.42f, L * 0.10f, 0), FVector(0.10f, 0.10f, L / 100.f * 0.28f), FRotator(70, 0, 0), Dark);
		AddMesh(Hull, CON, FVector(L * 0.42f, -L * 0.10f, 0), FVector(0.10f, 0.10f, L / 100.f * 0.28f), FRotator(70, 0, 0), Dark);
		// Longue queue fine en fouet (sur le pivot).
		AddMesh(TailPivot, CON, FVector(-L * 0.5f, 0, 0), FVector(0.05f, 0.05f, L / 100.f * 1.1f), FRotator(-90, 0, 0), Dark);
		break;
	}
	default: // SmallFish
	{
		SwimRate = 8.f;
		Body = AddMesh(Hull, SPH, FVector(0, 0, 0), FVector(L / 100.f, L / 100.f * 0.4f, L / 100.f * 0.5f), FRotator::ZeroRotator, Color);
		AddMesh(Hull, CON, FVector(0, 0, L * 0.22f), FVector(L / 100.f * 0.2f, 0.04f, L / 100.f * 0.22f), FRotator(0, 0, 0), Dark); // dorsale
		CaudalVertical(L / 100.f * 0.8f, L / 100.f * 0.55f);
		break;
	}
	}
}

void AWOTOLAmbientFish::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Angle += Speed * DeltaSeconds;

	const float A = Angle + Phase;
	const float C = FMath::Cos(A);
	const float Sn = FMath::Sin(A);
	SetActorLocation(CenterPoint + FVector(C * Radius, Sn * Radius,
		BaseZ + FMath::Sin(A * 2.f) * HeightAmp));

	FRotator Rot = FVector(-Sn, C, 0.f).Rotation();
	Rot.Pitch = FMath::Cos(A * 2.f) * 6.f; // pique/cabre doucement avec la houle
	SetActorRotation(Rot);

	const float t = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	// Battement de la QUEUE (lacet) -> propulsion crédible.
	if (TailPivot)
		TailPivot->SetRelativeRotation(FRotator(0.f, FMath::Sin(t * SwimRate) * 20.f, 0.f));

	// Léger balancement du corps entier (contre-mouvement de la tête).
	if (Body)
		Body->SetRelativeRotation(FRotator(0.f, FMath::Sin(t * SwimRate + 0.6f) * 4.f, 0.f));

	// Manta : les ailes BATTENT (roulis autour de l'axe d'avancée) -> vol sous-marin ondulant.
	if (Species == EFishSpecies::Ray && Wings.Num() >= 2)
	{
		const float flap = FMath::Sin(t * SwimRate) * 26.f;
		if (Wings[0]) Wings[0]->SetRelativeRotation(FRotator(0.f, 0.f,  flap));
		if (Wings[1]) Wings[1]->SetRelativeRotation(FRotator(0.f, 0.f, -flap));
	}

	// Frémissement : sillage de bulles derrière les grandes créatures.
	if (Species != EFishSpecies::SmallFish)
	{
		TrailTimer -= DeltaSeconds;
		if (TrailTimer <= 0.f)
		{
			TrailTimer = FMath::FRandRange(0.5f, 0.9f);
			AWOTOLBubbleBurst::Burst(GetWorld(),
				GetActorLocation() - GetActorForwardVector() * (BodyLen * 0.6f),
				FLinearColor(0.7f, 0.9f, 1.f, 1.f), 2);
		}
	}
}
