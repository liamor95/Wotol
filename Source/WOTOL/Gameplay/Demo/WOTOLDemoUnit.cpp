#include "WOTOLDemoUnit.h"
#include "Gameplay/Units/UnitDataAsset.h"
#include "Gameplay/Units/UnitAIStateComponent.h"
#include "Gameplay/AI/AIAdaptiveController.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Data/WOTOLTypes.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Core/FactionRegistrySubsystem.h"
#include "WOTOLDamageNumber.h"
#include "WOTOLBubbleBurst.h"

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

	// Conteneur visuel : toutes les pièces s'y attachent -> on l'anime (nage/inclinaison)
	VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
	VisualRoot->SetupAttachment(RootComponent);

	ShapeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShapeMesh"));
	ShapeMesh->SetupAttachment(VisualRoot);
	ShapeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Proxy de clic : suit VisualRoot (la couche visuelle) et bloque le tracé Pawn
	// (clic gauche/droit) -> on peut sélectionner/cibler une unité affichée en hauteur.
	ClickProxy = CreateDefaultSubobject<USphereComponent>(TEXT("ClickProxy"));
	ClickProxy->SetupAttachment(VisualRoot);
	ClickProxy->SetSphereRadius(70.f);
	ClickProxy->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ClickProxy->SetCollisionObjectType(ECC_Pawn);
	ClickProxy->SetCollisionResponseToAllChannels(ECR_Ignore);
	ClickProxy->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	ClickProxy->SetCanEverAffectNavigation(false);

	// Étiquette flottante nom + PV (suit la couche visuelle -> attachée à VisualRoot)
	NameTag = CreateDefaultSubobject<UTextRenderComponent>(TEXT("NameTag"));
	NameTag->SetupAttachment(VisualRoot);
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

	// Sur ORDRE d'attaque (cible imposée), l'unité se cale sur la couche de sa cible.
	// Sinon le joueur garde le contrôle TOTAL de la couche (boutons Monter/Descendre).
	UpdateCombatLayer();

	AnimateBody(DeltaSeconds);          // flottement de nage + couche visuelle + tentacules
	if (bArticulated)
	{
		AnimateArticulated(DeltaSeconds); // rig détaillé (Aquiloryons)
	}

	if (!NameTag) return;

	// Le boss s'appelle "Kraken" (créature neutre), pas le nom du mythique rival
	const FString DisplayName = (bCreatureBrain || bIsBoss)
		? FString(TEXT("Kraken"))
		: ((UnitData && !UnitData->DisplayName.IsEmpty()) ? UnitData->DisplayName.ToString() : GetName());

	// VRAIES valeurs de PV (ex: "1700 / 2000"), boss inclus (HealthScale)
	const int32 MaxHP = GetEffectiveMaxHealth();
	const int32 CurHP = FMath::Clamp(FMath::RoundToInt(CurrentHealth), 0, MaxHP);

	NameTag->SetText(FText::FromString(
		FString::Printf(TEXT("%s\n%d / %d"), *DisplayName, CurHP, MaxHP)));

	// Couleur d'étiquette : violet "calamar" pour le kraken, sinon couleur de faction
	const FLinearColor TagColor = (bCreatureBrain || bIsBoss)
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

	// La couche verticale démarre au fond (décalage visuel 0). Le corps physique
	// reste un marcheur normal -> déplacement/attaques fiables à toute "hauteur".
	CurLayer = DesiredZ;
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
		// VFX d'impact : éclat de bulles (eau) à l'endroit du coup
		AWOTOLBubbleBurst::Burst(GetWorld(), Loc, FLinearColor(0.65f, 0.88f, 1.f, 1.f), 6);
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

AUnitBase* AWOTOLDemoUnit::FindNearestEnemyUnit() const
{
	UWorld* W = GetWorld();
	if (!W) return nullptr;
	UFactionRegistrySubsystem* Reg = W->GetSubsystem<UFactionRegistrySubsystem>();
	if (!Reg) return nullptr;

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
	return Nearest;
}

