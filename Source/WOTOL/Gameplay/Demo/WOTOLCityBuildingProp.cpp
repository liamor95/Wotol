#include "WOTOLCityBuildingProp.h"
#include "WOTOLGlow.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	// Teinte propre à chaque catégorie (identité visuelle, indépendante de la faction qui
	// ne change que la nuance de l'anneau de sélection). Même ordre que
	// AWOTOLDemoHUD::CityCardCategory.
	FLinearColor CategoryTint(EDemoUnitCategory Cat)
	{
		switch (Cat)
		{
			case EDemoUnitCategory::Infanterie: return FLinearColor(0.55f, 0.65f, 0.85f, 1.f);
			case EDemoUnitCategory::Distance:   return FLinearColor(0.85f, 0.65f, 0.35f, 1.f);
			case EDemoUnitCategory::Montee:     return FLinearColor(0.55f, 0.85f, 0.55f, 1.f);
			case EDemoUnitCategory::Speciale:   return FLinearColor(0.75f, 0.45f, 0.85f, 1.f);
			case EDemoUnitCategory::Mythique:   return FLinearColor(0.95f, 0.55f, 0.25f, 1.f);
			default:                            return FLinearColor::White;
		}
	}
}

AWOTOLCityBuildingProp::AWOTOLCityBuildingProp()
{
	PrimaryActorTick.bCanEverTick = false; // rafraîchi explicitement par l'environnement

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;
}

void AWOTOLCityBuildingProp::BeginPlay()
{
	Super::BeginPlay();
	BuildVisual();
}

// Ajoute une pièce (mesh primitif), même utilitaire que AWOTOLDefenseStructure::BuildVisual —
// composants créés dynamiquement (pas de CreateDefaultSubobject) : évite tout chargement
// d'asset dans le constructeur, cohérent avec le reste du greybox du projet.
static UStaticMeshComponent* AddCityPiece(AActor* Owner, USceneComponent* Parent,
	const TCHAR* MeshPath, const FVector& Loc, const FVector& Scale)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Owner);
	if (!C) return nullptr;
	C->SetupAttachment(Parent);
	C->RegisterComponent();
	if (UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, MeshPath)) C->SetStaticMesh(M);
	C->SetRelativeLocation(Loc);
	C->SetRelativeScale3D(Scale);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	return C;
}

