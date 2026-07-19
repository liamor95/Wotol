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

	TierMesh = AddCityPiece(this, SceneRoot, M_CYL, FVector(0.f, 0.f, 60.f), FVector(1.1f, 1.1f, 1.5f));
	TierMID = TierMesh ? WOTOLGlow::MakeGlow(this, CategoryTint(Category)) : nullptr;
	if (TierMesh && TierMID) TierMesh->SetMaterial(0, TierMID);

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
	if (!Demo || !TierMesh) return;

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

		// Hauteur = niveau (1/2/3) ; un bâtiment "à construire" (Distance non posé) reste
		// bas et discret plutôt que d'afficher un bâtiment qui n'existe pas encore.
		const float HeightScale = (State == 1) ? 0.35f : (0.7f + 0.5f * static_cast<float>(Level - 1));
		TierMesh->SetRelativeScale3D(FVector(1.1f, 1.1f, HeightScale * 2.2f));
		TierMesh->SetRelativeLocation(FVector(0.f, 0.f, 40.f + HeightScale * 100.f));

		if (TierMID)
		{
			// Verrouillé/à construire = terne ; actif = couleur vive émissive (bloom).
			const FLinearColor Base = CategoryTint(Category);
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
