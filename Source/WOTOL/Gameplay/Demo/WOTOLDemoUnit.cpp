#include "WOTOLDemoUnit.h"
#include "Gameplay/Units/UnitDataAsset.h"
#include "Gameplay/Units/UnitAIStateComponent.h"
#include "Gameplay/AI/AIAdaptiveController.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Data/WOTOLTypes.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Core/FactionRegistrySubsystem.h"
#include "WOTOLDamageNumber.h"

namespace
{
	const TCHAR* M_CUBE = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* M_SPH  = TEXT("/Engine/BasicShapes/Sphere.Sphere");
	const TCHAR* M_CYL  = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* M_CONE = TEXT("/Engine/BasicShapes/Cone.Cone");
}

AWOTOLDemoUnit::AWOTOLDemoUnit()
{
	PrimaryActorTick.bCanEverTick = true;

	// CRITIQUE : composant machine d'états IA (sinon les attaques ne se déclenchent
	// jamais — il était ajouté côté Blueprint, absent des unités 100% C++).
	CreateDefaultSubobject<UUnitAIStateComponent>(TEXT("AIState"));

	ShapeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShapeMesh"));
	ShapeMesh->SetupAttachment(RootComponent);
	ShapeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Étiquette flottante nom + PV (au-dessus de la tête)
	NameTag = CreateDefaultSubobject<UTextRenderComponent>(TEXT("NameTag"));
	NameTag->SetupAttachment(RootComponent);
	NameTag->SetRelativeLocation(FVector(0.f, 0.f, 140.f));
	NameTag->SetHorizontalAlignment(EHTA_Center);
	NameTag->SetWorldSize(40.f);
	NameTag->SetText(FText::GetEmpty());

	// L'IA RTS possède automatiquement l'unité au spawn
	AIControllerClass = AAIAdaptiveController::StaticClass();
	AutoPossessAI     = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AWOTOLDemoUnit::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bCreatureBrain)
	{
		CreatureBrainTick(DeltaSeconds);
	}

	if (bArticulated)
	{
		AnimateArticulated(DeltaSeconds);
	}

	if (!NameTag) return;

	// Le boss s'appelle "Kraken" (créature neutre), pas le nom du mythique rival
	const FString DisplayName = bCreatureBrain
		? FString(TEXT("Kraken"))
		: ((UnitData && !UnitData->DisplayName.IsEmpty()) ? UnitData->DisplayName.ToString() : GetName());

	// VRAIES valeurs de PV (ex: "1700 / 2000"), boss inclus (HealthScale)
	const int32 MaxHP = GetEffectiveMaxHealth();
	const int32 CurHP = FMath::Clamp(FMath::RoundToInt(CurrentHealth), 0, MaxHP);

	NameTag->SetText(FText::FromString(
		FString::Printf(TEXT("%s\n%d / %d"), *DisplayName, CurHP, MaxHP)));

	// Couleur d'étiquette : violet "calamar" pour le kraken, sinon couleur de faction
	const FLinearColor TagColor = bCreatureBrain
		? FLinearColor(0.7f, 0.15f, 0.85f, 1.f)
		: FFactionColors::Get(GetFaction());
	NameTag->SetTextRenderColor(TagColor.ToFColor(true));

	// (Le kraken/Noxedrake a déjà ses couleurs fidèles — armure bleu-violet +
	//  craquelures cyan — construites dans AssembleSilhouette. Pas de surcharge.)

	// L'étiquette fait toujours face à la caméra du joueur
	if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		if (PC->PlayerCameraManager)
		{
			const FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
			// Le texte se lit dans le bon sens : son axe +X pointe VERS la caméra
			FRotator Face = (CamLoc - NameTag->GetComponentLocation()).Rotation();
			Face.Pitch = 0.f; Face.Roll = 0.f;
			NameTag->SetWorldRotation(Face);
		}
	}
}

void AWOTOLDemoUnit::BeginPlay()
{
	Super::BeginPlay();   // initialise UnitData -> stats, faction, rôle, CurrentHealth

	// Boss coriace : applique le multiplicateur de PV
	if (HealthScale > 1.f && UnitData)
	{
		CurrentHealth = UnitData->Stats.MaxHealth * HealthScale;
	}

	BuildGreyboxShape();
	OnUnitSelected.AddDynamic(this, &AWOTOLDemoUnit::HandleSelected);
	OnHealthChanged.AddDynamic(this, &AWOTOLDemoUnit::HandleHealthChanged);
	LastKnownHealth = CurrentHealth;
}