void AWOTOLCityBuildingProp::BuildVisual()
{
	if (!SceneRoot) return;
	const TCHAR* M_CYL = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");

	BaseMesh = AddCityPiece(this, SceneRoot, M_CYL, FVector(0.f, 0.f, 20.f), FVector(2.4f, 2.4f, 0.4f));
	if (BaseMesh)
	{
		// Seul le socle est cliquable (cible du raycast caméra isométrique) : la
		// silhouette au sol reste stable même quand la hauteur du corps change (niveau).
		BaseMesh->SetCollisionObjectType(ECC_WorldStatic);
		BaseMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		BaseMesh->SetCollisionResponseToAllChannels(ECR_Block);
		if (UMaterialInstanceDynamic* MID = WOTOLGlow::MakeMatte(this, FLinearColor(0.18f, 0.20f, 0.24f, 1.f)))
			BaseMesh->SetMaterial(0, MID);
	}

	// Amas procédural de pointes cristal/épines (MÊME technique que
	// AWOTOLDefenseStructure::BuildVisual::AddSpikeCluster) — remplace l'ancien plan texturé
	// avec illustration officielle collée (02/08/2026, demande explicite de Liamor : "tu dois
	// faire des formes toi-même, pas coller une image plate" pour les bâtiments de la cité).
	// Couleur = teinte de CATÉGORIE (CategoryTint, identité visuelle par type de bâtiment)
	// mélangée à l'accent de FACTION (cristal bleu Aquiloris / organique sombre Noxéens) ;
	// nombre de pointes légèrement croissant par catégorie pour varier les silhouettes.
	const TCHAR* M_CONE = TEXT("/Engine/BasicShapes/Cone.Cone");
	TierCluster = NewObject<USceneComponent>(this);
	if (TierCluster)
	{
		TierCluster->SetupAttachment(SceneRoot);
		TierCluster->RegisterComponent();
		TierCluster->SetRelativeLocation(FVector(0.f, 0.f, 20.f));

		const FLinearColor SpikeCol = FLinearColor::LerpUsingHSV(
			CategoryTint(Category), FFactionColors::Get(OwnerFaction), 0.35f);
		TierMID = WOTOLGlow::MakeGlow(this, SpikeCol);

		FRandomStream Rng(GetUniqueID() * 977 + 13);
		const int32 Count = 7 + static_cast<int32>(Category) * 2;
		for (int32 i = 0; i < Count; ++i)
		{
			const float Angle = (360.f / static_cast<float>(Count)) * static_cast<float>(i)
				+ Rng.FRandRange(-12.f, 12.f);
			const float Dist = Rng.FRandRange(60.f, 100.f);
			const FVector Pos(FMath::Cos(FMath::DegreesToRadians(Angle)) * Dist,
				FMath::Sin(FMath::DegreesToRadians(Angle)) * Dist, Rng.FRandRange(0.f, 10.f));
			const float SpikeH = Rng.FRandRange(90.f, 190.f);
			const float SpikeW = Rng.FRandRange(0.24f, 0.38f);
			const float Tilt = Rng.FRandRange(-8.f, 8.f);

			UStaticMeshComponent* Spike = NewObject<UStaticMeshComponent>(this);
			if (!Spike) continue;
			Spike->SetupAttachment(TierCluster);
			Spike->RegisterComponent();
			Spike->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			if (UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, M_CONE)) Spike->SetStaticMesh(M);
			Spike->SetRelativeLocationAndRotation(Pos,
				FRotator(Tilt, Rng.FRandRange(0.f, 360.f), Tilt));
			Spike->SetRelativeScale3D(FVector(SpikeW, SpikeW, SpikeH / 100.f));
			if (TierMID) Spike->SetMaterial(0, TierMID);
		}
		// Flèche centrale plus haute que le reste du cluster : silhouette reconnaissable de
		// loin, comme sur les planches de référence.
		UStaticMeshComponent* Spire = NewObject<UStaticMeshComponent>(this);
		if (Spire)
		{
			Spire->SetupAttachment(TierCluster);
			Spire->RegisterComponent();
			Spire->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			if (UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, M_CONE)) Spire->SetStaticMesh(M);
			Spire->SetRelativeScale3D(FVector(0.5f, 0.5f, 2.6f));
			if (TierMID) Spire->SetMaterial(0, TierMID);
		}
	}

	SelectionRing = AddCityPiece(this, SceneRoot, M_CYL, FVector(0.f, 0.f, 4.f), FVector(3.4f, 3.4f, 0.05f));
	if (SelectionRing)
	{
		if (UMaterialInstanceDynamic* RingMID = WOTOLGlow::MakeHalo(
				this, FFactionColors::Get(OwnerFaction) * 2.5f, 0.65f))
			SelectionRing->SetMaterial(0, RingMID);
		SelectionRing->SetVisibility(false);
	}
}

void AWOTOLCityBuildingProp::Refresh(const UDemoFlowSubsystem* Demo)
{
	if (!Demo || !TierCluster) return;

	const bool bUnlocked = Demo->IsCategoryUnlocked(Category);
	const bool bNeedsBuilding = (Category == EDemoUnitCategory::Distance)
		&& !Demo->IsRangedBuildingConstructed();
	const uint8 State = !bUnlocked ? 0 : (bNeedsBuilding ? 1 : 2);
	const int32 Level = bUnlocked ? FMath::Clamp(Demo->GetBuildingLevel(Category), 1, 3) : 1;
	const bool bSelected = Demo->HasCitySelection() && Demo->SelectedCityCategory == Category;

	if (State != LastState || Level != LastLevel)
	{
		LastState = State;
		LastLevel = Level;

		// "À construire" (Distance non posé) reste discret ; niveau 1-3 fait grandir le
		// cluster tout entier (échelle uniforme du parent, pas besoin de retoucher chaque
		// pointe individuellement).
		const float SizeScale = (State == 1) ? 0.55f : (0.7f + 0.5f * static_cast<float>(Level - 1));
		TierCluster->SetRelativeScale3D(FVector(SizeScale));

		if (TierMID)
		{
			// Verrouillé/à construire = terne ; actif = couleurs pleines.
			const FLinearColor Base = FLinearColor::LerpUsingHSV(
				CategoryTint(Category), FFactionColors::Get(OwnerFaction), 0.35f);
			const FLinearColor Emissive = (State == 2) ? Base * 2.0f : Base * 0.35f;
			TierMID->SetVectorParameterValue(TEXT("Color"), Emissive);
		}
	}

	if (bSelected != bLastSelected)
	{
		bLastSelected = bSelected;
		if (SelectionRing) SelectionRing->SetVisibility(bSelected);
	}
}
