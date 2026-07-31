#include "WOTOLCityEnvironment.h"
#include "WOTOLCityBuildingProp.h"
#include "WOTOLDemoHUD.h" // CityCardCount()/CityCardCategory() : source de vérité partagée avec le HUD 2D
#include "DemoFlowSubsystem.h"
#include "WOTOLGlow.h"
#include "WOTOLBuildingArt.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"

AWOTOLCityEnvironment::AWOTOLCityEnvironment()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;
}

void AWOTOLCityEnvironment::BeginPlay()
{
	Super::BeginPlay();
	BuildEnvironment();
}

static UStaticMeshComponent* AddCityDecor(AActor* Owner, USceneComponent* Parent,
	const TCHAR* MeshPath, const FVector& Loc, const FVector& Scale, UMaterialInstanceDynamic* MID)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Owner);
	if (!C) return nullptr;
	C->SetupAttachment(Parent);
	C->RegisterComponent();
	if (UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, MeshPath)) C->SetStaticMesh(M);
	C->SetRelativeLocation(Loc);
	C->SetRelativeScale3D(Scale);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision); // décor pur, non cliquable
	if (MID) C->SetMaterial(0, MID);
	return C;
}

void AWOTOLCityEnvironment::BuildEnvironment()
{
	const TCHAR* M_CYL  = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* M_CONE = TEXT("/Engine/BasicShapes/Cone.Cone");
	const bool bAq = (PlayerFaction != EFactionID::Noxeens);
	const FLinearColor Accent = FFactionColors::Get(PlayerFaction);
	const FLinearColor GroundColor = bAq ? FLinearColor(0.10f, 0.14f, 0.20f, 1.f)
	                                      : FLinearColor(0.08f, 0.16f, 0.13f, 1.f);

	// Sol : large disque plat (le "socle" visuel de toute la cité).
	AddCityDecor(this, SceneRoot, M_CYL, FVector(0.f, 0.f, -10.f), FVector(24.f, 24.f, 0.15f),
		WOTOLGlow::MakeMatte(this, GroundColor));

	// Hub central (palais/chef — Noyau Cristallin / Trône des Profondeurs) : plus haut et plus
	// large que les bâtiments de production, teinté par la faction. La flèche décorative
	// (cylindre + cône) reste du pur décor ; le SOCLE cliquable est un vrai
	// AWOTOLCityBuildingProp (Category=Chef, ajouté le 29/07/2026 — demande Liamor : le Chef se
	// gère via SON bâtiment, pas via une ligne dans un écran à part) qui ouvre la même fenêtre à
	// onglets que les autres bâtiments -> le joueur gère les stats/le Grade/l'Axe du Chef ici.
	AddCityDecor(this, SceneRoot, M_CYL, FVector(0.f, 0.f, 90.f), FVector(2.6f, 2.6f, 2.4f),
		WOTOLGlow::MakeMatte(this, FLinearColor(0.20f, 0.22f, 0.26f, 1.f)));
	AddCityDecor(this, SceneRoot, M_CONE, FVector(0.f, 0.f, 260.f), FVector(1.6f, 1.6f, 2.0f),
		WOTOLGlow::MakeGlow(this, Accent * 1.8f));
	{
		UWorld* HubW = GetWorld();
		if (HubW)
		{
			const FTransform HubTM(FRotator::ZeroRotator, GetActorLocation());
			if (AWOTOLCityBuildingProp* ChefProp = HubW->SpawnActorDeferred<AWOTOLCityBuildingProp>(
					AWOTOLCityBuildingProp::StaticClass(), HubTM, this))
			{
				ChefProp->Category = EDemoUnitCategory::Chef;
				ChefProp->OwnerFaction = PlayerFaction;
				UGameplayStatics::FinishSpawningActor(ChefProp, HubTM);
				Props.Add(ChefProp);
			}
		}
	}

	// Grand fond de cité (illustration officielle réelle, demande de Liamor du 25/07/2026) —
	// posé loin derrière la scène, orienté face à la caméra isométrique FIXE (même calcul que
	// AWOTOLCityBuildingProp, les angles ne changent jamais dans cette vue).
	// Absent silencieusement si le fichier officiel n'existe pas (décor kitbash seul visible).
	if (UTexture2D* Backdrop = WOTOLBuildingArt::GetCityBackdrop(PlayerFaction))
	{
		const FRotator CamLookRot(-55.f, 45.f, 0.f); // memes valeurs que AWOTOLCityCamera
		const FVector CamForward = FRotationMatrix(CamLookRot).GetScaledAxis(EAxis::X);
		// Bug trouvé au 1er test PC du 31/07/2026 : l'ancien décalage (-1400,-1400,900) était
		// DU MÊME CÔTÉ que la caméra elle-même (AWOTOLCityCamera::ResetToHub positionne la
		// caméra le long de -CamForward*TargetArmLength, soit environ (-1298,-1298,+2621)) et
		// à une distance INFÉRIEURE à celle de la caméra -> le fond, gigantesque (échelle 48),
		// se retrouvait placé ENTRE la caméra et la cité, recouvrant tout l'écran d'un simple
		// aplat bleu (ville entièrement invisible). Il doit être du côté OPPOSÉ (+CamForward),
		// loin AU-DELÀ du hub et de l'anneau de bâtiments (RingRadius=1500), pour rester
		// derrière eux du point de vue de la caméra orthographique.
		BackdropMesh = AddCityDecor(this, SceneRoot, TEXT("/Engine/BasicShapes/Plane.Plane"),
			CamForward * 3600.f, FVector(48.f, 48.f, 1.f), nullptr);
		if (BackdropMesh)
		{
			BackdropMesh->SetRelativeRotation(
				FRotationMatrix::MakeFromZX(-CamForward, FVector::UpVector).Rotator());
			if (UMaterialInstanceDynamic* MID = WOTOLGlow::MakeSprite(this, Backdrop))
				BackdropMesh->SetMaterial(0, MID);
		}
	}

	// Bâtiments de production : un par catégorie productible, en anneau autour du hub, dans
	// le MÊME ORDRE que les cartes du HUD 2D (index i <-> carte i) pour rester cohérent.
	UWorld* W = GetWorld();
	if (!W) return;
	const int32 N = AWOTOLDemoHUD::CityCardCount();
	for (int32 i = 0; i < N; ++i)
	{
		const float Angle = (360.f / static_cast<float>(N)) * static_cast<float>(i);
		const FVector Offset = FVector(FMath::Cos(FMath::DegreesToRadians(Angle)) * RingRadius,
			FMath::Sin(FMath::DegreesToRadians(Angle)) * RingRadius, 0.f);
		const FTransform PropTM(FRotator::ZeroRotator, GetActorLocation() + Offset);

		if (AWOTOLCityBuildingProp* Prop = W->SpawnActorDeferred<AWOTOLCityBuildingProp>(
				AWOTOLCityBuildingProp::StaticClass(), PropTM, this))
		{
			Prop->Category = AWOTOLDemoHUD::CityCardCategory(i);
			Prop->OwnerFaction = PlayerFaction;
			UGameplayStatics::FinishSpawningActor(Prop, PropTM);
			Props.Add(Prop);
		}
	}

	BuildAmbientBubbles();
}

