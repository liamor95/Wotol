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
#include "WOTOLCaptureObject.h"
#include "WOTOLCoverStructure.h"
#include "WOTOLInkZone.h"
#include "WOTOLBeam.h"
#include "WOTOLProjectileTracer.h"
#include "DemoFlowSubsystem.h"
#include "OceanCurrentSubsystem.h"
#include "Engine/GameInstance.h"

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

	// Ombre noire (dessinée légèrement décalée derrière) — fort contraste avec le décor
	NameTagShadow = CreateDefaultSubobject<UTextRenderComponent>(TEXT("NameTagShadow"));
	NameTagShadow->SetupAttachment(VisualRoot);
	NameTagShadow->SetRelativeLocation(FVector(0.f, 0.f, 140.f));
	NameTagShadow->SetHorizontalAlignment(EHTA_Center);
	NameTagShadow->SetWorldSize(44.f); // un peu plus gros que le nom = fin liseré noir centré
	NameTagShadow->SetTextRenderColor(FColor(0, 0, 0, 255));
	NameTagShadow->SetText(FText::GetEmpty());

	// Étiquette flottante nom + PV (sœur de l'ombre, positionnée devant chaque frame)
	NameTag = CreateDefaultSubobject<UTextRenderComponent>(TEXT("NameTag"));
	NameTag->SetupAttachment(VisualRoot);
	NameTag->SetRelativeLocation(FVector(0.f, 0.f, 140.f));
	NameTag->SetHorizontalAlignment(EHTA_Center);
	NameTag->SetWorldSize(40.f);
	NameTag->SetText(FText::GetEmpty());

	// L'IA RTS possède automatiquement l'unité au spawn
	AIControllerClass = AAIAdaptiveController::StaticClass();
	AutoPossessAI     = EAutoPossessAI::PlacedInWorldOrSpawned;

	// ORIENTATION : on gère TOUTE la rotation nous-mêmes dans Tick (face à l'ennemi en
	// combat, sinon face au déplacement). On DÉSACTIVE l'orientation auto du mouvement qui
	// se battait avec notre code -> fini le "regarde/frappe en arrière".
	bUseControllerRotationYaw = false;
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->bOrientRotationToMovement = false;
		Move->bUseControllerDesiredRotation = false;
	}
}