int32 AWOTOLDemoUnit::GetEffectiveMaxHealth() const
{
	const int32 BaseMax = UnitData ? UnitData->Stats.MaxHealth : 100;
	return FMath::RoundToInt(BaseMax * FMath::Max(1.f, HealthScale));
}

float AWOTOLDemoUnit::GetEffectiveHealthPercent() const
{
	const float Max = FMath::Max(1.f, (float)GetEffectiveMaxHealth());
	return FMath::Clamp(CurrentHealth / Max, 0.f, 1.f);
}

void AWOTOLDemoUnit::HandleHealthChanged(float NewHealth, float MaxHealth)
{
	// Chiffre de dégâts flottant rouge (uniquement quand on PERD des PV)
	if (LastKnownHealth >= 0.f && NewHealth < LastKnownHealth)
	{
		const float Dmg = LastKnownHealth - NewHealth;
		const FVector Loc = GetActorLocation() + FVector(0.f, 0.f, 60.f);
		AWOTOLDamageNumber::Spawn(GetWorld(), Loc, Dmg, FLinearColor(1.f, 0.f, 0.f, 1.f)); // rouge vif
	}
	LastKnownHealth = NewHealth;
}

void AWOTOLDemoUnit::HandleSelected(bool bSel)
{
	// Sélectionnée = toutes les pièces en blanc lumineux ; sinon couleur de base
	// propre à chaque pièce (conserve les accents or/violet au désélectionnement).
	for (int32 i = 0; i < PartMIDs.Num(); ++i)
	{
		if (!PartMIDs[i]) continue;
		const FLinearColor C = bSel ? FLinearColor(1.f, 1.f, 1.f, 1.f)
			: (PartBaseColors.IsValidIndex(i) ? PartBaseColors[i] : FFactionColors::Get(GetFaction()));
		PartMIDs[i]->SetVectorParameterValue(TEXT("Color"), C);
	}
}

void AWOTOLDemoUnit::CreatureBrainTick(float DeltaSeconds)
{
	if (!IsAlive()) return;
	UWorld* W = GetWorld();
	if (!W) return;
	UFactionRegistrySubsystem* Reg = W->GetSubsystem<UFactionRegistrySubsystem>();
	if (!Reg) return;

	// Cherche l'unité ennemie la plus proche
	const EFactionID EnemyFac = (GetFaction() == EFactionID::Aquiloris)
		? EFactionID::Noxeens : EFactionID::Aquiloris;

	AUnitBase* Nearest = nullptr;
	float Best = TNumericLimits<float>::Max();
	for (AUnitBase* U : Reg->GetUnitsForFaction(EnemyFac))
	{
		if (!U || !U->IsAlive()) continue;
		const float D = FVector::DistSquared(GetActorLocation(), U->GetActorLocation());
		if (D < Best) { Best = D; Nearest = U; }
	}
	if (!Nearest) return;

	FVector To = Nearest->GetActorLocation() - GetActorLocation();
	To.Z = 0.f;
	const float Dist = To.Size();

	// Se tourne vers la cible
	if (Dist > 1.f)
	{
		FRotator R = To.Rotation();
		R.Pitch = 0.f; R.Roll = 0.f;
		SetActorRotation(R);
	}

	const float Range = UnitData ? UnitData->Stats.AttackRange * 200.f : 200.f;
	const float Edge  = Dist - GetSimpleCollisionRadius() - Nearest->GetSimpleCollisionRadius();

	if (Edge <= Range)
	{
		PerformAttack(Nearest);   // throttlé par le cooldown interne de l'unité
	}
	else
	{
		AddMovementInput(To.GetSafeNormal(), 1.f); // avance vers la cible
	}
}

// Tailles réelles approximatives (mètres) — valeurs du GDD/document de démo
float AWOTOLDemoUnit::GetUnitHeightMeters(FName UnitID)
{
	// Aquiloris
	if (UnitID == TEXT("Aquis"))       return 1.80f;
	if (UnitID == TEXT("Aquiloryons")) return 1.75f;
	if (UnitID == TEXT("Aquilances"))  return 2.00f;
	if (UnitID == TEXT("Aquipheres"))  return 1.70f;
	if (UnitID == TEXT("Aquilombres")) return 1.55f;
	if (UnitID == TEXT("Leviaphenix")) return 4.00f;
	// Noxéens
	if (UnitID == TEXT("Noxar"))       return 1.50f;
	if (UnitID == TEXT("Noxeflare"))   return 1.70f;
	if (UnitID == TEXT("Noxebeast"))   return 2.50f;
	if (UnitID == TEXT("Noxeblast"))   return 1.60f;
	if (UnitID == TEXT("Noxeons"))     return 1.80f;
	if (UnitID == TEXT("Noxedrake"))   return 6.50f;
	return 1.75f; // défaut prototype
}

