#include "WOTOLAmbientFish.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "WOTOLBubbleBurst.h"

AWOTOLAmbientFish::AWOTOLAmbientFish()
{
	PrimaryActorTick.bCanEverTick = true;

	Hull = CreateDefaultSubobject<USceneComponent>(TEXT("Hull"));
	RootComponent = Hull;
}

UStaticMeshComponent* AWOTOLAmbientFish::MakePart(USceneComponent* Parent, const TCHAR* MeshPath,
	const FVector& RelLoc, const FVector& Scale, const FRotator& Rot, const FLinearColor& Color)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
	if (!C) return nullptr;
	C->SetupAttachment(Parent ? Parent : RootComponent);
	C->RegisterComponent();
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetCanEverAffectNavigation(false);
	if (UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, MeshPath)) C->SetStaticMesh(M);
	C->SetRelativeLocation(RelLoc);
	C->SetRelativeScale3D(Scale);
	C->SetRelativeRotation(Rot);
	if (UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, this))
		{
			MID->SetVectorParameterValue(TEXT("Color"), Color);
			C->SetMaterial(0, MID);
		}
	}
	return C;
}

void AWOTOLAmbientFish::Configure(const FVector& InCenter, float InRadius, float InSpeed,
	float InPhase, float InHeightAmp, float InBaseZ, const FLinearColor& Color, float SizeM,
	EFishSpecies InSpecies)
{
	CenterPoint = InCenter; Radius = InRadius; Speed = InSpeed;
	Phase = InPhase; HeightAmp = InHeightAmp; BaseZ = InBaseZ;
	Species = InSpecies;

	const TCHAR* SPH = TEXT("/Engine/BasicShapes/Sphere.Sphere");
	const TCHAR* CON = TEXT("/Engine/BasicShapes/Cone.Cone");
	const TCHAR* CUB = TEXT("/Engine/BasicShapes/Cube.Cube");
	const float S = SizeM;               // échelle globale (mètres approx)
	const float U = S * 100.f;           // 1 "unité de corps" en uu
	const FLinearColor Dark = Color * 0.7f;   // ventre/ailerons plus sombres

	// Nombre de segments de queue + amplitude d'ondulation selon l'espèce.
	int32 NSeg = 4;

	auto BuildBodyChain = [&](float HeadW, float HeadH, float SegTaper, float Len, float FlukeVert)
	{
		// Tête / corps principal (sphère allongée le long de +X = sens de nage).
		BodyLen = Len * U;
		Body = MakePart(Hull, SPH, FVector(0, 0, 0),
			FVector(Len * 0.9f, HeadW, HeadH), FRotator::ZeroRotator, Color);

		// Chaîne de segments vers l'arrière (-X), rétrécissants -> profil fuselé qui ONDULE.
		USceneComponent* Prev = Hull;
		float segX = -Len * U * 0.42f;   // départ derrière la tête
		const float step = (Len * U * 0.55f) / NSeg;
		for (int32 i = 0; i < NSeg; ++i)
		{
			USceneComponent* J = NewObject<USceneComponent>(this);
			J->SetupAttachment(Prev);
			J->RegisterComponent();
			J->SetRelativeLocation(FVector(i == 0 ? segX : -step, 0.f, 0.f));
			TailJoints.Add(J);
			const float taper = FMath::Pow(SegTaper, (float)(i + 1));
			MakePart(J, SPH, FVector(-step * 0.5f, 0, 0),
				FVector(step / 100.f * 1.3f, HeadW * taper, HeadH * taper), FRotator::ZeroRotator, Color);
			Prev = J;
		}
		// Nageoire caudale au bout de la chaîne.
		if (TailJoints.Num() > 0)
		{
			USceneComponent* End = TailJoints.Last();
			if (FlukeVert > 0.5f)
			{
				// Queue VERTICALE (poissons/requins) en forme de croissant : 2 cônes.
				MakePart(End, CON, FVector(-step * 0.7f, 0, U * 0.18f * S * 0.0f + step * 0.25f),
					FVector(0.5f * HeadW, 0.14f, step / 100.f * 1.4f), FRotator(-60.f, 0, 0), Dark);
				MakePart(End, CON, FVector(-step * 0.7f, 0, -step * 0.25f),
					FVector(0.5f * HeadW, 0.14f, step / 100.f * 1.4f), FRotator(60.f, 0, 0), Dark);
			}
			else
			{
				// Nageoire caudale HORIZONTALE (mammifères marins : dauphin/orque/baleine).
				MakePart(End, CON, FVector(-step * 0.7f, step * 0.30f, 0),
					FVector(0.5f * HeadW, 0.14f, step / 100.f * 1.4f), FRotator(0, 0, -60.f), Dark);
				MakePart(End, CON, FVector(-step * 0.7f, -step * 0.30f, 0),
					FVector(0.5f * HeadW, 0.14f, step / 100.f * 1.4f), FRotator(0, 0, 60.f), Dark);
			}
		}
	};

	switch (Species)
	{
	case EFishSpecies::Shark:
	{
		NSeg = 5; SwimRate = 3.2f;
		BuildBodyChain(/*HeadW*/0.42f, /*HeadH*/0.44f, /*taper*/0.82f, /*Len*/2.2f, /*flukeVert*/1.f);
		// Museau pointu (cône vers l'avant).
		MakePart(Body, CON, FVector(U * 1.0f, 0, 0), FVector(0.34f, 0.34f, U * 0.9f / 100.f),
			FRotator(90.f, 0, 0), Color);
		// Aileron dorsal TRIANGULAIRE (signature du requin).
		Fins.Add(MakePart(Hull, CON, FVector(-U * 0.15f, 0, U * 0.45f),
			FVector(0.34f, 0.14f, U * 0.7f / 100.f), FRotator(0, 0, 0), Dark));
		// Pectorales.
		Fins.Add(MakePart(Hull, CON, FVector(U * 0.1f, U * 0.42f, -U * 0.1f), FVector(0.3f, 0.1f, U * 0.5f / 100.f), FRotator(0, 0, 100.f), Dark));
		Fins.Add(MakePart(Hull, CON, FVector(U * 0.1f, -U * 0.42f, -U * 0.1f), FVector(0.3f, 0.1f, U * 0.5f / 100.f), FRotator(0, 0, -100.f), Dark));
		break;
	}
	case EFishSpecies::Dolphin:
	{
		NSeg = 5; SwimRate = 4.2f;
		BuildBodyChain(0.38f, 0.40f, 0.84f, 2.0f, /*flukeVert*/0.f);
		// Rostre (bec) fin.
		MakePart(Body, CON, FVector(U * 0.95f, 0, 0), FVector(0.22f, 0.22f, U * 0.7f / 100.f), FRotator(90.f, 0, 0), Color);
		// Aileron dorsal courbé (cône incliné vers l'arrière).
		Fins.Add(MakePart(Hull, CON, FVector(-U * 0.1f, 0, U * 0.42f), FVector(0.3f, 0.12f, U * 0.55f / 100.f), FRotator(-28.f, 0, 0), Dark));
		Fins.Add(MakePart(Hull, CON, FVector(U * 0.15f, U * 0.38f, -U * 0.05f), FVector(0.26f, 0.1f, U * 0.45f / 100.f), FRotator(0, 0, 105.f), Dark));
		Fins.Add(MakePart(Hull, CON, FVector(U * 0.15f, -U * 0.38f, -U * 0.05f), FVector(0.26f, 0.1f, U * 0.45f / 100.f), FRotator(0, 0, -105.f), Dark));
		break;
	}
	case EFishSpecies::Orca:
	{
		NSeg = 5; SwimRate = 3.6f;
		BuildBodyChain(0.5f, 0.52f, 0.85f, 2.4f, /*flukeVert*/0.f);
		MakePart(Body, SPH, FVector(U * 0.7f, 0, 0), FVector(0.7f, 0.42f, 0.44f), FRotator::ZeroRotator, Color);
		// Aileron dorsal HAUT ET DROIT (signature de l'orque).
		Fins.Add(MakePart(Hull, CON, FVector(-U * 0.05f, 0, U * 0.62f), FVector(0.34f, 0.14f, U * 1.05f / 100.f), FRotator(0, 0, 0), Dark));
		// Tache blanche approximée (ventre clair).
		MakePart(Body, SPH, FVector(0, 0, -U * 0.28f), FVector(1.5f, 0.36f, 0.16f),
			FRotator::ZeroRotator, FLinearColor(0.95f, 0.95f, 0.98f, 1.f));
		Fins.Add(MakePart(Hull, CON, FVector(U * 0.1f, U * 0.5f, -U * 0.12f), FVector(0.32f, 0.12f, U * 0.6f / 100.f), FRotator(0, 0, 100.f), Dark));
		Fins.Add(MakePart(Hull, CON, FVector(U * 0.1f, -U * 0.5f, -U * 0.12f), FVector(0.32f, 0.12f, U * 0.6f / 100.f), FRotator(0, 0, -100.f), Dark));
		break;
	}
	case EFishSpecies::Whale:
	{
		NSeg = 4; SwimRate = 2.0f;
		BuildBodyChain(0.72f, 0.66f, 0.9f, 3.4f, /*flukeVert*/0.f);
		// Grosse tête arrondie.
		MakePart(Body, SPH, FVector(U * 0.95f, 0, 0), FVector(0.9f, 0.66f, 0.6f), FRotator::ZeroRotator, Color);
		// Petit aileron dorsal bas + bosse.
		Fins.Add(MakePart(Hull, CON, FVector(-U * 0.5f, 0, U * 0.5f), FVector(0.3f, 0.14f, U * 0.35f / 100.f), FRotator(-20.f, 0, 0), Dark));
		Fins.Add(MakePart(Hull, CON, FVector(U * 0.2f, U * 0.6f, -U * 0.1f), FVector(0.4f, 0.14f, U * 0.7f / 100.f), FRotator(0, 0, 110.f), Dark));
		Fins.Add(MakePart(Hull, CON, FVector(U * 0.2f, -U * 0.6f, -U * 0.1f), FVector(0.4f, 0.14f, U * 0.7f / 100.f), FRotator(0, 0, -110.f), Dark));
		break;
	}
	case EFishSpecies::Ray:
	{
		NSeg = 4; SwimRate = 2.6f;
		// Corps PLAT en losange (cube aplati).
		BodyLen = 1.6f * U;
		Body = MakePart(Hull, CUB, FVector(0, 0, 0), FVector(1.4f * S, 1.9f * S, 0.12f * S), FRotator(0, 45.f, 0), Color);
		// Longue queue fine (chaîne).
		USceneComponent* Prev = Hull; const float step = U * 0.5f;
		for (int32 i = 0; i < NSeg; ++i)
		{
			USceneComponent* J = NewObject<USceneComponent>(this);
			J->SetupAttachment(Prev); J->RegisterComponent();
			J->SetRelativeLocation(FVector(i == 0 ? -U * 0.8f : -step, 0, 0));
			TailJoints.Add(J);
			const float t = FMath::Pow(0.8f, (float)(i + 1));
			MakePart(J, CON, FVector(-step * 0.5f, 0, 0), FVector(0.12f * t, 0.12f * t, step / 100.f), FRotator(-90.f, 0, 0), Dark);
			Prev = J;
		}
		// AILES (grandes nageoires latérales plates qui battent).
		Fins.Add(MakePart(Hull, CUB, FVector(0, U * 1.0f, 0), FVector(1.0f * S, 1.1f * S, 0.06f * S), FRotator(0, 45.f, 0), Color));
		Fins.Add(MakePart(Hull, CUB, FVector(0, -U * 1.0f, 0), FVector(1.0f * S, 1.1f * S, 0.06f * S), FRotator(0, 45.f, 0), Color));
		break;
	}
	default: // SmallFish
	{
		NSeg = 3; SwimRate = 9.f;
		BuildBodyChain(0.42f, 0.5f, 0.7f, 1.0f, /*flukeVert*/1.f);
		// Petit aileron dorsal.
		Fins.Add(MakePart(Hull, CON, FVector(0, 0, U * 0.28f), FVector(0.28f, 0.1f, U * 0.3f / 100.f), FRotator(0, 0, 0), Dark));
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
	const FVector Pos = CenterPoint + FVector(C * Radius, Sn * Radius,
		BaseZ + FMath::Sin(A * 2.f) * HeightAmp);
	SetActorLocation(Pos);

	// Oriente dans le sens de la nage (tangente) + léger tangage suivant la montée/descente.
	const FVector Tangent(-Sn, C, 0.f);
	FRotator Rot = Tangent.Rotation();
	Rot.Pitch = FMath::Cos(A * 2.f) * 6.f; // pique/cabre doucement avec la houle
	SetActorRotation(Rot);

	const float t = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	if (Species == EFishSpecies::Ray)
	{
		// Les AILES battent (roulis opposé) -> vol sous-marin caractéristique de la raie.
		const float flap = FMath::Sin(t * SwimRate) * 22.f;
		if (Fins.Num() >= 2)
		{
			if (Fins[0]) Fins[0]->SetRelativeRotation(FRotator(0, 45.f,  flap));
			if (Fins[1]) Fins[1]->SetRelativeRotation(FRotator(0, 45.f, -flap));
		}
	}
	// ONDULATION du corps : vague progressive le long des segments de queue -> nage crédible
	// (le corps fléchit, la queue fouette de plus en plus fort vers l'arrière).
	const int32 N = TailJoints.Num();
	for (int32 i = 0; i < N; ++i)
	{
		if (!TailJoints[i]) continue;
		const float amp = 5.f + 12.f * ((float)(i + 1) / (float)N); // amplitude croissante vers la queue
		const float yaw = FMath::Sin(t * SwimRate - i * 0.9f) * amp;
		TailJoints[i]->SetRelativeRotation(FRotator(0.f, yaw, 0.f));
	}

	// FRÉMISSEMENT DE L'EAU : sillage de bulles derrière les GRANDES créatures (les petits
	// bancs sont trop nombreux -> on les épargne pour rester léger). Émis à la queue.
	if (Species != EFishSpecies::SmallFish)
	{
		TrailTimer -= DeltaSeconds;
		if (TrailTimer <= 0.f)
		{
			TrailTimer = FMath::FRandRange(0.4f, 0.8f);
			const FVector Rear = GetActorLocation() - GetActorForwardVector() * (BodyLen * 0.6f);
			AWOTOLBubbleBurst::Burst(GetWorld(), Rear, FLinearColor(0.7f, 0.9f, 1.f, 1.f), 2);
		}
	}
}