void AWOTOLDemoUnit::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// MORTE : plus aucune logique de jeu (pas de compétence, pas de combat, pas de contrôle
	// de couche). On anime UNIQUEMENT la lente descente vers le fond + l'affalement.
	if (!IsAlive())
	{
		// DÉRIVE DU CADAVRE dans le COURANT : tant que le corps traverse les couches
		// hautes concernées par le courant, il est EMPORTÉ dans son sens -> descente EN
		// DIAGONALE. Sous ces couches (GetFactorAt -> 0), la dérive cesse et il tombe droit.
		if (UWorld* W = GetWorld())
			if (UOceanCurrentSubsystem* Cur = W->GetSubsystem<UOceanCurrentSubsystem>())
			{
				const float F = Cur->GetFactorAt(CurLayer); // intensité à la hauteur ACTUELLE du corps
				if (F > 0.01f)
					AddActorWorldOffset(Cur->GetDirection() * (Cur->GetStrength() * F * 0.6f * DeltaSeconds), false);
			}

		AnimateBody(DeltaSeconds);
		if (bArticulated) AnimateArticulated(DeltaSeconds);
		AnimateWhips(DeltaSeconds);
		return;
	}

	if (bCreatureBrain)
	{
		CreatureBrainTick(DeltaSeconds);
	}
	else
	{
		TickAbility(DeltaSeconds); // compétence active périodique (unités normales)
		if (TargetCover.IsValid()) TickAttackCover(DeltaSeconds); // attaque de décor ordonnée
	}

	// Sur ORDRE d'attaque (cible imposée), l'unité se cale sur la couche de sa cible.
	// Sinon le joueur garde le contrôle TOTAL de la couche (boutons Monter/Descendre).
	UpdateCombatLayer(DeltaSeconds);

	AnimateBody(DeltaSeconds);          // flottement de nage + couche visuelle + tentacules
	if (bArticulated)
	{
		AnimateArticulated(DeltaSeconds); // rig détaillé (Aquiloryons)
	}
	AnimateWhips(DeltaSeconds);          // fouets du Kraken (no-op si l'unité n'en a pas)

	// FACE À L'ENNEMI EN COMBAT : dès qu'un ennemi est à portée de combat, l'unité se
	// tourne vers lui (pas seulement dans l'état "Attacking", car en mêlée l'état peut
	// varier). -> le modèle regarde l'ennemi et le coup part VERS L'AVANT (fini le "frappe
	// en arrière"). Sinon, l'unité garde son orientation de déplacement (OrientToMovement).
	if (!bCreatureBrain)
	{
		FRotator Desired = GetActorRotation();
		bool bWant = false;
		// 1) Un ennemi à portée de combat -> on lui FAIT FACE (le coup part devant).
		if (AUnitBase* Foe = FindNearestEnemyUnit())
		{
			FVector To = Foe->GetActorLocation() - GetActorLocation();
			To.Z = 0.f;
			const float Dist = To.Size();
			const float AtkRange = UnitData ? UnitData->Stats.AttackRange * 200.f : 200.f;
			if (Dist > 1.f && Dist < AtkRange + 500.f) { Desired = To.Rotation(); bWant = true; }
		}
		// 2) Sinon, on regarde la direction de DÉPLACEMENT.
		if (!bWant)
		{
			FVector V = GetVelocity(); V.Z = 0.f;
			if (V.SizeSquared() > 100.f) { Desired = V.Rotation(); bWant = true; }
		}
		if (bWant)
		{
			Desired.Pitch = 0.f; Desired.Roll = 0.f;
			SetActorRotation(FMath::RInterpTo(GetActorRotation(), Desired, DeltaSeconds, 22.f));
		}
	}

	// COURANT OCÉANIQUE : sur les couches HAUTES, la dérive pousse physiquement l'unité
	// (joueur, ennemi ET Kraken). N'agit qu'EN BATAILLE (pas pendant le placement, sinon
	// les unités posées s'envoleraient). La résistance vient de leur propre déplacement.
	if (IsAlive())
	{
		if (UOceanCurrentSubsystem* Cur = GetWorld() ? GetWorld()->GetSubsystem<UOceanCurrentSubsystem>() : nullptr)
		{
			const FVector Drift = Cur->GetDriftAt(CurLayer);
			if (!Drift.IsNearlyZero())
			{
				bool bInBattle = false;
				if (UGameInstance* GI = GetGameInstance())
					if (UDemoFlowSubsystem* D = GI->GetSubsystem<UDemoFlowSubsystem>())
						bInBattle = (D->GetScreen() == EDemoScreen::Playing);
				if (bInBattle)
				{
					AddActorWorldOffset(Drift * DeltaSeconds, true); // dérive (respecte la collision)
				}
			}
		}
	}

	// RALENTI par l'encre du Kraken : tant que SlowUntil est actif, vitesse fortement
	// réduite ; sinon vitesse de base restaurée.
	if (GetCharacterMovement() && BaseWalkSpeed > 0.f)
	{
		const float NowS = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
		GetCharacterMovement()->MaxWalkSpeed = (NowS < SlowUntil) ? BaseWalkSpeed * 0.4f : BaseWalkSpeed;
	}

	if (!NameTag) return;

	// ANTI-EMPILEMENT : en pleine bataille, on n'affiche l'étiquette (nom + PV) que pour
	// les unités SÉLECTIONNÉES (+ le boss) -> plus de bouillie de texte quand les unités se
	// regroupent. En préparation/hors-jeu, on montre tout (les unités sont espacées).
	// Étiquette nom + PV TOUJOURS visible (pour distinguer chaque unité). Le contour noir
	// centré assure la lisibilité ; les formations espacent les unités.
	if (!NameTag->IsVisible())
	{
		NameTag->SetVisibility(true);
		if (NameTagShadow) NameTagShadow->SetVisibility(true);
	}

	// Le boss s'appelle "Kraken" (créature neutre), pas le nom du mythique rival
	const FString DisplayName = (bCreatureBrain || bIsBoss)
		? FString(TEXT("Kraken"))
		: ((UnitData && !UnitData->DisplayName.IsEmpty()) ? UnitData->DisplayName.ToString() : GetName());

	// VRAIES valeurs de PV (ex: "1700 / 2000"), boss inclus (HealthScale)
	const int32 MaxHP = GetEffectiveMaxHealth();
	const int32 CurHP = FMath::Clamp(FMath::RoundToInt(CurrentHealth), 0, MaxHP);

	const FText TagText = FText::FromString(
		FString::Printf(TEXT("%s\n%d / %d"), *DisplayName, CurHP, MaxHP));
	NameTag->SetText(TagText);
	if (NameTagShadow) NameTagShadow->SetText(TagText); // même texte, en noir, derrière

	// Couleur d'étiquette VIVE et LUMINEUSE, distincte par camp (survoltée pour "briller"
	// sur l'ombre noire = fort contraste, lisible dans l'ambiance sous-marine sombre) :
	// BLEU Aquiloris, VERT Noxéens, VIOLET Kraken.
	FLinearColor TagColor;
	if (bCreatureBrain || bIsBoss)                 TagColor = FLinearColor(1.10f, 0.45f, 1.60f, 1.f); // violet
	else if (GetFaction() == EFactionID::Aquiloris) TagColor = FLinearColor(0.35f, 0.85f, 1.70f, 1.f); // bleu
	else if (GetFaction() == EFactionID::Noxeens)   TagColor = FLinearColor(0.40f, 1.70f, 0.60f, 1.f); // vert
	else                                            TagColor = FFactionColors::Get(GetFaction()) * 1.5f;
	NameTag->SetTextRenderColor(TagColor.ToFColor(false)); // false = pas de clamp sRGB -> plus lumineux

	// L'étiquette + son ombre font face à la caméra ; l'ombre est décalée derrière et
	// en bas-droite (en espace écran) pour créer un fort contraste (liseré noir).
	if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		if (PC->PlayerCameraManager)
		{
			const FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
			const FVector NameLoc = NameTag->GetComponentLocation();
			FRotator Face = (CamLoc - NameLoc).Rotation();
			Face.Pitch = 0.f; Face.Roll = 0.f;
			NameTag->SetWorldRotation(Face);
			if (NameTagShadow)
			{
				// CONTOUR (pas d'ombre décalée) : le noir est CENTRÉ et un peu plus gros,
				// juste DERRIÈRE le texte -> il déborde en fin liseré = lettres bien
				// détourées, sans double-vision.
				NameTagShadow->SetWorldRotation(Face);
				NameTagShadow->SetWorldLocation(NameLoc - Face.Vector() * 1.5f);
			}
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

	// PLAFOND DE VERTICALITÉ propre à UNE unité précise : le NOXEBEAST (montée Noxéen) est
	// limité au grade 1 (sol + 1re couche = 800) — il ne peut pas nager sur les 2 couches
	// hautes. Ciblé par son identifiant, PAS par le rôle : les Aquilances (montées) ne sont
	// PAS concernées et peuvent monter jusqu'au grade 3.
	if (UnitData && UnitData->DisplayName.ToString().Contains(TEXT("Noxebeast")))
	{
		MaxLayerZ = 800.f;
		DesiredZ  = FMath::Min(DesiredZ, MaxLayerZ);
	}

	BuildGreyboxShape();
	OnUnitSelected.AddDynamic(this, &AWOTOLDemoUnit::HandleSelected);
	OnHealthChanged.AddDynamic(this, &AWOTOLDemoUnit::HandleHealthChanged);
	OnUnitDied.AddDynamic(this, &AWOTOLDemoUnit::HandleDeath);
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

void AWOTOLDemoUnit::SetHealthToFull()
{
	CurrentHealth  = (float)GetEffectiveMaxHealth();
	LastKnownHealth = CurrentHealth;
}

float AWOTOLDemoUnit::GetEffectiveHealthPercent() const
{
	const float Max = FMath::Max(1.f, (float)GetEffectiveMaxHealth());
	return FMath::Clamp(CurrentHealth / Max, 0.f, 1.f);
}

USceneComponent* AWOTOLDemoUnit::GetFloatingTextAnchor() const
{
	return VisualRoot ? VisualRoot.Get() : Super::GetFloatingTextAnchor();
}

void AWOTOLDemoUnit::HandleHealthChanged(float NewHealth, float MaxHealth)
{
	// Chiffre de dégâts flottant rouge (uniquement quand on PERD des PV), ACCROCHÉ à
	// l'unité : il suit l'unité et reste à sa hauteur (couche verticale). Petit décalage
	// latéral aléatoire -> les coups successifs ne se superposent pas.
	if (LastKnownHealth >= 0.f && NewHealth < LastKnownHealth)
	{
		const float Dmg = LastKnownHealth - NewHealth;
		const FVector Anchor = GetFloatingTextAnchor()
			? GetFloatingTextAnchor()->GetComponentLocation() : GetActorLocation();
		const FVector Jitter(FMath::FRandRange(-35.f, 35.f), FMath::FRandRange(-35.f, 35.f), 0.f);
		if (AWOTOLDamageNumber* N = AWOTOLDamageNumber::Spawn(
				GetWorld(), Anchor, Dmg, FLinearColor(0.55f, 0.f, 0.f, 1.f))) // rouge FONCÉ (contraste sur sol clair)
		{
			N->SetFollow(GetFloatingTextAnchor(), FVector(0.f, 0.f, 110.f) + Jitter);
		}
		// VFX d'impact : éclat de bulles (eau) à la position visuelle de l'unité
		AWOTOLBubbleBurst::Burst(GetWorld(), Anchor + FVector(0, 0, 60.f),
			FLinearColor(0.65f, 0.88f, 1.f, 1.f), 6);
	}
	LastKnownHealth = NewHealth;
}

void AWOTOLDemoUnit::HandleDeath(AUnitBase* /*Unit*/)
{
	// L'unité est morte : elle ne nage plus, ne bouge plus, n'est plus jouable.
	DesiredZ = 0.f; // elle va couler vers le fond (interpolé lentement dans AnimateBody)

	// Stoppe net tout déplacement physique et coupe le moteur de marche.
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->DisableMovement();
	}
	// Coupe le cerveau de créature (le Kraken) le cas échéant.
	bCreatureBrain = false;

	// Non sélectionnable / non ciblable : on désactive le capteur de clic.
	if (ClickProxy)
	{
		ClickProxy->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	// Retire l'anneau/disque de sélection s'il était affiché.
	SetSelected(false);
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
// Fait tendre DesiredZ vers GoalZ, mais seulement APRÈS un délai de réaction : quand la
// cible change de hauteur, on note la nouvelle hauteur et on attend ~1,8 s avant de la
// suivre -> l'ennemi met un temps à s'adapter, il ne monte/descend plus en même temps que
// le joueur. (Le contrôle MANUEL du joueur, lui, reste instantané : voir ChangeLayerForSelection.)
void AWOTOLDemoUnit::AdaptLayerTo(float GoalZ, float Dt)
{
	const float Clamped = FMath::Clamp(GoalZ, 0.f, 2400.f);
	if (!FMath::IsNearlyEqual(Clamped, LayerAdaptGoal, 1.f))
	{
		LayerAdaptGoal  = Clamped;       // nouvelle hauteur de cible détectée
		LayerReactTimer = 1.8f;          // temps d'adaptation avant de la suivre
	}
	if (LayerReactTimer > 0.f)
	{
		LayerReactTimer -= Dt;
		if (LayerReactTimer <= 0.f)
		{
			DesiredZ = LayerAdaptGoal;   // l'unité commence enfin à changer de couche
		}
	}
}

void AWOTOLDemoUnit::UpdateCombatLayer(float Dt)
{
	if (bCreatureBrain) return; // le boss gère sa couche dans son cerveau
	UUnitAIStateComponent* S = FindComponentByClass<UUnitAIStateComponent>();
	if (!S) return;
	if (AWOTOLDemoUnit* T = Cast<AWOTOLDemoUnit>(S->ForceTarget.Get()))
	{
		if (T->IsAlive())
		{
			AdaptLayerTo(T->GetDesiredZ(), Dt);
		}
	}
}

void AWOTOLDemoUnit::CreatureBrainTick(float DeltaSeconds)
{
	if (!IsAlive()) return;
	UWorld* W = GetWorld();
	if (!W) return;

	// COUP DE FOUET périodique : toutes les ~2 s, le Kraken balaie ses grands
	// tentacules -> repousse et blesse les unités devant lui (il tient plus longtemps
	// et représente un vrai défi). L'animation de déroulé est jouée par AnimateWhips.
	WhipCooldown -= DeltaSeconds;
	if (WhipCooldown <= 0.f)
	{
		DoWhipStrike();
		WhipCooldown = 2.6f; // moins fréquent (2.0 -> 2.6)
	}

	AUnitBase* Nearest = FindNearestEnemyUnit();
	if (!Nearest) return;

	// JET D'ENCRE périodique (~12 s) : le céphalopode projette de l'encre sur un groupe
	// ennemi -> flaque IRRÉGULIÈRE au sol qui RALENTIT et réduit la PRÉCISION des unités
	// dessus (+ léger poison). Même capacité quelle que soit la faction affrontée.
	InkCooldown -= DeltaSeconds;
	if (InkCooldown <= 0.f)
	{
		InkCooldown = FMath::FRandRange(11.f, 14.f);
		DoInkJet(Nearest);
	}

	// ── MANŒUVRE : le Kraken n'est pas STATIQUE. Toutes les ~4-8 s il déclenche un
	// contournement : il tourne autour de l'ennemi (strafe latéral) et change de couche
	// verticale pour attaquer sous un autre angle / anticiper. Occupant 2 niveaux, sa
	// BASE est bornée à [0, 800] (sommet <= couche 4 = surface, il ne sort pas de l'eau).
	BossRepositionCD -= DeltaSeconds;
	if (BossRepositionCD <= 0.f)
	{
		BossRepositionCD = FMath::FRandRange(5.f, 8.f);
		BossRepoTimer    = FMath::FRandRange(2.5f, 4.f);
		BossStrafeDir    = (FMath::FRand() < 0.5f) ? 1.f : -1.f;
		BossLayerGoal    = (FMath::FRand() < 0.5f) ? 0.f : 800.f; // plonge au sol ou remonte d'1 couche
	}
	const bool bManeuver = (BossRepoTimer > 0.f);
	if (bManeuver) BossRepoTimer -= DeltaSeconds;

	// Couche : pendant la manœuvre il vise sa propre couche (BossLayerGoal), sinon il
	// rejoint celle de sa cible — toujours borné à 2 niveaux d'occupation.
	float LayerGoal = BossLayerGoal;
	if (!bManeuver)
		if (AWOTOLDemoUnit* T = Cast<AWOTOLDemoUnit>(Nearest)) LayerGoal = T->GetDesiredZ();
	AdaptLayerTo(FMath::Clamp(LayerGoal, 0.f, 800.f), DeltaSeconds);

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

	// Déplacement LATÉRAL pendant la manœuvre : il CONTOURNE l'ennemi (utilise l'espace)
	// même quand il est déjà à portée, au lieu de rester planté au centre.
	if (bManeuver && Dist > 1.f)
	{
		const FVector Fwd  = To.GetSafeNormal();
		const FVector Side = FVector::CrossProduct(FVector::UpVector, Fwd) * BossStrafeDir;
		const FVector Push = (Side * 0.85f + Fwd * (Edge > Range ? 0.4f : -0.15f)).GetSafeNormal();
		AddMovementInput(Push, 1.f);
	}

	if (Edge <= Range)
	{
		PerformAttack(Nearest);   // throttlé par le cooldown interne de l'unité

		// ATTAQUE CRITIQUE (boss) : de temps en temps, aléatoirement, le colosse assène un
		// coup dévastateur -> gros dégâts bonus + libellé "CRITIQUE". Cadencé par un cooldown.
		CritCooldown -= DeltaSeconds;
		if (CritCooldown <= 0.f && FMath::FRand() < 0.28f && Nearest->IsAlive())
		{
			CritCooldown = FMath::FRandRange(7.f, 11.f); // encore moins fréquent
			Nearest->TakeDamageFromUnit(120.f, this);    // relevé (85->120) : qq pertes côté Noxéens
			const FVector CritLoc = Nearest->GetActorLocation() + FVector(0, 0, 90.f);
			if (AWOTOLDamageNumber* N = AWOTOLDamageNumber::SpawnText(W, CritLoc, TEXT("CRITIQUE !"),
					FLinearColor(1.f, 0.35f, 0.f, 1.f)))
			{
				N->SetFollow(Nearest->GetFloatingTextAnchor(), FVector(0, 0, 140.f));
			}
			AWOTOLBubbleBurst::Burst(W, CritLoc, FLinearColor(1.f, 0.5f, 0.2f, 1.f), 12);
		}
	}
	else
	{
		CritCooldown -= DeltaSeconds;
		AddMovementInput(To.GetSafeNormal(), 1.f); // avance vers la cible
	}
}

// ─── Attaque de décor ordonnée par le joueur ────────────────────────────────
void AWOTOLDemoUnit::OrderAttackCover(AWOTOLCoverStructure* Cover)
{
	TargetCover = Cover;
	LastPlayerOrderTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	// Coupe l'ordre de déplacement auto pour ne pas être détourné.
	if (UUnitAIStateComponent* S = FindComponentByClass<UUnitAIStateComponent>())
		S->bFollowingPlayerOrder = false;
}

void AWOTOLDemoUnit::TickAttackCover(float Dt)
{
	AWOTOLCoverStructure* Cov = TargetCover.Get();
	if (!Cov || Cov->IsDestroyed()) { TargetCover = nullptr; return; }
	if (!IsAlive() || !UnitData) return;

	FVector To = Cov->GetActorLocation() - GetActorLocation(); To.Z = 0.f;
	const float Dist = To.Size();
	// Se tourne vers la structure.
	if (Dist > 1.f) { FRotator R = To.Rotation(); R.Pitch = 0.f; R.Roll = 0.f;
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), R, Dt, 12.f)); }

	const float Range = UnitData->Stats.AttackRange * 200.f;
	const bool  bRanged = (UnitData->Stats.AttackType == EUnitAttackType::Ranged);
	const float Reach = bRanged ? FMath::Max(Range, 900.f) : (Range + 150.f);

	if (Dist <= Reach)
	{
		CoverAttackTimer -= Dt;
		if (CoverAttackTimer <= 0.f)
		{
			CoverAttackTimer = FMath::Max(0.5f, UnitData->Stats.AttackCooldown);
			const float Dmg = UnitData->Stats.AttackDPS * UnitData->Stats.AttackCooldown * GlobalDamageScale;
			Cov->TakeCoverDamage(Dmg, this); // désagrège la structure
			// Tir/impact visuel
			const FVector From = (GetFloatingTextAnchor() ? GetFloatingTextAnchor()->GetComponentLocation()
				: GetActorLocation()) + FVector(0, 0, 40.f);
			const FVector Hit = Cov->GetActorLocation() + FVector(0, 0, 200.f);
			const FLinearColor Col = (GetFaction() == EFactionID::Aquiloris)
				? FLinearColor(0.3f, 0.95f, 1.f, 1.f) : FLinearColor(0.55f, 0.35f, 1.f, 1.f);
			AWOTOLProjectileTracer::Fire(GetWorld(), From, Hit, Col, 1.6f);
		}
	}
	else
	{
		AddMovementInput(To.GetSafeNormal(), 1.f); // s'approche de la structure
	}
}