// ─── Helpers kitbash ────────────────────────────────────────────────────────
UStaticMeshComponent* AWOTOLDemoUnit::AddPart(const TCHAR* MeshPath, const FVector& RelLoc,
	const FVector& RelScale, const FRotator& RelRot, const FLinearColor& Color)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
	if (!C) return nullptr;
	C->SetupAttachment(RootComponent);
	C->RegisterComponent();
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, MeshPath))
	{
		C->SetStaticMesh(M);
	}
	C->SetRelativeLocationAndRotation(RelLoc, RelRot);
	C->SetRelativeScale3D(RelScale);

	if (UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, this))
		{
			MID->SetVectorParameterValue(TEXT("Color"), Color);
			C->SetMaterial(0, MID);
			PartMIDs.Add(MID);
			PartBaseColors.Add(Color);
		}
	}
	Parts.Add(C);
	return C;
}

void AWOTOLDemoUnit::SetupMainPart(const TCHAR* MeshPath, const FVector& RelLoc,
	const FVector& RelScale, const FRotator& RelRot, const FLinearColor& Color)
{
	if (!ShapeMesh) return;
	if (UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, MeshPath))
	{
		ShapeMesh->SetStaticMesh(M);
	}
	ShapeMesh->SetRelativeLocationAndRotation(RelLoc, RelRot);
	ShapeMesh->SetRelativeScale3D(RelScale);

	if (UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, this))
		{
			MID->SetVectorParameterValue(TEXT("Color"), Color);
			ShapeMesh->SetMaterial(0, MID);
			ShapeMID = MID;
			PartMIDs.Add(MID);
			PartBaseColors.Add(Color);
		}
	}
}

