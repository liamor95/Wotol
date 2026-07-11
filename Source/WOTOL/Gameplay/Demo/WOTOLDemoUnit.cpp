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
#include "EngineUtils.h"
#include "WOTOLInkZone.h"
#include "WOTOLBeam.h"
#include "WOTOLGlow.h"
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
	NameTagShadow->SetWorldSize(56.f); // un peu plus gros que le nom = fin liseré noir centré
	NameTagShadow->SetTextRenderColor(FColor(0, 0, 0, 255));
	NameTagShadow->SetText(FText::GetEmpty());

	// Étiquette flottante nom + PV (sœur de l'ombre, positionnée devant chaque frame)
	NameTag = CreateDefaultSubobject<UTextRenderComponent>(TEXT("NameTag"));
	NameTag->SetupAttachment(VisualRoot);
	NameTag->SetRelativeLocation(FVector(0.f, 0.f, 140.f));
	NameTag->SetHorizontalAlignment(EHTA_Center);
	NameTag->SetWorldSize(50.f);
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

		// IA TACTIQUE (décor destructible) : périodiquement, si l'unité n'a pas déjà une
		// cible de couverture, elle cherche une structure à faire tomber sur un groupe
		// d'ennemis derrière. Vaut pour les DEUX armées (choix autonome), sans écraser un
		// ordre EXPLICITE du joueur (TargetCover posé par OrderAttackCover, bCoverTactic=false).
		bool bUnderPlayerOrder = false;
		if (UUnitAIStateComponent* S = FindComponentByClass<UUnitAIStateComponent>())
			bUnderPlayerOrder = S->bFollowingPlayerOrder;
		// Un ordre explicite du joueur reprend la main sur un choix tactique automatique.
		if (bUnderPlayerOrder && bCoverTactic) { TargetCover = nullptr; bCoverTactic = false; }

		CoverTacticTimer -= DeltaSeconds;
		if (!bUnderPlayerOrder && !TargetCover.IsValid() && CoverTacticTimer <= 0.f)
		{
			CoverTacticTimer = FMath::FRandRange(1.5f, 3.5f);
			if (AWOTOLCoverStructure* Tac = FindTacticalCover())
			{
				TargetCover = Tac;
				bCoverTactic = true;
			}
		}
		if (TargetCover.IsValid()) TickAttackCover(DeltaSeconds); // attaque de décor (ordre OU tactique)
	}

	// LISIBILITÉ : séparation douce entre unités d'une même couche (anti-amas illisible
	// autour du Cristalliseur / en mêlée). Ne concerne PAS le boss (créature géante).
	if (!bCreatureBrain && !bIsBoss)
	{
		ApplySoftSeparation(DeltaSeconds);
		// Synergie Aquiloris (lance ↔ bouclier) réévaluée ~3 fois/s (pas chaque frame).
		SynergyTimer -= DeltaSeconds;
		if (SynergyTimer <= 0.f) { SynergyTimer = 0.33f; UpdateAquilorisSynergy(); }

		// AQUILOMBRES : passif « invisible si immobile » (arrière-ligne protégée).
		if (UnitData && UnitData->GetFName() == TEXT("Aquilombres")) UpdateStealth(DeltaSeconds);
		// LÉVIAPHÉNIX : aura d'amplification (réévaluée ~2 fois/s) + passif d'auto-défense
		// (coup de nageoire / coup de queue quand on l'attaque au contact) + anim propre.
		if (UnitData && UnitData->GetFName() == TEXT("Leviaphenix"))
		{
			AuraTimer -= DeltaSeconds;
			if (AuraTimer <= 0.f) { AuraTimer = 0.5f; TickAura(DeltaSeconds); }
			LeviphenixDefenseTick(DeltaSeconds);
			AnimateLeviphenix(DeltaSeconds);
		}
		// DÉCROISSANCE des buffs d'aura reçus : reviennent seuls à la normale hors du rayon
		// (le Léviaphénix les rafraîchit tant que l'allié reste à portée).
		AuraDamageMult    = FMath::FInterpTo(AuraDamageMult, 1.f, DeltaSeconds, 1.5f);
		AuraDefenseMult   = FMath::FInterpTo(AuraDefenseMult, 1.f, DeltaSeconds, 1.5f);
		AuraCooldownRate  = FMath::FInterpTo(AuraCooldownRate, 1.f, DeltaSeconds, 1.5f);
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
		// LANCE : par défaut GARDE PASSIVE (diagonale). Passe en garde AGRESSIVE quand un
		// ennemi est à portée d'engagement/charge OU pendant un coup.
		LanceAggroTarget = (AttackAnimTimer > 0.f) ? 1.f : 0.f;
		// 1) Un ennemi à portée de combat -> on lui FAIT FACE (le coup part devant).
		if (AUnitBase* Foe = FindNearestEnemyUnit())
		{
			FVector To = Foe->GetActorLocation() - GetActorLocation();
			To.Z = 0.f;
			const float Dist = To.Size();
			const float AtkRange = UnitData ? UnitData->Stats.AttackRange * 200.f : 200.f;
			if (Dist > 1.f && Dist < AtkRange + 500.f) { Desired = To.Rotation(); bWant = true; }
			// Lance abaissée (agressive) dès que l'ennemi est en portée d'engagement/charge.
			if (LanceJoint && Dist < AtkRange + 700.f) LanceAggroTarget = 1.f;
			// BOUCLIER : garde PROACTIVE — dès qu'un ennemi est au contact et que l'unité ne
			// frappe pas, elle LÈVE le bouclier (rempart / formation tortue devant les lignes).
			if (bHasShield && AttackAnimTimer <= 0.f && Dist < AtkRange + 350.f && GetVelocity().Size2D() < 40.f)
				ShieldGuardTimer = FMath::Max(ShieldGuardTimer, 0.25f);
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

	// NB : la furtivité d'Aquilombres est « invisible pour l'ENNEMI » seulement — le JOUEUR
	// continue de la voir (silhouette fantôme) ET son nom/PV restent affichés pour la suivre.

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
	// Teintes poussées au MAXIMUM de luminance non-éclairée (le TextRender est un matériau
	// UNLIT : la couleur = émissif direct). Sur l'ambiance sombre du fond, ça donne des
	// lettres/chiffres qui BRILLENT comme les cristaux/yeux (même lecture lumineuse).
	FLinearColor TagColor;
	if (bCreatureBrain || bIsBoss)                 TagColor = FLinearColor(1.00f, 0.55f, 1.00f, 1.f); // violet vif
	else if (GetFaction() == EFactionID::Aquiloris) TagColor = FLinearColor(0.55f, 1.00f, 1.00f, 1.f); // cyan éclatant
	else if (GetFaction() == EFactionID::Noxeens)   TagColor = FLinearColor(0.55f, 1.00f, 0.65f, 1.f); // vert éclatant
	else                                            TagColor = FFactionColors::Get(GetFaction()) * 1.5f;
	NameTag->SetTextRenderColor(TagColor.ToFColor(false)); // false = pas de clamp sRGB -> lettres au max de brillance

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

	// NETTOYAGE HUD : l'unité morte ne sert plus au combat -> on RETIRE son nom et ses PV
	// (plus de surcharge d'infos). Seul le mesh reste, qui coule au fond. Comme le Tick
	// sort en amont pour les unités mortes, l'étiquette ne sera plus jamais ré-affichée.
	if (NameTag)       NameTag->SetVisibility(false);
	if (NameTagShadow) NameTagShadow->SetVisibility(false);
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
		if (U->IsHiddenFromEnemies()) continue; // furtive : invisible pour les hostiles (dont le Kraken)
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

		// CLAQUE DE TENTACULE EN ZONE (boss) : le colosse écrase le sol -> gros dégâts à
		// TOUTES les unités proches de la cible (celles massées autour en MEURENT). C'est ce
		// qui inflige de VRAIES pertes au joueur pendant qu'il abat le Kraken.
		CritCooldown -= DeltaSeconds;
		if (CritCooldown <= 0.f && FMath::FRand() < 0.35f && Nearest->IsAlive())
		{
			CritCooldown = FMath::FRandRange(8.f, 12.f); // modéré (pas de wipe)
			const FVector CritLoc = Nearest->GetActorLocation();
			const float SlamR = 300.f;                    // zone plus serrée
			if (UFactionRegistrySubsystem* Reg = W->GetSubsystem<UFactionRegistrySubsystem>())
			{
				const EFactionID Foe = (GetFaction() == EFactionID::Aquiloris) ? EFactionID::Noxeens : EFactionID::Aquiloris;
				for (AUnitBase* U : Reg->GetUnitsForFaction(Foe))
				{
					if (!U || !U->IsAlive()) continue;
					if (FVector::DistSquared2D(U->GetActorLocation(), CritLoc) > SlamR * SlamR) continue;
					U->TakeDamageFromUnit(170.f, this); // modéré -> quelques pertes, pas un wipe
				}
			}
			if (AWOTOLDamageNumber* N = AWOTOLDamageNumber::SpawnText(W, CritLoc + FVector(0, 0, 90.f),
					TEXT("ÉCRASEMENT !"), FLinearColor(1.f, 0.35f, 0.f, 1.f)))
			{
				N->SetFollow(Nearest->GetFloatingTextAnchor(), FVector(0, 0, 140.f));
			}
			AWOTOLBubbleBurst::Burst(W, CritLoc + FVector(0, 0, 30.f), FLinearColor(1.f, 0.5f, 0.2f, 1.f), 24);
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

// IA TACTIQUE : cherche une structure DESTRUCTIBLE derrière laquelle des ennemis sont
// groupés -> l'abattre la fera tomber SUR eux (elle bascule dans le sens du tir). Retourne
// la meilleure cible, ou nullptr. Utilisé par les DEUX armées quand elles ne sont pas
// micro-gérées par le joueur.
AWOTOLCoverStructure* AWOTOLDemoUnit::FindTacticalCover() const
{
	UWorld* W = GetWorld();
	if (!W || !UnitData) return nullptr;
	UFactionRegistrySubsystem* Reg = W->GetSubsystem<UFactionRegistrySubsystem>();
	if (!Reg) return nullptr;

	const EFactionID EnemyFac = (GetFaction() == EFactionID::Aquiloris)
		? EFactionID::Noxeens : EFactionID::Aquiloris;
	const FVector Me = GetActorLocation();

	AWOTOLCoverStructure* Best = nullptr;
	int32 BestCount = 1; // il faut AU MOINS 2 ennemis derrière pour que ça vaille le coup

	for (TActorIterator<AWOTOLCoverStructure> It(W); It; ++It)
	{
		AWOTOLCoverStructure* Cov = *It;
		if (!Cov || Cov->IsDestroyed() || Cov->IsIndestructible()) continue;

		FVector ToCov = Cov->GetActorLocation() - Me; ToCov.Z = 0.f;
		const float DCov = ToCov.Size();
		if (DCov < 200.f || DCov > 3000.f) continue;         // ni collée ni trop loin
		const FVector FallDir = ToCov.GetSafeNormal();        // sens où la structure tombera

		// Compte les ennemis situés DERRIÈRE la structure, dans le cône de chute, à portée
		// de la longueur qui balaiera le sol.
		int32 Count = 0;
		const float Reach = Cov->GetPillarLen() + 350.f;
		for (AUnitBase* U : Reg->GetUnitsForFaction(EnemyFac))
		{
			if (!U || !U->IsAlive()) continue;
			FVector ToU = U->GetActorLocation() - Cov->GetActorLocation(); ToU.Z = 0.f;
			const float DU = ToU.Size();
			if (DU > Reach) continue;
			if (FVector::DotProduct(ToU.GetSafeNormal(), FallDir) < 0.55f) continue; // bien derrière
			++Count;
		}
		if (Count > BestCount) { BestCount = Count; Best = Cov; }
	}
	return Best;
}

void AWOTOLDemoUnit::TickAttackCover(float Dt)
{
	AWOTOLCoverStructure* Cov = TargetCover.Get();
	if (!Cov || Cov->IsDestroyed()) { TargetCover = nullptr; bCoverTactic = false; return; }
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
	if (Id == TEXT("Aquilombres")) return 16.f; // spéciale : coup qui fait mal -> gros cooldown (pas spammable)
	if (Id == TEXT("Leviaphenix")) return 20.f; // mythique (réserve phase 3)
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

	AbilityCooldown -= Dt * AuraCooldownRate; // une aura (Léviaphénix) accélère la recharge
	if (AbilityCooldown > 0.f) return;

	// LECTURE DU CHAMP DE BATAILLE : on ne lance la compétence QUE si elle est légitime
	// (UseAbility renvoie false s'il n'y a aucune cible/motif réel). Dans ce cas l'unité
	// GARDE sa capacité et retente très vite -> elle ne frappe jamais dans le vide.
	if (UseAbility())
		AbilityCooldown = GetAbilityCooldownFor(UnitData->GetFName()); // lancée -> plein cooldown
	else
		AbilityCooldown = 0.4f; // pas de cible légitime : on garde la capacité, on retente bientôt
}

bool AWOTOLDemoUnit::UseAbility()
{
	const FString Id = UnitData ? UnitData->GetFName().ToString() : FString();
	if (Id == TEXT("Aquis"))          return Ability_Shockwave();
	else if (Id == TEXT("Noxar"))     return Ability_Laser();
	else if (Id == TEXT("Noxeblast")) return Ability_ProjectileBurst();
	else if (Id == TEXT("Noxeflare")) return Ability_BlindFlash();
	else if (Id == TEXT("Aquipheres") || Id == TEXT("Aquispheres")) return Ability_Hydrolaser();
	else if (Id == TEXT("Aquilombres")) return Ability_ShadowStrike();
	else if (Id == TEXT("Leviaphenix")) return Ability_Resonance();
	else if (Id == TEXT("Noxebeast"))   return Ability_Charge();
	// (Aquiloryons/Aquilances/Aquispheres/Noxebeast : leur "compétence" est leur
	//  comportement de formation/charge géré par le cerveau tactique.)
	return false;
}

// AQUIS — Lame Photonique (2 MODES, l'IA choisit selon la lecture du champ de bataille) :
//   • DÉFENSIF (agglutiné par PLUSIEURS ennemis) : frappe le SOL -> onde de choc CIRCULAIRE
//     courte (~2-3 m) qui REPOUSSE tout autour + zone marquée au sol (répit).
//   • ATTAQUE (1 ou peu d'ennemis) : frappe la CIBLE -> onde de choc SUR l'ennemi (expulsion
//     + dégâts sur lui seul).
// Dégâts amplifiés par la JAUGE D'IMPACT (énergie parée accumulée), consommée à l'usage.
// Renvoie false si aucune cible légitime (capacité gardée).
bool AWOTOLDemoUnit::Ability_Shockwave()
{
	UWorld* W = GetWorld(); if (!W) return false;
	UFactionRegistrySubsystem* Reg = W->GetSubsystem<UFactionRegistrySubsystem>(); if (!Reg) return false;
	const EFactionID Enemy = (GetFaction() == EFactionID::Aquiloris) ? EFactionID::Noxeens : EFactionID::Aquiloris;
	const FVector Origin = GetActorLocation();

	// ── LECTURE : combien d'ennemis COLLÉS autour (rayon court ~3 m) + le plus proche. ──
	const float CloseRadius = 320.f;   // ~3 m : zone "agglutiné"
	const float ReachRadius = 420.f;   // portée d'un coup de lame direct
	int32 SurroundCount = 0;
	AUnitBase* Nearest = nullptr; float NearestD2 = TNumericLimits<float>::Max();
	for (AUnitBase* U : Reg->GetUnitsForFaction(Enemy))
	{
		if (!U || !U->IsAlive()) continue;
		FVector To = U->GetActorLocation() - Origin; To.Z = 0.f;
		const float D2 = To.SizeSquared();
		if (D2 < CloseRadius * CloseRadius) ++SurroundCount;
		if (D2 < NearestD2) { NearestD2 = D2; Nearest = U; }
	}
	const float NearestDist = FMath::Sqrt(NearestD2);

	// LÉGITIMITÉ : aucun ennemi à portée exploitable -> on GARDE la capacité.
	if (SurroundCount == 0 && (!Nearest || NearestDist > ReachRadius))
		return false;

	// Dégâts = base + jauge d'impact (énergie parée), plafonnés ; puis on VIDE la jauge.
	const float Damage = 90.f + FMath::Min(ImpactGauge, 600.f);
	ImpactGauge = 0.f;
	const FLinearColor Wave(0.75f, 0.95f, 1.f, 1.f);

	// L'attaque coïncide avec un coup de lame (animation de frappe).
	AttackAnimTimer = 0.55f;

	// ── MODE DÉFENSIF : plusieurs ennemis collés -> ONDE AU SOL, dégagement circulaire. ──
	if (SurroundCount >= 2)
	{
		const float Radius = 300.f; // ~3 m
		for (AUnitBase* U : Reg->GetUnitsForFaction(Enemy))
		{
			if (!U || !U->IsAlive()) continue;
			FVector To = U->GetActorLocation() - Origin; To.Z = 0.f;
			if (To.Size() > Radius) continue;
			// Recul modulé par l'ancrage au sol de la CIBLE (bouclier planté = recule peu).
			const float KB = Cast<AWOTOLDemoUnit>(U) ? Cast<AWOTOLDemoUnit>(U)->GetKnockbackScale() : 1.f;
			U->LaunchCharacter(To.GetSafeNormal() * (1200.f * KB) + FVector(0, 0, 300.f * KB), true, true);
			U->TakeDamageFromUnit(Damage * 0.7f, this); // réparti sur le groupe
		}
		// VISUEL : flash central au sol + FRONT circulaire d'anneaux qui se propage (~3 m).
		AWOTOLBubbleBurst::Burst(W, Origin + FVector(0, 0, 20.f), FLinearColor(1.f, 1.f, 1.f, 1.f), 30);
		const int32 Ring = 16;
		for (int32 i = 0; i < Ring; ++i)
		{
			const float A = 2.f * PI * i / Ring;
			const FVector P = Origin + FVector(FMath::Cos(A), FMath::Sin(A), 0.f) * Radius + FVector(0, 0, 18.f);
			AWOTOLBubbleBurst::Burst(W, P, Wave, 7); // marque circulaire au sol
		}
		AWOTOLDamageNumber::SpawnText(W, Origin + FVector(0, 0, 160.f), TEXT("Onde de Choc"), Wave);
		return true;
	}

	// ── MODE ATTAQUE : peu d'ennemis -> onde de choc CONCENTRÉE sur la cible frappée. ──
	if (Nearest)
	{
		FVector To = Nearest->GetActorLocation() - Origin; To.Z = 0.f;
		const float KB = Cast<AWOTOLDemoUnit>(Nearest) ? Cast<AWOTOLDemoUnit>(Nearest)->GetKnockbackScale() : 1.f;
		Nearest->LaunchCharacter(To.GetSafeNormal() * (1500.f * KB) + FVector(0, 0, 350.f * KB), true, true); // expulse la cible
		Nearest->TakeDamageFromUnit(Damage, this);
		// VISUEL : impact concentré SUR l'ennemi (pas au sol).
		const FVector Hit = (Nearest->GetFloatingTextAnchor() ? Nearest->GetFloatingTextAnchor()->GetComponentLocation()
			: Nearest->GetActorLocation());
		AWOTOLBubbleBurst::Burst(W, Hit + FVector(0, 0, 40.f), FLinearColor(1.f, 1.f, 1.f, 1.f), 22);
		const int32 Ring = 8;
		for (int32 i = 0; i < Ring; ++i)
		{
			const float A = 2.f * PI * i / Ring;
			const FVector P = Hit + FVector(FMath::Cos(A), FMath::Sin(A), 0.f) * 90.f + FVector(0, 0, 40.f);
			AWOTOLBubbleBurst::Burst(W, P, Wave, 4);
		}
		AWOTOLDamageNumber::SpawnText(W, Hit + FVector(0, 0, 120.f), TEXT("Lame Photonique"), Wave);
		return true;
	}
	return false;
}

// NOXAR — Rayon laser : cible l'OBJECTIF (bâtiment adverse) si présent, sinon l'ennemi
// le plus proche. Gros dégâts, ponctuel.
bool AWOTOLDemoUnit::Ability_Laser()
{
	UWorld* W = GetWorld(); if (!W) return false;
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
	if (!bHasTarget) return false; // aucune cible légitime -> on garde la capacité

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
	return true;
}

// NOXEBLAST — Rafale : plusieurs projectiles sur l'ennemi le plus proche.
bool AWOTOLDemoUnit::Ability_ProjectileBurst()
{
	UWorld* W = GetWorld(); if (!W) return false;
	AUnitBase* Foe = FindNearestEnemyUnit(); if (!Foe) return false; // pas de cible -> capacité gardée
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
	return true;
}

// NOXEFLARE — Éblouissement : flash qui aveugle les ennemis proches DEVANT (précision ~0).
bool AWOTOLDemoUnit::Ability_BlindFlash()
{
	UWorld* W = GetWorld(); if (!W) return false;
	UFactionRegistrySubsystem* Reg = W->GetSubsystem<UFactionRegistrySubsystem>(); if (!Reg) return false;
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
	// LÉGITIMITÉ : aucun ennemi devant à portée -> on garde la capacité (pas de flash inutile).
	if (Hit == 0) return false;
	// Flash VIOLET (éblouissement bioluminescent des Noxeflare).
	AWOTOLBubbleBurst::Burst(W, Origin + Fwd * 120.f + FVector(0, 0, 60.f), FLinearColor(0.7f, 0.35f, 1.f, 1.f), 20);
	AWOTOLDamageNumber::SpawnText(W, Origin + FVector(0, 0, 150.f), TEXT("Eblouissement"),
		FLinearColor(0.72f, 0.4f, 1.f, 1.f));
	return true;
}

// AQUISPHÈRES — tir de base : boule HYDROLASER (traînée de bulles) vers l'ennemi le plus
// proche, OU COUP DE CROSSE si l'ennemi est au contact TRÈS rapproché. Cosmétique : les
// dégâts, eux, sont appliqués par le combat de base (PerformAttack).
void AWOTOLDemoUnit::FireHydrolaserOrMelee()
{
	UWorld* W = GetWorld(); if (!W) return;
	AUnitBase* Foe = FindNearestEnemyUnit(); if (!Foe) return;
	const FVector Anchor = GetFloatingTextAnchor() ? GetFloatingTextAnchor()->GetComponentLocation() : GetActorLocation();
	const FVector Muzzle = Anchor + GetActorForwardVector() * 100.f + FVector(0, 0, 30.f);
	const float Dist = FVector::Dist2D(GetActorLocation(), Foe->GetActorLocation());

	if (Dist < 260.f)
	{
		// COUP DE CROSSE (corps-à-corps très rapproché) : pas de projectile, juste l'impact.
		AWOTOLBubbleBurst::Burst(W, Muzzle, FLinearColor(0.7f, 0.9f, 1.f, 1.f), 8);
		return;
	}
	// TIR HYDROLASER : boule cyan avec TRAÎNÉE DE BULLES jusqu'à la cible.
	const FVector To = (Foe->GetFloatingTextAnchor() ? Foe->GetFloatingTextAnchor()->GetComponentLocation()
		: Foe->GetActorLocation()) + FVector(0, 0, 30.f);
	AWOTOLProjectileTracer::Fire(W, Muzzle, To, FLinearColor(0.35f, 0.85f, 1.f, 1.f), 1.1f, /*bBolt=*/false, /*bBubbleTrail=*/true);
}

// AQUISPHÈRES — compétence à 2 axes, choisie par lecture du champ de bataille :
//   • HYDROPOMPE (ennemis groupés) : RAFALE de boules sur une ZONE (AoE).
//   • HYDROSNIPER (cible isolée)   : UNE grosse boule mono-cible, plus de dégâts.
// Renvoie false s'il n'y a aucune cible légitime (capacité gardée).
bool AWOTOLDemoUnit::Ability_Hydrolaser()
{
	UWorld* W = GetWorld(); if (!W) return false;
	UFactionRegistrySubsystem* Reg = W->GetSubsystem<UFactionRegistrySubsystem>(); if (!Reg) return false;
	const EFactionID Enemy = (GetFaction() == EFactionID::Aquiloris) ? EFactionID::Noxeens : EFactionID::Aquiloris;
	AUnitBase* Nearest = FindNearestEnemyUnit(); if (!Nearest) return false;

	const FVector Anchor = GetFloatingTextAnchor() ? GetFloatingTextAnchor()->GetComponentLocation() : GetActorLocation();
	const FVector Muzzle = Anchor + GetActorForwardVector() * 100.f + FVector(0, 0, 30.f);
	const FVector Center = Nearest->GetActorLocation();
	const FLinearColor OrbCol(0.35f, 0.85f, 1.f, 1.f);

	// Combien d'ennemis GROUPÉS autour de la cible -> décide zone vs mono.
	int32 Cluster = 0;
	for (AUnitBase* U : Reg->GetUnitsForFaction(Enemy))
		if (U && U->IsAlive() && FVector::Dist2D(U->GetActorLocation(), Center) < 400.f) ++Cluster;

	AttackAnimTimer = 0.55f; // recul/anim de tir

	if (Cluster >= 3)
	{
		// HYDROPOMPE : rafale de boules réparties sur la zone + dégâts AoE modérés.
		for (int32 i = 0; i < 6; ++i)
		{
			const FVector Jit(FMath::FRandRange(-260.f, 260.f), FMath::FRandRange(-260.f, 260.f), FMath::FRandRange(-20.f, 60.f));
			AWOTOLProjectileTracer::Fire(W, Muzzle, Center + Jit, OrbCol, 0.9f, false, /*bBubbleTrail=*/true);
		}
		for (AUnitBase* U : Reg->GetUnitsForFaction(Enemy))
			if (U && U->IsAlive() && FVector::Dist2D(U->GetActorLocation(), Center) < 350.f)
				U->TakeDamageFromUnit(95.f, this);
		AWOTOLDamageNumber::SpawnText(W, Muzzle + FVector(0, 0, 110.f), TEXT("Hydropompe"), OrbCol);
		return true;
	}

	// HYDROSNIPER : une GROSSE boule mono-cible, plus de dégâts que le tir de base.
	const FVector To = (Nearest->GetFloatingTextAnchor() ? Nearest->GetFloatingTextAnchor()->GetComponentLocation()
		: Center) + FVector(0, 0, 30.f);
	AWOTOLProjectileTracer::Fire(W, Muzzle, To, OrbCol, 1.9f, /*bBolt=*/false, /*bBubbleTrail=*/true);
	Nearest->TakeDamageFromUnit(260.f, this);
	AWOTOLDamageNumber::SpawnText(W, Muzzle + FVector(0, 0, 110.f), TEXT("Hydrosniper"), OrbCol);
	return true;
}

// NOXEBEAST — Fracasse-Fosse : CHARGE frontale destructrice qui REPOUSSE/renverse les
// ennemis devant + les interrompt (annule leur fenêtre de coup) + dégâts. Renvoie false
// s'il n'y a personne devant à charger.
bool AWOTOLDemoUnit::Ability_Charge()
{
	UWorld* W = GetWorld(); if (!W) return false;
	UFactionRegistrySubsystem* Reg = W->GetSubsystem<UFactionRegistrySubsystem>(); if (!Reg) return false;
	const EFactionID Enemy = (GetFaction() == EFactionID::Aquiloris) ? EFactionID::Noxeens : EFactionID::Aquiloris;
	AUnitBase* Foe = FindNearestEnemyUnit(); if (!Foe) return false;
	const FVector C   = GetActorLocation();
	FVector Fwd = Foe->GetActorLocation() - C; Fwd.Z = 0.f; Fwd = Fwd.GetSafeNormal();

	int32 Hit = 0;
	for (AUnitBase* U : Reg->GetUnitsForFaction(Enemy))
	{
		if (!U || !U->IsAlive()) continue;
		FVector D = U->GetActorLocation() - C; D.Z = 0.f;
		if (D.Size() > 480.f) continue;
		if (FVector::DotProduct(D.GetSafeNormal(), Fwd) < 0.25f) continue; // seulement DEVANT
		const float KB = Cast<AWOTOLDemoUnit>(U) ? Cast<AWOTOLDemoUnit>(U)->GetKnockbackScale() : 1.f;
		U->LaunchCharacter(D.GetSafeNormal() * (1300.f * KB) + FVector(0, 0, 300.f * KB), true, true); // renverse
		U->TakeDamageFromUnit(120.f, this);
		if (AWOTOLDemoUnit* DU = Cast<AWOTOLDemoUnit>(U)) DU->AttackAnimTimer = 0.f; // interrompt son coup
		++Hit;
	}
	if (Hit == 0) return false;
	AttackAnimTimer = 0.55f;
	AWOTOLBubbleBurst::Burst(W, C + Fwd * 120.f + FVector(0, 0, 40.f), FLinearColor(0.3f, 1.2f, 0.5f, 1.f), 24);
	AWOTOLDamageNumber::SpawnText(W, C + FVector(0, 0, 150.f), TEXT("Fracasse-Fosse"), FLinearColor(0.35f, 1.3f, 0.55f, 1.f));
	return true;
}

// ══════════ AQUILOMBRES (spéciale furtive — réserve phase 3) ══════════
// PASSIF « invisible si immobile » : dès qu'elle reste IMMOBILE un court instant (ni
// déplacement horizontal, ni changement de couche, ni attaque), elle se DISSIMULE (corps
// estompé, nom masqué, très difficile à toucher). Elle RÉAPPARAÎT dès qu'elle BOUGE (pour
// se défendre / pour changer de hauteur) ou qu'elle ATTAQUE, puis se re-cache si elle
// redevient immobile.
void AWOTOLDemoUnit::UpdateStealth(float Dt)
{
	if (!IsAlive()) { if (bStealthed) { bStealthed = false; SetStealthVisual(false); } return; }
	const float Speed = GetVelocity().Size2D();
	const bool  bAttacking  = (AttackAnimTimer > 0.f);
	const bool  bChangeLayer = FMath::Abs(CurLayer - DesiredZ) > 25.f; // monte/descend d'une couche
	if (Speed < 15.f && !bAttacking && !bChangeLayer)
	{
		StealthTimer += Dt;
		if (StealthTimer > 0.8f && !bStealthed) { bStealthed = true; SetStealthVisual(true); }
	}
	else
	{
		StealthTimer = 0.f;
		if (bStealthed) { bStealthed = false; SetStealthVisual(false); }
	}
}

// Rendu de la furtivité : l'unité prend un aspect FANTÔME (silhouette spectrale bleu pâle)
// -> invisible pour l'ennemi (géré par IsHiddenFromEnemies), mais le JOUEUR la distingue
// encore sur le plateau. Le nom/PV, eux, restent affichés (on ne les touche pas ici).
void AWOTOLDemoUnit::SetStealthVisual(bool bOn)
{
	// Teinte spectrale uniforme (bleu-cyan pâle) qui remplace les couleurs propres quand elle
	// est furtive ; retour aux couleurs de base quand elle réapparaît.
	const FLinearColor Ghost(0.32f, 0.52f, 0.78f, 1.f);
	for (int32 i = 0; i < PartMIDs.Num(); ++i)
	{
		if (!PartMIDs[i]) continue;
		const FLinearColor Base = PartBaseColors.IsValidIndex(i) ? PartBaseColors[i] : FFactionColors::Get(GetFaction());
		PartMIDs[i]->SetVectorParameterValue(TEXT("Color"), bOn ? Ghost : Base);
	}
}

// OMBRES GLISSÉES : au bon moment, l'assassin DISPARAÎT dans un NUAGE DE FUMÉE, se TÉLÉPORTE
// derrière la DERNIÈRE ligne ennemie DE SA COUCHE VERTICALE (ou derrière le boss/mythique),
// porte une frappe critique SURPRISE (crit % très élevé), puis RÉAPPARAÎT instantanément à
// sa place. CONTRAINTE anti-abus : ne cible que sa PROPRE couche verticale + gros cooldown.
bool AWOTOLDemoUnit::Ability_ShadowStrike()
{
	UWorld* W = GetWorld(); if (!W) return false;
	UFactionRegistrySubsystem* Reg = W->GetSubsystem<UFactionRegistrySubsystem>(); if (!Reg) return false;
	const EFactionID Enemy = (GetFaction() == EFactionID::Aquiloris) ? EFactionID::Noxeens : EFactionID::Aquiloris;

	// Cible : d'abord un BOSS/MYTHIQUE s'il existe (téléport dans son dos). Sinon, la ligne
	// ennemie la PLUS AU FOND (la plus éloignée) SUR MA COUCHE VERTICALE uniquement.
	AUnitBase* Target = nullptr;
	for (AUnitBase* U : Reg->GetUnitsForFaction(Enemy))
	{
		AWOTOLDemoUnit* D = Cast<AWOTOLDemoUnit>(U);
		if (U && U->IsAlive() && D && (D->bIsBoss || D->bCreatureBrain)) { Target = U; break; }
	}
	if (!Target)
	{
		float Deepest = -1.f;
		for (AUnitBase* U : Reg->GetUnitsForFaction(Enemy))
		{
			if (!U || !U->IsAlive()) continue;
			const float TheirLayer = Cast<AWOTOLDemoUnit>(U) ? Cast<AWOTOLDemoUnit>(U)->CurLayer : 0.f;
			if (FMath::Abs(CurLayer - TheirLayer) > 200.f) continue; // MÊME couche verticale seulement
			const float Dist = FVector::Dist2D(GetActorLocation(), U->GetActorLocation());
			if (Dist > Deepest) { Deepest = Dist; Target = U; } // la plus au fond
		}
	}
	if (!Target) return false; // aucune cible sur ma couche -> je garde la capacité

	const FVector Start  = GetActorLocation();
	const FVector Behind = Target->GetActorLocation() - Target->GetActorForwardVector() * 170.f;
	const FLinearColor Smoke(0.10f, 0.16f, 0.30f, 1.f); // fumée abyssale bleu-nuit

	// 1) DISPARITION : nuage de fumée à sa position de départ.
	AWOTOLBubbleBurst::Burst(W, Start + FVector(0, 0, 70.f), Smoke, 26);
	// 2) FRAPPE derrière la cible : fumée d'apparition + gros crit surprise (crit % très élevé).
	AWOTOLBubbleBurst::Burst(W, Behind + FVector(0, 0, 60.f), Smoke, 22);
	AWOTOLBubbleBurst::Burst(W, Target->GetActorLocation() + FVector(0, 0, 50.f), FLinearColor(0.25f, 0.5f, 1.3f, 1.f), 18); // éclat cyan (bloom)
	Target->TakeDamageFromUnit(420.f, this); // coup dans le dos, dévastateur (à équilibrer au global)
	AttackAnimTimer = 0.55f;
	// 3) RÉAPPARITION instantanée à sa place : re-fumée à l'origine (elle n'a jamais quitté sa
	// ligne arrière -> reste protégée).
	AWOTOLBubbleBurst::Burst(W, Start + FVector(0, 0, 70.f), Smoke, 14);
	AWOTOLDamageNumber::SpawnText(W, Target->GetActorLocation() + FVector(0, 0, 130.f), TEXT("Ombres Glissees"), FLinearColor(0.35f, 0.55f, 1.3f, 1.f));
	return true;
}

// ══════════ LÉVIAPHÉNIX (mythique — réserve phase 3) ══════════
// AURA passive (Résonance Technologique) : amplifie les alliés proches — +dégâts, +défense
// (moins de dégâts subis), recharges accélérées. Rafraîchie tant qu'ils restent à portée.
void AWOTOLDemoUnit::TickAura(float /*Dt*/)
{
	UWorld* W = GetWorld(); if (!W) return;
	UFactionRegistrySubsystem* Reg = W->GetSubsystem<UFactionRegistrySubsystem>(); if (!Reg) return;
	const FVector C = GetActorLocation();
	const float R2 = 800.f * 800.f;
	for (AUnitBase* U : Reg->GetUnitsForFaction(GetFaction()))
	{
		if (!U || !U->IsAlive()) continue;
		if (FVector::DistSquared(C, U->GetActorLocation()) > R2) continue;
		// Buffs passifs VOLONTAIREMENT MODESTES : ils se CUMULENT (multiplicativement) avec
		// le bonus de territoire/grade + la synergie de faction + la compétence -> on garde
		// de la marge pour ne pas trivialiser la partie (pas de +70% qui la finit en 2 s).
		U->AuraDamageMult  = FMath::Max(U->AuraDamageMult, 1.10f);   // +10% dégâts
		U->AuraDefenseMult = FMath::Min(U->AuraDefenseMult, 0.92f);  // -8% dégâts subis
		if (AWOTOLDemoUnit* D = Cast<AWOTOLDemoUnit>(U)) D->AuraCooldownRate = FMath::Max(D->AuraCooldownRate, 1.20f); // recharges +20%
	}
}

// RÉSONANCE (compétence) : pulse d'amplification — SOIGNE les alliés proches et leur donne
// un GROS buff temporaire (dégâts/défense). Sans allié à portée -> pas déclenchée.
bool AWOTOLDemoUnit::Ability_Resonance()
{
	UWorld* W = GetWorld(); if (!W) return false;
	UFactionRegistrySubsystem* Reg = W->GetSubsystem<UFactionRegistrySubsystem>(); if (!Reg) return false;
	const FVector C = GetActorLocation();
	const float R2 = 900.f * 900.f;
	int32 n = 0;
	for (AUnitBase* U : Reg->GetUnitsForFaction(GetFaction()))
	{
		if (!U || !U->IsAlive()) continue;
		if (FVector::DistSquared(C, U->GetActorLocation()) > R2) continue;
		U->TakeDamageFromUnit(-120.f, this);                       // soin (valeur négative)
		// Buff temporaire renforcé mais RAISONNABLE (cumul-aware) : le gros cooldown (20 s)
		// évite l'empilement permanent -> pic ponctuel, pas un +X% constant.
		U->AuraDamageMult  = FMath::Max(U->AuraDamageMult, 1.25f);
		U->AuraDefenseMult = FMath::Min(U->AuraDefenseMult, 0.82f);
		++n;
	}
	if (n == 0) return false;
	// VFX : onde dorée d'amplification.
	AWOTOLBubbleBurst::Burst(W, C + FVector(0, 0, 90.f), FLinearColor(2.0f, 1.6f, 0.6f, 1.f), 30);
	const int32 Ring = 16;
	for (int32 i = 0; i < Ring; ++i)
	{
		const float a = 2.f * PI * i / Ring;
		AWOTOLBubbleBurst::Burst(W, C + FVector(FMath::Cos(a), FMath::Sin(a), 0.f) * 400.f + FVector(0, 0, 30.f), FLinearColor(1.6f, 1.3f, 0.5f, 1.f), 5);
	}
	AWOTOLDamageNumber::SpawnText(W, C + FVector(0, 0, 220.f), TEXT("Resonance Technologique"), FLinearColor(1.8f, 1.5f, 0.6f, 1.f));
	return true;
}

// PASSIF de DÉFENSE : quand un ennemi arrive au CONTACT, le Léviaphénix se défend seul —
// COUP DE NAGEOIRE (ennemi devant/côté) ou COUP DE QUEUE (ennemi derrière) qui REPOUSSE les
// ennemis proches (brassage de l'eau) + petits dégâts. Sur cooldown (pas en continu).
void AWOTOLDemoUnit::LeviphenixDefenseTick(float Dt)
{
	if (!IsAlive()) return;
	if (LeviDefCooldown > 0.f) LeviDefCooldown -= Dt;
	if (LeviDefTimer   > 0.f) { LeviDefTimer -= Dt; return; } // balayage en cours
	if (LeviDefCooldown > 0.f) return;

	UWorld* W = GetWorld(); if (!W) return;
	UFactionRegistrySubsystem* Reg = W->GetSubsystem<UFactionRegistrySubsystem>(); if (!Reg) return;
	const EFactionID Enemy = (GetFaction() == EFactionID::Aquiloris) ? EFactionID::Noxeens : EFactionID::Aquiloris;
	const FVector C   = GetActorLocation();
	const FVector Fwd = GetActorForwardVector();
	const float Reach = 360.f;

	// Y a-t-il un ennemi AU CONTACT ? Détermine aussi s'il est plutôt DEVANT ou DERRIÈRE.
	bool bAny = false, bBehind = false;
	for (AUnitBase* U : Reg->GetUnitsForFaction(Enemy))
	{
		if (!U || !U->IsAlive()) continue;
		FVector D = U->GetActorLocation() - C; D.Z = 0.f;
		if (D.Size() > Reach) continue;
		bAny = true;
		if (FVector::DotProduct(D.GetSafeNormal(), Fwd) < -0.15f) bBehind = true;
	}
	if (!bAny) return; // personne au contact -> rien à repousser

	// Déclenche le balayage : queue si l'ennemi est derrière, sinon nageoire.
	LeviDefKind    = bBehind ? 1 : 0;
	LeviDefTimer   = 0.6f;
	LeviDefCooldown = FMath::FRandRange(2.4f, 3.4f);
	LeviTailDir    = (FMath::FRand() < 0.5f) ? 1.f : -1.f;

	// REPOUSSE + petits dégâts + brassage de l'eau (bulles) tout autour au contact.
	for (AUnitBase* U : Reg->GetUnitsForFaction(Enemy))
	{
		if (!U || !U->IsAlive()) continue;
		FVector D = U->GetActorLocation() - C; D.Z = 0.f;
		if (D.Size() > Reach + 60.f) continue;
		const float KB = Cast<AWOTOLDemoUnit>(U) ? Cast<AWOTOLDemoUnit>(U)->GetKnockbackScale() : 1.f;
		U->LaunchCharacter(D.GetSafeNormal() * (1000.f * KB) + FVector(0, 0, 250.f * KB), true, true);
		U->TakeDamageFromUnit(40.f, this); // expulse plus qu'il ne blesse (chasser, pas tuer)
		AWOTOLBubbleBurst::Burst(W, U->GetActorLocation() + FVector(0, 0, 40.f), FLinearColor(0.6f, 0.85f, 1.f, 1.f), 6);
	}
	AWOTOLBubbleBurst::Burst(W, C + FVector(0, 0, 60.f), FLinearColor(0.7f, 0.9f, 1.f, 1.f), 14);
}

// Animation propre au Léviaphénix : ondulation permanente des nageoires/queue (mécanique
// des fluides) + BALAYAGE ample lors d'un coup de nageoire / coup de queue défensif.
void AWOTOLDemoUnit::AnimateLeviphenix(float Dt)
{
	// Progression du balayage : 0 -> 1 -> 0 sur la fenêtre (cloche sinusoïdale).
	const float Sw = (LeviDefTimer > 0.f) ? FMath::Sin((1.f - LeviDefTimer / 0.6f) * PI) : 0.f;
	const float FinIdle  = FMath::Sin(AnimClock * 1.5f + BobSeed) * 8.f;   // battement doux
	const float TailIdle = FMath::Sin(AnimClock * 1.2f + BobSeed) * 12.f;  // ondulation de queue

	auto SetJoint = [&](USceneComponent* J, const FRotator& Target)
	{
		if (J) J->SetRelativeRotation(FMath::RInterpTo(J->GetRelativeRotation(), Target, Dt, 14.f));
	};

	// COUP DE NAGEOIRE (LeviDefKind==0) : les deux grandes nageoires se rabattent vers l'avant.
	const float FinBeat = (LeviDefKind == 0) ? Sw * -75.f : 0.f;
	SetJoint(LeviFinR, FRotator(FinBeat,  FinIdle, 0.f));
	SetJoint(LeviFinL, FRotator(FinBeat, -FinIdle, 0.f));
	// COUP DE QUEUE (LeviDefKind==1) : la queue balaie latéralement (yaw) d'un grand angle.
	const float TailYaw = (LeviDefKind == 1) ? (TailIdle + Sw * 80.f * LeviTailDir) : TailIdle;
	SetJoint(LeviTail, FRotator(0.f, TailYaw, 0.f));
}

// AQUIS — Interception des dégâts entrants : la lame photonique PARE une part des dégâts
// (directs OU énergétiques) et BANQUE l'énergie bloquée dans la jauge d'impact. Cette
// énergie est relâchée par l'onde de choc (dégâts proportionnels à ce qui a été bloqué).
float AWOTOLDemoUnit::TakeDamageFromUnit(float Damage, AUnitBase* InstigatorUnit)
{
	if (Damage > 0.f && IsAlive() && UnitData)
	{
		const FName Id = UnitData->GetFName();

		// Texte flottant de MITIGATION (parade/blocage/esquive/couvert) accroché à l'unité,
		// pour que le joueur VOIE ces événements (throttlé pour ne pas surcharger l'écran).
		auto ShowMitig = [&](const TCHAR* Txt, const FLinearColor& Col)
		{
			if (FMath::FRand() > 0.55f) return; // ~55% du temps -> lisible sans spam
			if (UWorld* W = GetWorld())
			{
				USceneComponent* A = GetFloatingTextAnchor();
				const FVector L = (A ? A->GetComponentLocation() : GetActorLocation());
				if (AWOTOLDamageNumber* N = AWOTOLDamageNumber::SpawnText(W, L, Txt, Col))
					N->SetFollow(A, FVector(FMath::FRandRange(-30.f, 30.f), FMath::FRandRange(-30.f, 30.f), 130.f));
			}
		};

		// AQUIS — parade + jauge d'impact (relâchée par l'onde de choc).
		if (Id == TEXT("Aquis"))
		{
			const float Blocked = Damage * 0.35f;
			ImpactGauge = FMath::Min(ImpactGauge + Blocked, 900.f);
			Damage     -= Blocked;
			if (UWorld* W = GetWorld())
			{
				const FVector At = (GetFloatingTextAnchor() ? GetFloatingTextAnchor()->GetComponentLocation()
					: GetActorLocation()) + FVector(0, 0, 60.f);
				AWOTOLBubbleBurst::Burst(W, At, FLinearColor(0.4f, 0.9f, 1.f, 1.f), 4);
			}
			ShowMitig(TEXT("Pare"), FLinearColor(0.4f, 0.95f, 1.6f, 1.f)); // parade de lame (cyan)
		}
		// AQUILOMBRES — INVISIBLE : dissimulée, difficile à toucher (dégâts très réduits) ;
		// être touchée la RÉVÈLE (elle doit se re-cacher en s'immobilisant à nouveau).
		else if (Id == TEXT("Aquilombres") && bStealthed)
		{
			Damage *= 0.30f;
			bStealthed = false;
			SetStealthVisual(false);
			StealthTimer = 0.f;
			ShowMitig(TEXT("Esquive"), FLinearColor(0.35f, 0.6f, 1.3f, 1.f)); // se dérobe dans l'ombre
		}
		// AQUILANCES — PROTÉGÉES par un bouclier devant (synergie lance↔bouclier) : quand
		// l'Aquilance est calée derrière un Aquiloryon, elle encaisse nettement moins.
		else if (Id == TEXT("Aquilances") && bLanceGuarded)
		{
			Damage *= 0.72f; // -28% : couverte par le mur de boucliers
			ShowMitig(TEXT("Couvert"), FLinearColor(0.5f, 1.f, 1.3f, 1.f));
		}
		// AQUILORYONS — BOUCLIER : blocage frontal dont l'efficacité dépend de l'ANCRAGE AU
		// SOL (planté = bloque bien ; en hauteur = peu d'appui) et de la SYNERGIE (mur de
		// boucliers : plus il y a d'Aquiloryons proches, plus le rempart est solide).
		else if (bHasShield && InstigatorUnit)
		{
			FVector ToAtk = InstigatorUnit->GetActorLocation() - GetActorLocation(); ToAtk.Z = 0.f;
			const bool bFront = FVector::DotProduct(ToAtk.GetSafeNormal(), GetActorForwardVector()) > 0.15f;
			if (bFront)
			{
				const float Grounded = GetGroundedFactor();               // 1 au sol .. 0 en hauteur
				const int32 Allies   = CountNearbyShieldAllies(360.f);     // mur de boucliers
				const float Synergy  = FMath::Min(Allies, 3) * 0.06f;      // +0..18% de blocage
				// Blocage : 12% en pleine hauteur -> 45% bien ancré au sol, + synergie (max ~60%).
				const float Block = FMath::Min(FMath::Lerp(0.12f, 0.45f, Grounded) + Synergy, 0.60f);
				Damage *= (1.f - Block);
				ShieldGuardTimer = 0.6f;                                   // pose de garde (bouclier levé)
				if (UWorld* W = GetWorld())
				{
					const FVector At = GetActorLocation() + GetActorForwardVector() * 45.f + FVector(0, 0, 90.f);
					AWOTOLBubbleBurst::Burst(W, At, FLinearColor(0.45f, 1.f, 1.6f, 1.f), 3);
				}
				ShowMitig(TEXT("Bloque"), FLinearColor(0.45f, 1.f, 1.7f, 1.f)); // blocage au bouclier
			}
		}
	}
	return Super::TakeDamageFromUnit(Damage, InstigatorUnit);
}

// Facteur d'ancrage au sol : 1 quand l'unité est au fond (couche 0), tend vers 0 à mesure
// qu'elle s'élève (elle perd son appui). Basé sur la couche visuelle courante.
float AWOTOLDemoUnit::GetGroundedFactor() const
{
	const float Grade = 800.f; // hauteur d'une couche de référence
	return 1.f - FMath::Clamp(CurLayer / Grade, 0.f, 1.f);
}

// Recul subi : bien ancré au sol (bouclier) = recule peu ; en hauteur = projeté plus loin.
float AWOTOLDemoUnit::GetKnockbackScale() const
{
	const float Air = 1.f - GetGroundedFactor(); // 0 au sol .. 1 en hauteur
	if (bHasShield) return FMath::Lerp(0.45f, 1.5f, Air); // planté = encaisse, en l'air = valdingue
	return FMath::Lerp(0.9f, 1.3f, Air);
}

// Compte les Aquiloryons alliés VIVANTS proches (mur de boucliers) -> synergie Aquiloris.
int32 AWOTOLDemoUnit::CountNearbyShieldAllies(float Radius) const
{
	UWorld* W = GetWorld(); if (!W) return 0;
	UFactionRegistrySubsystem* Reg = W->GetSubsystem<UFactionRegistrySubsystem>(); if (!Reg) return 0;
	const FVector MyLoc = GetActorLocation();
	const float R2 = Radius * Radius;
	int32 Count = 0;
	for (AUnitBase* U : Reg->GetUnitsForFaction(GetFaction()))
	{
		if (!U || U == this || !U->IsAlive()) continue;
		AWOTOLDemoUnit* D = Cast<AWOTOLDemoUnit>(U);
		if (!D || !D->bHasShield) continue;
		if (FVector::DistSquared(MyLoc, U->GetActorLocation()) < R2) ++Count;
	}
	return Count;
}

// SYNERGIE AQUILORIS (lance ↔ bouclier) : réévaluée périodiquement. « Devant » = vers
// l'ennemi (l'unité lui fait face en combat). Une Aquilance a une lance DERRIÈRE un
// bouclier -> l'Aquilance est protégée (bLanceGuarded) et le bouclier frappe plus fort
// (SynergyDamageMult). Traduit le « corps commun » / esprit de ruche de la faction.
void AWOTOLDemoUnit::UpdateAquilorisSynergy()
{
	SynergyDamageMult = 1.f;
	bLanceGuarded     = false;
	if (!IsAlive() || !UnitData || GetFaction() != EFactionID::Aquiloris) return;

	const FName Id = UnitData->GetFName();
	const bool bIsLance = (Id == TEXT("Aquilances"));
	if (!bHasShield && !bIsLance) return; // seuls les boucliers et les lances tissent cette synergie

	UWorld* W = GetWorld(); if (!W) return;
	UFactionRegistrySubsystem* Reg = W->GetSubsystem<UFactionRegistrySubsystem>(); if (!Reg) return;
	const FVector Fwd   = GetActorForwardVector(); // vers l'ennemi quand l'unité combat
	const FVector MyLoc = GetActorLocation();

	for (AUnitBase* U : Reg->GetUnitsForFaction(EFactionID::Aquiloris))
	{
		if (!U || U == this || !U->IsAlive() || !U->GetUnitData()) continue;
		AWOTOLDemoUnit* D = Cast<AWOTOLDemoUnit>(U);
		if (!D) continue;
		FVector Rel = U->GetActorLocation() - MyLoc; Rel.Z = 0.f;
		const float Dist = Rel.Size();
		if (Dist > 360.f || Dist < 1.f) continue;
		const float Along = FVector::DotProduct(Rel.GetSafeNormal(), Fwd); // >0 = devant, <0 = derrière

		if (bHasShield && D->GetUnitData()->GetFName() == TEXT("Aquilances") && Along < -0.25f)
		{
			// Une lance me couvre par l'arrière -> mon bouclier attaque plus fort.
			SynergyDamageMult = 1.30f;
		}
		else if (bIsLance && D->HasShield() && Along > 0.25f)
		{
			// Un bouclier me protège par l'avant -> je suis couverte.
			bLanceGuarded = true;
		}
	}
}

// SÉPARATION DOUCE (lisibilité) : écarte gentiment les unités qui se chevauchent sur une
// MÊME couche verticale (~1 m d'espace mini) pour éviter l'amas illisible autour du
// Cristalliseur / en mêlée. Les unités de couches DIFFÉRENTES ne s'écartent pas (la
// verticalité est préservée : une unité montée en hauteur n'est jamais gênée par le sol).
void AWOTOLDemoUnit::ApplySoftSeparation(float Dt)
{
	UWorld* W = GetWorld(); if (!W) return;
	// Uniquement EN BATAILLE (au placement, les formations espacent déjà les unités).
	if (UGameInstance* GI = GetGameInstance())
		if (UDemoFlowSubsystem* D = GI->GetSubsystem<UDemoFlowSubsystem>())
			if (D->GetScreen() != EDemoScreen::Playing) return;

	UFactionRegistrySubsystem* Reg = W->GetSubsystem<UFactionRegistrySubsystem>(); if (!Reg) return;
	const FVector MyLoc = GetActorLocation();
	const float MyR = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleRadius() : 40.f;
	// Écart mini = juste assez pour DISTINGUER les silhouettes (pas d'empilement), mais
	// STRICTEMENT INFÉRIEUR à la portée de mêlée bord-à-bord (~55 uu, voir IsInAttackRange) :
	// sinon la séparation empêche les unités de s'approcher assez pour se FRAPPER (aucun coup
	// de base ne portait). 35 uu -> silhouettes séparées ET contact de combat possible.
	const float Gap = 35.f;

	FVector Push = FVector::ZeroVector;
	const EFactionID Factions[2] = { EFactionID::Aquiloris, EFactionID::Noxeens };
	for (const EFactionID F : Factions)
	{
		for (AUnitBase* U : Reg->GetUnitsForFaction(F))
		{
			if (!U || U == this || !U->IsAlive()) continue;
			if (AWOTOLDemoUnit* Other = Cast<AWOTOLDemoUnit>(U))
			{
				if (Other->bIsBoss || Other->bCreatureBrain) continue; // pas de séparation avec le boss
				// MÊME COUCHE seulement (verticalité préservée).
				if (FMath::Abs(CurLayer - Other->CurLayer) > 180.f) continue;
			}
			FVector D = MyLoc - U->GetActorLocation(); D.Z = 0.f;
			const float Dist = D.Size();
			float OtherR = 40.f;
			if (ACharacter* C = Cast<ACharacter>(U))
				OtherR = C->GetCapsuleComponent() ? C->GetCapsuleComponent()->GetScaledCapsuleRadius() : 40.f;
			const float MinSep = MyR + OtherR + Gap;
			if (Dist > 1.f && Dist < MinSep)
				Push += D.GetSafeNormal() * (MinSep - Dist);
		}
	}
	if (!Push.IsNearlyZero())
	{
		// Nudge DOUX (respecte la collision du décor), plafonné -> pas d'à-coup ni de tremblement.
		const FVector Step = Push.GetClampedToMaxSize(60.f) * FMath::Min(1.f, 6.f * Dt);
		AddActorWorldOffset(Step, true);
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

	// Couleur VIVE (énergie, yeux, épée, cristaux) -> matériau ÉMISSIF (ça brille).
	// Couleur normale (chair, cuirasse, roche) -> matériau MAT rugueux (fini le plastique
	// lisse : la lumière crée du relief/ombrage sur la peau et la carapace).
	const bool bEmissive = (Color.R > 1.2f || Color.G > 1.2f || Color.B > 1.2f);
	// Émissif des unités PLAFONNÉ (~1.15) : les parties vives restent colorées/brillantes
	// mais ne « bloomment » plus en gros pâté lumineux quand l'armée est massée (déploiement).
	const FLinearColor UnitEmissive(FMath::Min(Color.R, 1.15f), FMath::Min(Color.G, 1.15f), FMath::Min(Color.B, 1.15f), 1.f);
	if (UMaterialInstanceDynamic* MID = bEmissive
			? WOTOLGlow::MakeGlow(this, UnitEmissive) : WOTOLGlow::MakeMatte(this, Color))
	{
		C->SetMaterial(0, MID);
		PartMIDs.Add(MID);
		PartBaseColors.Add(Color);
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

	const bool bEmissive = (Color.R > 1.2f || Color.G > 1.2f || Color.B > 1.2f);
	// Émissif des unités PLAFONNÉ (~1.15) : les parties vives restent colorées/brillantes
	// mais ne « bloomment » plus en gros pâté lumineux quand l'armée est massée (déploiement).
	const FLinearColor UnitEmissive(FMath::Min(Color.R, 1.15f), FMath::Min(Color.G, 1.15f), FMath::Min(Color.B, 1.15f), 1.f);
	if (UMaterialInstanceDynamic* MID = bEmissive
			? WOTOLGlow::MakeGlow(this, UnitEmissive) : WOTOLGlow::MakeMatte(this, Color))
	{
		ShapeMesh->SetMaterial(0, MID);
		ShapeMID = MID;
		PartMIDs.Add(MID);
		PartBaseColors.Add(Color);
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

	// ── CHEVALIER AQUILORIS (réf.) : armure bleu+or, crête de pics, YEUX BLEUS devant,
	// pauldrons/gemme dorés, cape flottante derrière. Avant = -X (flip 180°). ──
	const FLinearColor AqEyeGlow(0.35f, 0.75f, 1.80f, 1.f);   // yeux bleus lumineux
	const FLinearColor AqEnergyHi(0.55f, 1.30f, 2.60f, 1.f);  // énergie bleue (épée/bouclier/pointe)
	const FLinearColor AqCrest(0.55f, 0.78f, 1.00f, 1.f);     // crête blanc-bleu
	const FLinearColor AqCape(0.06f, 0.11f, 0.26f, 1.f);      // cape bleu sombre
	auto BuildAquiKnight = [&](float BodyW)
	{
		BuildArticulatedHumanoid(H, AqArmor, BodyW);
		// Yeux bleus lumineux DEVANT (-X)
		AddPart(M_SPH, FVector(-11, 6, H * 0.34f), FVector(0.06f, 0.06f, 0.07f), NoRot, AqEyeGlow);
		AddPart(M_SPH, FVector(-11, -6, H * 0.34f), FVector(0.06f, 0.06f, 0.07f), NoRot, AqEyeGlow);
		// Crête de pics sur le crâne (du front -X vers l'arrière +X), la plus haute au milieu
		for (int32 cc = 0; cc < 5; ++cc)
		{
			const float cx = -H * 0.05f + cc * H * 0.045f;
			const float mid = 1.f - FMath::Abs(cc - 2) * 0.28f;
			AddPart(M_CONE, FVector(cx, 0, H * 0.44f), FVector(0.06f, 0.11f, h * 0.20f * mid), FRotator(-28.f, 0, 0), AqCrest);
		}
		// Pauldrons dorés (épaules) + gemme losange dorée sur le torse (devant)
		AddPart(M_SPH, FVector(0, H * 0.17f, H * 0.22f), FVector(BodyW * 0.5f, BodyW * 0.5f, BodyW * 0.42f), NoRot, AqGold);
		AddPart(M_SPH, FVector(0, -H * 0.17f, H * 0.22f), FVector(BodyW * 0.5f, BodyW * 0.5f, BodyW * 0.42f), NoRot, AqGold);
		AddPart(M_CONE, FVector(-H * 0.14f, 0, H * 0.16f), FVector(0.10f, 0.10f, h * 0.10f), FRotator(-90.f, 0, 0), AqGold);
		// Lisérés dorés verticaux sur le torse (devant)
		AddPart(M_CUBE, FVector(-H * 0.15f, 0, H * 0.02f), FVector(0.02f, 0.05f, h * 0.30f), NoRot, AqGold);
		// Cape sombre flottante DERRIÈRE (+X)
		RegisterWiggle(AddPart(M_CUBE, FVector(H * 0.13f, 0, -H * 0.06f), FVector(0.04f, BodyW * 1.7f, h * 0.55f), FRotator(10.f, 0, 0), AqCape), 0.f);
	};

	const FString Id = UnitID.ToString();

	// ───────────────── AQUILORIS (bleu acier + or + énergie cyan) ─────────────
	if (Id == TEXT("Aquis")) // Chef (réf 4077) : chevalier bleu+or + GRANDE épée d'énergie
	{
		BuildAquiKnight(0.40f);
		// Grande lame d'énergie photonique tenue main droite (garde dorée + longue lame bleue)
		MakeBone(JRElbow, M_CYL, FVector(0, H * 0.02f, -H * 0.18f), FVector(0.06f, 0.06f, h * 0.10f), NoRot, AqGold);        // poignée
		MakeBone(JRElbow, M_CUBE, FVector(0, H * 0.02f, -H * 0.24f), FVector(0.05f, 0.22f, 0.05f), NoRot, AqGold);           // garde
		MakeBone(JRElbow, M_CONE, FVector(0, H * 0.02f, -H * 0.58f), FVector(0.10f, 0.10f, h * 0.62f), FRotator(180.f, 0, 0), AqEnergyHi); // longue lame
		return;
	}
	if (Id == TEXT("Aquiloryons")) // Infanterie (réf 4079) : chevalier bleu+or + épée + BOUCLIER d'énergie
	{
		bHasShield = true; // porte-bouclier : pose de garde + blocage renforcé (cerveau défensif)
		BuildAquiKnight(0.34f);
		// Épée d'énergie (main droite)
		MakeBone(JRElbow, M_CYL, FVector(0, 0, -H * 0.18f), FVector(0.05f, 0.05f, h * 0.09f), NoRot, AqGold);            // poignée
		MakeBone(JRElbow, M_CONE, FVector(0, 0, -H * 0.46f), FVector(0.08f, 0.08f, h * 0.46f), FRotator(180.f, 0, 0), AqEnergyHi); // lame
		// ── BOUCLIER ÉNERGÉTIQUE (bras gauche) fidèle à la réf : grand ÉCU bombé bleu-cyan
		// lumineux (bloom), porté DEVANT l'avant-bras. Disque légèrement bombé + bord renforcé
		// + croix d'énergie centrale + cadre doré. Face plate tournée vers l'AVANT (-X après flip).
		const FLinearColor ShieldGlow(0.55f, 1.35f, 2.70f, 1.f); // énergie cyan HDR (bloom)
		// Corps de l'écu : cylindre TRÈS plat (disque) présenté de face devant l'avant-bras.
		MakeBone(JLElbow, M_CYL, FVector(-H * 0.09f, 0, -H * 0.13f), FVector(h * 0.42f, h * 0.30f, 0.03f), FRotator(0, 0, 90.f), ShieldGlow);
		// Bombé central (dôme) pour le volume + reflet.
		MakeBone(JLElbow, M_SPH, FVector(-H * 0.11f, 0, -H * 0.13f), FVector(0.16f, h * 0.24f, h * 0.30f), NoRot, ShieldGlow);
		// Cadre doré (bord de l'écu) : 4 arêtes fines encadrant le disque.
		MakeBone(JLElbow, M_CUBE, FVector(-H * 0.085f, 0,  h * 20.f - H * 0.13f), FVector(0.02f, h * 0.42f, 0.02f), NoRot, AqGold);
		MakeBone(JLElbow, M_CUBE, FVector(-H * 0.085f, 0, -h * 20.f - H * 0.13f), FVector(0.02f, h * 0.42f, 0.02f), NoRot, AqGold);
		// Croix d'énergie centrale (nervure verticale + horizontale) qui marque l'écu.
		MakeBone(JLElbow, M_CUBE, FVector(-H * 0.10f, 0, -H * 0.13f), FVector(0.02f, 0.03f, h * 0.30f), NoRot, AqGold);
		MakeBone(JLElbow, M_CUBE, FVector(-H * 0.10f, 0, -H * 0.13f), FVector(0.02f, h * 0.26f, 0.03f), NoRot, AqGold);
		return;
	}
	if (Id == TEXT("Aquilances")) // Montée (réf) : CAVALIER bleu+or sur MONTURE baleine/raie + LANCE d'énergie
	{
		// Avant = +X (pas de flip). MONTURE fidèle à la réf : grosse créature marine LISSE
		// (baleine/raie), tête ronde émoussée, corps bulbeux, larges nageoires pectorales
		// plates, dos moucheté d'or, queue qui S'AFFINE puis se termine par une NAGEOIRE
		// CAUDALE (fluke). Nageoires LÉGÈRES (ondulation subtile). Fini l'ancien corps segmenté.
		const FLinearColor MountBlue(0.14f, 0.24f, 0.52f, 1.f);
		const FLinearColor Belly    (0.22f, 0.34f, 0.60f, 1.f);
		const FLinearColor DarkEye  (0.02f, 0.02f, 0.03f, 1.f);
		const FLinearColor LanceEnergy(0.50f, 1.20f, 2.90f, 1.f); // énergie cristalline bleue HDR (bloom)

		// Corps bulbeux + tête ronde émoussée à l'avant (+X) + ventre plus clair.
		SetupMainPart(M_SPH, FVector(0, 0, -H * 0.22f), FVector(h * 0.95f, h * 0.78f, h * 0.60f), NoRot, MountBlue); // corps
		AddPart(M_SPH, FVector(H * 0.55f, 0, -H * 0.24f), FVector(h * 0.62f, h * 0.60f, h * 0.50f), NoRot, MountBlue); // tête ronde
		AddPart(M_SPH, FVector(H * 0.82f, 0, -H * 0.28f), FVector(h * 0.36f, h * 0.42f, h * 0.34f), NoRot, MountBlue); // museau émoussé
		AddPart(M_SPH, FVector(H * 0.15f, 0, -H * 0.34f), FVector(h * 0.70f, h * 0.60f, h * 0.34f), NoRot, Belly);     // ventre clair
		// Un œil sombre de chaque flanc (façon réf : gros œil rond).
		AddPart(M_SPH, FVector(H * 0.70f, H * 0.30f, -H * 0.18f), FVector(0.09f, 0.09f, 0.10f), NoRot, DarkEye);
		AddPart(M_SPH, FVector(H * 0.70f, -H * 0.30f, -H * 0.18f), FVector(0.09f, 0.09f, 0.10f), NoRot, DarkEye);
		// Mouchetures dorées (peau piquetée d'or) réparties sur le dos/flancs.
		for (int32 g = 0; g < 10; ++g)
		{
			const float gx = H * (0.42f - g * 0.09f);
			const float gy = FMath::Sin(g * 1.7f) * H * 0.28f;
			AddPart(M_SPH, FVector(gx, gy, -H * 0.10f + FMath::Cos(g * 1.3f) * H * 0.06f), FVector(0.03f, 0.03f, 0.03f), NoRot, AqGold);
		}
		// Petite CRÊTE dorsale DOUCE (nageoires légères, subtiles, peu mobiles).
		for (int32 d = 0; d < 5; ++d)
			AddPart(M_CONE, FVector(H * (0.25f - d * 0.13f), 0, H * 0.05f), FVector(0.05f, 0.14f, h * 0.12f), FRotator(-8.f, 0, 0), MountBlue);
		// GRANDES NAGEOIRES PECTORALES plates et larges (flippers), balayées vers l'arrière —
		// ondulation TRÈS légère (elles ne battent pas fort).
		RegisterWiggle(AddPart(M_CONE, FVector(H * 0.05f, H * 0.42f, -H * 0.34f), FVector(0.70f, 0.09f, h * 0.5f), FRotator(6.f, 22.f, 80.f), MountBlue), 0.5f);
		RegisterWiggle(AddPart(M_CONE, FVector(H * 0.05f, -H * 0.42f, -H * 0.34f), FVector(0.70f, 0.09f, h * 0.5f), FRotator(6.f, -22.f, -80.f), MountBlue), 3.6f);
		// QUEUE : le corps S'AFFINE vers l'arrière, la queue remonte et se termine par une
		// NAGEOIRE CAUDALE (fluke à 2 lobes larges et plats) — fidèle à la réf.
		RegisterWiggle(AddPart(M_CONE, FVector(-H * 0.55f, 0, -H * 0.16f), FVector(0.34f, 0.30f, h * 0.55f), FRotator(-108.f, 0, 0), MountBlue), 0.f);  // base épaisse
		RegisterWiggle(AddPart(M_CONE, FVector(-H * 0.92f, 0, H * 0.02f), FVector(0.20f, 0.18f, h * 0.45f), FRotator(-125.f, 0, 0), MountBlue), 0.5f);  // s'affine + remonte
		RegisterWiggle(AddPart(M_CONE, FVector(-H * 1.12f, H * 0.14f, H * 0.22f), FVector(0.42f, 0.08f, h * 0.30f), FRotator(-120.f, 30.f, 20.f), MountBlue), 0.7f);  // lobe fluke D
		RegisterWiggle(AddPart(M_CONE, FVector(-H * 1.12f, -H * 0.14f, H * 0.22f), FVector(0.42f, 0.08f, h * 0.30f), FRotator(-120.f, -30.f, -20.f), MountBlue), 0.7f); // lobe fluke G

		// ── CAVALIER : chevalier bleu+or (peau bleue, crête, yeux bleus lumineux) assis sur le dos. ──
		const FVector Seat(H * 0.12f, 0, H * 0.02f);
		AddPart(M_CYL, Seat + FVector(0, 0, H * 0.10f), FVector(0.16f, 0.14f, h * 0.10f), NoRot, AqArmor);              // bassin
		AddPart(M_CYL, Seat + FVector(-2, 0, H * 0.28f), FVector(0.24f, 0.20f, h * 0.28f), FRotator(6.f, 0, 0), AqArmor); // torse cuirassé
		AddPart(M_CONE, Seat + FVector(-H * 0.10f, 0, H * 0.30f), FVector(0.08f, 0.08f, h * 0.06f), FRotator(-90.f, 0, 0), AqGold); // gemme dorée torse
		AddPart(M_CUBE, Seat + FVector(-4, 0, H * 0.34f), FVector(0.05f, 0.30f, h * 0.16f), FRotator(4.f, 0, 0), AqGold); // pauldrons or
		AddPart(M_SPH, Seat + FVector(0, 0, H * 0.50f), FVector(0.17f, 0.17f, 0.19f), NoRot, FLinearColor(0.18f, 0.28f, 0.52f, 1.f)); // tête peau bleue
		AddPart(M_SPH, Seat + FVector(-9, 4, H * 0.51f), FVector(0.04f, 0.04f, 0.045f), NoRot, AqEyeGlow); // yeux bleus lumineux
		AddPart(M_SPH, Seat + FVector(-9, -4, H * 0.51f), FVector(0.04f, 0.04f, 0.045f), NoRot, AqEyeGlow);
		// Crête de nageoires-cheveux rejetée en arrière (+X derrière la tête).
		for (int32 c = 0; c < 5; ++c)
		{
			const float t = (c - 2) / 2.f;
			AddPart(M_CONE, Seat + FVector(6.f + FMath::Abs(t) * 3.f, t * 5.f, H * 0.60f), FVector(0.03f, 0.04f, h * 0.10f), FRotator(50.f, 0, t * 15.f), AqCrest);
		}
		// Jambes qui enfourchent la monture.
		AddPart(M_CYL, Seat + FVector(2, 20, -H * 0.06f), FVector(0.09f, 0.09f, h * 0.26f), FRotator(24.f, 0, 24.f), AqArmor);
		AddPart(M_CYL, Seat + FVector(2, -20, -H * 0.06f), FVector(0.09f, 0.09f, h * 0.26f), FRotator(24.f, 0, -24.f), AqArmor);
		// Bras qui tiennent la lance (avant/arrière).
		AddPart(M_CYL, Seat + FVector(12, 15, H * 0.26f), FVector(0.06f, 0.06f, h * 0.20f), FRotator(65.f, 0, 35.f), FLinearColor(0.18f, 0.28f, 0.52f, 1.f));
		AddPart(M_CYL, Seat + FVector(-4, 13, H * 0.30f), FVector(0.06f, 0.06f, h * 0.18f), FRotator(80.f, 0, 20.f), FLinearColor(0.18f, 0.28f, 0.52f, 1.f));

		// ── LANCE sur une ARTICULATION (LanceJoint) pour l'animer (coup de lance = poussée
		// vers l'avant). Longue hampe sombre, garde dorée, LAME D'ÉNERGIE CRISTALLINE bleue
		// au bout (+X, bloom) = la partie qui frappe ; talon doré (butt) à l'arrière. ──
		LanceJoint = MakeJoint(VisualRoot, Seat + FVector(14, 14, H * 0.22f));
		MakeBone(LanceJoint, M_CYL, FVector(H * 0.35f, 0, 0), FVector(0.045f, 0.045f, h * 0.95f), FRotator(90.f, 0, 0), FLinearColor(0.03f, 0.03f, 0.04f, 1.f)); // hampe noire longue
		MakeBone(LanceJoint, M_SPH, FVector(H * 0.72f, 0, 0), FVector(0.09f, 0.09f, 0.09f), NoRot, AqGold);                                                      // garde ornée
		MakeBone(LanceJoint, M_CONE, FVector(H * 0.72f, H * 0.06f, 0), FVector(0.035f, 0.035f, h * 0.13f), FRotator(0, 0, 90.f), AqGold);                          // ailette garde D
		MakeBone(LanceJoint, M_CONE, FVector(H * 0.72f, -H * 0.06f, 0), FVector(0.035f, 0.035f, h * 0.13f), FRotator(0, 0, -90.f), AqGold);                        // ailette garde G
		MakeBone(LanceJoint, M_SPH, FVector(H * 0.84f, 0, 0), FVector(0.10f, 0.10f, 0.10f), NoRot, LanceEnergy);                                                  // halo à la base de la lame
		MakeBone(LanceJoint, M_CONE, FVector(H * 1.04f, 0, 0), FVector(0.11f, 0.11f, h * 0.55f), FRotator(90.f, 0, 0), LanceEnergy);                              // LAME d'énergie cristalline
		MakeBone(LanceJoint, M_CONE, FVector(-H * 0.30f, 0, 0), FVector(0.05f, 0.05f, h * 0.14f), FRotator(-90.f, 0, 0), AqGold);                                 // talon doré (butt-spike)
		LanceHome = LanceJoint ? LanceJoint->GetRelativeLocation() : FVector::ZeroVector;
		return;
	}
	if (Id == TEXT("Aquipheres") || Id == TEXT("Aquispheres")) // Distance (réf) : chevalier + GROS CANON à 2 mains
	{
		bTwoHandWeapon = true; // tenu à DEUX MAINS (pose de port + de tir)
		BuildAquiKnight(0.32f);
		// ── GROS CANON RECTANGULAIRE tech (réf) : corps bleu-gris massif + liserés/culasse
		// dorés, bouche évasée, ORBE d'énergie tourbillonnante au canon. Tenu à DEUX MAINS
		// devant le corps, pointé vers l'AVANT (-X après le flip). Attaché à la main droite ;
		// le bras gauche vient tenir le fût (pose 2 mains, voir AnimateArticulated). ──
		const FLinearColor CannonBody(0.10f, 0.14f, 0.30f, 1.f); // bleu-gris sombre
		MakeBone(JRElbow, M_CUBE, FVector(-H * 0.22f, 0, -H * 0.12f), FVector(0.16f, 0.15f, h * 0.44f), FRotator(90.f, 0, 0), CannonBody); // corps rectangulaire
		MakeBone(JRElbow, M_CUBE, FVector(-H * 0.06f, 0, -H * 0.10f), FVector(0.14f, 0.17f, h * 0.16f), FRotator(90.f, 0, 0), AqGold);     // culasse dorée
		MakeBone(JRElbow, M_CUBE, FVector(-H * 0.24f, 0, -H * 0.04f), FVector(0.03f, 0.16f, h * 0.34f), FRotator(90.f, 0, 0), AqGold);     // liseré or (dessus)
		MakeBone(JRElbow, M_CONE, FVector(-H * 0.46f, 0, -H * 0.12f), FVector(0.23f, 0.23f, h * 0.16f), FRotator(-90.f, 0, 0), AqGold);    // bouche évasée
		MakeBone(JRElbow, M_SPH,  FVector(-H * 0.56f, 0, -H * 0.12f), FVector(0.24f, 0.24f, 0.24f), NoRot, AqEnergyHi);                    // ORBE tourbillonnante (bloom)
		MakeBone(JRElbow, M_CYL,  FVector(-H * 0.30f, 0, -H * 0.22f), FVector(0.05f, 0.05f, h * 0.10f), NoRot, AqArmor);                   // poignée avant (foregrip main G)
		return;
	}
	if (Id == TEXT("Aquilombres")) // Spéciale (réf) : duelliste Aquiloris élancée, peau bleue,
	{                              // crête de nageoires, armure bleu nuit + liserés or, gemme losange
		// cyan sur le torse, voiles flottantes, lames d'énergie cyan. Avant = -X (humanoïde flip).
		const FLinearColor Skin(0.18f, 0.28f, 0.52f, 1.f);    // peau bleue
		const FLinearColor Armor(0.05f, 0.08f, 0.22f, 1.f);   // armure bleu nuit
		const FLinearColor GemGlow(0.35f, 1.30f, 1.90f, 1.f); // gemme/lames cyan lumineuses
		const FLinearColor EyeGold(1.70f, 1.30f, 0.35f, 1.f); // yeux dorés lumineux
		BuildArticulatedHumanoid(H, Armor, 0.24f);
		// Tête en peau bleue par-dessus (visage devant = -X)
		AddPart(M_SPH, FVector(-2, 0, H * 0.33f), FVector(0.20f, 0.20f, 0.22f), NoRot, Skin);
		AddPart(M_SPH, FVector(-11, 5, H * 0.34f), FVector(0.045f, 0.045f, 0.05f), NoRot, EyeGold);
		AddPart(M_SPH, FVector(-11, -5, H * 0.34f), FVector(0.045f, 0.045f, 0.05f), NoRot, EyeGold);
		// Crête de nageoires-cheveux rejetée vers l'arrière (+X)
		for (int32 cc = 0; cc < 6; ++cc)
		{
			const float t = (cc - 2.5f) / 2.5f;
			AddPart(M_CONE, FVector(H * 0.04f + FMath::Abs(t) * H * 0.02f, t * 8.f, H * 0.42f),
				FVector(0.04f, 0.05f, h * (0.20f - FMath::Abs(t) * 0.05f)), FRotator(52.f, 0, t * 18.f), Skin);
		}
		// Col montant + pauldrons dorés
		AddPart(M_SPH, FVector(0, H * 0.14f, H * 0.22f), FVector(0.13f, 0.13f, 0.11f), NoRot, AqGold);
		AddPart(M_SPH, FVector(0, -H * 0.14f, H * 0.22f), FVector(0.13f, 0.13f, 0.11f), NoRot, AqGold);
		// Gemme losange CYAN lumineuse sur le plastron (devant)
		AddPart(M_CONE, FVector(-H * 0.15f, 0, H * 0.16f), FVector(0.09f, 0.09f, h * 0.08f), FRotator(-90.f, 0, 0), GemGlow);
		AddPart(M_CONE, FVector(-H * 0.15f, 0, H * 0.16f), FVector(0.09f, 0.09f, h * 0.08f), FRotator(90.f, 0, 0), GemGlow);
		// Liserés dorés verticaux (devant)
		AddPart(M_CUBE, FVector(-H * 0.145f, 6, H * 0.0f), FVector(0.015f, 0.02f, h * 0.26f), NoRot, AqGold);
		AddPart(M_CUBE, FVector(-H * 0.145f, -6, H * 0.0f), FVector(0.015f, 0.02f, h * 0.26f), NoRot, AqGold);
		// Jupe/voile fendue : panneaux flottants sur les côtés + derrière (ondulent)
		RegisterWiggle(AddPart(M_CUBE, FVector(H * 0.06f, H * 0.10f, -H * 0.22f), FVector(0.03f, 0.10f, h * 0.44f), FRotator(6.f, 0, 10.f), Armor), 0.f);
		RegisterWiggle(AddPart(M_CUBE, FVector(H * 0.06f, -H * 0.10f, -H * 0.22f), FVector(0.03f, 0.10f, h * 0.44f), FRotator(6.f, 0, -10.f), Armor), 3.14f);
		RegisterWiggle(AddPart(M_CUBE, FVector(H * 0.12f, 0, -H * 0.20f), FVector(0.03f, 0.16f, h * 0.46f), FRotator(10.f, 0, 0), Armor), 1.2f);
		// Lames d'énergie cyan dans chaque main (duelliste furtive)
		MakeBone(JRElbow, M_CONE, FVector(0, 0, -H * 0.24f), FVector(0.04f, 0.04f, h * 0.22f), FRotator(180.f, 0, 0), GemGlow);
		MakeBone(JLElbow, M_CONE, FVector(0, 0, -H * 0.24f), FVector(0.04f, 0.04f, h * 0.22f), FRotator(180.f, 0, 0), GemGlow);
		return;
	}
	if (Id == TEXT("Leviaphenix")) // Mythique Aquiloris (réf) : dragon-phénix marin élancé,
	{                              // écailles bleu nuit, crête + nageoires bleu glacé lumineuses,
		// cœur d'énergie doré sur le torse, longue queue effilée. Avant = +X (pas de flip).
		const FLinearColor ScaleBody = AqArmor;
		const FLinearColor FinGlow(0.45f, 0.95f, 1.75f, 1.f);   // membranes/crête bleu glacé lumineuses
		const FLinearColor CoreGlow(2.00f, 1.55f, 0.55f, 1.f);  // cœur d'énergie doré lumineux
		const FLinearColor EyeGlow(0.35f, 0.75f, 1.80f, 1.f);   // yeux bleus lumineux
		const FLinearColor Beakish(0.30f, 0.38f, 0.55f, 1.f);   // bec/griffes bleu-gris

		// Torse élancé + bas-ventre qui file vers la queue
		SetupMainPart(M_SPH, FVector(0, 0, H * 0.02f), FVector(h * 0.40f, h * 0.32f, h * 0.52f), NoRot, ScaleBody);
		AddPart(M_SPH, FVector(-H * 0.04f, 0, -H * 0.28f), FVector(h * 0.34f, h * 0.28f, h * 0.34f), NoRot, ScaleBody);
		// Cou incurvé + tête draconique haute devant (+X)
		AddPart(M_CYL, FVector(H * 0.06f, 0, H * 0.34f), FVector(h * 0.18f, h * 0.18f, h * 0.20f), FRotator(18.f, 0, 0), ScaleBody);
		AddPart(M_SPH, FVector(H * 0.16f, 0, H * 0.50f), FVector(h * 0.22f, h * 0.20f, h * 0.22f), NoRot, ScaleBody);
		AddPart(M_CONE, FVector(H * 0.30f, 0, H * 0.48f), FVector(0.16f, 0.16f, h * 0.22f), FRotator(78.f, 0, 0), Beakish); // bec
		// Yeux bleus lumineux (devant = +X)
		AddPart(M_SPH, FVector(H * 0.24f, 8, H * 0.53f), FVector(0.06f, 0.06f, 0.07f), NoRot, EyeGlow);
		AddPart(M_SPH, FVector(H * 0.24f, -8, H * 0.53f), FVector(0.06f, 0.06f, 0.07f), NoRot, EyeGlow);
		// CRÊTE de plumes-nageoires bleu glacé en éventail derrière la tête
		for (int32 cc = 0; cc < 7; ++cc)
		{
			const float t = (cc - 3) / 3.f;
			AddPart(M_CONE, FVector(H * 0.02f - FMath::Abs(t) * H * 0.05f, t * 10.f, H * 0.62f),
				FVector(0.05f, 0.10f, h * (0.34f - FMath::Abs(t) * 0.10f)), FRotator(-42.f, 0, t * 22.f), FinGlow);
		}
		// CŒUR D'ÉNERGIE doré lumineux sur le torse (devant)
		AddPart(M_SPH, FVector(H * 0.20f, 0, H * 0.10f), FVector(0.16f, 0.16f, 0.16f), NoRot, CoreGlow);
		// Bras/nageoires avant élancés + griffes
		for (int32 s = -1; s <= 1; s += 2)
		{
			AddPart(M_CYL, FVector(H * 0.05f, s * H * 0.16f, H * 0.06f), FVector(0.07f, 0.07f, h * 0.22f), FRotator(40.f, 0, s * 20.f), ScaleBody);
			AddPart(M_CONE, FVector(H * 0.14f, s * H * 0.22f, -H * 0.10f), FVector(0.05f, 0.05f, h * 0.14f), FRotator(120.f, 0, s * 20.f), Beakish);
		}
		// GRANDES NAGEOIRES PECTORALES sur ARTICULATIONS (servent au PASSIF : coup de nageoire).
		// Membrane large + fronde secondaire, portées par un pivot animable (balayage).
		LeviFinR = MakeJoint(VisualRoot, FVector(-H * 0.02f, H * 0.16f, H * 0.06f));
		MakeBone(LeviFinR, M_CONE, FVector(0, H * 0.20f, 0), FVector(0.40f, 0.13f, h * 0.55f), FRotator(0, 0, 88.f), FinGlow);
		MakeBone(LeviFinR, M_CONE, FVector(-H * 0.10f, H * 0.16f, -H * 0.06f), FVector(0.28f, 0.10f, h * 0.38f), FRotator(0, 0, 72.f), FinGlow);
		LeviFinL = MakeJoint(VisualRoot, FVector(-H * 0.02f, -H * 0.16f, H * 0.06f));
		MakeBone(LeviFinL, M_CONE, FVector(0, -H * 0.20f, 0), FVector(0.40f, 0.13f, h * 0.55f), FRotator(0, 0, -88.f), FinGlow);
		MakeBone(LeviFinL, M_CONE, FVector(-H * 0.10f, -H * 0.16f, -H * 0.06f), FVector(0.28f, 0.10f, h * 0.38f), FRotator(0, 0, -72.f), FinGlow);
		// LONGUE QUEUE en S sur ARTICULATION (sert au PASSIF : coup de queue) + fluke lumineux.
		LeviTail = MakeJoint(VisualRoot, FVector(-H * 0.06f, 0, -H * 0.30f));
		MakeBone(LeviTail, M_CONE, FVector(-H * 0.04f, 0, -H * 0.22f), FVector(0.22f, 0.18f, h * 0.42f), FRotator(-100.f, 0, 0), ScaleBody);
		MakeBone(LeviTail, M_CONE, FVector(H * 0.04f, 0, -H * 0.52f), FVector(0.15f, 0.12f, h * 0.38f), FRotator(-70.f, 0, 0), ScaleBody);
		MakeBone(LeviTail, M_CONE, FVector(H * 0.16f, 0, -H * 0.72f), FVector(0.10f, 0.08f, h * 0.30f), FRotator(-50.f, 0, 0), ScaleBody);
		MakeBone(LeviTail, M_CUBE, FVector(H * 0.26f, 0, -H * 0.86f), FVector(0.04f, 0.40f, h * 0.24f), FRotator(0, 25.f, 0), FinGlow);
		MakeBone(LeviTail, M_CUBE, FVector(H * 0.26f, 0, -H * 0.90f), FVector(0.04f, 0.40f, h * 0.24f), FRotator(0, -25.f, 0), FinGlow);

		// ── INDICATEUR D'AURA : anneau lumineux au sol matérialisant le RAYON DE SOUTIEN
		// (~800 uu). Le joueur voit clairement où placer ses unités pour bénéficier du buff. ──
		const FLinearColor AuraRing(1.30f, 1.00f, 0.40f, 1.f); // or lumineux (émissif faible)
		const float AuraR = 800.f;
		for (int32 a = 0; a < 28; ++a)
		{
			const float ang = 2.f * PI * a / 28.f;
			AddPart(M_SPH, FVector(FMath::Cos(ang) * AuraR, FMath::Sin(ang) * AuraR, -H * 0.55f),
				FVector(0.22f, 0.22f, 0.22f), NoRot, AuraRing);
		}
		return;
	}

	// ───────────────── NOXÉENS (corps sombre + lumens) ─────────────────
	// NB : identité Noxéenne = VERT/abyssal bioluminescent (distinct du Kraken violet).
	if (Id == TEXT("Noxar")) // Chef : humanoïde sombre à VEINES D'ÉNERGIE BLEUES + 2 tentacules (réf 124)
	{
		const FLinearColor BlueGlow(0.30f, 0.85f, 1.80f, 1.f); // énergie bleue LUMINEUSE (émissif)
		BuildArticulatedHumanoid(H, NoxDark, 0.40f);           // chef : carrure plus large
		// AVANT du corps = -X (après le flip 180°). Visage/veines DEVANT, tentacules DERRIÈRE.
		// Deux yeux bleus lumineux (sur le VISAGE, devant)
		AddPart(M_SPH, FVector(-12, 7, H * 0.33f), FVector(0.08f, 0.08f, 0.09f), NoRot, BlueGlow);
		AddPart(M_SPH, FVector(-12, -7, H * 0.33f), FVector(0.08f, 0.08f, 0.09f), NoRot, BlueGlow);
		// VEINES D'ÉNERGIE bleues sur le TORSE (devant = -X)
		AddPart(M_CONE, FVector(-H * 0.18f, 0, H * 0.12f), FVector(0.05f, 0.05f, h * 0.22f), NoRot, BlueGlow);
		for (int32 v = 0; v < 6; ++v)
		{
			const float a = -1.5f + v * 0.6f;
			AddPart(M_CONE, FVector(-H * 0.19f, FMath::Sin(a) * 16.f, H * (0.02f + 0.04f * v)),
				FVector(0.035f, 0.035f, h * 0.12f), FRotator(0, 0, FMath::RadiansToDegrees(a)), BlueGlow);
		}
		// Avant-bras hérissés de pics
		for (int32 side = -1; side <= 1; side += 2)
			for (int32 k = 0; k < 3; ++k)
				MakeBone(side < 0 ? JLElbow : JRElbow, M_CONE, FVector(0.05f, side * 4.f, -H * (0.04f + k * 0.05f)),
					FVector(0.045f, 0.045f, h * 0.11f), FRotator(0, 0, side * 60.f), NoxDark);
		// 2 longues TENTACULES bleues dans le DOS (+X = derrière après le flip), qui ondulent
		RegisterWiggle(AddPart(M_CONE, FVector(16, 18, H * 0.32f), FVector(0.055f, 0.055f, h * 1.0f), FRotator(42.f, 0, 42.f), BlueGlow), 0.f);
		RegisterWiggle(AddPart(M_CONE, FVector(16, -18, H * 0.32f), FVector(0.055f, 0.055f, h * 1.0f), FRotator(42.f, 0, -42.f), BlueGlow), 3.14f);
		return;
	}
	if (Id == TEXT("Noxeflare")) // Infanterie (réf 4082) : humanoïde VIOLET, AMAS D'YEUX + couronne de cornes
	{
		const FLinearColor Violet(0.10f, 0.05f, 0.16f, 1.f);   // corps violet sombre
		const FLinearColor VioGlow(0.80f, 0.30f, 1.75f, 1.f);  // yeux/taches violets LUMINEUX
		BuildArticulatedHumanoid(H, Violet, 0.34f);
		// AMAS D'YEUX violets sur le VISAGE (-X = devant) : plusieurs petits yeux groupés
		for (int32 e = 0; e < 8; ++e)
		{
			const float ey = FMath::Sin(e * 2.0f) * 9.f;
			const float ez = H * 0.33f + FMath::Cos(e * 1.7f) * H * 0.05f;
			AddPart(M_SPH, FVector(-11, ey, ez), FVector(0.045f, 0.045f, 0.05f), NoRot, VioGlow);
		}
		// COURONNE DE CORNES recourbées autour de la tête (crâne)
		for (int32 c = 0; c < 7; ++c)
		{
			const float a = -1.4f + c * 0.47f;
			AddPart(M_CONE, FVector(FMath::Cos(a) * 4.f, FMath::Sin(a) * 16.f, H * 0.44f),
				FVector(0.05f, 0.05f, h * 0.20f), FRotator(-35.f, 0, FMath::RadiansToDegrees(a) * 0.5f), Violet);
		}
		// TACHES violettes lumineuses sur le torse (devant)
		for (int32 t = 0; t < 5; ++t)
			AddPart(M_SPH, FVector(-H * 0.17f, FMath::Sin(t * 1.3f) * 14.f, H * (0.14f - t * 0.05f)), FVector(0.05f, 0.05f, 0.05f), NoRot, VioGlow);
		// Avant-bras à pics + longues griffes
		for (int32 side = -1; side <= 1; side += 2)
		{
			for (int32 k = 0; k < 3; ++k)
				MakeBone(side < 0 ? JLElbow : JRElbow, M_CONE, FVector(0.05f, side * 4.f, -H * (0.04f + k * 0.05f)),
					FVector(0.045f, 0.045f, h * 0.11f), FRotator(0, 0, side * 60.f), Violet);
			MakeBone(side < 0 ? JLElbow : JRElbow, M_CONE, FVector(-0.04f, side * 3.f, -H * 0.24f), FVector(0.04f, 0.04f, h * 0.16f), FRotator(150.f, 0, 0), Violet); // longue griffe
		}
		// Quelques tentacules dans le DOS (+X)
		for (int32 i = 0; i < 4; ++i)
		{
			const float Side = (i % 2 == 0) ? 1.f : -1.f;
			RegisterWiggle(AddPart(M_CONE, FVector(14, Side * 18.f, H * (0.30f - (i / 2) * 0.12f)),
				FVector(0.045f, 0.045f, h * 0.7f), FRotator(30.f, 0, Side * 45.f), Violet), i * 1.1f);
		}
		return;
	}
	if (Id == TEXT("Noxeblast")) // Distance : humanoïde VIOLET à taches bioluminescentes + PLUSIEURS tentacules (réf 123)
	{
		const FLinearColor Violet(0.10f, 0.05f, 0.16f, 1.f);   // corps violet sombre
		const FLinearColor VioGlow(0.75f, 0.30f, 1.70f, 1.f);  // taches/énergie violettes LUMINEUSES
		const FLinearColor BlueEye(0.35f, 0.75f, 1.80f, 1.f);  // yeux bleus lumineux
		BuildArticulatedHumanoid(H, Violet, 0.36f);
		// AVANT = -X (après flip). Visage/taches DEVANT, tentacules DERRIÈRE (+X).
		// Deux grands yeux bleus lumineux (sur le visage)
		AddPart(M_SPH, FVector(-12, 8, H * 0.33f), FVector(0.09f, 0.09f, 0.10f), NoRot, BlueEye);
		AddPart(M_SPH, FVector(-12, -8, H * 0.33f), FVector(0.09f, 0.09f, 0.10f), NoRot, BlueEye);
		// TACHES bioluminescentes violettes sur le TORSE (devant = -X)
		for (int32 t = 0; t < 6; ++t)
		{
			const float a = t * 1.05f;
			AddPart(M_SPH, FVector(-H * 0.17f, FMath::Sin(a) * 16.f, H * (0.16f - t * 0.045f)),
				FVector(0.05f, 0.05f, 0.05f), NoRot, VioGlow);
		}
		// Avant-bras hérissés de pics + griffes
		for (int32 side = -1; side <= 1; side += 2)
			for (int32 k = 0; k < 3; ++k)
				MakeBone(side < 0 ? JLElbow : JRElbow, M_CONE, FVector(0.05f, side * 4.f, -H * (0.04f + k * 0.05f)),
					FVector(0.045f, 0.045f, h * 0.11f), FRotator(0, 0, side * 60.f), Violet);
		// PLUSIEURS tentacules (6) qui rayonnent du DOS (+X = derrière), longues et ondulantes
		for (int32 i = 0; i < 6; ++i)
		{
			const float Side = (i % 2 == 0) ? 1.f : -1.f;
			const float Up   = 0.34f - (i / 2) * 0.12f;
			const float Spread = 35.f + (i / 2) * 18.f;
			RegisterWiggle(AddPart(M_CONE, FVector(14, Side * 20.f, H * Up),
				FVector(0.05f, 0.05f, h * (0.95f - (i / 2) * 0.12f)), FRotator(30.f, 0, Side * Spread), VioGlow), i * 1.0f);
		}
		return;
	}
	if (Id == TEXT("Noxebeast")) // Montée : QUADRUPÈDE cuirassé façon réf (dos hérissé, défenses, griffes)
	{
		const FLinearColor Scale2(0.14f, 0.12f, 0.09f, 1.f);       // écailles bronze un peu plus claires
		const FLinearColor EyeGlow(0.35f, 1.60f, 0.55f, 1.f);      // yeux verts LUMINEUX (émissif)
		const FLinearColor TuskC(0.55f, 0.42f, 0.20f, 1.f);

		// ── CORPS massif, épaules HAUTES à l'avant qui redescendent vers l'arrière (posture voûtée) ──
		SetupMainPart(M_SPH, FVector(H * 0.05f, 0, -H * 0.06f), FVector(h * 1.05f, h * 0.95f, h * 0.78f), NoRot, NoxBronze); // poitrail bombé
		AddPart(M_SPH, FVector(-H * 0.55f, 0, -H * 0.18f), FVector(h * 0.85f, h * 0.80f, h * 0.55f), NoRot, NoxBronze);      // croupe plus basse
		AddPart(M_CYL, FVector(-H * 0.28f, 0, -H * 0.14f), FVector(h * 0.80f, h * 0.80f, h * 0.9f), FRotator(90.f, 0, 0), NoxBronze); // tronc

		// ── PLAQUES D'ÉCAILLES sur le dos et les flancs (relief cuirassé) ──
		for (int32 p = 0; p < 6; ++p)
		{
			const float px = H * (0.35f - p * 0.16f);
			AddPart(M_CUBE, FVector(px, 0, H * 0.30f - p * 1.f), FVector(0.16f, 0.42f, 0.05f), FRotator(0, 0, 0), Scale2);      // dalle dorsale
			AddPart(M_CUBE, FVector(px, H * 0.30f, -H * 0.10f), FVector(0.12f, 0.10f, 0.06f), FRotator(0, 0, 20.f), Scale2);   // écaille flanc D
			AddPart(M_CUBE, FVector(px, -H * 0.30f, -H * 0.10f), FVector(0.12f, 0.10f, 0.06f), FRotator(0, 0, -20.f), Scale2); // écaille flanc G
		}

		// ── RANGÉE DE PICS DORSAUX (de la nuque à la queue), taille décroissante ──
		for (int32 s = 0; s < 7; ++s)
		{
			const float sx = H * (0.30f - s * 0.16f);
			const float sh = h * (0.34f - s * 0.03f);
			AddPart(M_CONE, FVector(sx, 0, H * 0.34f), FVector(0.10f, 0.10f, sh), FRotator(-18.f, 0, 0), Scale2);
		}
		// PICS D'ÉPAULES (deux gros bouquets à l'avant, comme la réf)
		for (int32 side = -1; side <= 1; side += 2)
		{
			for (int32 k = 0; k < 3; ++k)
				AddPart(M_CONE, FVector(H * (0.10f + k * 0.05f), side * H * 0.34f, H * (0.05f + k * 0.06f)),
					FVector(0.09f, 0.09f, h * (0.34f - k * 0.05f)), FRotator(0, 0, side * (55.f - k * 12.f)), Scale2);
		}

		// ── TÊTE basse + gueule + défenses recourbées + yeux verts lumineux ──
		AddPart(M_SPH, FVector(H * 0.62f, 0, -H * 0.06f), FVector(h * 0.50f, h * 0.52f, h * 0.44f), NoRot, NoxBronze);   // crâne
		AddPart(M_CONE, FVector(H * 0.86f, 0, -H * 0.14f), FVector(h * 0.34f, h * 0.30f, h * 0.30f), FRotator(78.f, 0, 0), NoxBronze); // museau
		AddPart(M_SPH, FVector(H * 0.74f, 15, H * 0.06f), FVector(0.10f, 0.10f, 0.10f), NoRot, EyeGlow);                 // œil vert G
		AddPart(M_SPH, FVector(H * 0.74f, -15, H * 0.06f), FVector(0.10f, 0.10f, 0.10f), NoRot, EyeGlow);                // œil vert D
		AddPart(M_CONE, FVector(H * 0.70f, 12, H * 0.16f), FVector(0.06f, 0.06f, h * 0.14f), FRotator(0, 0, 40.f), Scale2);  // corne sourcil
		AddPart(M_CONE, FVector(H * 0.70f, -12, H * 0.16f), FVector(0.06f, 0.06f, h * 0.14f), FRotator(0, 0, -40.f), Scale2);
		// deux GRANDES défenses qui remontent (recourbées)
		AddPart(M_CONE, FVector(H * 0.80f, 20, -H * 0.24f), FVector(0.10f, 0.10f, h * 0.40f), FRotator(150.f, 0, 12.f), TuskC);
		AddPart(M_CONE, FVector(H * 0.80f, -20, -H * 0.24f), FVector(0.10f, 0.10f, h * 0.40f), FRotator(150.f, 0, -12.f), TuskC);

		// ── 4 PATTES ÉPAISSES SEGMENTÉES (cuisse + tibia + pied à 3 griffes), animées ──
		const float LegX = H * 0.40f, LegY = H * 0.36f;
		auto BuildLeg = [&](USceneComponent* Joint)
		{
			MakeBone(Joint, M_CYL, FVector(0, 0, -H * 0.12f), FVector(0.22f, 0.22f, h * 0.24f), NoRot, NoxBronze);     // cuisse épaisse
			MakeBone(Joint, M_CYL, FVector(H * 0.02f, 0, -H * 0.30f), FVector(0.17f, 0.17f, h * 0.22f), NoRot, NoxBronze); // tibia
			MakeBone(Joint, M_SPH, FVector(H * 0.04f, 0, -H * 0.42f), FVector(0.20f, 0.24f, 0.14f), NoRot, NoxBronze);  // patte
			for (int32 cclaw = -1; cclaw <= 1; ++cclaw) // 3 griffes vers l'avant
				MakeBone(Joint, M_CONE, FVector(H * 0.12f, cclaw * 9.f, -H * 0.44f), FVector(0.05f, 0.05f, h * 0.12f), FRotator(70.f, 0, 0), TuskC);
		};
		JRShoulder = MakeJoint(VisualRoot, FVector(LegX, LegY, -H * 0.08f));   BuildLeg(JRShoulder);  // avant droit
		JLShoulder = MakeJoint(VisualRoot, FVector(LegX, -LegY, -H * 0.08f));  BuildLeg(JLShoulder);  // avant gauche
		JRHip = MakeJoint(VisualRoot, FVector(-LegX, LegY, -H * 0.12f));       BuildLeg(JRHip);       // arrière droit
		JLHip = MakeJoint(VisualRoot, FVector(-LegX, -LegY, -H * 0.12f));      BuildLeg(JLHip);       // arrière gauche

		// ── QUEUE épaisse segmentée à pics, qui remue ──
		RegisterWiggle(AddPart(M_CONE, FVector(-H * 0.85f, 0, -H * 0.10f), FVector(0.18f, 0.18f, h * 0.45f), FRotator(-105.f, 0, 0), NoxBronze), 0.f);
		RegisterWiggle(AddPart(M_CONE, FVector(-H * 1.15f, 0, -H * 0.02f), FVector(0.10f, 0.10f, h * 0.30f), FRotator(-100.f, 0, 0), Scale2), 0.6f);
		bArticulated = true; // démarche quadrupède (les 4 pattes s'animent)
		return;
	}
	if (Id == TEXT("Noxeons")) // Spéciale (réf) : colosse abyssal noir couvert de pustules
	{                          // bioluminescentes VERTES, grappe d'yeux verts, plaques épineuses,
		// tentacules dorsaux. Avant = -X (humanoïde flip). Carrure massive.
		const FLinearColor Body(0.06f, 0.08f, 0.07f, 1.f);      // chair noire abyssale
		const FLinearColor GreenGlow(0.30f, 1.65f, 0.50f, 1.f); // pustules/yeux verts lumineux
		BuildArticulatedHumanoid(H, Body, 0.42f);
		// GRAPPE D'YEUX verts lumineux sur la tête (devant = -X)
		for (int32 e = 0; e < 9; ++e)
		{
			const float ey = FMath::Sin(e * 2.1f) * 7.f;
			const float ez = H * 0.34f + FMath::Cos(e * 1.6f) * H * 0.045f;
			AddPart(M_SPH, FVector(-10, ey, ez), FVector(0.045f, 0.045f, 0.05f), NoRot, GreenGlow);
		}
		// Couronne de pics autour du crâne
		for (int32 cc = 0; cc < 8; ++cc)
		{
			const float a = -1.5f + cc * 0.43f;
			AddPart(M_CONE, FVector(H * 0.02f, FMath::Sin(a) * 13.f, H * 0.44f),
				FVector(0.05f, 0.05f, h * 0.18f), FRotator(-30.f, 0, FMath::RadiansToDegrees(a) * 0.4f), Body);
		}
		// PUSTULES vertes réparties sur le torse (devant = -X)
		for (int32 t = 0; t < 10; ++t)
		{
			AddPart(M_SPH, FVector(-H * 0.17f, FMath::Sin(t * 1.7f) * 18.f, H * (0.16f - t * 0.028f)),
				FVector(0.04f, 0.04f, 0.04f), NoRot, GreenGlow);
		}
		// Plaques épineuses sur épaules + avant-bras + longues griffes
		for (int32 s = -1; s <= 1; s += 2)
		{
			for (int32 k = 0; k < 3; ++k)
				AddPart(M_CONE, FVector(-H * 0.02f, s * H * 0.16f, H * (0.20f - k * 0.05f)),
					FVector(0.06f, 0.06f, h * (0.20f - k * 0.04f)), FRotator(0, 0, s * (60.f - k * 14.f)), Body);
			for (int32 k = 0; k < 3; ++k)
				MakeBone(s < 0 ? JLElbow : JRElbow, M_CONE, FVector(0.03f, s * 4.f, -H * (0.04f + k * 0.05f)),
					FVector(0.05f, 0.05f, h * 0.13f), FRotator(0, 0, s * 60.f), Body);
			MakeBone(s < 0 ? JLElbow : JRElbow, M_CONE, FVector(-0.04f, s * 3.f, -H * 0.26f),
				FVector(0.05f, 0.05f, h * 0.18f), FRotator(150.f, 0, 0), Body); // longue griffe
		}
		// 2 grands TENTACULES dorsaux (+X) qui ondulent, pointe verte lumineuse
		for (int32 s = -1; s <= 1; s += 2)
		{
			RegisterWiggle(AddPart(M_CONE, FVector(H * 0.10f, s * 16.f, H * 0.30f), FVector(0.06f, 0.06f, h * 1.05f), FRotator(38.f, 0, s * 40.f), Body), s < 0 ? 0.f : 3.14f);
			RegisterWiggle(AddPart(M_CONE, FVector(H * 0.32f, s * 34.f, H * 0.04f), FVector(0.045f, 0.045f, h * 0.18f), FRotator(90.f, 0, s * 40.f), GreenGlow), s < 0 ? 0.5f : 2.6f);
		}
		return;
	}
	if (Id == TEXT("Noxedrake")) // Mythique Noxéen (réf) : dragon abyssal QUADRUPÈDE, écailles
	{                            // noires luisantes, crête + épines dorsales VERTES bioluminescentes,
		// grappe d'yeux verts, gueule à crocs, longue queue. Avant = +X (comme Noxebeast).
		// NB : le boss NEUTRE de la phase 1 reste le Kraken céphalopode (chemin séparé dans
		// BuildGreyboxShape) ; ce modèle-ci est le mythique Noxéen jouable de la phase 2.
		const FLinearColor Scale(0.05f, 0.06f, 0.05f, 1.f);      // écailles noires
		const FLinearColor Scale2(0.09f, 0.11f, 0.09f, 1.f);     // écailles plus claires
		const FLinearColor GreenGlow(0.30f, 1.70f, 0.50f, 1.f);  // épines/yeux verts lumineux
		const FLinearColor Fang(0.75f, 0.72f, 0.55f, 1.f);       // crocs ivoire

		// Corps : poitrail avant haut -> tronc -> croupe
		SetupMainPart(M_SPH, FVector(H * 0.10f, 0, -H * 0.02f), FVector(h * 0.95f, h * 0.80f, h * 0.66f), NoRot, Scale);
		AddPart(M_SPH, FVector(-H * 0.52f, 0, -H * 0.16f), FVector(h * 0.72f, h * 0.66f, h * 0.48f), NoRot, Scale);
		AddPart(M_CYL, FVector(-H * 0.24f, 0, -H * 0.10f), FVector(h * 0.68f, h * 0.68f, h * 0.82f), FRotator(90.f, 0, 0), Scale);

		// Cou serpentin dressé + tête (avant +X, en hauteur)
		AddPart(M_CYL, FVector(H * 0.48f, 0, H * 0.18f), FVector(h * 0.34f, h * 0.34f, h * 0.34f), FRotator(52.f, 0, 0), Scale);
		AddPart(M_SPH, FVector(H * 0.72f, 0, H * 0.40f), FVector(h * 0.40f, h * 0.42f, h * 0.34f), NoRot, Scale);
		AddPart(M_CONE, FVector(H * 0.96f, 0, H * 0.34f), FVector(h * 0.24f, h * 0.20f, h * 0.30f), FRotator(72.f, 0, 0), Scale); // museau allongé
		// Grappe d'YEUX verts lumineux sur le crâne (devant)
		for (int32 e = 0; e < 6; ++e)
		{
			const float ey = FMath::Sin(e * 2.0f) * 12.f;
			const float ez = H * 0.44f + FMath::Cos(e * 1.7f) * H * 0.03f;
			AddPart(M_SPH, FVector(H * 0.82f, ey, ez), FVector(0.10f, 0.10f, 0.11f), NoRot, GreenGlow);
		}
		// Mâchoire basse + rangée de crocs
		AddPart(M_CONE, FVector(H * 0.98f, 0, H * 0.26f), FVector(h * 0.18f, h * 0.16f, h * 0.18f), FRotator(-96.f, 0, 0), Scale2);
		for (int32 f = 0; f < 5; ++f)
		{
			const float fy = (f - 2) * 7.f;
			AddPart(M_CONE, FVector(H * (0.90f + 0.02f * (f % 2)), fy, H * 0.30f), FVector(0.05f, 0.05f, h * 0.12f), FRotator(150.f, 0, 0), Fang);
		}
		// Barbillons sous la mâchoire (ondulent)
		for (int32 s = -1; s <= 1; s += 2)
			RegisterWiggle(AddPart(M_CONE, FVector(H * 0.94f, s * 10.f, H * 0.16f), FVector(0.03f, 0.03f, h * 0.22f), FRotator(150.f, 0, s * 10.f), Scale2), s < 0 ? 0.f : 2.0f);

		// GRANDE CRÊTE + ÉPINES DORSALES vertes lumineuses (de la nuque +X vers la queue -X)
		for (int32 s = 0; s < 10; ++s)
		{
			const float sx = H * (0.50f - s * 0.14f);
			const float sz = H * (0.34f - s * 0.028f);
			const float sh = h * (0.55f - s * 0.035f);
			AddPart(M_CONE, FVector(sx, 0, sz), FVector(0.09f, 0.15f, sh), FRotator(-16.f, 0, 0), GreenGlow);
		}
		// Lignes bioluminescentes vertes le long des flancs
		for (int32 s = -1; s <= 1; s += 2)
			for (int32 k = 0; k < 5; ++k)
				AddPart(M_SPH, FVector(H * (0.30f - k * 0.16f), s * H * 0.34f, -H * 0.06f), FVector(0.05f, 0.05f, 0.05f), NoRot, GreenGlow);

		// 4 PATTES SEGMENTÉES (cuisse + tibia + patte + 3 griffes), animées
		const float LegX = H * 0.36f, LegY = H * 0.34f;
		auto BuildLeg = [&](USceneComponent* Joint)
		{
			MakeBone(Joint, M_CYL, FVector(0, 0, -H * 0.10f), FVector(0.20f, 0.20f, h * 0.22f), NoRot, Scale);
			MakeBone(Joint, M_CYL, FVector(H * 0.02f, 0, -H * 0.28f), FVector(0.15f, 0.15f, h * 0.20f), NoRot, Scale);
			MakeBone(Joint, M_SPH, FVector(H * 0.04f, 0, -H * 0.40f), FVector(0.18f, 0.22f, 0.13f), NoRot, Scale);
			for (int32 cl = -1; cl <= 1; ++cl)
				MakeBone(Joint, M_CONE, FVector(H * 0.12f, cl * 8.f, -H * 0.42f), FVector(0.05f, 0.05f, h * 0.12f), FRotator(70.f, 0, 0), Fang);
		};
		JRShoulder = MakeJoint(VisualRoot, FVector(LegX, LegY, -H * 0.06f));   BuildLeg(JRShoulder);
		JLShoulder = MakeJoint(VisualRoot, FVector(LegX, -LegY, -H * 0.06f));  BuildLeg(JLShoulder);
		JRHip = MakeJoint(VisualRoot, FVector(-LegX, LegY, -H * 0.10f));       BuildLeg(JRHip);
		JLHip = MakeJoint(VisualRoot, FVector(-LegX, -LegY, -H * 0.10f));      BuildLeg(JLHip);

		// LONGUE QUEUE segmentée qui ondule, terminée par un aiguillon caudal vert lumineux
		RegisterWiggle(AddPart(M_CONE, FVector(-H * 0.80f, 0, -H * 0.10f), FVector(0.20f, 0.20f, h * 0.5f), FRotator(-100.f, 0, 0), Scale), 0.f);
		RegisterWiggle(AddPart(M_CONE, FVector(-H * 1.12f, 0, -H * 0.02f), FVector(0.13f, 0.13f, h * 0.42f), FRotator(-96.f, 0, 0), Scale2), 0.6f);
		RegisterWiggle(AddPart(M_CONE, FVector(-H * 1.40f, 0, H * 0.04f), FVector(0.08f, 0.14f, h * 0.30f), FRotator(-90.f, 0, 0), GreenGlow), 1.0f);
		bArticulated = true; // démarche quadrupède (les 4 pattes s'animent)
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
	// Couleur VIVE (énergie, yeux, épée, cristaux) -> matériau ÉMISSIF (ça brille).
	// Couleur normale (chair, cuirasse, roche) -> matériau MAT rugueux (fini le plastique
	// lisse : la lumière crée du relief/ombrage sur la peau et la carapace).
	const bool bEmissive = (Color.R > 1.2f || Color.G > 1.2f || Color.B > 1.2f);
	// Émissif des unités PLAFONNÉ (~1.15) : les parties vives restent colorées/brillantes
	// mais ne « bloomment » plus en gros pâté lumineux quand l'armée est massée (déploiement).
	const FLinearColor UnitEmissive(FMath::Min(Color.R, 1.15f), FMath::Min(Color.G, 1.15f), FMath::Min(Color.B, 1.15f), 1.f);
	if (UMaterialInstanceDynamic* MID = bEmissive
			? WOTOLGlow::MakeGlow(this, UnitEmissive) : WOTOLGlow::MakeMatte(this, Color))
	{
		C->SetMaterial(0, MID);
		PartMIDs.Add(MID);
		PartBaseColors.Add(Color);
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
	const float LegW = FMath::Max(0.14f, BodyW * 0.44f); // pattes plus épaisses (fini les mini-pattes)
	JRHip = MakeJoint(VisualRoot, FVector(0, H * 0.08f, -H * 0.05f));
	MakeBone(JRHip, M_CYL, FVector(0, 0, -H * 0.10f), FVector(LegW, LegW, h * 0.20f), NoRot, Col);
	JRKnee = MakeJoint(JRHip, FVector(0, 0, -H * 0.20f));
	MakeBone(JRKnee, M_CYL, FVector(0, 0, -H * 0.10f), FVector(LegW * 0.9f, LegW * 0.9f, h * 0.22f), NoRot, Col);
	MakeBone(JRKnee, M_CUBE, FVector(-H * 0.07f, 0, -H * 0.21f), FVector(0.24f, LegW * 1.15f, 0.06f), NoRot, Col); // pied vers l'AVANT (compense le flip 180 deg)

	// Jambe GAUCHE
	JLHip = MakeJoint(VisualRoot, FVector(0, -H * 0.08f, -H * 0.05f));
	MakeBone(JLHip, M_CYL, FVector(0, 0, -H * 0.10f), FVector(LegW, LegW, h * 0.20f), NoRot, Col);
	JLKnee = MakeJoint(JLHip, FVector(0, 0, -H * 0.20f));
	MakeBone(JLKnee, M_CYL, FVector(0, 0, -H * 0.10f), FVector(LegW * 0.9f, LegW * 0.9f, h * 0.22f), NoRot, Col);
	MakeBone(JLKnee, M_CUBE, FVector(-H * 0.07f, 0, -H * 0.21f), FVector(0.24f, LegW * 1.15f, 0.06f), NoRot, Col); // pied vers l'AVANT

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
void AWOTOLDemoUnit::OnAttackAnimTrigger()
{
	// Un coup vient d'être porté : arme la fenêtre d'anim d'attaque et (re)démarre le swing.
	AttackAnimTimer = 0.55f;
	SwingProgress = 0.f;

	// FRÉMISSEMENT DE L'EAU : traînée de bulles à la POINTE DE L'ARME (main droite) qui suit
	// le geste -> le coup "brasse" l'eau (cohérence sous-marine), pas des particules figées.
	if (UWorld* W = GetWorld())
	{
		FVector Tip = GetActorLocation() + GetActorForwardVector() * 90.f + FVector(0, 0, 70.f);
		if (JRElbow) Tip = JRElbow->GetComponentLocation() + GetActorForwardVector() * 40.f;
		AWOTOLBubbleBurst::Burst(W, Tip, FLinearColor(0.7f, 0.9f, 1.f, 1.f), 6);
	}

	// AQUISPHÈRES : chaque coup de base = tir de boule Hydrolaser (traînée de bulles) OU
	// coup de crosse si l'ennemi est au contact très rapproché.
	if (UnitData && (UnitData->GetFName() == TEXT("Aquipheres") || UnitData->GetFName() == TEXT("Aquispheres")))
		FireHydrolaserOrMelee();

	// AQUILOMBRES — assassin : FORT taux de critique. Coup de dague ×2 quand elle frappe
	// dans le DOS de la cible (le gros crit surprise vient de la compétence, voir plus bas).
	if (UnitData && UnitData->GetFName() == TEXT("Aquilombres"))
	{
		float Crit = 1.f;
		if (bStealthed) Crit = 3.f; // première frappe SURPRISE depuis l'invisibilité
		else if (AUnitBase* Foe = FindNearestEnemyUnit())
		{
			const FVector ToMe = (GetActorLocation() - Foe->GetActorLocation()).GetSafeNormal2D();
			if (FVector::DotProduct(ToMe, Foe->GetActorForwardVector()) < -0.25f) Crit = 2.f; // dans son dos
		}
		NextHitCritMult = Crit;
		if (Crit > 1.f && GetWorld())
			AWOTOLBubbleBurst::Burst(GetWorld(), GetActorLocation() + GetActorForwardVector() * 60.f + FVector(0, 0, 60.f),
				FLinearColor(0.2f, 0.5f, 1.2f, 1.f), 6);
		// Attaquer la RÉVÈLE (elle sort de l'ombre pour frapper).
		if (bStealthed) { bStealthed = false; SetStealthVisual(false); }
	}

	// ── NOXÉENS : attaque de base THÉMATIQUE (griffe / morsure / patte / tentacule) — aucune
	// unité ne reste passive au corps-à-corps, chacune a son effet propre (agressivité de la
	// faction). Cosmétique ; les dégâts sont appliqués par le combat de base (PerformAttack). ──
	if (UnitData && GetFaction() == EFactionID::Noxeens)
	{
		const FName Nid = UnitData->GetFName();
		UWorld* W = GetWorld();
		AUnitBase* Foe = FindNearestEnemyUnit();
		const FVector Strike = Foe
			? ((Foe->GetFloatingTextAnchor() ? Foe->GetFloatingTextAnchor()->GetComponentLocation() : Foe->GetActorLocation()) + FVector(0, 0, 40.f))
			: (GetActorLocation() + GetActorForwardVector() * 90.f + FVector(0, 0, 60.f));
		const FLinearColor NoxGreen(0.30f, 1.20f, 0.50f, 1.f);
		const FLinearColor NoxViolet(0.75f, 0.35f, 1.40f, 1.f);
		auto Slash = [&](const FLinearColor& Col, int32 n)
		{
			if (!W) return;
			for (int32 i = 0; i < 3; ++i)
				AWOTOLBubbleBurst::Burst(W, Strike + FVector(0, 0, -15.f + i * 15.f) + GetActorForwardVector() * (i * 10.f), Col, n);
		};
		if      (Nid == TEXT("Noxar"))     Slash(NoxGreen, 5);   // coup de griffe abyssal
		else if (Nid == TEXT("Noxeflare")) Slash(NoxViolet, 5);  // griffe/morsure crépusculaire
		else if (Nid == TEXT("Noxebeast"))  Slash(NoxGreen, 7);  // coup de patte avant massif
		else if (Nid == TEXT("Noxeons"))    Slash(NoxGreen, 6);  // fouet de tentacule dorsal
		else if (Nid == TEXT("Noxedrake"))  Slash(NoxGreen, 8);  // morsure / coup de queue
		else if (Nid == TEXT("Noxeblast"))
		{
			// Corps-à-corps : décharge de paume. + PASSIF « Yeux des Abysses » : EXÉCUTE les
			// cibles AVEUGLÉES/désorientées (combo Noxeflare -> Noxeblast : aveugle puis exécute).
			Slash(NoxViolet, 5);
			if (Foe && W && W->GetTimeSeconds() < Foe->BlindedUntil)
			{
				NextHitCritMult = 1.6f;
				AWOTOLDamageNumber::SpawnText(W, Strike + FVector(0, 0, 90.f), TEXT("Execution"), NoxViolet);
			}
		}
	}
}

void AWOTOLDemoUnit::AnimateArticulated(float Dt)
{
	if (!bArticulated) return;

	// ACTION-BASED : l'anim d'ATTAQUE ne joue QUE dans la courte fenêtre après un vrai coup.
	// La NAGE joue dès que l'unité SE DÉPLACE : soit elle bouge vraiment (vélocité), soit
	// elle est dans un état de déplacement (va vers l'ennemi / patrouille / repli / ordre
	// joueur). -> fini le "glisse sans animation" pendant les déplacements ordonnés.
	EUnitAIState St = EUnitAIState::Idle; bool bOrderMove = false;
	if (UUnitAIStateComponent* S = FindComponentByClass<UUnitAIStateComponent>())
	{
		St = S->GetCurrentState();
		bOrderMove = S->bFollowingPlayerOrder || S->bAttackMoveActive;
	}
	const float Speed = GetVelocity().Size2D();
	const bool bAttacking = (AttackAnimTimer > 0.f);
	const bool bInMoveState = (St == EUnitAIState::Seeking || St == EUnitAIState::Patrolling
		|| St == EUnitAIState::Retreating || bOrderMove);
	const bool bMoving = !bAttacking && (Speed > 6.f || bInMoveState);
	const bool bDead = !IsAlive();
	// NAGE vs MARCHE : dès que l'unité est décollée du sol (couche haute), elle NAGE (corps
	// projeté vers l'avant, brasse + battement de jambes) au lieu de "marcher dans le vide".
	const bool bSwim = (CurLayer > 60.f);

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
		SwingProgress = FMath::Min(1.f, SwingProgress + Dt * 3.0f); // UN coup net par attaque
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
	else if (bMoving && bSwim)
	{
		// NAGE (en hauteur) : buste incliné vers l'avant, bras en BRASSE symétrique, jambes
		// en battement souple. Fini la "marche dans le vide" sur les couches verticales.
		const float s = FMath::Sin(AnimPhase);
		rHip =  s * 20.f;  lHip = -s * 20.f;                   // battement de jambes doux
		rKnee = 16.f + FMath::Abs(s) * 22.f;
		lKnee = 16.f + FMath::Abs(s) * 22.f;
		rSho = -52.f + s * 26.f;  lSho = -52.f - s * 26.f;     // bras tendus devant, brasse
		rEl  = -32.f - FMath::Max(0.f,  s) * 30.f;
		lEl  = -32.f - FMath::Max(0.f, -s) * 30.f;
		torsoPitch = 22.f;                                     // colonne inclinée vers l'avant (nage)
		torsoRoll  = FMath::Sin(AnimPhase * 1.5f) * 3.f;
	}
	else if (bMoving)
	{
		// MARCHE au sol (appui des pieds) : foulée alternée, buste légèrement penché.
		const float s = FMath::Sin(AnimPhase);
		rHip =  s * 42.f;  lHip = -s * 42.f;                   // jambes bien alternées
		rKnee = 15.f + FMath::Max(0.f, -s) * 55.f;             // genou plie en fin de foulée
		lKnee = 15.f + FMath::Max(0.f,  s) * 55.f;
		rSho = -s * 38.f;  lSho =  s * 38.f;                   // bras opposés amples
		rEl  = -20.f - FMath::Max(0.f, -s) * 25.f;             // coudes fléchis à la marche
		lEl  = -20.f - FMath::Max(0.f,  s) * 25.f;
		torsoRoll = FMath::Sin(AnimPhase * 2.f) * 2.5f;
		torsoPitch = 4.f;                                      // légèrement penché en avant
	}
	else // idle : léger flottement + frémissement sous-marin permanent
	{
		const float s = FMath::Sin(AnimPhase);
		rSho = 6.f + s * 4.f;  lSho = 6.f - s * 4.f;
		rEl = -12.f + FMath::Cos(AnimPhase * 1.3f) * 3.f; lEl = -12.f - FMath::Cos(AnimPhase * 1.3f) * 3.f; // bras qui frémissent dans le courant
		torsoRoll = s * 1.5f;
	}

	// ── BOUCLIER : pose de GARDE (écu levé en travers devant) quand l'unité vient de bloquer
	// (ShieldGuardTimer) — lecture claire du blocage. Prime sur la pose de bras gauche.
	if (bHasShield && !bAttacking && !bDead && ShieldGuardTimer > 0.f)
	{
		lShoRoll = -58.f; lSho = 22.f; lEl = -80.f;           // avant-bras en travers, écu haut
		torsoPitch = FMath::Max(torsoPitch, 2.f);
	}

	// ── ARME À DEUX MAINS (Aquisphères) : les DEUX bras tiennent le canon devant le corps
	// (main droite = crosse, main gauche = fût). Recul sec au tir. Le canon suit la main
	// droite ; la main gauche vient le tenir (bras croisé devant). ──
	if (bTwoHandWeapon && !bDead)
	{
		const float Recoil = bAttacking ? -18.f : 0.f;       // léger recul du bras au tir
		rSho = -48.f + Recoil; rEl = -58.f;                  // bras droit tient la crosse, canon en avant
		lShoRoll = -34.f; lSho = -40.f; lEl = -70.f;         // bras gauche croisé sur le fût
		torsoPitch = FMath::Max(torsoPitch, bAttacking ? 6.f : 3.f);
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

	// Décrément des fenêtres d'anim (AnimateBody est appelée pour TOUTES les unités, une
	// fois par frame -> point unique de décompte) : attaque + garde au bouclier.
	if (AttackAnimTimer  > 0.f) AttackAnimTimer  = FMath::Max(0.f, AttackAnimTimer - Dt);
	if (ShieldGuardTimer > 0.f) ShieldGuardTimer = FMath::Max(0.f, ShieldGuardTimer - Dt);

	// ACTION-BASED : à-coup d'attaque SEULEMENT après un vrai coup ; nage dès que l'unité
	// se déplace (vélocité OU état de déplacement) -> animation fluide pendant les ordres.
	EUnitAIState St2 = EUnitAIState::Idle; bool bOrderMove2 = false;
	if (UUnitAIStateComponent* S = FindComponentByClass<UUnitAIStateComponent>())
	{
		St2 = S->GetCurrentState();
		bOrderMove2 = S->bFollowingPlayerOrder || S->bAttackMoveActive;
	}
	const float Speed  = GetVelocity().Size2D();
	const bool  bAttacking = (AttackAnimTimer > 0.f);
	const bool  bInMoveState2 = (St2 == EUnitAIState::Seeking || St2 == EUnitAIState::Patrolling
		|| St2 == EUnitAIState::Retreating || bOrderMove2);
	const bool  bMoving = !bAttacking && (Speed > 6.f || bInMoveState2);

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

	// ── AQUILANCES : BASCULE GARDE PASSIVE <-> AGRESSIVE + COUP DE LANCE ──
	// • Garde PASSIVE (au repos / placement / pas d'ennemi) : lance portée en DIAGONALE
	//   (pointe haut-avant, talon bas-arrière) — au-dessus de la monture, sans la traverser.
	// • Garde AGRESSIVE (ennemi à portée / charge) : la lance S'ABAISSE à l'horizontale,
	//   couchée vers l'avant (+X), prête à percer.
	// • Pendant un coup : ESTOC — la lance jaillit vers l'avant puis se rétracte.
	// La bascule est INTERPOLÉE (mouvement fluide, pas de saut).
	if (LanceJoint)
	{
		// Aggro interpolé vers sa cible (transition douce passive <-> agressive).
		LanceAggro = FMath::FInterpTo(LanceAggro, bDead ? 0.f : LanceAggroTarget, Dt, 6.f);

		// Rotation : diagonale (garde) -> horizontale (couchée) selon l'agressivité.
		const FRotator PassiveRot(38.f, 18.f, 0.f); // pointe relevée + un peu en travers (au-dessus de la monture)
		const FRotator CouchedRot(2.f, 0.f, 0.f);   // presque à plat, pointée devant
		const FRotator GoalRot = FMath::Lerp(PassiveRot, CouchedRot, LanceAggro);
		LanceJoint->SetRelativeRotation(FMath::RInterpTo(LanceJoint->GetRelativeRotation(), GoalRot, Dt, 10.f));

		// Estoc (translation +X) uniquement au moment d'un coup ; sinon léger frémissement.
		float Thrust = 0.f;
		if (bAttacking)
		{
			const float s = FMath::Sin(AnimClock * 12.f);
			Thrust = FMath::Max(0.f, s) * 85.f;
		}
		else
		{
			Thrust = FMath::Sin(AnimClock * 1.4f + BobSeed) * 4.f; // frémissement au repos (courant)
		}
		const FVector Goal = LanceHome + FVector(Thrust, 0.f, 0.f);
		LanceJoint->SetRelativeLocation(FMath::VInterpTo(LanceJoint->GetRelativeLocation(), Goal, Dt, 18.f));
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