// ─── Compétences ACTIVES ────────────────────────────────────────────────────
float AWOTOLDemoUnit::GetAbilityCooldownFor(FName Id) const
{
	// Valeurs du tableur (colonne CD), en secondes.
	if (Id == TEXT("Aquis"))       return 12.f;
	if (Id == TEXT("Aquiloryons")) return 10.f;
	if (Id == TEXT("Aquilances"))  return 14.f;
	if (Id == TEXT("Aquipheres") || Id == TEXT("Aquispheres")) return 8.f;
	if (Id == TEXT("Noxar"))       return 12.f;
	if (Id == TEXT("Noxeflare"))   return 10.f;
	if (Id == TEXT("Noxebeast"))   return 14.f;
	if (Id == TEXT("Noxeblast"))   return 8.f;
	return 12.f;
}

void AWOTOLDemoUnit::TickAbility(float Dt)
{
	if (!IsAlive() || !UnitData) return;

	// Compétences uniquement EN BATAILLE (pas au placement).
	if (UGameInstance* GI = GetGameInstance())
		if (UDemoFlowSubsystem* D = GI->GetSubsystem<UDemoFlowSubsystem>())
			if (D->GetScreen() != EDemoScreen::Playing) return;

	if (!bAbilityInit)
	{
		bAbilityInit  = true;
		AbilityCooldown = GetAbilityCooldownFor(UnitData->GetFName()) * FMath::FRandRange(0.6f, 1.1f);
	}

	AbilityCooldown -= Dt;
	if (AbilityCooldown > 0.f) return;
	AbilityCooldown = GetAbilityCooldownFor(UnitData->GetFName());
	UseAbility();
}

void AWOTOLDemoUnit::UseAbility()
{
	const FString Id = UnitData ? UnitData->GetFName().ToString() : FString();
	if (Id == TEXT("Aquis"))        Ability_Shockwave();
	else if (Id == TEXT("Noxar"))   Ability_Laser();
	else if (Id == TEXT("Noxeblast")) Ability_ProjectileBurst();
	else if (Id == TEXT("Noxeflare")) Ability_BlindFlash();
	// (Aquiloryons/Aquilances/Aquispheres/Noxebeast : leur "compétence" est leur
	//  comportement de formation/charge géré par le cerveau tactique.)
}