// Assemble une silhouette reconnaissable par unité (corps + tête + accessoires).
// Tout est exprimé par rapport au CENTRE de la capsule ; H = hauteur totale (UE).
void AWOTOLDemoUnit::AssembleSilhouette(FName UnitID, EUnitRole UnitRole, float H,
	const FLinearColor& Base, const FLinearColor& Accent)
{
	const FRotator NoRot = FRotator::ZeroRotator;
	const float h = H / 100.f; // facteur d'échelle vertical (mesh primitif = 100 UE)

	// ── Palette FIDÈLE aux références (couleurs propres à chaque unité) ──
	// Aquiloris : armure bleu acier, lisérés or, énergie cyan
	const FLinearColor AqArmor (0.11f, 0.20f, 0.50f, 1.f);
	const FLinearColor AqGold  (0.95f, 0.78f, 0.25f, 1.f);
	const FLinearColor AqEnergy(0.45f, 0.88f, 1.00f, 1.f);
	// Noxéens : corps sombre, lumens violets / bleus / verts selon l'unité
	const FLinearColor NoxDark  (0.08f, 0.07f, 0.13f, 1.f);
	const FLinearColor NoxViolet(0.62f, 0.20f, 0.98f, 1.f);
	const FLinearColor NoxBlue  (0.25f, 0.60f, 1.00f, 1.f);
	const FLinearColor NoxGreen (0.28f, 0.95f, 0.42f, 1.f);
	const FLinearColor NoxBronze(0.10f, 0.09f, 0.07f, 1.f);
	const FLinearColor Tusk     (0.55f, 0.42f, 0.20f, 1.f);

	// Corps humanoïde générique (torse + tête) — couleur passée en paramètre.
	auto BuildHumanoid = [&](float BodyW, const FLinearColor& Col)
	{
		SetupMainPart(M_CYL, FVector(0, 0, -H * 0.06f),
			FVector(BodyW, BodyW, h * 0.5f), NoRot, Col);              // torse
		AddPart(M_SPH, FVector(0, 0, H * 0.33f),
			FVector(BodyW * 0.95f, BodyW * 0.95f, BodyW * 0.95f), NoRot, Col); // tête
	};

	const FString Id = UnitID.ToString();

	// ───────────────── AQUILORIS (bleu acier + or + énergie cyan) ─────────────
	if (Id == TEXT("Aquis")) // Chef : épée photonique + cape
	{
		BuildHumanoid(0.34f, AqArmor);
		AddPart(M_CONE, FVector(20, 36, H * 0.20f), FVector(0.10f, 0.10f, h * 0.7f), FRotator(0, 0, 8.f), AqEnergy); // épée
		AddPart(M_CUBE, FVector(-22, 0, H * 0.05f), FVector(0.05f, 0.55f, h * 0.45f), FRotator(8.f, 0, 0), AqArmor); // cape
		AddPart(M_CONE, FVector(0, 0, H * 0.50f), FVector(0.18f, 0.18f, h * 0.12f), NoRot, AqGold);                  // crête or
		return;
	}
	if (Id == TEXT("Aquiloryons")) // Infanterie : ARTICULÉ + animé (épée + bouclier)
	{
		BuildArticulatedAquiloryons(H, AqArmor, AqEnergy);
		return;
	}
	if (Id == TEXT("Aquilances")) // Montée : cavalier sur monture + lance
	{
		SetupMainPart(M_SPH, FVector(10, 0, -H * 0.22f),
			FVector(h * 1.4f, h * 0.7f, h * 0.55f), NoRot, AqArmor);                                      // monture
		AddPart(M_CONE, FVector(70, 0, -H * 0.20f), FVector(0.5f, 0.5f, h * 0.3f), FRotator(70.f, 0, 0), AqArmor); // tête monture
		AddPart(M_CYL, FVector(-10, 0, H * 0.10f), FVector(0.26f, 0.26f, h * 0.3f), NoRot, AqArmor);      // cavalier corps
		AddPart(M_SPH, FVector(-10, 0, H * 0.34f), FVector(0.26f, 0.26f, 0.26f), NoRot, AqArmor);         // cavalier tête
		AddPart(M_CYL, FVector(20, 22, H * 0.18f), FVector(0.05f, 0.05f, h * 0.9f), FRotator(20.f, 0, 60.f), AqEnergy); // lance
		return;
	}
	if (Id == TEXT("Aquipheres") || Id == TEXT("Aquispheres")) // Distance : canon à sphère
	{
		BuildHumanoid(0.32f, AqArmor);
		AddPart(M_CYL, FVector(42, 10, H * 0.04f), FVector(0.16f, 0.16f, h * 0.5f), FRotator(90.f, 0, 0), AqGold); // canon (liseré or)
		AddPart(M_SPH, FVector(42 + H * 0.28f, 10, H * 0.04f), FVector(0.22f, 0.22f, 0.22f), NoRot, AqEnergy);     // sphère d'énergie
		return;
	}
	if (Id == TEXT("Aquilombres")) // Spéciale : assassin furtif (bleu nuit) + dague
	{
		BuildHumanoid(0.26f, FLinearColor(0.05f, 0.07f, 0.20f, 1.f));
		AddPart(M_CONE, FVector(16, 24, H * 0.10f), FVector(0.07f, 0.07f, h * 0.35f), FRotator(0, 0, 20.f), AqEnergy);
		return;
	}
	if (Id == TEXT("Leviaphenix")) // Mythique Aquiloris : grand corps + ailes or
	{
		SetupMainPart(M_SPH, FVector(0, 0, 0), FVector(h * 0.7f, h * 0.5f, h * 0.8f), NoRot, AqArmor);
		AddPart(M_CONE, FVector(20, 0, H * 0.45f), FVector(0.6f, 0.6f, h * 0.3f), NoRot, AqGold);                  // tête/bec or
		AddPart(M_CUBE, FVector(-10, 70, H * 0.1f), FVector(0.1f, h * 0.6f, h * 0.5f), FRotator(0, 0, 25.f), AqGold);  // aile
		AddPart(M_CUBE, FVector(-10, -70, H * 0.1f), FVector(0.1f, h * 0.6f, h * 0.5f), FRotator(0, 0, -25.f), AqGold);
		return;
	}

	// ───────────────── NOXÉENS (corps sombre + lumens) ─────────────────
	if (Id == TEXT("Noxar")) // Chef : humanoïde sombre tentaculé (lumens violets)
	{
		BuildHumanoid(0.34f, NoxDark);
		AddPart(M_SPH, FVector(8, 0, H * 0.33f), FVector(0.12f, 0.12f, 0.12f), NoRot, NoxViolet); // yeux violets
		for (int32 i = 0; i < 4; ++i)
		{
			const float Side = (i % 2 == 0) ? 1.f : -1.f;
			const float Up   = (i < 2) ? 0.30f : 0.18f;
			AddPart(M_CONE, FVector(-8, Side * 26, H * Up),
				FVector(0.07f, 0.07f, h * 0.4f), FRotator(0, 0, Side * 50.f), NoxViolet);
		}
		return;
	}
	if (Id == TEXT("Noxeflare")) // Infanterie : corps violet sombre, amas d'yeux violets
	{
		BuildHumanoid(0.32f, FLinearColor(0.12f, 0.06f, 0.18f, 1.f));
		AddPart(M_SPH, FVector(8, 0, H * 0.33f), FVector(0.13f, 0.13f, 0.13f), NoRot, NoxViolet); // amas d'yeux
		AddPart(M_CONE, FVector(0, 16, H * 0.42f), FVector(0.08f, 0.08f, h * 0.18f), FRotator(0, 0, 30.f), NoxViolet);
		AddPart(M_CONE, FVector(0, -16, H * 0.42f), FVector(0.08f, 0.08f, h * 0.18f), FRotator(0, 0, -30.f), NoxViolet);
		return;
	}
	if (Id == TEXT("Noxeblast")) // Distance : sombre, yeux + 2 tentacules dorsales BLEUES
	{
		BuildHumanoid(0.32f, NoxDark);
		AddPart(M_SPH, FVector(8, 0, H * 0.33f), FVector(0.10f, 0.10f, 0.10f), NoRot, NoxBlue); // yeux bleus
		AddPart(M_CONE, FVector(-14, 18, H * 0.30f), FVector(0.06f, 0.06f, h * 0.6f), FRotator(-30.f, 0, 35.f), NoxBlue);
		AddPart(M_CONE, FVector(-14, -18, H * 0.30f), FVector(0.06f, 0.06f, h * 0.6f), FRotator(-30.f, 0, -35.f), NoxBlue);
		return;
	}
	if (Id == TEXT("Noxebeast")) // Montée : quadrupède cuirassé bronze, yeux verts, défenses
	{
		SetupMainPart(M_CUBE, FVector(0, 0, -H * 0.18f),
			FVector(h * 1.3f, h * 0.85f, h * 0.55f), NoRot, NoxBronze); // corps massif
		AddPart(M_CUBE, FVector(H * 0.55f, 0, -H * 0.10f), FVector(h * 0.45f, h * 0.6f, h * 0.4f), NoRot, NoxBronze); // tête
		AddPart(M_SPH, FVector(H * 0.78f, 14, -H * 0.06f), FVector(0.07f, 0.07f, 0.07f), NoRot, NoxGreen);            // œil vert
		AddPart(M_SPH, FVector(H * 0.78f, -14, -H * 0.06f), FVector(0.07f, 0.07f, 0.07f), NoRot, NoxGreen);
		AddPart(M_CONE, FVector(H * 0.7f, 22, -H * 0.22f), FVector(0.08f, 0.08f, h * 0.25f), FRotator(120.f, 0, 0), Tusk); // défense
		AddPart(M_CONE, FVector(H * 0.7f, -22, -H * 0.22f), FVector(0.08f, 0.08f, h * 0.25f), FRotator(120.f, 0, 0), Tusk);
		const float LegZ = -H * 0.36f, LegX = H * 0.32f, LegY = H * 0.30f;
		for (int32 i = 0; i < 4; ++i)
		{
			const float Sx = (i < 2) ? 1.f : -1.f;
			const float Sy = (i % 2 == 0) ? 1.f : -1.f;
			AddPart(M_CYL, FVector(Sx * LegX, Sy * LegY, LegZ), FVector(0.16f, 0.16f, h * 0.18f), NoRot, NoxBronze);
		}
		AddPart(M_CONE, FVector(-H * 0.1f, 0, H * 0.06f), FVector(0.12f, 0.12f, h * 0.2f), NoRot, NoxBronze); // épine dorsale
		return;
	}
	if (Id == TEXT("Noxeons")) // Spéciale : organisme bioluminescent vert
	{
		SetupMainPart(M_SPH, FVector(0, 0, -H * 0.1f), FVector(h * 0.6f, h * 0.6f, h * 0.55f), NoRot, NoxDark);
		for (int32 i = 0; i < 5; ++i)
		{
			const float Ang = 2.f * PI * i / 5.f;
			AddPart(M_CONE, FVector(FMath::Cos(Ang) * 20.f, FMath::Sin(Ang) * 20.f, -H * 0.3f),
				FVector(0.07f, 0.07f, h * 0.3f), FRotator(0, FMath::RadiansToDegrees(Ang), 30.f), NoxGreen);
		}
		return;
	}
	if (Id == TEXT("Noxedrake")) // Mythique / boss "Kraken" : armure bleu-violet + craquelures cyan
	{
		const FLinearColor KrakArmor(0.14f, 0.11f, 0.26f, 1.f);
		const FLinearColor KrakGlow (0.20f, 0.85f, 1.00f, 1.f);
		SetupMainPart(M_SPH, FVector(0, 0, H * 0.05f), FVector(h * 0.55f, h * 0.55f, h * 0.5f), NoRot, KrakArmor); // tête bulbeuse
		AddPart(M_CONE, FVector(-10, 0, H * 0.35f), FVector(h * 0.6f, h * 0.6f, h * 0.5f), NoRot, KrakArmor);      // manteau pointu
		AddPart(M_SPH, FVector(H * 0.4f, 18, H * 0.08f), FVector(0.12f, 0.12f, 0.12f), NoRot, KrakGlow);           // œil
		AddPart(M_SPH, FVector(H * 0.4f, -18, H * 0.08f), FVector(0.12f, 0.12f, 0.12f), NoRot, KrakGlow);
		for (int32 i = 0; i < 6; ++i)
		{
			const float Ang = PI * (i / 5.f) - PI * 0.5f; // -90°..+90°
			AddPart(M_CONE, FVector(H * 0.25f + FMath::Cos(Ang) * 20.f, FMath::Sin(Ang) * 40.f, -H * 0.25f),
				FVector(0.14f, 0.14f, h * 0.55f), FRotator(120.f, FMath::RadiansToDegrees(Ang), 0), KrakArmor);
		}
		AddPart(M_CYL, FVector(H * 0.6f, 16, -H * 0.1f), FVector(0.06f, 0.06f, h * 0.9f), FRotator(80.f, 0, 0), KrakArmor); // fouet
		AddPart(M_CYL, FVector(H * 0.6f, -16, -H * 0.1f), FVector(0.06f, 0.06f, h * 0.9f), FRotator(80.f, 0, 0), KrakArmor);
		return;
	}

	// ───────────────── Fallback générique (rôle) ─────────────────
	switch (UnitRole)
	{
		case EUnitRole::Chef:       BuildHumanoid(0.36f, Base); break;
		case EUnitRole::Montee:     SetupMainPart(M_CUBE, FVector(0,0,-H*0.1f), FVector(h*0.9f,h*0.6f,h*0.6f), NoRot, Base); break;
		case EUnitRole::Distance:   BuildHumanoid(0.30f, Base); AddPart(M_CONE, FVector(36,0,0), FVector(0.14f,0.14f,h*0.3f), FRotator(90.f,0,0), Accent); break;
		case EUnitRole::Mythique:   SetupMainPart(M_SPH, FVector(0,0,0), FVector(h*0.7f,h*0.7f,h*0.8f), NoRot, Base); break;
		case EUnitRole::Speciale:   BuildHumanoid(0.24f, Base); break;
		default:                    BuildHumanoid(0.32f, Base); break;
	}
}