// Quand une CIBLE d'attaque est imposée (clic droit sur un ennemi), l'unité se cale
// sur la COUCHE de cette cible pour la frapper à son niveau (plus de coups dans le
// vide). N'écrase PAS le contrôle manuel de couche (qui n'impose pas de cible).
void AWOTOLDemoUnit::UpdateCombatLayer()
{
	if (bCreatureBrain) return; // le boss gère sa couche dans son cerveau
	UUnitAIStateComponent* S = FindComponentByClass<UUnitAIStateComponent>();
	if (!S) return;
	if (AWOTOLDemoUnit* T = Cast<AWOTOLDemoUnit>(S->ForceTarget.Get()))
	{
		if (T->IsAlive())
		{
			DesiredZ = FMath::Clamp(T->GetDesiredZ(), 0.f, 2400.f);
		}
	}
}

void AWOTOLDemoUnit::CreatureBrainTick(float DeltaSeconds)
{
	if (!IsAlive()) return;
	UWorld* W = GetWorld();
	if (!W) return;

	AUnitBase* Nearest = FindNearestEnemyUnit();
	if (!Nearest) return;

	// Le boss rejoint la couche visuelle de sa cible (plonge / remonte)
	if (AWOTOLDemoUnit* T = Cast<AWOTOLDemoUnit>(Nearest))
	{
		DesiredZ = FMath::Clamp(T->GetDesiredZ(), 0.f, 2400.f);
	}

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
	C->SetupAttachment(VisualRoot ? VisualRoot.Get() : RootComponent.Get());
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
	if (Id == TEXT("Aquis")) // Chef : ARTICULÉ + épée + cape + crête
	{
		BuildArticulatedHumanoid(H, AqArmor, 0.36f);
		MakeBone(JRElbow, M_CONE, FVector(0, 0, -H * 0.34f), FVector(0.06f, 0.06f, h * 0.55f), FRotator(180.f, 0, 0), AqEnergy); // épée
		AddPart(M_CUBE, FVector(-16, 0, H * 0.05f), FVector(0.05f, 0.55f, h * 0.45f), FRotator(8.f, 0, 0), AqArmor); // cape
		AddPart(M_CONE, FVector(0, 0, H * 0.46f), FVector(0.18f, 0.18f, h * 0.12f), NoRot, AqGold);                  // crête or
		return;
	}
	if (Id == TEXT("Aquiloryons")) // Infanterie : ARTICULÉ (épée + bouclier)
	{
		BuildArticulatedAquiloryons(H, AqArmor, AqEnergy);
		return;
	}
	if (Id == TEXT("Aquilances")) // Montée : cavalier sur monture + lance
	{
		SetupMainPart(M_SPH, FVector(10, 0, -H * 0.22f),
			FVector(h * 1.4f, h * 0.7f, h * 0.55f), NoRot, AqArmor);                                      // corps de la monture (poisson)
		AddPart(M_CONE, FVector(70, 0, -H * 0.20f), FVector(0.5f, 0.5f, h * 0.3f), FRotator(70.f, 0, 0), AqArmor); // tête monture
		// Nageoires latérales
		AddPart(M_CONE, FVector(10, 55, -H * 0.22f), FVector(0.3f, 0.3f, h * 0.25f), FRotator(0, 0, 80.f), AqArmor);
		AddPart(M_CONE, FVector(10, -55, -H * 0.22f), FVector(0.3f, 0.3f, h * 0.25f), FRotator(0, 0, -80.f), AqArmor);
		// QUEUE (nageoire caudale) qui bat — registre wiggle
		RegisterWiggle(AddPart(M_CONE, FVector(-70, 0, -H * 0.20f), FVector(0.45f, 0.10f, h * 0.5f),
			FRotator(90.f, 0, 0), AqArmor), 0.f);
		// Cavalier
		AddPart(M_CYL, FVector(-10, 0, H * 0.10f), FVector(0.26f, 0.26f, h * 0.3f), NoRot, AqArmor);      // cavalier corps
		AddPart(M_SPH, FVector(-10, 0, H * 0.34f), FVector(0.26f, 0.26f, 0.26f), NoRot, AqArmor);         // cavalier tête
		AddPart(M_CYL, FVector(20, 22, H * 0.18f), FVector(0.05f, 0.05f, h * 0.9f), FRotator(20.f, 0, 60.f), AqEnergy); // lance
		return;
	}
	if (Id == TEXT("Aquipheres") || Id == TEXT("Aquispheres")) // Distance : ARTICULÉ + canon
	{
		BuildArticulatedHumanoid(H, AqArmor, 0.32f);
		// Canon tenu par la main droite (prolonge l'avant-bras vers l'avant)
		MakeBone(JRElbow, M_CYL, FVector(H * 0.22f, 0, -H * 0.15f), FVector(0.15f, 0.15f, h * 0.34f), FRotator(90.f, 0, 0), AqGold);
		MakeBone(JRElbow, M_SPH, FVector(H * 0.40f, 0, -H * 0.15f), FVector(0.20f, 0.20f, 0.20f), NoRot, AqEnergy); // sphère d'énergie
		return;
	}
	if (Id == TEXT("Aquilombres")) // Spéciale : ARTICULÉ furtif (bleu nuit) + dague
	{
		BuildArticulatedHumanoid(H, FLinearColor(0.05f, 0.07f, 0.20f, 1.f), 0.26f);
		MakeBone(JRElbow, M_CONE, FVector(0, 0, -H * 0.26f), FVector(0.05f, 0.05f, h * 0.28f), FRotator(180.f, 0, 0), AqEnergy); // dague
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
	// NB : identité Noxéenne = VERT/abyssal bioluminescent (distinct du Kraken violet).
	if (Id == TEXT("Noxar")) // Chef : ARTICULÉ vert-abyssal + tentacules dorsales
	{
		BuildArticulatedHumanoid(H, FLinearColor(0.05f, 0.11f, 0.10f, 1.f), 0.34f);
		AddPart(M_SPH, FVector(10, 0, H * 0.33f), FVector(0.13f, 0.10f, 0.10f), NoRot, NoxGreen); // yeux verts
		// Tentacules dorsales (attachées au torse via VisualRoot) qui ondulent
		for (int32 i = 0; i < 4; ++i)
		{
			const float Side = (i % 2 == 0) ? 1.f : -1.f;
			const float Up   = (i < 2) ? 0.28f : 0.16f;
			RegisterWiggle(AddPart(M_CONE, FVector(-12, Side * 22, H * Up),
				FVector(0.06f, 0.06f, h * 0.42f), FRotator(0, 0, Side * 50.f), NoxGreen), i * 1.3f);
		}
		return;
	}
	if (Id == TEXT("Noxeflare")) // Infanterie : ARTICULÉ vert-abyssal, amas d'yeux verts
	{
		BuildArticulatedHumanoid(H, FLinearColor(0.06f, 0.13f, 0.11f, 1.f), 0.32f);
		AddPart(M_SPH, FVector(10, 0, H * 0.33f), FVector(0.14f, 0.11f, 0.11f), NoRot, NoxGreen); // amas d'yeux
		AddPart(M_CONE, FVector(2, 14, H * 0.42f), FVector(0.07f, 0.07f, h * 0.16f), FRotator(0, 0, 30.f), NoxGreen);
		AddPart(M_CONE, FVector(2, -14, H * 0.42f), FVector(0.07f, 0.07f, h * 0.16f), FRotator(0, 0, -30.f), NoxGreen);
		return;
	}
	if (Id == TEXT("Noxeblast")) // Distance : ARTICULÉ sombre + 2 tentacules dorsales BLEUES
	{
		BuildArticulatedHumanoid(H, NoxDark, 0.32f);
		AddPart(M_SPH, FVector(10, 0, H * 0.33f), FVector(0.11f, 0.09f, 0.09f), NoRot, NoxBlue); // yeux bleus
		RegisterWiggle(AddPart(M_CONE, FVector(-16, 16, H * 0.28f), FVector(0.055f, 0.055f, h * 0.6f), FRotator(-30.f, 0, 35.f), NoxBlue), 0.f);
		RegisterWiggle(AddPart(M_CONE, FVector(-16, -16, H * 0.28f), FVector(0.055f, 0.055f, h * 0.6f), FRotator(-30.f, 0, -35.f), NoxBlue), 3.14f);
		return;
	}
	if (Id == TEXT("Noxebeast")) // Montée : QUADRUPÈDE cuirassé bronze (4 pattes + queue animées)
	{
		// Corps arrondi (sphère allongée = plus doux qu'un cube)
		SetupMainPart(M_SPH, FVector(0, 0, -H * 0.16f),
			FVector(h * 1.35f, h * 0.90f, h * 0.60f), NoRot, NoxBronze);
		AddPart(M_SPH, FVector(H * 0.60f, 0, -H * 0.06f), FVector(h * 0.5f, h * 0.55f, h * 0.45f), NoRot, NoxBronze); // tête (arrondie)
		AddPart(M_SPH, FVector(H * 0.80f, 14, -H * 0.02f), FVector(0.08f, 0.08f, 0.08f), NoRot, NoxGreen);            // œil vert
		AddPart(M_SPH, FVector(H * 0.80f, -14, -H * 0.02f), FVector(0.08f, 0.08f, 0.08f), NoRot, NoxGreen);
		AddPart(M_CONE, FVector(H * 0.72f, 22, -H * 0.20f), FVector(0.08f, 0.08f, h * 0.25f), FRotator(120.f, 0, 0), Tusk); // défenses
		AddPart(M_CONE, FVector(H * 0.72f, -22, -H * 0.20f), FVector(0.08f, 0.08f, h * 0.25f), FRotator(120.f, 0, 0), Tusk);

		// 4 PATTES articulées (hanches) — avant vs arrière, animées en marche
		const float LegX = H * 0.34f, LegY = H * 0.34f;
		JRShoulder = MakeJoint(VisualRoot, FVector(LegX, LegY, -H * 0.12f));   // avant droit
		MakeBone(JRShoulder, M_CYL, FVector(0, 0, -H * 0.13f), FVector(0.18f, 0.18f, h * 0.26f), NoRot, NoxBronze);
		JLShoulder = MakeJoint(VisualRoot, FVector(LegX, -LegY, -H * 0.12f));  // avant gauche
		MakeBone(JLShoulder, M_CYL, FVector(0, 0, -H * 0.13f), FVector(0.18f, 0.18f, h * 0.26f), NoRot, NoxBronze);
		JRHip = MakeJoint(VisualRoot, FVector(-LegX, LegY, -H * 0.12f));       // arrière droit
		MakeBone(JRHip, M_CYL, FVector(0, 0, -H * 0.13f), FVector(0.18f, 0.18f, h * 0.26f), NoRot, NoxBronze);
		JLHip = MakeJoint(VisualRoot, FVector(-LegX, -LegY, -H * 0.12f));      // arrière gauche
		MakeBone(JLHip, M_CYL, FVector(0, 0, -H * 0.13f), FVector(0.18f, 0.18f, h * 0.26f), NoRot, NoxBronze);

		// QUEUE qui remue (registre wiggle)
		RegisterWiggle(AddPart(M_CONE, FVector(-H * 0.75f, 0, -H * 0.10f),
			FVector(0.14f, 0.14f, h * 0.4f), FRotator(-100.f, 0, 0), NoxBronze), 0.f);
		AddPart(M_CONE, FVector(-H * 0.1f, 0, H * 0.06f), FVector(0.12f, 0.12f, h * 0.2f), NoRot, NoxBronze); // épine dorsale
		bArticulated = true; // fait bouger les 4 pattes (démarche quadrupède)
		return;
	}
	if (Id == TEXT("Noxeons")) // Spéciale : organisme bioluminescent vert
	{
		SetupMainPart(M_SPH, FVector(0, 0, -H * 0.1f), FVector(h * 0.6f, h * 0.6f, h * 0.55f), NoRot, NoxDark);
		for (int32 i = 0; i < 5; ++i)
		{
			const float Ang = 2.f * PI * i / 5.f;
			RegisterWiggle(AddPart(M_CONE, FVector(FMath::Cos(Ang) * 20.f, FMath::Sin(Ang) * 20.f, -H * 0.3f),
				FVector(0.07f, 0.07f, h * 0.3f), FRotator(0, FMath::RadiansToDegrees(Ang), 30.f), NoxGreen), i * 1.2f);
		}
		return;
	}
	if (Id == TEXT("Noxedrake")) // Mythique / boss "KRAKEN" : céphalopode géant
	{
		const FLinearColor KrakArmor(0.16f, 0.12f, 0.30f, 1.f); // manteau bleu-violet
		const FLinearColor KrakSkin (0.24f, 0.16f, 0.34f, 1.f); // chair un peu plus claire
		const FLinearColor KrakGlow (0.20f, 0.85f, 1.00f, 1.f); // yeux/craquelures cyan
		const FLinearColor Beak     (0.05f, 0.05f, 0.06f, 1.f);

		// MANTEAU : grand cône lisse pointant vers le haut/arrière (capuchon du calamar)
		AddPart(M_CONE, FVector(-H * 0.15f, 0, H * 0.30f), FVector(h * 0.55f, h * 0.55f, h * 0.85f),
			FRotator(-18.f, 0, 0), KrakArmor);
		// TÊTE bulbeuse (grosse sphère) au centre
		SetupMainPart(M_SPH, FVector(0, 0, H * 0.02f), FVector(h * 0.60f, h * 0.60f, h * 0.55f), NoRot, KrakSkin);
		// Bourrelet frontal (là où partent les bras) — adoucit la jonction
		AddPart(M_SPH, FVector(H * 0.30f, 0, -H * 0.10f), FVector(h * 0.45f, h * 0.50f, h * 0.35f), NoRot, KrakSkin);
		// GROS YEUX cyan de chaque côté
		AddPart(M_SPH, FVector(H * 0.30f, H * 0.28f, H * 0.05f), FVector(h * 0.14f, h * 0.14f, h * 0.14f), NoRot, KrakGlow);
		AddPart(M_SPH, FVector(H * 0.30f, -H * 0.28f, H * 0.05f), FVector(h * 0.14f, h * 0.14f, h * 0.14f), NoRot, KrakGlow);
		// BEC sombre au centre-avant
		AddPart(M_CONE, FVector(H * 0.42f, 0, -H * 0.16f), FVector(h * 0.12f, h * 0.12f, h * 0.18f), FRotator(60.f, 0, 0), Beak);

		// 8 TENTACULES organisés en éventail vers l'avant/bas, ondulant en séquence
		for (int32 i = 0; i < 8; ++i)
		{
			const float t = (i / 7.f) - 0.5f;              // -0.5..0.5
			const float Yaw = t * 150.f;                   // éventail net (pas d'amas)
			const FVector Root(H * 0.30f, t * H * 0.55f, -H * 0.20f);
			RegisterWiggle(AddPart(M_CONE, Root,
				FVector(0.16f, 0.16f, h * 0.7f), FRotator(120.f, Yaw, 0), KrakArmor), (float)i * 0.6f);
		}
		// 2 longs FOUETS barbelés vers l'avant
		RegisterWiggle(AddPart(M_CYL, FVector(H * 0.5f, H * 0.12f, -H * 0.05f),
			FVector(0.06f, 0.06f, h * 1.1f), FRotator(85.f, 0, 0), KrakArmor), 0.5f);
		RegisterWiggle(AddPart(M_CYL, FVector(H * 0.5f, -H * 0.12f, -H * 0.05f),
			FVector(0.06f, 0.06f, h * 1.1f), FRotator(85.f, 0, 0), KrakArmor), 2.5f);
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
	if (ClickProxy) ClickProxy->SetSphereRadius(FMath::Max(CapR, CapH * 0.8f));

	// Déphasage d'animation propre à chaque unité (désync le flottement)
	BobSeed = FMath::Fmod(GetActorLocation().X * 0.021f + GetActorLocation().Y * 0.013f, 6.283f);

	// Disque d'équipe au sol — SAUF pour le boss/mythique (son disque géant cachait
	// les petites unités). Rayon plafonné pour ne jamais masquer le combat.
	if (UnitRole != EUnitRole::Mythique && !bCreatureBrain)
	{
		const float MarkerR = FMath::Min(CapR, 70.f);
		AddTeamMarker(MarkerR, -CapH + 2.f, FFactionColors::Get(GetFaction()));
	}
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
	J->SetupAttachment(Parent ? Parent : (VisualRoot ? VisualRoot.Get() : RootComponent.Get()));
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

// ─── Squelette humanoïde générique (coudes + genoux) — réutilisé par tous ──────
void AWOTOLDemoUnit::BuildArticulatedHumanoid(float H, const FLinearColor& Col, float BodyW)
{
	const float h = H / 100.f;
	const FRotator NoRot = FRotator::ZeroRotator;

	// Torse + cou + tête
	SetupMainPart(M_CYL, FVector(0, 0, H * 0.04f), FVector(BodyW, BodyW * 0.85f, h * 0.36f), NoRot, Col);
	AddPart(M_CYL, FVector(0, 0, H * 0.25f), FVector(BodyW * 0.42f, BodyW * 0.42f, h * 0.06f), NoRot, Col); // cou
	AddPart(M_SPH, FVector(0, 0, H * 0.33f), FVector(BodyW * 0.82f, BodyW * 0.82f, BodyW * 0.9f), NoRot, Col); // tête
	// Épaulières (rondeurs qui adoucissent la silhouette)
	AddPart(M_SPH, FVector(4, H * 0.15f, H * 0.21f), FVector(BodyW * 0.55f, BodyW * 0.55f, BodyW * 0.5f), NoRot, Col);
	AddPart(M_SPH, FVector(4, -H * 0.15f, H * 0.21f), FVector(BodyW * 0.55f, BodyW * 0.55f, BodyW * 0.5f), NoRot, Col);

	const float ArmW = FMath::Max(0.07f, BodyW * 0.26f);

	// Bras DROIT : épaule → bras → coude → avant-bras → main
	JRShoulder = MakeJoint(VisualRoot, FVector(4.f, H * 0.15f, H * 0.21f));
	MakeBone(JRShoulder, M_CYL, FVector(0, 0, -H * 0.09f), FVector(ArmW, ArmW, h * 0.18f), NoRot, Col);
	JRElbow = MakeJoint(JRShoulder, FVector(0, 0, -H * 0.18f));
	MakeBone(JRElbow, M_CYL, FVector(0, 0, -H * 0.08f), FVector(ArmW * 0.9f, ArmW * 0.9f, h * 0.16f), NoRot, Col);
	MakeBone(JRElbow, M_SPH, FVector(0, 0, -H * 0.15f), FVector(ArmW * 1.1f, ArmW * 1.1f, ArmW * 1.1f), NoRot, Col); // main

	// Bras GAUCHE
	JLShoulder = MakeJoint(VisualRoot, FVector(4.f, -H * 0.15f, H * 0.21f));
	MakeBone(JLShoulder, M_CYL, FVector(0, 0, -H * 0.09f), FVector(ArmW, ArmW, h * 0.18f), NoRot, Col);
	JLElbow = MakeJoint(JLShoulder, FVector(0, 0, -H * 0.18f));
	MakeBone(JLElbow, M_CYL, FVector(0, 0, -H * 0.08f), FVector(ArmW * 0.9f, ArmW * 0.9f, h * 0.16f), NoRot, Col);
	MakeBone(JLElbow, M_SPH, FVector(0, 0, -H * 0.15f), FVector(ArmW * 1.1f, ArmW * 1.1f, ArmW * 1.1f), NoRot, Col);

	// Jambe DROITE : hanche → cuisse → genou → tibia → pied
	const float LegW = FMath::Max(0.09f, BodyW * 0.30f);
	JRHip = MakeJoint(VisualRoot, FVector(0, H * 0.08f, -H * 0.05f));
	MakeBone(JRHip, M_CYL, FVector(0, 0, -H * 0.10f), FVector(LegW, LegW, h * 0.20f), NoRot, Col);
	JRKnee = MakeJoint(JRHip, FVector(0, 0, -H * 0.20f));
	MakeBone(JRKnee, M_CYL, FVector(0, 0, -H * 0.10f), FVector(LegW * 0.9f, LegW * 0.9f, h * 0.20f), NoRot, Col);
	MakeBone(JRKnee, M_CUBE, FVector(H * 0.03f, 0, -H * 0.20f), FVector(0.14f, LegW, 0.05f), NoRot, Col); // pied

	// Jambe GAUCHE
	JLHip = MakeJoint(VisualRoot, FVector(0, -H * 0.08f, -H * 0.05f));
	MakeBone(JLHip, M_CYL, FVector(0, 0, -H * 0.10f), FVector(LegW, LegW, h * 0.20f), NoRot, Col);
	JLKnee = MakeJoint(JLHip, FVector(0, 0, -H * 0.20f));
	MakeBone(JLKnee, M_CYL, FVector(0, 0, -H * 0.10f), FVector(LegW * 0.9f, LegW * 0.9f, h * 0.20f), NoRot, Col);
	MakeBone(JLKnee, M_CUBE, FVector(H * 0.03f, 0, -H * 0.20f), FVector(0.14f, LegW, 0.05f), NoRot, Col);

	bArticulated = true;
}

// Aquiloryons = humanoïde + épée (main droite) + bouclier cristal (main gauche)
void AWOTOLDemoUnit::BuildArticulatedAquiloryons(float H, const FLinearColor& Armor, const FLinearColor& Energy)
{
	const float h = H / 100.f;
	BuildArticulatedHumanoid(H, Armor, 0.34f);
	// Épée dans la main droite (prolonge l'avant-bras)
	MakeBone(JRElbow, M_CONE, FVector(0, 0, -H * 0.30f), FVector(0.06f, 0.06f, h * 0.42f), FRotator(180.f, 0, 0), Energy);
	// Bouclier au bras gauche
	MakeBone(JLElbow, M_CUBE, FVector(H * 0.10f, 0, -H * 0.10f), FVector(0.07f, 0.42f, h * 0.40f), FRotator::ZeroRotator, Energy);
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

	float rSho = 0.f, rEl = 0.f, lShoRoll = 0.f, lSho = 0.f, lEl = 0.f;
	float rHip = 0.f, lHip = 0.f, rKnee = 12.f, lKnee = 12.f, torsoRoll = 0.f, torsoPitch = 0.f;

	if (bDead)
	{
		// s'affaisse (jambes pliées, bras tombants)
		rSho = 70.f; lSho = 70.f; rHip = 40.f; lHip = 40.f; rKnee = 70.f; lKnee = 70.f; torsoRoll = 80.f;
	}
	else if (bAttacking)
	{
		SwingProgress += Dt * 2.4f;
		if (SwingProgress > 1.f) SwingProgress -= 1.f;
		const float Sw = FMath::Sin(SwingProgress * PI);       // 0→1→0 : armer puis frapper
		rSho     = FMath::Lerp(45.f, -85.f, Sw);               // épaule : arme puis abat
		rEl      = FMath::Lerp(-70.f, 10.f, Sw);               // coude : replie puis déploie
		lShoRoll = -55.f;                                      // bras gauche levé en travers
		lEl      = -55.f;
		torsoPitch = -6.f - 8.f * Sw;                          // le buste accompagne le coup
		rKnee = 25.f; lKnee = 18.f;                            // appui/transfert de poids
	}
	else if (bMoving)
	{
		const float s = FMath::Sin(AnimPhase);
		rHip =  s * 30.f;  lHip = -s * 30.f;                   // jambes alternées
		rKnee = 15.f + FMath::Max(0.f, -s) * 45.f;             // genou plie en fin de foulée
		lKnee = 15.f + FMath::Max(0.f,  s) * 45.f;
		rSho = -s * 24.f;  lSho =  s * 24.f;                   // bras opposés
		rEl  = -20.f - FMath::Max(0.f, -s) * 25.f;             // coudes fléchis à la marche
		lEl  = -20.f - FMath::Max(0.f,  s) * 25.f;
		torsoRoll = FMath::Sin(AnimPhase * 2.f) * 2.5f;
		torsoPitch = 4.f;                                      // légèrement penché en avant
	}
	else // idle : léger flottement
	{
		const float s = FMath::Sin(AnimPhase);
		rSho = 6.f + s * 4.f;  lSho = 6.f - s * 4.f;
		rEl = -12.f; lEl = -12.f;
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
	Set(JRKnee,     FRotator(rKnee, 0, 0));
	Set(JLKnee,     FRotator(lKnee, 0, 0));
	if (ShapeMesh)
	{
		ShapeMesh->SetRelativeRotation(
			FMath::RInterpTo(ShapeMesh->GetRelativeRotation(), FRotator(torsoPitch, 0, torsoRoll), Dt, 8.f));
	}
}

void AWOTOLDemoUnit::RegisterWiggle(USceneComponent* Comp, float Phase)
{
	if (!Comp) return;
	WiggleComps.Add(Comp);
	WiggleBase.Add(Comp->GetRelativeRotation());
	WigglePhase.Add(Phase);
}

// Animation GÉNÉRIQUE (toutes unités) : flottement de nage + inclinaison + tentacules.
void AWOTOLDemoUnit::AnimateBody(float Dt)
{
	AnimClock += Dt;
	const bool bDead = !IsAlive();

	EUnitAIState St = EUnitAIState::Idle;
	if (UUnitAIStateComponent* S = FindComponentByClass<UUnitAIStateComponent>())
	{
		St = S->GetCurrentState();
	}
	const float Speed  = GetVelocity().Size2D();
	const bool  bMoving = (Speed > 10.f) || St == EUnitAIState::Seeking
		|| St == EUnitAIState::Patrolling || St == EUnitAIState::Retreating;
	const bool  bAttacking = (St == EUnitAIState::Attacking);

	// Couche verticale VISUELLE : monte/descend en douceur vers DesiredZ
	CurLayer = FMath::FInterpTo(CurLayer, DesiredZ, Dt, 2.5f);

	// ── Flottement du conteneur visuel (nage) + inclinaison + couche ──
	if (VisualRoot)
	{
		const float Amp  = bDead ? 0.f : (bMoving ? 2.5f : 5.5f);
		const float Bob  = FMath::Sin(AnimClock * 1.6f + BobSeed) * Amp;
		const float Roll = FMath::Sin(AnimClock * 1.25f + BobSeed) * 2.0f;
		float Pitch = 0.f, Lunge = 0.f;
		// Attaque des unités NON articulées : petit à-coup vers l'avant
		if (!bArticulated && bAttacking)
		{
			const float s = FMath::Sin(AnimClock * 7.f);
			Pitch = -6.f - 5.f * FMath::Abs(s);
			Lunge = 6.f * FMath::Max(0.f, s);
		}
		if (bDead) { Pitch = 70.f; }

		VisualRoot->SetRelativeLocation(
			FMath::VInterpTo(VisualRoot->GetRelativeLocation(), FVector(Lunge, 0.f, Bob + CurLayer), Dt, 10.f));
		VisualRoot->SetRelativeRotation(
			FMath::RInterpTo(VisualRoot->GetRelativeRotation(), FRotator(Pitch, 0.f, Roll), Dt, 8.f));
	}

	// ── Ondulation des appendices (tentacules) ──
	for (int32 i = 0; i < WiggleComps.Num(); ++i)
	{
		if (!WiggleComps[i]) continue;
		const float P = WigglePhase.IsValidIndex(i) ? WigglePhase[i] : 0.f;
		const FRotator Base = WiggleBase.IsValidIndex(i) ? WiggleBase[i] : FRotator::ZeroRotator;
		const FRotator Osc(FMath::Sin(AnimClock * 2.2f + P) * 16.f, 0.f,
			FMath::Cos(AnimClock * 1.8f + P) * 12.f);
		WiggleComps[i]->SetRelativeRotation(Base + Osc);
	}
}