// AQUIS — Lame Photonique : frappe le sol -> onde de choc qui REPOUSSE et blesse les
// ennemis autour (répit). N'agit que s'il y a des ennemis proches.
void AWOTOLDemoUnit::Ability_Shockwave()
{
	UWorld* W = GetWorld(); if (!W) return;
	UFactionRegistrySubsystem* Reg = W->GetSubsystem<UFactionRegistrySubsystem>(); if (!Reg) return;
	const EFactionID Enemy = (GetFaction() == EFactionID::Aquiloris) ? EFactionID::Noxeens : EFactionID::Aquiloris;
	const FVector Origin = GetActorLocation();
	const float Radius = 700.f;
	int32 Hit = 0;
	for (AUnitBase* U : Reg->GetUnitsForFaction(Enemy))
	{
		if (!U || !U->IsAlive()) continue;
		FVector To = U->GetActorLocation() - Origin; To.Z = 0.f;
		if (To.Size() > Radius) continue;
		U->LaunchCharacter(To.GetSafeNormal() * 1400.f + FVector(0, 0, 350.f), true, true);
		U->TakeDamageFromUnit(70.f, this);
		++Hit;
	}
	// ── ONDE DE CHOC blanc-bleu : éclat central + FRONT circulaire (anneau de jaillissements
	// au bord du rayon) -> lecture d'une onde qui se propage et REPOUSSE tout autour. ──
	const FLinearColor Wave(0.75f, 0.95f, 1.f, 1.f);
	AWOTOLBubbleBurst::Burst(W, Origin + FVector(0, 0, 30.f), FLinearColor(1.f, 1.f, 1.f, 1.f), 30); // flash central blanc
	const int32 Ring = 14;
	for (int32 i = 0; i < Ring; ++i)
	{
		const float A = 2.f * PI * i / Ring;
		const FVector P = Origin + FVector(FMath::Cos(A), FMath::Sin(A), 0.f) * (Radius * 0.72f) + FVector(0, 0, 25.f);
		AWOTOLBubbleBurst::Burst(W, P, Wave, 6); // front de l'onde
	}
	if (Hit > 0)
		AWOTOLDamageNumber::SpawnText(W, Origin + FVector(0, 0, 160.f), TEXT("Lame Photonique"), Wave);
}

// NOXAR — Rayon laser : cible l'OBJECTIF (bâtiment adverse) si présent, sinon l'ennemi
// le plus proche. Gros dégâts, ponctuel.
void AWOTOLDemoUnit::Ability_Laser()
{
	UWorld* W = GetWorld(); if (!W) return;
	const FVector From = (GetFloatingTextAnchor() ? GetFloatingTextAnchor()->GetComponentLocation()
		: GetActorLocation()) + FVector(0, 0, 40.f);

	FVector To = From; bool bHasTarget = false;
	// 1) Objectif : bâtiment de capture adverse (le sien = celui qu'il n'a pas)
	if (UGameInstance* GI = GetGameInstance())
		if (UDemoFlowSubsystem* D = GI->GetSubsystem<UDemoFlowSubsystem>())
			if (AWOTOLCaptureObject* Obj = Cast<AWOTOLCaptureObject>(D->GetCaptureObject()))
				if (Obj->OwnerFaction != GetFaction() && Obj->GetHealthPercent() > 0.f)
				{
					Obj->ApplyDamage(200.f); // réduit 650->200 : menace réelle sans pulvériser l'objectif
					To = Obj->GetActorLocation() + FVector(0, 0, 120.f);
					bHasTarget = true;
				}
	// 2) Sinon : ennemi le plus proche
	if (!bHasTarget)
		if (AUnitBase* Foe = FindNearestEnemyUnit())
		{
			Foe->TakeDamageFromUnit(520.f, this);
			To = (Foe->GetFloatingTextAnchor() ? Foe->GetFloatingTextAnchor()->GetComponentLocation()
				: Foe->GetActorLocation()) + FVector(0, 0, 40.f);
			bHasTarget = true;
		}
	if (!bHasTarget) return;

	// Couleur du rayon = couleur de FACTION (vert Noxéen / cyan Aquiloris).
	const FLinearColor BeamCol = (GetFaction() == EFactionID::Noxeens)
		? FLinearColor(0.3f, 1.f, 0.45f, 1.f) : FLinearColor(0.3f, 0.9f, 1.f, 1.f);

	// Décision : s'il y a PLUSIEURS ennemis alignés devant -> BALAYAGE horizontal du rayon
	// (touche tout le banc) ; sinon rayon FIXE sur la cible unique. Le trait est CONTINU.
	const FVector Fwd = GetActorForwardVector();
	TArray<float> FrontYaws; float MinYaw = 999.f, MaxYaw = -999.f; int32 Front = 0;
	if (UFactionRegistrySubsystem* Reg = W->GetSubsystem<UFactionRegistrySubsystem>())
	{
		const EFactionID Enemy = (GetFaction() == EFactionID::Aquiloris) ? EFactionID::Noxeens : EFactionID::Aquiloris;
		for (AUnitBase* U : Reg->GetUnitsForFaction(Enemy))
		{
			if (!U || !U->IsAlive()) continue;
			FVector D = U->GetActorLocation() - From; D.Z = 0.f;
			if (D.Size() > 1600.f) continue;
			if (FVector::DotProduct(D.GetSafeNormal(), Fwd) < 0.3f) continue; // devant seulement
			const float Y = D.Rotation().Yaw;
			MinYaw = FMath::Min(MinYaw, Y); MaxYaw = FMath::Max(MaxYaw, Y); ++Front;
		}
	}

	if (Front >= 3 && (MaxYaw - MinYaw) > 12.f)
	{
		// BALAYAGE : le rayon balaie de MinYaw à MaxYaw en ~1 s (le bras suit ce mouvement),
		// touchant chaque ennemi traversé (dégâts répartis, un peu moins par cible).
		AWOTOLBeam::Fire(W, From, MinYaw - 6.f, MaxYaw + 6.f, 1600.f, BeamCol, this, 240.f);
		AWOTOLDamageNumber::SpawnText(W, From + FVector(0, 0, 120.f), TEXT("Rayon — Balayage"), BeamCol);
	}
	else
	{
		// RAYON FIXE sur la cible unique (bâtiment ou ennemi le plus proche).
		// Visée 3D COMPLÈTE : le rayon s'incline vers la couche de la cible (Kraken en
		// lévitation au-dessus -> le rayon MONTE vraiment jusqu'à lui, il ne part plus à plat).
		const FVector D3 = To - From;                 // delta RÉEL (From/To = centres des mesh flottants)
		const FRotator Aim = D3.Rotation();           // yaw + pitch
		AWOTOLBeam::Fire(W, From, Aim.Yaw, Aim.Yaw, FMath::Max(600.f, D3.Size() + 100.f), BeamCol, this, 0.f, Aim.Pitch);
		AWOTOLDamageNumber::SpawnText(W, From + FVector(0, 0, 120.f), TEXT("Rayon Laser"), BeamCol);
	}
}

// NOXEBLAST — Rafale : plusieurs projectiles sur l'ennemi le plus proche.
void AWOTOLDemoUnit::Ability_ProjectileBurst()
{
	UWorld* W = GetWorld(); if (!W) return;
	AUnitBase* Foe = FindNearestEnemyUnit(); if (!Foe) return;
	const FVector From = (GetFloatingTextAnchor() ? GetFloatingTextAnchor()->GetComponentLocation()
		: GetActorLocation()) + FVector(0, 0, 40.f);
	const FVector To = (Foe->GetFloatingTextAnchor() ? Foe->GetFloatingTextAnchor()->GetComponentLocation()
		: Foe->GetActorLocation()) + FVector(0, 0, 40.f);
	for (int32 i = 0; i < 5; ++i)
	{
		const FVector Jit(FMath::FRandRange(-40.f, 40.f), FMath::FRandRange(-40.f, 40.f), FMath::FRandRange(-20.f, 40.f));
		AWOTOLProjectileTracer::Fire(W, From, To + Jit, FLinearColor(0.55f, 0.35f, 1.f, 1.f), 1.0f, /*bBolt=*/true);
	}
	Foe->TakeDamageFromUnit(180.f, this);
	AWOTOLDamageNumber::SpawnText(W, From + FVector(0, 0, 110.f), TEXT("Rafale"),
		FLinearColor(0.6f, 0.4f, 1.f, 1.f));
}