void AWOTOLDemoUnit::BuildGreyboxShape()
{
	if (!ShapeMesh) return;

	const EUnitRole UnitRole = UnitData ? UnitData->Role : EUnitRole::Infanterie;
	const FName UnitID       = UnitData ? UnitData->GetFName() : NAME_None;
	const float HeightU      = GetUnitHeightMeters(UnitID) * 100.f; // mètres -> UE units

	// Couleur d'ÉQUIPE en base (lisibilité RTS) + accent caractéristique de faction.
	const FLinearColor Base   = FFactionColors::Get(GetFaction());
	const FLinearColor Accent = (GetFaction() == EFactionID::Aquiloris)
		? FLinearColor(0.98f, 0.80f, 0.25f, 1.f)   // or/cyan Aquiloris
		: FLinearColor(0.65f, 0.20f, 0.95f, 1.f);  // violet bioluminescent Noxéen

	AssembleSilhouette(UnitID, UnitRole, HeightU, Base, Accent);

	// COLLISION : capsule dimensionnée selon le gabarit du rôle (empêche les
	// chevauchements et les unités qui rentrent dans la créature géante).
	float WidthFactor = 0.40f; // humanoïde par défaut
	switch (UnitRole)
	{
		case EUnitRole::Montee:   WidthFactor = 0.80f; break;
		case EUnitRole::Mythique: WidthFactor = 1.60f; break;
		case EUnitRole::Chef:     WidthFactor = 0.45f; break;
		default: break;
	}
	const float CapH = FMath::Max(40.f, HeightU * 0.5f);
	const float CapR = FMath::Max(24.f, HeightU * WidthFactor * 0.5f);
	GetCapsuleComponent()->SetCapsuleSize(CapR, CapH);
	if (NameTag) NameTag->SetRelativeLocation(FVector(0.f, 0.f, CapH + 50.f));

	// Disque d'équipe au sol (bleu Aquiloris / vert Noxéen) — repère de camp.
	AddTeamMarker(CapR, -CapH + 4.f, FFactionColors::Get(GetFaction()));
}

