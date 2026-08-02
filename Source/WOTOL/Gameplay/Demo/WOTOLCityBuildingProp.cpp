#include "WOTOLCityBuildingProp.h"
#include "WOTOLGlow.h"
#include "WOTOLBuildingArt.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
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

	// Illustration officielle réelle (planche détourée, WOTOLBuildingArt) si disponible,
	// affichée sur un plan orienté face à la caméra isométrique FIXE de la vue Cité (jamais
	// de rotation possible -> l'illusion tient à tout niveau de zoom, demande de Liamor du
	// 25/07/2026). Repli automatique sur l'ancien kitbash (cylindre émissif) si l'image
	// officielle est absente (fichier Content/UI/Buildings/ manquant). Le Chef n'a pas
	// d'icône par CATEGORIE (pas de carte de recrutement) mais a son propre bâtiment-siège
	// dédié (Noyau Cristallin / Trône des profondeurs, ajouté le 02/08/2026).
	UTexture2D* Art = (Category == EDemoUnitCategory::Chef)
		? WOTOLBuildingArt::GetSiegeBuildingIcon(OwnerFaction)
		: WOTOLBuildingArt::GetBuildingIcon(OwnerFaction, Category);
	if (Art)
	{
		const TCHAR* M_PLANE = TEXT("/Engine/BasicShapes/Plane.Plane");
		TierMesh = AddCityPiece(this, SceneRoot, M_PLANE, FVector(0.f, 0.f, 90.f), FVector(3.2f, 3.2f, 1.f));
		if (TierMesh)
		{
			// Le plan par défaut a sa normale locale +Z. On calcule la direction "vers la
			// caméra" (opposé du regard) à partir des angles FIXES de AWOTOLCityCamera
			// (FixedYaw=45/FixedPitch=-55, jamais modifiables) et on construit une rotation
			// dont l'axe Z pointe vers cette direction, avec le +Z du monde comme référence
			// "haut" pour ne pas voir l'image tourner sur elle-même dans son propre plan.
			const FRotator CamLookRot(-55.f, 45.f, 0.f);
			const FVector CamForward = FRotationMatrix(CamLookRot).GetScaledAxis(EAxis::X);
			const FVector ToCamera = -CamForward;
			TierMesh->SetRelativeRotation(FRotationMatrix::MakeFromZX(ToCamera, FVector::UpVector).Rotator());
			bUsingRealArt = true;
			TierMID = WOTOLGlow::MakeSprite(this, Art);
			if (TierMID) TierMesh->SetMaterial(0, TierMID);
		}
	}
	if (!bUsingRealArt)
	{
		TierMesh = AddCityPiece(this, SceneRoot, M_CYL, FVector(0.f, 0.f, 60.f), FVector(1.1f, 1.1f, 1.5f));
		TierMID = TierMesh ? WOTOLGlow::MakeGlow(this, CategoryTint(Category)) : nullptr;
		if (TierMesh && TierMID) TierMesh->SetMaterial(0, TierMID);
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

		// "À construire" (Distance non posé) reste discret ; niveau 1-3 fait grandir le
		// bâtiment. Un bâtiment "à construire" (Distance non posé) reste bas et discret
		// plutôt que d'afficher un bâtiment qui n'existe pas encore.
		const float SizeScale = (State == 1) ? 0.55f : (0.7f + 0.5f * static_cast<float>(Level - 1));

		if (bUsingRealArt)
		{
			// Plan texturé : la TAILLE (échelle uniforme du plan) reflète le niveau, pas la
			// hauteur (une illustration plate ne peut pas "s'étirer" sans se déformer).
			TierMesh->SetRelativeScale3D(FVector(3.2f * SizeScale, 3.2f * SizeScale, 1.f));
			if (TierMID)
			{
				// Verrouillé/à construire = terne ; actif = couleurs pleines de la planche.
				TierMID->SetScalarParameterValue(TEXT("Brightness"), (State == 2) ? 1.0f : 0.4f);
			}
		}
		else
		{
			// Repli kitbash : hauteur = niveau (comportement d'origine, inchangé).
			TierMesh->SetRelativeScale3D(FVector(1.1f, 1.1f, SizeScale * 2.2f));
			TierMesh->SetRelativeLocation(FVector(0.f, 0.f, 40.f + SizeScale * 100.f));
			if (TierMID)
			{
				const FLinearColor Base = CategoryTint(Category);
				const FLinearColor Emissive = (State == 2) ? Base * 2.0f : Base * 0.35f;
				TierMID->SetVectorParameterValue(TEXT("Color"), Emissive);
			}
		}
	}

	if (bSelected != bLastSelected)
	{
		bLastSelected = bSelected;
		if (SelectionRing) SelectionRing->SetVisibility(bSelected);
	}
}