// NOXEFLARE — Éblouissement : flash qui aveugle les ennemis proches DEVANT (précision ~0).
void AWOTOLDemoUnit::Ability_BlindFlash()
{
	UWorld* W = GetWorld(); if (!W) return;
	UFactionRegistrySubsystem* Reg = W->GetSubsystem<UFactionRegistrySubsystem>(); if (!Reg) return;
	const EFactionID Enemy = (GetFaction() == EFactionID::Aquiloris) ? EFactionID::Noxeens : EFactionID::Aquiloris;
	const FVector Origin = GetActorLocation();
	const FVector Fwd = GetActorForwardVector();
	const float Radius = 650.f, Now = W->GetTimeSeconds();
	int32 Hit = 0;
	for (AUnitBase* U : Reg->GetUnitsForFaction(Enemy))
	{
		if (!U || !U->IsAlive()) continue;
		FVector To = U->GetActorLocation() - Origin; To.Z = 0.f;
		if (To.Size() > Radius) continue;
		if (FVector::DotProduct(To.GetSafeNormal(), Fwd) < 0.f) continue; // seulement devant
		U->BlindedUntil = Now + 4.f; // précision quasi nulle pendant 4 s
		++Hit;
	}
	// Flash VIOLET (éblouissement bioluminescent des Noxeflare).
	AWOTOLBubbleBurst::Burst(W, Origin + Fwd * 120.f + FVector(0, 0, 60.f), FLinearColor(0.7f, 0.35f, 1.f, 1.f), 20);
	if (Hit > 0)
		AWOTOLDamageNumber::SpawnText(W, Origin + FVector(0, 0, 150.f), TEXT("Eblouissement"),
			FLinearColor(0.72f, 0.4f, 1.f, 1.f));
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
		MakeBone(JRElbow, M_CONE, FVector(0, 0, -H * 0.34f), FVector(0.06f, 0.06f, h * 0.55f), FRotator(180.f, 0, 0), FLinearColor(0.45f, 1.3f, 2.6f, 1.f)); // épée énergie bleue lumineuse
		AddPart(M_CUBE, FVector(-16, 0, H * 0.05f), FVector(0.05f, 0.55f, h * 0.45f), FRotator(8.f, 0, 0), AqArmor); // cape
		AddPart(M_CONE, FVector(0, 0, H * 0.46f), FVector(0.18f, 0.18f, h * 0.12f), NoRot, AqGold);                  // crête or
		return;
	}
	if (Id == TEXT("Aquiloryons")) // Infanterie : ARTICULÉ (épée + bouclier)
	{
		BuildArticulatedAquiloryons(H, AqArmor, AqEnergy);
		return;
	}
	if (Id == TEXT("Aquilances")) // Montée : CAVALIER (avec jambes) sur MONTURE marine + lance
	{
		// ── MONTURE : corps de créature marine effilé (poisson-raie cuirassé) ──
		SetupMainPart(M_SPH, FVector(10, 0, -H * 0.24f),
			FVector(h * 1.5f, h * 0.72f, h * 0.52f), NoRot, AqArmor);                                   // corps
		AddPart(M_SPH, FVector(72, 0, -H * 0.22f), FVector(h * 0.55f, h * 0.5f, h * 0.42f), NoRot, AqArmor); // tête arrondie
		AddPart(M_CONE, FVector(96, 0, -H * 0.24f), FVector(0.34f, 0.34f, h * 0.24f), FRotator(80.f, 0, 0), AqArmor); // museau
		// Yeux bioluminescents de la monture
		AddPart(M_SPH, FVector(80, 26, -H * 0.16f), FVector(0.13f, 0.13f, 0.13f), NoRot, AqEnergy);
		AddPart(M_SPH, FVector(80, -26, -H * 0.16f), FVector(0.13f, 0.13f, 0.13f), NoRot, AqEnergy);
		// Grandes nageoires latérales (ailes de raie) qui ondulent
		RegisterWiggle(AddPart(M_CONE, FVector(6, 62, -H * 0.24f), FVector(0.34f, 0.10f, h * 0.34f), FRotator(0, 0, 88.f), AqArmor), 0.4f);
		RegisterWiggle(AddPart(M_CONE, FVector(6, -62, -H * 0.24f), FVector(0.34f, 0.10f, h * 0.34f), FRotator(0, 0, -88.f), AqArmor), 3.5f);
		// Nageoire dorsale + QUEUE caudale qui bat
		AddPart(M_CONE, FVector(0, 0, -H * 0.02f), FVector(0.10f, 0.30f, h * 0.22f), FRotator(0, 0, 0), AqEnergy);
		RegisterWiggle(AddPart(M_CONE, FVector(-74, 0, -H * 0.22f), FVector(0.42f, 0.10f, h * 0.5f), FRotator(90.f, 0, 0), AqArmor), 0.f);

		// ── CAVALIER : buste + tête + DEUX JAMBES qui enfourchent la monture + bras ──
		const FVector Seat(-8, 0, H * 0.02f); // assise sur le dos de la monture
		AddPart(M_CYL, Seat + FVector(0, 0, H * 0.14f), FVector(0.24f, 0.20f, h * 0.26f), FRotator(6.f, 0, 0), AqGold); // torse cuirassé
		AddPart(M_SPH, Seat + FVector(2, 0, H * 0.32f), FVector(0.22f, 0.22f, 0.22f), NoRot, AqArmor);                  // tête (casque)
		AddPart(M_SPH, Seat + FVector(12, 0, H * 0.33f), FVector(0.10f, 0.16f, 0.10f), NoRot, AqEnergy);               // visière lumineuse
		// Jambes : de part et d'autre du corps de la monture, pliées vers le bas (enfourchement)
		AddPart(M_CYL, Seat + FVector(2, 20, -H * 0.02f), FVector(0.09f, 0.09f, h * 0.30f), FRotator(24.f, 0, 20.f), AqArmor);  // cuisse D
		AddPart(M_CYL, Seat + FVector(2, -20, -H * 0.02f), FVector(0.09f, 0.09f, h * 0.30f), FRotator(24.f, 0, -20.f), AqArmor); // cuisse G
		AddPart(M_CYL, Seat + FVector(18, 26, -H * 0.16f), FVector(0.08f, 0.08f, h * 0.24f), FRotator(60.f, 0, 10.f), AqArmor);  // tibia D
		AddPart(M_CYL, Seat + FVector(18, -26, -H * 0.16f), FVector(0.08f, 0.08f, h * 0.24f), FRotator(60.f, 0, -10.f), AqArmor);// tibia G
		// Bras droit tendu qui tient la LANCE, bras gauche sur les rênes
		AddPart(M_CYL, Seat + FVector(16, 16, H * 0.18f), FVector(0.07f, 0.07f, h * 0.22f), FRotator(70.f, 0, 40.f), AqArmor);  // bras D
		AddPart(M_CYL, Seat + FVector(14, -14, H * 0.14f), FVector(0.07f, 0.07f, h * 0.16f), FRotator(50.f, 0, -30.f), AqArmor);// bras G
		// LANCE longue effilée, pointe énergétique en avant
		AddPart(M_CYL, FVector(55, 22, H * 0.12f), FVector(0.05f, 0.05f, h * 1.05f), FRotator(78.f, 0, 0), AqArmor);
		AddPart(M_CONE, FVector(120, 22, H * 0.10f), FVector(0.09f, 0.09f, h * 0.3f), FRotator(80.f, 0, 0), AqEnergy);          // pointe
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
		BuildKrakenCephalopod(H);
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

	// Le BOSS "Kraken" est une créature NEUTRE : toujours le MÊME céphalopode, quelle
	// que soit la faction rivale (sinon on affronte le mythique adverse — phénix, etc.).
	if (bIsBoss || bCreatureBrain)
	{
		const float KrakH = 6.5f * 100.f; // taille fixe du Kraken (indépendante du mythique)
		BuildKrakenCephalopod(KrakH);
		// ── CONCEPT : le Kraken OCCUPE 2 NIVEAUX de verticalité ──
		// Contrairement aux petites unités qui tiennent sur 1 niveau, le Kraken est si
		// imposant que son modèle compte comme 2 niveaux de hauteur. Sa BASE (le point le
		// plus bas) repose sur son niveau courant (le sol en phase 1) et le corps s'étend
		// vers le haut sur ~2 niveaux. -> pas de lévitation : la base est posée au sol, et
		// le capteur de clic couvre TOUTE la colonne (2 niveaux) pour un ciblage fiable.
		bTwoLayerCreature = true;
		// Le corps est modélisé autour de l'origine (parties basses jusqu'à ~-0.46*H) :
		// on descend juste ce qu'il faut pour que la base touche le fond (annule le petit
		// lift de spawn) sans l'enterrer. [Réglable : -0.10 à peine posé .. -0.30 enfoncé]
		VisualBaseZ = -KrakH * 0.18f;
		// Empreinte de collision ≈ corps visible : les unités mêlée s'arrêtent au bord du
		// modèle (assez près pour être "collées", assez loin pour ne pas entrer dans le bec).
		GetCapsuleComponent()->SetCapsuleSize(KrakH * 0.38f, FMath::Max(40.f, KrakH * 0.5f));
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		if (NameTag)       NameTag->SetRelativeLocation(FVector(0.f, 0.f, KrakH * 0.75f + 50.f));
		if (NameTagShadow) NameTagShadow->SetRelativeLocation(FVector(0.f, 0.f, KrakH * 0.75f + 50.f));
		// Capteur de clic couvrant la COLONNE de 2 niveaux : grand rayon + centré sur la
		// mi-hauteur du corps -> on peut cliquer partout sur le Kraken (haut ou bas).
		if (ClickProxy)
		{
			ClickProxy->SetSphereRadius(KrakH * 0.75f);
			ClickProxy->SetRelativeLocation(FVector(0.f, 0.f, KrakH * 0.25f));
		}
		// NB : pas de bloqueur physique WorldStatic sur le Kraken — il bloquait aussi la
		// capsule (au sol) des unités en HAUTEUR visuelle, qui restaient figées contre lui.
		// La non-pénétration est assurée par la distance d'arrêt mêlée (rayon de capsule
		// ci-dessus) et l'encerclement par emplacements (ComputeEncircleSlot).
		BobSeed = FMath::Fmod(GetActorLocation().X * 0.021f + GetActorLocation().Y * 0.013f, 6.283f);
		return; // pas de disque d'équipe ni de silhouette d'unité pour le boss
	}

	const float HeightU      = GetUnitHeightMeters(UnitID) * 100.f; // mètres -> UE units

	// Couleur d'ÉQUIPE en base (lisibilité RTS) + accent caractéristique de faction.
	const FLinearColor Base   = FFactionColors::Get(GetFaction());
	const FLinearColor Accent = (GetFaction() == EFactionID::Aquiloris)
		? FLinearColor(0.98f, 0.80f, 0.25f, 1.f)   // or/cyan Aquiloris
		: FLinearColor(0.65f, 0.20f, 0.95f, 1.f);  // violet bioluminescent Noxéen

	AssembleSilhouette(UnitID, UnitRole, HeightU, Base, Accent);

	// COLLISION "SECONDE PEAU" : capsule serrée au plus près du gabarit RÉEL de l'unité
	// (largeur ≈ celle du corps, pas 1 m de rab). Ça permet aux unités de s'approcher au
	// CONTACT les unes des autres et du Kraken sans se chevaucher. Rayon = demi-largeur.
	float WidthFactor = 0.28f; // humanoïde svelte par défaut (torse ~0.28×hauteur)
	switch (UnitRole)
	{
		case EUnitRole::Montee:   WidthFactor = 0.42f; break; // monture : un peu plus large
		case EUnitRole::Mythique: WidthFactor = 1.05f; break; // gros mythique (mais resserré)
		case EUnitRole::Chef:     WidthFactor = 0.32f; break;
		default: break;
	}
	const float CapH = FMath::Max(40.f, HeightU * 0.5f);
	const float CapR = FMath::Max(18.f, HeightU * WidthFactor * 0.5f);
	GetCapsuleComponent()->SetCapsuleSize(CapR, CapH);
	// La verticalité est VISUELLE : le corps physique reste au sol. Pour ne pas bloquer
	// une unité montée en hauteur derrière un obstacle au sol, les unités ne se bloquent
	// PLUS entre elles (elles se croisent). Elles bloquent toujours le décor (sol/murs).
	// La sélection/ciblage passe par le ClickProxy (canal Pawn), pas la capsule.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	if (NameTag)       NameTag->SetRelativeLocation(FVector(0.f, 0.f, CapH + 50.f));
	if (NameTagShadow) NameTagShadow->SetRelativeLocation(FVector(0.f, 0.f, CapH + 50.f));
	// Proxy de clic RECENTRÉ à mi-hauteur du corps et dimensionné pour couvrir TOUTE la
	// silhouette (le mesh monte depuis VisualRoot ; une sphère aux pieds ratait le haut du
	// corps -> on cliquait le mesh sans rien sélectionner). Comme il est attaché à VisualRoot,
	// il suit la couche de verticalité : la cible reste cliquable même en lévitation.
	if (ClickProxy)
	{
		ClickProxy->SetSphereRadius(FMath::Max(CapR * 1.15f, HeightU * 0.6f));
		ClickProxy->SetRelativeLocation(FVector(0.f, 0.f, HeightU * 0.35f));
	}

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
	// Attaché à VisualRoot : le disque SUIT la couche verticale (l'unité montée en
	// hauteur emmène son marqueur avec elle -> on peut superposer les unités).
	C->SetupAttachment(VisualRoot ? VisualRoot.Get() : RootComponent.Get());
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
	// Ces humanoïdes étaient construits dos-devant : on retourne tout le visuel de 180°.
	bVisualYawFlip = true;
	if (VisualRoot) VisualRoot->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
}