void AWOTOLDemoUnit::AddTeamMarker(float Radius, float ZFeet, const FLinearColor& Color)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
	if (!C) return;
	C->SetupAttachment(RootComponent);
	C->RegisterComponent();
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, M_CYL))
	{
		C->SetStaticMesh(M);
	}
	// Cylindre TRÈS plat = disque ; rayon un peu plus large que la capsule.
	const float RScale = (Radius * 1.3f) / 50.f;
	C->SetRelativeLocation(FVector(0.f, 0.f, ZFeet));
	C->SetRelativeScale3D(FVector(RScale, RScale, 0.04f));

	if (UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, this))
		{
			MID->SetVectorParameterValue(TEXT("Color"), Color);
			C->SetMaterial(0, MID);
		}
	}
	TeamMarker = C;
}

// ─── Articulation (pivot) ───────────────────────────────────────────────────
USceneComponent* AWOTOLDemoUnit::MakeJoint(USceneComponent* Parent, const FVector& RelLoc)
{
	USceneComponent* J = NewObject<USceneComponent>(this);
	if (!J) return nullptr;
	J->SetupAttachment(Parent ? Parent : RootComponent.Get());
	J->RegisterComponent();
	J->SetRelativeLocation(RelLoc);
	return J;
}

// ─── "Os" suspendu à une articulation ───────────────────────────────────────
UStaticMeshComponent* AWOTOLDemoUnit::MakeBone(USceneComponent* Joint, const TCHAR* MeshPath,
	const FVector& Offset, const FVector& Scale, const FRotator& Rot, const FLinearColor& Color)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
	if (!C) return nullptr;
	C->SetupAttachment(Joint ? Joint : RootComponent.Get());
	C->RegisterComponent();
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, MeshPath))
	{
		C->SetStaticMesh(M);
	}
	C->SetRelativeLocationAndRotation(Offset, Rot);
	C->SetRelativeScale3D(Scale);
	if (UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, this))
		{
			MID->SetVectorParameterValue(TEXT("Color"), Color);
			C->SetMaterial(0, MID);
			PartMIDs.Add(MID);
			PartBaseColors.Add(Color);
		}
	}
	Parts.Add(C);
	return C;
}