// Petites sphères émissives qui montent en boucle autour de l'anneau de bâtiments — garde la
// vue Cité "vivante" (demande de Liamor du 25/07/2026), en particulier maintenant que les
// bâtiments sont des illustrations 2D (plates) plutôt qu'un kitbash 3D animé par nature.
void AWOTOLCityEnvironment::BuildAmbientBubbles()
{
	const FLinearColor Accent = FFactionColors::Get(PlayerFaction);
	constexpr int32 NumBubbles = 18;
	for (int32 i = 0; i < NumBubbles; ++i)
	{
		const float Angle = FMath::FRand() * 360.f;
		const float Radius = FMath::FRandRange(200.f, RingRadius * 1.15f);
		const FVector Origin = FVector(FMath::Cos(FMath::DegreesToRadians(Angle)) * Radius,
			FMath::Sin(FMath::DegreesToRadians(Angle)) * Radius, 0.f);

		UStaticMeshComponent* Bubble = AddCityDecor(this, SceneRoot,
			TEXT("/Engine/BasicShapes/Sphere.Sphere"), Origin, FVector(0.12f, 0.12f, 0.12f),
			WOTOLGlow::MakeGlow(this, Accent * 1.6f));
		if (!Bubble) continue;

		AmbientBubbles.Add(Bubble);
		BubbleOrigin.Add(Origin);
		BubblePhase.Add(FMath::FRand() * 100.f);
		BubbleSpeed.Add(FMath::FRandRange(18.f, 42.f));
	}
}

void AWOTOLCityEnvironment::TickAmbientBubbles(float DeltaSeconds)
{
	constexpr float CycleHeight = 260.f;
	for (int32 i = 0; i < AmbientBubbles.Num(); ++i)
	{
		UStaticMeshComponent* Bubble = AmbientBubbles[i];
		if (!Bubble) continue;
		BubblePhase[i] += DeltaSeconds * BubbleSpeed[i];
		const float Z = FMath::Fmod(BubblePhase[i], CycleHeight);
		const float Sway = FMath::Sin(BubblePhase[i] * 0.05f) * 20.f;
		Bubble->SetRelativeLocation(BubbleOrigin[i] + FVector(Sway, 0.f, 20.f + Z));
	}
}

void AWOTOLCityEnvironment::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Ne rafraîchit les bâtiments QUE pendant l'écran Cité : coût négligeable (poignée
	// d'acteurs) mais inutile ailleurs (la vue n'est de toute façon pas rendue à l'écran).
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo || Demo->GetScreen() != EDemoScreen::City) return;

	TickAmbientBubbles(DeltaSeconds);

	for (AWOTOLCityBuildingProp* Prop : Props)
	{
		if (Prop) Prop->Refresh(Demo);
	}
}
