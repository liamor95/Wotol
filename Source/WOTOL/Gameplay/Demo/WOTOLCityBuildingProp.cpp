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

	// BUG CORRIGE (retour terrain 02/08/2026 : "le noyau cristallin on peut pas cliquer
	// dessus") : le socle cliquable était de la MÊME taille pour les 6 catégories, alors que
	// le Chef (hub) repose sur un dallage bien plus large (SpawnPlaza 260 vs 170, voir
	// AWOTOLGreyboxEnvironment::BuildCityLayout) — le joueur clique naturellement n'importe
	// où sur ce grand dallage/piédestal en s'attendant à toucher le bâtiment le plus
	// proéminent de la cité, et retombait hors du petit disque de collision. Le socle du
	// Chef est maintenant élargi dans la même proportion que son dallage (260/170).
	const bool bChefProp = (Category == EDemoUnitCategory::Chef);
	const FVector BaseScale = bChefProp ? FVector(3.7f, 3.7f, 0.5f) : FVector(2.4f, 2.4f, 0.4f);
	BaseMesh = AddCityPiece(this, SceneRoot, M_CYL, FVector(0.f, 0.f, 20.f), BaseScale);
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

	// Direction artistique DISTINCTE par faction (demande explicite de Liamor du 02/08/2026 :
	// "chaque faction a sa cité", pas juste une couleur qui change sur la même forme) :
	//   AQUILORIS : TOUR-CRISTAL À ÉTAGES DÉCROISSANTS, géométrie propre/architecturale
	//     (cylindres rétrécissants + flèche conique), planches "Puits des courants
	//     cristallins"/"Rempart cristallin".
	//   NOXEENS : AMAS ORGANIQUE de pointes/épines irrégulières + pods bioluminescents,
	//     silhouette de ruche/corail plutôt que de tour géométrique, planches "Entraves
	//     abyssales"/"Fosse nourricière".
	// Dans les deux cas : PAS un simple amas de pointes façon oursin (rejeté explicitement le
	// 02/08/2026 : "formes dégueulasses en guise de bâtiment") — la version Aquiloris a une
	// vraie silhouette de tour, la version Noxeens un vrai amas organique dense et texturé.
	const TCHAR* M_CONE = TEXT("/Engine/BasicShapes/Cone.Cone");
	const TCHAR* M_CYL2 = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* M_SPH  = TEXT("/Engine/BasicShapes/Sphere.Sphere");
	const bool bAqBuilding = (OwnerFaction != EFactionID::Noxeens);
	TierCluster = NewObject<USceneComponent>(this);
	if (TierCluster)
	{
		TierCluster->SetupAttachment(SceneRoot);
		TierCluster->RegisterComponent();
		TierCluster->SetRelativeLocation(FVector(0.f, 0.f, 20.f));

		const FLinearColor SpireCol = FLinearColor::LerpUsingHSV(
			CategoryTint(Category), FFactionColors::Get(OwnerFaction), 0.35f);
		TierMID = WOTOLGlow::MakeGlow(this, SpireCol);
		TierMatteMID = WOTOLGlow::MakeMatte(this, SpireCol * 0.6f);

		auto AddTierPiece = [&](const TCHAR* Mesh, const FVector& Pos, const FVector& Scale,
			const FRotator& Rot, UMaterialInstanceDynamic* MID)
		{
			UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
			if (!C) return;
			C->SetupAttachment(TierCluster);
			C->RegisterComponent();
			C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			if (UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, Mesh)) C->SetStaticMesh(M);
			C->SetRelativeLocationAndRotation(Pos, Rot);
			C->SetRelativeScale3D(Scale);
			if (MID) C->SetMaterial(0, MID);
		};

		FRandomStream Rng(GetUniqueID() * 977 + 13);
		if (bAqBuilding)
		{
			// 3 étages cylindriques rétrécissants + une flèche conique au sommet.
			const int32 Tiers = 3;
			float R = 1.0f, Z = 0.f;
			for (int32 t = 0; t < Tiers; ++t)
			{
				const float TierH = 0.55f;
				AddTierPiece(M_CYL2, FVector(0.f, 0.f, Z + TierH * 50.f),
					FVector(R, R, TierH), FRotator::ZeroRotator, TierMID);
				Z += TierH * 100.f;
				R *= 0.62f;
			}
			AddTierPiece(M_CONE, FVector(0.f, 0.f, Z + 90.f), FVector(R * 1.3f, R * 1.3f, 1.8f),
				FRotator::ZeroRotator, TierMID);

			// 4 pointes d'accent en diagonale à la base (silhouette "couronne").
			for (int32 i = 0; i < 4; ++i)
			{
				const float Angle = 45.f + i * 90.f;
				const float Dist = 95.f;
				const FVector Pos(FMath::Cos(FMath::DegreesToRadians(Angle)) * Dist,
					FMath::Sin(FMath::DegreesToRadians(Angle)) * Dist, 30.f);
				AddTierPiece(M_CONE, Pos, FVector(0.22f, 0.22f, 1.1f),
					FRotator(Rng.FRandRange(-6.f, 6.f), Rng.FRandRange(0.f, 360.f), Rng.FRandRange(-6.f, 6.f)),
					TierMID);
			}
		}
		else
		{
			// Cocon central (silhouette de ruche, base large arrondie) + amas dense d'épines
			// sombres jitterées tout autour + 2-3 pods bioluminescents nichés dedans.
			AddTierPiece(M_SPH, FVector(0.f, 0.f, 55.f), FVector(1.05f, 1.05f, 0.9f),
				FRotator::ZeroRotator, TierMatteMID);
			const int32 ThornCount = 10 + static_cast<int32>(Category) * 2;
			for (int32 i = 0; i < ThornCount; ++i)
			{
				const float Angle = (360.f / static_cast<float>(ThornCount)) * static_cast<float>(i)
					+ Rng.FRandRange(-14.f, 14.f);
				const float Dist = Rng.FRandRange(55.f, 100.f);
				const FVector Pos(FMath::Cos(FMath::DegreesToRadians(Angle)) * Dist,
					FMath::Sin(FMath::DegreesToRadians(Angle)) * Dist, Rng.FRandRange(0.f, 90.f));
				const float ThornH = Rng.FRandRange(90.f, 210.f);
				const float ThornW = Rng.FRandRange(0.16f, 0.28f);
				const float Tilt = Rng.FRandRange(-20.f, 20.f);
				AddTierPiece(M_CONE, Pos, FVector(ThornW, ThornW, ThornH / 100.f),
					FRotator(Tilt, Rng.FRandRange(0.f, 360.f), Tilt), TierMatteMID);
			}
			const int32 PodCount = 3;
			for (int32 i = 0; i < PodCount; ++i)
			{
				const float Angle = Rng.FRandRange(0.f, 360.f);
				const float Dist = Rng.FRandRange(20.f, 60.f);
				const FVector Pos(FMath::Cos(FMath::DegreesToRadians(Angle)) * Dist,
					FMath::Sin(FMath::DegreesToRadians(Angle)) * Dist, Rng.FRandRange(50.f, 130.f));
				AddTierPiece(M_SPH, Pos, FVector(0.28f, 0.28f, 0.28f), FRotator::ZeroRotator, TierMID);
			}
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

		// Verrouillé/à construire = terne ; actif = couleurs pleines. TierMatteMID (épines/
		// cocon Noxéens) est mis à jour EN PLUS de TierMID (flèche Aquiloris/pods Noxéens) —
		// nullptr et ignoré silencieusement côté Aquiloris.
		const FLinearColor Base = FLinearColor::LerpUsingHSV(
			CategoryTint(Category), FFactionColors::Get(OwnerFaction), 0.35f);
		const FLinearColor Emissive = (State == 2) ? Base * 2.0f : Base * 0.35f;
		if (TierMID) TierMID->SetVectorParameterValue(TEXT("Color"), Emissive);
		if (TierMatteMID) TierMatteMID->SetVectorParameterValue(TEXT("Color"), Emissive * 0.6f);
	}

	if (bSelected != bLastSelected)
	{
		bLastSelected = bSelected;
		if (SelectionRing) SelectionRing->SetVisibility(bSelected);
	}
}