// ─── Aquiloryons articulé (torse + tête + 2 bras + 2 jambes + épée + bouclier) ──
void AWOTOLDemoUnit::BuildArticulatedAquiloryons(float H, const FLinearColor& Armor, const FLinearColor& Energy)
{
	const float h = H / 100.f;
	const FRotator NoRot = FRotator::ZeroRotator;

	// Torse (corps principal) + tête
	SetupMainPart(M_CYL, FVector(0, 0, H * 0.04f), FVector(0.34f, 0.22f, h * 0.40f), NoRot, Armor);
	AddPart(M_SPH, FVector(0, 0, H * 0.32f), FVector(0.22f, 0.22f, 0.22f), NoRot, Armor);

	// ── Bras DROIT (épée) : épaule → bras → coude → avant-bras → main → épée ──
	JRShoulder = MakeJoint(RootComponent, FVector(6.f, H * 0.16f, H * 0.22f));
	MakeBone(JRShoulder, M_CYL, FVector(0, 0, -H * 0.10f), FVector(0.09f, 0.09f, h * 0.22f), NoRot, Armor);
	JRElbow = MakeJoint(JRShoulder, FVector(0, 0, -H * 0.21f));
	MakeBone(JRElbow, M_CYL, FVector(0, 0, -H * 0.09f), FVector(0.08f, 0.08f, h * 0.20f), NoRot, Armor);
	MakeBone(JRElbow, M_SPH, FVector(0, 0, -H * 0.18f), FVector(0.10f, 0.10f, 0.10f), NoRot, Armor);          // main
	MakeBone(JRElbow, M_CONE, FVector(H * 0.05f, 0, -H * 0.20f), FVector(0.07f, 0.07f, h * 0.5f),
		FRotator(-90.f, 0, 0), Energy);                                                                      // épée (pointe +X)

	// ── Bras GAUCHE (bouclier) ──
	JLShoulder = MakeJoint(RootComponent, FVector(6.f, -H * 0.16f, H * 0.22f));
	MakeBone(JLShoulder, M_CYL, FVector(0, 0, -H * 0.10f), FVector(0.09f, 0.09f, h * 0.22f), NoRot, Armor);
	JLElbow = MakeJoint(JLShoulder, FVector(0, 0, -H * 0.21f));
	MakeBone(JLElbow, M_CYL, FVector(0, 0, -H * 0.09f), FVector(0.08f, 0.08f, h * 0.20f), NoRot, Armor);
	MakeBone(JLElbow, M_CUBE, FVector(H * 0.10f, 0, -H * 0.10f), FVector(0.07f, 0.42f, h * 0.40f), NoRot, Energy); // bouclier

	// ── Jambes (hanche → jambe complète) ──
	JRHip = MakeJoint(RootComponent, FVector(0, H * 0.09f, -H * 0.06f));
	MakeBone(JRHip, M_CYL, FVector(0, 0, -H * 0.14f), FVector(0.10f, 0.10f, h * 0.28f), NoRot, Armor);
	JLHip = MakeJoint(RootComponent, FVector(0, -H * 0.09f, -H * 0.06f));
	MakeBone(JLHip, M_CYL, FVector(0, 0, -H * 0.14f), FVector(0.10f, 0.10f, h * 0.28f), NoRot, Armor);

	bArticulated = true;
}

