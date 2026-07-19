#include "WOTOLCityEnvironment.h"
#include "WOTOLCityBuildingProp.h"
#include "WOTOLDemoHUD.h" // CityCardCount()/CityCardCategory() : source de vérité partagée avec le HUD 2D
#include "DemoFlowSubsystem.h"
#include "WOTOLGlow.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
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

	// Hub central décoratif (palais/chef) : plus haut et plus large que les bâtiments de
	// production, teinté par la faction. Purement visuel (pas de sélection/fiche technique).
	AddCityDecor(this, SceneRoot, M_CYL, FVector(0.f, 0.f, 90.f), FVector(2.6f, 2.6f, 2.4f),
		WOTOLGlow::MakeMatte(this, FLinearColor(0.20f, 0.22f, 0.26f, 1.f)));
	AddCityDecor(this, SceneRoot, M_CONE, FVector(0.f, 0.f, 260.f), FVector(1.6f, 1.6f, 2.0f),
		WOTOLGlow::MakeGlow(this, Accent * 1.8f));

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
}

void AWOTOLCityEnvironment::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Ne rafraîchit les bâtiments QUE pendant l'écran Cité : coût négligeable (poignée
	// d'acteurs) mais inutile ailleurs (la vue n'est de toute façon pas rendue à l'écran).
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo || Demo->GetScreen() != EDemoScreen::City) return;

	for (AWOTOLCityBuildingProp* Prop : Props)
	{
		if (Prop) Prop->Refresh(Demo);
	}
}
