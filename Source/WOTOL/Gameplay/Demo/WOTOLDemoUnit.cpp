#include "WOTOLDemoUnit.h"
#include "Gameplay/Units/UnitDataAsset.h"
#include "Gameplay/Units/UnitAIStateComponent.h"
#include "Gameplay/AI/AIAdaptiveController.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/CapsuleComponent.h"
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

	// Recolore le kraken en violet "calamar" une seule fois (toutes ses pièces)
	if (bCreatureBrain && !bCreatureStyled && PartMIDs.Num() > 0)
	{
		const FLinearColor Squid(0.55f, 0.1f, 0.7f, 1.f);
		const FLinearColor SquidAccent(0.2f, 0.85f, 1.f, 1.f); // craquelures cyan
		for (int32 i = 0; i < PartMIDs.Num(); ++i)
		{
			if (!PartMIDs[i]) continue;
			// la 1re pièce (manteau pointu) garde un liseré cyan, le reste violet
			const FLinearColor C = (i % 4 == 1) ? SquidAccent : Squid;
			PartMIDs[i]->SetVectorParameterValue(TEXT("Color"), C);
			if (PartBaseColors.IsValidIndex(i)) PartBaseColors[i] = C;
		}
		bCreatureStyled = true;
	}

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
void AWOTOLDemoUnit::AssembleSilhouette(FName UnitID, EUnitRole Role, float H,
	const FLinearColor& Base, const FLinearColor& Accent)
{
	const FRotator NoRot = FRotator::ZeroRotator;
	const float h = H / 100.f; // facteur d'échelle vertical (mesh primitif = 100 UE)

	// Corps humanoïde générique (torse + tête) réutilisé par la plupart des unités.
	auto BuildHumanoid = [&](float BodyW)
	{
		SetupMainPart(M_CYL, FVector(0, 0, -H * 0.06f),
			FVector(BodyW, BodyW, h * 0.5f), NoRot, Base);              // torse
		AddPart(M_SPH, FVector(0, 0, H * 0.33f),
			FVector(BodyW * 0.95f, BodyW * 0.95f, BodyW * 0.95f), NoRot, Base); // tête
	};

	const FString Id = UnitID.ToString();

	// ───────────────── AQUILORIS ─────────────────
	if (Id == TEXT("Aquis")) // Chef : épée photonique + cape
	{
		BuildHumanoid(0.34f);
		AddPart(M_CONE, FVector(20, 36, H * 0.20f), FVector(0.10f, 0.10f, h * 0.7f), FRotator(0, 0, 8.f), Accent); // épée
		AddPart(M_CUBE, FVector(-22, 0, H * 0.05f), FVector(0.05f, 0.55f, h * 0.45f), FRotator(8.f, 0, 0), Base);  // cape
		AddPart(M_CONE, FVector(0, 0, H * 0.50f), FVector(0.18f, 0.18f, h * 0.12f), NoRot, Accent);                // crête
		return;
	}
	if (Id == TEXT("Aquiloryons")) // Infanterie : épée + bouclier cristal
	{
		BuildHumanoid(0.34f);
		AddPart(M_CONE, FVector(18, 34, H * 0.20f), FVector(0.09f, 0.09f, h * 0.55f), FRotator(0, 0, 6.f), Accent); // épée
		AddPart(M_CUBE, FVector(16, -38, H * 0.02f), FVector(0.08f, 0.45f, h * 0.4f), FRotator(0, 0, -10.f), Accent); // bouclier
		return;
	}
	if (Id == TEXT("Aquilances")) // Montée : cavalier sur monture + lance
	{
		// Monture (corps allongé bas)
		SetupMainPart(M_SPH, FVector(10, 0, -H * 0.22f),
			FVector(h * 1.4f, h * 0.7f, h * 0.55f), NoRot, Base);
		AddPart(M_CONE, FVector(70, 0, -H * 0.20f), FVector(0.5f, 0.5f, h * 0.3f), FRotator(70.f, 0, 0), Base); // tête monture
		// Cavalier
		AddPart(M_CYL, FVector(-10, 0, H * 0.10f), FVector(0.26f, 0.26f, h * 0.3f), NoRot, Base);
		AddPart(M_SPH, FVector(-10, 0, H * 0.34f), FVector(0.26f, 0.26f, 0.26f), NoRot, Base);
		AddPart(M_CYL, FVector(20, 22, H * 0.18f), FVector(0.05f, 0.05f, h * 0.9f), FRotator(20.f, 0, 60.f), Accent); // lance
		return;
	}
	if (Id == TEXT("Aquipheres") || Id == TEXT("Aquispheres")) // Distance : canon à sphère
	{
		BuildHumanoid(0.32f);
		AddPart(M_CYL, FVector(42, 10, H * 0.04f), FVector(0.16f, 0.16f, h * 0.5f), FRotator(90.f, 0, 0), Base); // canon (axe +X)
		AddPart(M_SPH, FVector(42 + H * 0.28f, 10, H * 0.04f), FVector(0.22f, 0.22f, 0.22f), NoRot, Accent);     // sphère d'énergie
		return;
	}
	if (Id == TEXT("Aquilombres")) // Spéciale : assassin furtif, fin + dague
	{
		BuildHumanoid(0.26f);
		AddPart(M_CONE, FVector(16, 24, H * 0.10f), FVector(0.07f, 0.07f, h * 0.35f), FRotator(0, 0, 20.f), Accent);
		return;
	}
	if (Id == TEXT("Leviaphenix")) // Mythique Aquiloris : grand corps + ailes
	{
		SetupMainPart(M_SPH, FVector(0, 0, 0), FVector(h * 0.7f, h * 0.5f, h * 0.8f), NoRot, Base);
		AddPart(M_CONE, FVector(20, 0, H * 0.45f), FVector(0.6f, 0.6f, h * 0.3f), NoRot, Base);                 // tête/bec
		AddPart(M_CUBE, FVector(-10, 70, H * 0.1f), FVector(0.1f, h * 0.6f, h * 0.5f), FRotator(0, 0, 25.f), Accent); // aile
		AddPart(M_CUBE, FVector(-10, -70, H * 0.1f), FVector(0.1f, h * 0.6f, h * 0.5f), FRotator(0, 0, -25.f), Accent);
		return;
	}

	// ───────────────── NOXÉENS ─────────────────
	if (Id == TEXT("Noxar")) // Chef : humanoïde tentaculé
	{
		BuildHumanoid(0.34f);
		for (int32 i = 0; i < 4; ++i)
		{
			const float Side = (i % 2 == 0) ? 1.f : -1.f;
			const float Up   = (i < 2) ? 0.30f : 0.18f;
			AddPart(M_CONE, FVector(-8, Side * 26, H * Up),
				FVector(0.07f, 0.07f, h * 0.4f), FRotator(0, 0, Side * 50.f), Accent);
		}
		return;
	}
	if (Id == TEXT("Noxeflare")) // Infanterie : corps sombre, yeux/épines violets
	{
		BuildHumanoid(0.32f);
		AddPart(M_SPH, FVector(8, 0, H * 0.33f), FVector(0.12f, 0.12f, 0.12f), NoRot, Accent); // amas d'yeux violets
		AddPart(M_CONE, FVector(0, 16, H * 0.42f), FVector(0.08f, 0.08f, h * 0.18f), FRotator(0, 0, 30.f), Accent);
		AddPart(M_CONE, FVector(0, -16, H * 0.42f), FVector(0.08f, 0.08f, h * 0.18f), FRotator(0, 0, -30.f), Accent);
		return;
	}
	if (Id == TEXT("Noxeblast")) // Distance : 2 tentacules dorsales lumineuses
	{
		BuildHumanoid(0.32f);
		AddPart(M_SPH, FVector(8, 0, H * 0.33f), FVector(0.10f, 0.10f, 0.10f), NoRot, Accent); // yeux
		AddPart(M_CONE, FVector(-14, 18, H * 0.30f), FVector(0.06f, 0.06f, h * 0.6f), FRotator(-30.f, 0, 35.f), Accent);
		AddPart(M_CONE, FVector(-14, -18, H * 0.30f), FVector(0.06f, 0.06f, h * 0.6f), FRotator(-30.f, 0, -35.f), Accent);
		return;
	}
	if (Id == TEXT("Noxebeast")) // Montée : quadrupède cuirassé (défenses + épines)
	{
		SetupMainPart(M_CUBE, FVector(0, 0, -H * 0.18f),
			FVector(h * 1.3f, h * 0.85f, h * 0.55f), NoRot, Base); // corps massif
		AddPart(M_CUBE, FVector(H * 0.55f, 0, -H * 0.10f), FVector(h * 0.45f, h * 0.6f, h * 0.4f), NoRot, Base); // tête
		AddPart(M_SPH, FVector(H * 0.78f, 14, -H * 0.06f), FVector(0.07f, 0.07f, 0.07f), NoRot, Accent);          // œil vert
		AddPart(M_SPH, FVector(H * 0.78f, -14, -H * 0.06f), FVector(0.07f, 0.07f, 0.07f), NoRot, Accent);
		AddPart(M_CONE, FVector(H * 0.7f, 22, -H * 0.22f), FVector(0.08f, 0.08f, h * 0.25f), FRotator(120.f, 0, 0), Accent); // défense
		AddPart(M_CONE, FVector(H * 0.7f, -22, -H * 0.22f), FVector(0.08f, 0.08f, h * 0.25f), FRotator(120.f, 0, 0), Accent);
		// 4 pattes
		const float LegZ = -H * 0.36f, LegX = H * 0.32f, LegY = H * 0.30f;
		for (int32 i = 0; i < 4; ++i)
		{
			const float Sx = (i < 2) ? 1.f : -1.f;
			const float Sy = (i % 2 == 0) ? 1.f : -1.f;
			AddPart(M_CYL, FVector(Sx * LegX, Sy * LegY, LegZ), FVector(0.16f, 0.16f, h * 0.18f), NoRot, Base);
		}
		// épines dorsales
		AddPart(M_CONE, FVector(-H * 0.1f, 0, H * 0.06f), FVector(0.12f, 0.12f, h * 0.2f), NoRot, Base);
		return;
	}
	if (Id == TEXT("Noxeons")) // Spéciale : organisme bioluminescent
	{
		SetupMainPart(M_SPH, FVector(0, 0, -H * 0.1f), FVector(h * 0.6f, h * 0.6f, h * 0.55f), NoRot, Base);
		for (int32 i = 0; i < 5; ++i)
		{
			const float Ang = 2.f * PI * i / 5.f;
			AddPart(M_CONE, FVector(FMath::Cos(Ang) * 20.f, FMath::Sin(Ang) * 20.f, -H * 0.3f),
				FVector(0.07f, 0.07f, h * 0.3f), FRotator(0, FMath::RadiansToDegrees(Ang), 30.f), Accent);
		}
		return;
	}
	if (Id == TEXT("Noxedrake")) // Mythique / boss "Kraken" : manteau + tentacules
	{
		// Tête bulbeuse
		SetupMainPart(M_SPH, FVector(0, 0, H * 0.05f), FVector(h * 0.55f, h * 0.55f, h * 0.5f), NoRot, Base);
		// Manteau pointu (capuchon) — pièce 1 = accent cyan côté kraken
		AddPart(M_CONE, FVector(-10, 0, H * 0.35f), FVector(h * 0.6f, h * 0.6f, h * 0.5f), NoRot, Accent);
		// Yeux
		AddPart(M_SPH, FVector(H * 0.4f, 18, H * 0.08f), FVector(0.12f, 0.12f, 0.12f), NoRot, Accent);
		AddPart(M_SPH, FVector(H * 0.4f, -18, H * 0.08f), FVector(0.12f, 0.12f, 0.12f), NoRot, Accent);
		// Tentacules splayés vers le bas/avant
		for (int32 i = 0; i < 6; ++i)
		{
			const float Ang = PI * (i / 5.f) - PI * 0.5f; // -90°..+90°
			AddPart(M_CONE, FVector(H * 0.25f + FMath::Cos(Ang) * 20.f, FMath::Sin(Ang) * 40.f, -H * 0.25f),
				FVector(0.14f, 0.14f, h * 0.55f), FRotator(120.f, FMath::RadiansToDegrees(Ang), 0), Base);
		}
		// 2 longs fouets vers l'avant
		AddPart(M_CYL, FVector(H * 0.6f, 16, -H * 0.1f), FVector(0.06f, 0.06f, h * 0.9f), FRotator(80.f, 0, 0), Base);
		AddPart(M_CYL, FVector(H * 0.6f, -16, -H * 0.1f), FVector(0.06f, 0.06f, h * 0.9f), FRotator(80.f, 0, 0), Base);
		return;
	}

	// ───────────────── Fallback générique (rôle) ─────────────────
	switch (Role)
	{
		case EUnitRole::Chef:       BuildHumanoid(0.36f); break;
		case EUnitRole::Montee:     SetupMainPart(M_CUBE, FVector(0,0,-H*0.1f), FVector(h*0.9f,h*0.6f,h*0.6f), NoRot, Base); break;
		case EUnitRole::Distance:   BuildHumanoid(0.30f); AddPart(M_CONE, FVector(36,0,0), FVector(0.14f,0.14f,h*0.3f), FRotator(90.f,0,0), Accent); break;
		case EUnitRole::Mythique:   SetupMainPart(M_SPH, FVector(0,0,0), FVector(h*0.7f,h*0.7f,h*0.8f), NoRot, Base); break;
		case EUnitRole::Speciale:   BuildHumanoid(0.24f); break;
		default:                    BuildHumanoid(0.32f); break;
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
}