// ─── Animation procédurale (pilotée par l'état IA + la vitesse) ─────────────
void AWOTOLDemoUnit::AnimateArticulated(float Dt)
{
	if (!bArticulated) return;

	EUnitAIState St = EUnitAIState::Idle;
	if (UUnitAIStateComponent* S = FindComponentByClass<UUnitAIStateComponent>())
	{
		St = S->GetCurrentState();
	}
	const float Speed = GetVelocity().Size2D();
	const bool bMoving = (Speed > 10.f) || St == EUnitAIState::Seeking
		|| St == EUnitAIState::Patrolling || St == EUnitAIState::Retreating;
	const bool bAttacking = (St == EUnitAIState::Attacking);
	const bool bDead = !IsAlive();

	AnimPhase += Dt * (bMoving ? 9.f : 2.5f);

	float rSho = 0.f, rEl = 0.f, lShoRoll = 0.f, lSho = 0.f, lEl = 0.f, rHip = 0.f, lHip = 0.f, torsoRoll = 0.f;

	if (bDead)
	{
		// s'affaisse (jambes pliées, bras tombants)
		rSho = 70.f; lSho = 70.f; rHip = 60.f; lHip = 60.f; torsoRoll = 80.f;
	}
	else if (bAttacking)
	{
		SwingProgress += Dt * 2.4f;
		if (SwingProgress > 1.f) SwingProgress -= 1.f;
		const float Sw = FMath::Sin(SwingProgress * PI);       // 0→1→0 : armer puis frapper
		rSho     = FMath::Lerp(45.f, -85.f, Sw);               // lève l'épée puis abat
		rEl      = FMath::Lerp(-35.f, 5.f, Sw);
		lShoRoll = -65.f;                                      // bouclier levé en travers
		lEl      = -45.f;
	}
	else if (bMoving)
	{
		const float s = FMath::Sin(AnimPhase);
		rHip =  s * 30.f;  lHip = -s * 30.f;                   // jambes alternées
		rSho = -s * 22.f;  lSho =  s * 22.f;                   // bras opposés
		torsoRoll = FMath::Sin(AnimPhase * 2.f) * 2.5f;
	}
	else // idle : léger flottement
	{
		const float s = FMath::Sin(AnimPhase);
		rSho = 6.f + s * 4.f;  lSho = 6.f - s * 4.f;
		torsoRoll = s * 1.5f;
	}

	auto Set = [&](USceneComponent* J, const FRotator& Target)
	{
		if (!J) return;
		J->SetRelativeRotation(FMath::RInterpTo(J->GetRelativeRotation(), Target, Dt, 12.f));
	};
	Set(JRShoulder, FRotator(rSho, 0, 0));
	Set(JRElbow,    FRotator(rEl, 0, 0));
	Set(JLShoulder, FRotator(lSho, 0, lShoRoll));
	Set(JLElbow,    FRotator(lEl, 0, 0));
	Set(JRHip,      FRotator(rHip, 0, 0));
	Set(JLHip,      FRotator(lHip, 0, 0));
	if (ShapeMesh)
	{
		ShapeMesh->SetRelativeRotation(
			FMath::RInterpTo(ShapeMesh->GetRelativeRotation(), FRotator(0, 0, torsoRoll), Dt, 8.f));
	}
}