// Aquiloryons = humanoïde + épée (main droite) + bouclier cristal (main gauche)
void AWOTOLDemoUnit::BuildArticulatedAquiloryons(float H, const FLinearColor& Armor, const FLinearColor& Energy)
{
	const float h = H / 100.f;
	BuildArticulatedHumanoid(H, Armor, 0.34f);
	// Énergie BLEUE survoltée (>1) -> épée + bouclier paraissent LUMINEUX (énergétiques).
	const FLinearColor BlueGlow(0.45f, 1.30f, 2.60f, 1.f);
	// Épée dans la main droite (prolonge l'avant-bras)
	MakeBone(JRElbow, M_CONE, FVector(0, 0, -H * 0.30f), FVector(0.06f, 0.06f, h * 0.42f), FRotator(180.f, 0, 0), BlueGlow);
	// Bouclier au bras gauche (plaque de cristal-énergie lumineuse)
	MakeBone(JLElbow, M_CUBE, FVector(H * 0.10f, 0, -H * 0.10f), FVector(0.07f, 0.42f, h * 0.40f), FRotator::ZeroRotator, BlueGlow);
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
		SwingProgress += Dt * 3.0f;                            // frappe plus VIVE
		if (SwingProgress > 1.f) SwingProgress -= 1.f;
		// Armé LENT (recul) puis ABATTAGE SEC : deux phases nettes pour un coup lisible.
		const float Wind  = FMath::Clamp(SwingProgress / 0.45f, 0.f, 1.f);      // 0..1 wind-up
		const float Strike= FMath::Clamp((SwingProgress - 0.45f) / 0.35f, 0.f, 1.f); // abattage
		const float Sw = (SwingProgress < 0.45f) ? Wind : (1.f - Strike);
		rSho     = FMath::Lerp(75.f, -110.f, 1.f - Sw);       // AMPLE : lève haut puis abat bas
		rEl      = FMath::Lerp(-95.f, 15.f, 1.f - Sw);        // coude armé serré puis déployé
		lShoRoll = -60.f;                                      // bras gauche/bouclier en travers
		lSho     = 20.f; lEl = -70.f;                          // bouclier LEVÉ devant (garde)
		torsoPitch = -8.f - 16.f * (1.f - Sw);                // gros accompagnement du buste
		torsoRoll  = -14.f * (1.f - Sw);                      // épaule qui pivote dans le coup
		rKnee = 30.f; lKnee = 20.f;                            // fente/appui marqué
		rHip = 14.f; lHip = -10.f;
	}
	else if (bMoving)
	{
		const float s = FMath::Sin(AnimPhase);
		rHip =  s * 42.f;  lHip = -s * 42.f;                   // jambes bien alternées (nage)
		rKnee = 15.f + FMath::Max(0.f, -s) * 55.f;             // genou plie en fin de foulée
		lKnee = 15.f + FMath::Max(0.f,  s) * 55.f;
		rSho = -s * 38.f;  lSho =  s * 38.f;                   // bras opposés amples
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

	// MORT : l'unité ne nage plus -> elle COULE lentement vers le fond (couche 0) et s'y
	// immobilise. On force la couche cible au sol et on descend TRÈS doucement.
	if (bDead) DesiredZ = 0.f;
	// Couche verticale VISUELLE : monte/descend vers DesiredZ (lentement si morte = elle coule).
	CurLayer = FMath::FInterpTo(CurLayer, DesiredZ, Dt, bDead ? 0.5f : 2.5f);

	// ── Flottement du conteneur visuel (nage) + inclinaison + couche ──
	if (VisualRoot)
	{
		const float Amp  = bDead ? 0.f : (bMoving ? 2.5f : 5.5f);
		const float Bob  = FMath::Sin(AnimClock * 1.6f + BobSeed) * Amp;
		const float Roll = FMath::Sin(AnimClock * 1.25f + BobSeed) * 2.0f;
		float Pitch = 0.f, Lunge = 0.f;
		// Attaque : à-coup vers l'AVANT (+X) — pour TOUTES les unités (articulées comprises),
		// afin que la frappe se lise clairement vers l'ennemi (le corps se projette devant).
		if (bAttacking)
		{
			const float s = FMath::Sin(AnimClock * 7.f);
			Pitch = -5.f - 4.f * FMath::Abs(s);
			Lunge = 14.f * FMath::Max(0.f, s); // projection nette vers l'avant
		}
		if (bDead) { Pitch = 70.f; }

		const float FlipYaw = bVisualYawFlip ? 180.f : 0.f; // humanoïdes retournés
		VisualRoot->SetRelativeLocation(
			FMath::VInterpTo(VisualRoot->GetRelativeLocation(), FVector(Lunge, 0.f, Bob + CurLayer + VisualBaseZ), Dt, 10.f));
		VisualRoot->SetRelativeRotation(
			FMath::RInterpTo(VisualRoot->GetRelativeRotation(), FRotator(Pitch, FlipYaw, Roll), Dt, 8.f));
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

// ─── KRAKEN : céphalopode géant + 2 fouets — design UNIQUE (indépendant de la faction)
void AWOTOLDemoUnit::BuildKrakenCephalopod(float H)
{
	const FRotator NoRot = FRotator::ZeroRotator;
	const float h = H / 100.f;
	// Palette fidèle à la réf : armure acier bleu-violet sombre, plaques + claires,
	// veines/yeux cyan bioluminescents, dessous de tentacules violet, bec noir.
	const FLinearColor KrakArmor (0.11f, 0.13f, 0.24f, 1.f);
	const FLinearColor KrakPlate (0.17f, 0.15f, 0.28f, 1.f);
	const FLinearColor KrakGlow  (0.22f, 0.85f, 1.00f, 1.f);
	const FLinearColor KrakPurple(0.30f, 0.20f, 0.42f, 1.f);
	const FLinearColor Beak      (0.04f, 0.04f, 0.05f, 1.f);

	// ── MANTEAU (corps arrière) : masse BULBEUSE effilée et COUCHÉE vers l'arrière
	// (comme un vrai calmar), pas un cône pointu dressé vers le haut. ──
	// Grosse poche arrondie inclinée en arrière (le "sac" du céphalopode).
	AddPart(M_SPH, FVector(-H * 0.30f, 0, H * 0.16f), FVector(h * 1.05f, h * 0.62f, h * 0.66f),
		FRotator(-22.f, 0, 0), KrakArmor);
	// Pointe arrière effilée (fuseau) qui prolonge le manteau vers l'arrière.
	AddPart(M_CONE, FVector(-H * 0.62f, 0, H * 0.26f), FVector(h * 0.34f, h * 0.34f, h * 0.6f),
		FRotator(-70.f, 0, 0), KrakArmor);
	// Deux petites nageoires du manteau (comme les ailerons d'un calmar) à l'arrière.
	AddPart(M_CONE, FVector(-H * 0.5f, H * 0.24f, H * 0.22f), FVector(h * 0.28f, h * 0.08f, h * 0.4f), FRotator(0, 20.f, 80.f), KrakPlate);
	AddPart(M_CONE, FVector(-H * 0.5f, -H * 0.24f, H * 0.22f), FVector(h * 0.28f, h * 0.08f, h * 0.4f), FRotator(0, -20.f, -80.f), KrakPlate);
	// Crête dorsale DOUCE (bosses arrondies) le long du dos, plus d'épines dressées.
	for (int32 i = 0; i < 4; ++i)
	{
		const float u = i / 3.f;
		AddPart(M_SPH, FVector(-H * (0.05f + 0.14f * u), 0, H * (0.34f - 0.02f * u)),
			FVector(h * (0.16f - 0.02f * i), h * 0.14f, h * 0.12f), NoRot, KrakPlate);
	}

	// ── GRANDS AILERONS latéraux pointus (silhouette "bat-wing" de la réf) ──
	for (int32 s = -1; s <= 1; s += 2)
	{
		// Aileron plat et effilé, écarté et relevé
		AddPart(M_CONE, FVector(-H * 0.05f, s * H * 0.30f, H * 0.30f),
			FVector(h * 0.62f, h * 0.09f, h * 0.95f), FRotator(-8.f, s * 42.f, s * 58.f), KrakArmor);
		// Nervure lumineuse cyan sur l'aileron
		AddPart(M_CONE, FVector(-H * 0.05f, s * H * 0.32f, H * 0.32f),
			FVector(h * 0.30f, h * 0.03f, h * 0.7f), FRotator(-8.f, s * 42.f, s * 58.f), KrakGlow);
		// Pointe secondaire (bord dentelé)
		AddPart(M_CONE, FVector(-H * 0.02f, s * H * 0.44f, H * 0.10f),
			FVector(0.10f, 0.06f, h * 0.4f), FRotator(10.f, s * 55.f, s * 70.f), KrakPlate);
	}

	// ── TÊTE bulbeuse plaquée ──
	SetupMainPart(M_SPH, FVector(0, 0, H * 0.03f), FVector(h * 0.58f, h * 0.56f, h * 0.52f), NoRot, KrakArmor);
	AddPart(M_SPH, FVector(H * 0.16f, 0, H * 0.14f), FVector(h * 0.40f, h * 0.42f, h * 0.30f), NoRot, KrakPlate); // front plaqué
	AddPart(M_SPH, FVector(H * 0.30f, 0, -H * 0.10f), FVector(h * 0.44f, h * 0.48f, h * 0.34f), NoRot, KrakPurple); // bourrelet des bras
	// Veines cyan sur la tête
	AddPart(M_CONE, FVector(H * 0.10f, H * 0.18f, H * 0.16f), FVector(0.05f, 0.03f, h * 0.28f), FRotator(20.f, 30.f, 20.f), KrakGlow);
	AddPart(M_CONE, FVector(H * 0.10f, -H * 0.18f, H * 0.16f), FVector(0.05f, 0.03f, h * 0.28f), FRotator(20.f, -30.f, -20.f), KrakGlow);

	// ── GROS YEUX cyan + arcade sombre ──
	for (int32 s = -1; s <= 1; s += 2)
	{
		AddPart(M_SPH, FVector(H * 0.30f, s * H * 0.27f, H * 0.06f), FVector(h * 0.15f, h * 0.15f, h * 0.15f), NoRot, KrakGlow);
		AddPart(M_SPH, FVector(H * 0.30f, s * H * 0.27f, H * 0.06f), FVector(h * 0.19f, h * 0.19f, h * 0.10f), NoRot, KrakArmor); // paupière/arcade
	}

	// ── BEC noir (mâchoires haute + basse) au centre-avant ──
	AddPart(M_CONE, FVector(H * 0.44f, 0, -H * 0.10f), FVector(h * 0.13f, h * 0.13f, h * 0.20f), FRotator(55.f, 0, 0), Beak);
	AddPart(M_CONE, FVector(H * 0.44f, 0, -H * 0.24f), FVector(h * 0.11f, h * 0.11f, h * 0.16f), FRotator(125.f, 0, 0), Beak);

	// ── 8 TENTACULES effilés (2 segments = fuseau qui s'affine), éventail avant/bas ──
	for (int32 i = 0; i < 8; ++i)
	{
		const float t   = (i / 7.f) - 0.5f;            // -0.5..0.5
		const float Yaw = t * 165.f;
		const float ph  = (float)i * 0.55f;
		const FVector Root(H * 0.30f, t * H * 0.52f, -H * 0.16f);
		// segment de base (large)
		RegisterWiggle(AddPart(M_CONE, Root, FVector(0.22f, 0.22f, h * 0.42f),
			FRotator(118.f, Yaw, 0), KrakPurple), ph);
		// segment de pointe (fin), un peu plus bas/avant
		const FVector Tip(H * 0.34f, t * H * 0.60f, -H * 0.46f);
		RegisterWiggle(AddPart(M_CONE, Tip, FVector(0.12f, 0.12f, h * 0.40f),
			FRotator(128.f, Yaw, 0), KrakPurple), ph + 0.4f);
	}

	// ── 2 longs FOUETS segmentés à pointe BARBELÉE (les "grands tentacules") ──
	BuildWhipTentacle(FVector(H * 0.40f, H * 0.13f, -H * 0.02f),  1.f, KrakPlate, H);
	BuildWhipTentacle(FVector(H * 0.40f, -H * 0.13f, -H * 0.02f), -1.f, KrakPlate, H);
}

// ─── Fouets du Kraken : tentacule articulé (chaîne de pivots) ────────────────
void AWOTOLDemoUnit::BuildWhipTentacle(const FVector& RootLoc, float SideSign,
	const FLinearColor& Color, float H)
{
	const float h = H / 100.f;
	const int32 Segs = 4;                  // plus long (comme la réf)
	const float SegLen = H * 0.40f;
	const FLinearColor Glow(0.22f, 0.85f, 1.00f, 1.f);
	TArray<TObjectPtr<USceneComponent>>& Chain = (SideSign >= 0.f) ? WhipJointsR : WhipJointsL;

	USceneComponent* Parent = VisualRoot;
	for (int32 i = 0; i < Segs; ++i)
	{
		// 1er pivot à la racine (sur le corps) ; les suivants au bout du segment précédent (+X local).
		const FVector Off = (i == 0) ? RootLoc : FVector(SegLen, 0.f, 0.f);
		USceneComponent* J = MakeJoint(Parent, Off);
		if (!J) break;
		// Segment couché le long de +X (cylindre pivoté), effilé vers la pointe.
		const float w = FMath::Lerp(0.13f, 0.05f, (Segs > 1) ? (float)i / (Segs - 1) : 0.f);
		MakeBone(J, M_CYL, FVector(SegLen * 0.5f, 0.f, 0.f),
			FVector(w, w, SegLen / 100.f), FRotator(90.f, 0.f, 0.f), Color);
		// petite bague lumineuse cyan à chaque jointure (segments armurés)
		MakeBone(J, M_SPH, FVector(0.f, 0.f, 0.f), FVector(w * 1.15f, w * 1.15f, w * 0.5f), FRotator::ZeroRotator, Glow);
		Chain.Add(J);
		Parent = J;
	}
	// POINTE BARBELÉE : lame centrale + 2 crochets latéraux (masse tranchante de la réf).
	if (Parent && Parent != VisualRoot)
	{
		MakeBone(Parent, M_CONE, FVector(SegLen * 0.55f, 0.f, 0.f),
			FVector(0.08f, 0.08f, h * 0.22f), FRotator(90.f, 0.f, 0.f), Color);            // lame
		MakeBone(Parent, M_CONE, FVector(SegLen * 0.45f, 0.f, SegLen * 0.14f),
			FVector(0.05f, 0.05f, h * 0.14f), FRotator(55.f, 0.f, 0.f), Color);            // crochet haut
		MakeBone(Parent, M_CONE, FVector(SegLen * 0.45f, 0.f, -SegLen * 0.14f),
			FVector(0.05f, 0.05f, h * 0.14f), FRotator(125.f, 0.f, 0.f), Color);           // crochet bas
	}
}

void AWOTOLDemoUnit::AnimateWhips(float Dt)
{
	if (WhipJointsL.Num() == 0 && WhipJointsR.Num() == 0) return;

	// Avance le déroulé du coup en cours (fin -> retour au repos)
	if (WhipStrike >= 0.f)
	{
		WhipStrike += Dt * 2.0f;
		if (WhipStrike > 1.f) WhipStrike = -1.f;
	}

	auto AnimateChain = [&](TArray<TObjectPtr<USceneComponent>>& Chain, float PhaseOff)
	{
		for (int32 i = 0; i < Chain.Num(); ++i)
		{
			if (!Chain[i]) continue;
			float Pitch;
			if (WhipStrike >= 0.f)
			{
				// Déroulé PROPAGÉ : chaque segment claque avec un léger retard (effet fouet).
				const float Local = FMath::Clamp(WhipStrike * 1.7f - i * 0.30f, 0.f, 1.f);
				Pitch = FMath::Lerp(60.f, -75.f, Local); // armé vers le haut -> fouette vers l'avant/bas
			}
			else
			{
				// Repos : léger ondoiement + courbure douce.
				Pitch = 16.f + FMath::Sin(AnimClock * 2.f + PhaseOff + i * 0.7f) * 10.f;
			}
			const float Speed = (WhipStrike >= 0.f) ? 20.f : 4.f;
			Chain[i]->SetRelativeRotation(
				FMath::RInterpTo(Chain[i]->GetRelativeRotation(), FRotator(Pitch, 0.f, 0.f), Dt, Speed));
		}
	};
	AnimateChain(WhipJointsR, 0.f);
	AnimateChain(WhipJointsL, 1.5f);
}

void AWOTOLDemoUnit::DoInkJet(AUnitBase* Target)
{
	UWorld* W = GetWorld();
	if (!W || !Target) return;

	// Point d'impact au SOL sous la cible. On TRACE vers le bas pour trouver la vraie
	// surface (sol OU relief central surélevé) -> la flaque se pose DESSUS, jamais cachée
	// sous une bosse du terrain.
	FVector Ground = Target->GetActorLocation();
	{
		const FVector A = Ground + FVector(0, 0, 400.f);
		const FVector Bt = Ground - FVector(0, 0, 2000.f);
		FHitResult Hit;
		FCollisionQueryParams Q; Q.AddIgnoredActor(this); Q.AddIgnoredActor(Target);
		if (W->LineTraceSingleByObjectType(Hit, A, Bt, FCollisionObjectQueryParams(ECC_WorldStatic), Q))
			Ground.Z = Hit.ImpactPoint.Z + 6.f;
		else
			Ground.Z = 6.f;
	}

	// ── VISUEL : jet d'encre depuis la gueule vers le point d'impact + éclaboussure ──
	const FVector Mouth = (GetFloatingTextAnchor() ? GetFloatingTextAnchor()->GetComponentLocation()
		: GetActorLocation()) + GetActorForwardVector() * 120.f + FVector(0, 0, 40.f);
	const FLinearColor InkCol(0.15f, 0.05f, 0.28f, 1.f); // encre violet sombre
	AWOTOLProjectileTracer::Fire(W, Mouth, Ground + FVector(0, 0, 60.f), InkCol, 2.4f);
	AWOTOLBubbleBurst::Burst(W, Ground + FVector(0, 0, 20.f), InkCol, 20);
	AWOTOLDamageNumber::SpawnText(W, GetActorLocation() + FVector(0, 0, 200.f),
		TEXT("Jet d'encre"), InkCol);

	// ── FLAQUE persistante (ralenti + précision réduite pour qui s'y trouve) ──
	FActorSpawnParameters P; P.Owner = this;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AWOTOLInkZone* Zone = W->SpawnActor<AWOTOLInkZone>(
			AWOTOLInkZone::StaticClass(), Ground, FRotator::ZeroRotator, P))
	{
		Zone->Radius   = 400.f;
		Zone->Lifetime = 9.5f;
		// Hauteur MONDE du crachat = sol + couche visuelle du Kraken : le nuage flotte là
		// ~1,5 s (ondulation) puis s'écoule goutte à goutte au sol (géré par la zone).
		Zone->FloatHeight = Ground.Z + FMath::Max(280.f, CurLayer + 260.f);
		Zone->Caster   = this;
	}

	// Aveuglement IMMÉDIAT des unités proches de l'impact (le jet les asperge).
	const float Now = W->GetTimeSeconds();
	if (UFactionRegistrySubsystem* Reg = W->GetSubsystem<UFactionRegistrySubsystem>())
	{
		const EFactionID Enemy = (GetFaction() == EFactionID::Aquiloris)
			? EFactionID::Noxeens : EFactionID::Aquiloris;
		for (AUnitBase* U : Reg->GetUnitsForFaction(Enemy))
		{
			if (!U || !U->IsAlive()) continue;
			if (FVector::DistSquared2D(U->GetActorLocation(), Ground) < 400.f * 400.f)
				U->BlindedUntil = Now + 3.f; // précision réduite quelques secondes
		}
	}
}

void AWOTOLDemoUnit::DoWhipStrike()
{
	WhipStrike = 0.f; // lance l'animation de déroulé

	UWorld* W = GetWorld();
	if (!W) return;
	UFactionRegistrySubsystem* Reg = W->GetSubsystem<UFactionRegistrySubsystem>();
	if (!Reg) return;

	const EFactionID Enemy = (GetFaction() == EFactionID::Aquiloris)
		? EFactionID::Noxeens : EFactionID::Aquiloris;
	const FVector Origin = GetActorLocation();
	const FVector Fwd    = GetActorForwardVector();
	const float   Reach  = 1000.f;

	for (AUnitBase* U : Reg->GetUnitsForFaction(Enemy))
	{
		if (!U || !U->IsAlive()) continue;
		FVector To = U->GetActorLocation() - Origin;
		To.Z = 0.f;
		const float D = To.Size();
		if (D > Reach) continue;
		if (FVector::DotProduct(To.GetSafeNormal(), Fwd) < 0.15f) continue; // seulement ce qui est DEVANT

		// Balaie / repousse les unités (coup de fouet) + dégâts CONSÉQUENTS (colosse)
		const FVector Push = To.GetSafeNormal() * 1300.f + FVector(0.f, 0.f, 400.f);
		U->LaunchCharacter(Push, true, true);
		U->TakeDamageFromUnit(60.f, this); // relevé (42->60) : les Noxéens doivent subir qq pertes
	}
}
