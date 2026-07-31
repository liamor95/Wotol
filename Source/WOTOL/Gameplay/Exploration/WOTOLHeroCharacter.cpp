#include "WOTOLHeroCharacter.h"
#include "Gameplay/Units/HeroLoadoutDataAsset.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Core/WOTOLGameInstance.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "InputCoreTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Gameplay/Demo/WOTOLGlow.h"
#include "Gameplay/Demo/WOTOLDemoUnit.h"
#include "Gameplay/Demo/WOTOLDamageNumber.h"

namespace
{
	const TCHAR* M_CUBE = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* M_SPH  = TEXT("/Engine/BasicShapes/Sphere.Sphere");
	const TCHAR* M_CYL  = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* M_CONE = TEXT("/Engine/BasicShapes/Cone.Cone");
}

AWOTOLHeroCharacter::AWOTOLHeroCharacter()
{
	// Tick actif : sensation "action" (inclinaison en virage, FOV dynamique au sprint/dash,
	// recharge de la ruée) — distincte du pilotage RTS des phases de bataille tactique.
	PrimaryActorTick.bCanEverTick = true;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength        = 400.f;
	SpringArm->bUsePawnControlRotation = true;
	SpringArm->bEnableCameraLag        = true;
	SpringArm->CameraLagSpeed          = 9.f; // léger retard de caméra = sensation de poids/inertie

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;
	Camera->FieldOfView = 90.f;

	// Le corps détaillé (BuildHeroBody) est construit en BeginPlay, PAS ici : il dépend de la
	// Faction (Aquiloris/Noxéens), connue seulement une fois le GameInstance interrogé.

	bUseControllerRotationYaw = false;

	// ── NAGE EN VOLUME (canon WOTOL) : pas de marche terrestre. Le héros FLOTTE et se
	// déplace librement en 3D (avant/arrière/gauche/droite + monter/descendre), sans
	// gravité. On utilise le mode Flying du CharacterMovement pour un contrôle simple.
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;   // le corps s'oriente vers la nage
	Move->RotationRate = FRotator(0.f, 360.f, 0.f);
	Move->DefaultLandMovementMode = MOVE_Flying;
	Move->MaxFlySpeed = SwimSpeed;
	Move->MaxAcceleration = 1400.f;
	Move->BrakingDecelerationFlying = 1200.f; // dérive douce (sensation aquatique)
	Move->GravityScale = 0.f;
}

void AWOTOLHeroCharacter::BeginPlay()
{
	Super::BeginPlay();
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	GetCharacterMovement()->MaxFlySpeed = SwimSpeed;
	if (Camera) BaseFOV = Camera->FieldOfView;

	if (UWOTOLGameInstance* GI = Cast<UWOTOLGameInstance>(GetGameInstance()))
	{
		Faction = GI->GetSelectedFaction();
	}

	// Corps détaillé fidèle au Chef de la faction (remplace l'ancienne silhouette greybox
	// à une seule forme) — dépend de la Faction, donc construit ici plutôt qu'au constructeur.
	BuildHeroBody();
}

void AWOTOLHeroCharacter::SetLoadout(UHeroLoadoutDataAsset* Loadout)
{
	CurrentLoadout = Loadout;
	if (Loadout)
	{
		Faction = Loadout->Faction;
	}
}

void AWOTOLHeroCharacter::SetupPlayerInputComponent(UInputComponent* Input)
{
	Super::SetupPlayerInputComponent(Input);

	Input->BindAxis("MoveForward", this, &AWOTOLHeroCharacter::MoveForward);
	Input->BindAxis("MoveRight",   this, &AWOTOLHeroCharacter::MoveRight);
	Input->BindAxis("MoveUp",      this, &AWOTOLHeroCharacter::MoveUp);
	Input->BindAxis("Turn",        this, &ACharacter::AddControllerYawInput);
	Input->BindAxis("LookUp",      this, &ACharacter::AddControllerPitchInput);

	// Sensation ACTION : sprint maintenu (Alt gauche) + ruée courte (touche C, recharge).
	Input->BindAction("Sprint", IE_Pressed,  this, &AWOTOLHeroCharacter::StartSprint);
	Input->BindAction("Sprint", IE_Released, this, &AWOTOLHeroCharacter::StopSprint);
	Input->BindAction("Dash",   IE_Pressed,  this, &AWOTOLHeroCharacter::PerformDash);

	// Zoom caméra (molette), même touche que AWOTOLBattleCamera.
	Input->BindAxisKey(EKeys::MouseWheelAxis, this, &AWOTOLHeroCharacter::InputZoom);

	// Mode ACTION : attaque de base sur l'action "Interact" (F/Entrée, déjà déclarée dans
	// DefaultInput.ini mais jamais branchée jusqu'ici) ; compétence sur R, MÊME touche que
	// l'activation de compétence en bataille RTS (AWOTOLPlayerController_Battle::
	// ActivateSelectionAbility) — cohérence d'entrée entre les deux modes de jeu.
	Input->BindAction("Interact", IE_Pressed, this, &AWOTOLHeroCharacter::PerformAttack);
	Input->BindKey(EKeys::R, IE_Pressed, this, &AWOTOLHeroCharacter::PerformAbility);
}

void AWOTOLHeroCharacter::MoveForward(float Value)
{
	// Mémorisé CHAQUE tick (y compris à 0) : MoveForward/MoveRight sont deux callbacks
	// d'axe séparés appelés dans un ordre non garanti, donc on ne peut pas accumuler un
	// vecteur direction directement dans l'un des deux sans risquer de se faire écraser
	// par l'autre — on stocke juste la valeur brute, recomposée à la demande.
	CurrentForwardInput = Value;
	if (Value != 0.f)
	{
		const FRotator Rot(0.f, GetControlRotation().Yaw, 0.f);
		AddMovementInput(FRotationMatrix(Rot).GetUnitAxis(EAxis::X), Value);
	}
}

void AWOTOLHeroCharacter::MoveRight(float Value)
{
	// Mémorisé CHAQUE tick (y compris à 0) pour que l'inclinaison (banking) en Tick()
	// revienne bien à plat quand le joueur relâche la touche de direction latérale.
	CurrentLateralInput = Value;
	if (Value != 0.f)
	{
		const FRotator Rot(0.f, GetControlRotation().Yaw, 0.f);
		AddMovementInput(FRotationMatrix(Rot).GetUnitAxis(EAxis::Y), Value);
	}
}

void AWOTOLHeroCharacter::MoveUp(float Value)
{
	// Montée / descente verticale (nage) — indépendante de l'orientation caméra pour
	// rester simple à comprendre : Espace/E = descendre, Maj/Ctrl = monter (cf.
	// Config/DefaultInput.ini, inversé le 31/07/2026).
	if (Value == 0.f) return;
	AddMovementInput(FVector::UpVector, Value);
}

void AWOTOLHeroCharacter::StartSprint()
{
	bIsSprinting = true;
	GetCharacterMovement()->MaxFlySpeed = SprintSpeed;
}

void AWOTOLHeroCharacter::StopSprint()
{
	bIsSprinting = false;
	GetCharacterMovement()->MaxFlySpeed = SwimSpeed;
}

void AWOTOLHeroCharacter::PerformDash()
{
	if (!IsDashReady()) return;

	const FRotator Rot(0.f, GetControlRotation().Yaw, 0.f);
	FVector Direction = FRotationMatrix(Rot).GetUnitAxis(EAxis::X) * CurrentForwardInput
		+ FRotationMatrix(Rot).GetUnitAxis(EAxis::Y) * CurrentLateralInput;
	if (Direction.IsNearlyZero())
	{
		// Pas de direction de nage tenue : rue dans l'axe du regard (geste toujours utile).
		Direction = GetControlRotation().Vector();
	}
	Direction = Direction.GetSafeNormal();

	GetCharacterMovement()->Velocity += Direction * DashImpulse;
	DashCooldownRemaining = DashCooldown;
	DashFOVPunchRemaining = DashFOVPunchDuration; // petit coup de zoom arrière, pas de shake asset en greybox
}

void AWOTOLHeroCharacter::InputZoom(float V) { ZoomInput += V; }

void AWOTOLHeroCharacter::TickZoom(float DeltaSeconds)
{
	if (FMath::IsNearlyZero(ZoomInput) || !SpringArm) { ZoomInput = 0.f; return; }

	// Zoom par CRAN (pas d'échelle par DeltaSeconds), même formule que AWOTOLBattleCamera::
	// TickZoom : chaque cran de molette rapproche d'un pas franc, proportionnel à la distance
	// actuelle (accélère de loin), borné [MinArmLength, MaxArmLength].
	const float Step = ZoomSpeed * (0.6f + SpringArm->TargetArmLength / MaxArmLength);
	SpringArm->TargetArmLength = FMath::Clamp(
		SpringArm->TargetArmLength - ZoomInput * Step, MinArmLength, MaxArmLength);
	ZoomInput = 0.f;
}

void AWOTOLHeroCharacter::PerformAttack()
{
	if (!IsAttackReady()) return;
	AttackCooldownRemaining = AttackCooldown;
	AttackLungeRemaining = 0.18f; // petite impulsion avant, cf. Tick
	TryHitFeedback(AttackRange, TEXT("Touche !"));
}

void AWOTOLHeroCharacter::PerformAbility()
{
	if (!IsAbilityReady()) return;
	AbilityCooldownRemaining = AbilityCooldown;
	TryHitFeedback(AttackRange * 1.8f, TEXT("Competence !"));
}

// Feedback visuel UNIQUEMENT (texte flottant, même système que "Pare"/"Esquive" en bataille) —
// ne modifie PAS les PV réels de la cible : le seul AWOTOLDemoUnit présent pendant l'exploration
// est le Kraken (ExplorationCreature côté WOTOLDemoDirector), dont les PV sont calibrés pour la
// bataille RTS qui suit la découverte. Un geste d'exploration ne doit pas fausser ce calibrage.
void AWOTOLHeroCharacter::TryHitFeedback(float Range, const TCHAR* Label)
{
	TArray<AActor*> Units;
	UGameplayStatics::GetAllActorsOfClass(this, AWOTOLDemoUnit::StaticClass(), Units);
	const FVector Fwd = GetActorForwardVector();
	for (AActor* A : Units)
	{
		AWOTOLDemoUnit* U = Cast<AWOTOLDemoUnit>(A);
		if (!U || !U->IsAlive()) continue;
		const FVector ToUnit = U->GetActorLocation() - GetActorLocation();
		if (ToUnit.Size() > Range) continue;
		if (FVector::DotProduct(Fwd, ToUnit.GetSafeNormal()) < 0.5f) continue; // devant seulement (~60°)
		AWOTOLDamageNumber::SpawnText(GetWorld(), U->GetActorLocation() + FVector(0.f, 0.f, 120.f),
			Label, FLinearColor(0.9f, 0.95f, 1.f, 1.f));
		break; // une seule cible à la fois
	}
}

void AWOTOLHeroCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (DashCooldownRemaining > 0.f)
	{
		DashCooldownRemaining = FMath::Max(0.f, DashCooldownRemaining - DeltaSeconds);
	}
	if (AttackCooldownRemaining > 0.f)
	{
		AttackCooldownRemaining = FMath::Max(0.f, AttackCooldownRemaining - DeltaSeconds);
	}
	if (AbilityCooldownRemaining > 0.f)
	{
		AbilityCooldownRemaining = FMath::Max(0.f, AbilityCooldownRemaining - DeltaSeconds);
	}
	if (AttackLungeRemaining > 0.f)
	{
		// Petite impulsion avant qui s'estompe : le coup se SENT physiquement, pas juste un texte.
		AttackLungeRemaining = FMath::Max(0.f, AttackLungeRemaining - DeltaSeconds);
		AddMovementInput(GetActorForwardVector(), 0.6f);
	}

	TickZoom(DeltaSeconds);
	AnimateSwim(DeltaSeconds);

	// Inclinaison douce en virage (banking) : sensation de nage dirigée, pas un rail figé.
	// Appliquée à la CAMÉRA (bUsePawnControlRotation=false), pas au SpringArm : celui-ci a
	// bUsePawnControlRotation=true et recalcule sa rotation depuis le contrôleur chaque
	// tick, ce qui écraserait un roll posé directement dessus.
	// Angle réduit (18° -> 6°, 31/07/2026) : remonté par Liamor comme une "distorsion" façon
	// fisheye en tournant à gauche/droite — trop prononcé pour une caméra suivant le joueur.
	const float TargetRoll = FMath::Clamp(-CurrentLateralInput * 6.f, -6.f, 6.f);
	CurrentBankRoll = FMath::FInterpTo(CurrentBankRoll, TargetRoll, DeltaSeconds, 5.f);
	if (Camera)
	{
		FRotator Local = Camera->GetRelativeRotation();
		Local.Roll = CurrentBankRoll;
		Camera->SetRelativeRotation(Local);
	}

	// FOV dynamique : légère ouverture en sprint + coup de zoom arrière bref sur la ruée
	// (pas d'asset de camera shake en greybox) — sensation de vitesse/action, bien distincte
	// de la caméra RTS fixe des phases de bataille.
	if (DashFOVPunchRemaining > 0.f)
	{
		DashFOVPunchRemaining = FMath::Max(0.f, DashFOVPunchRemaining - DeltaSeconds);
	}
	if (Camera)
	{
		const float SprintFOV = bIsSprinting ? BaseFOV + 8.f : BaseFOV;
		const float DashAlpha = DashFOVPunchDuration > 0.f ? DashFOVPunchRemaining / DashFOVPunchDuration : 0.f;
		const float TargetFOV = SprintFOV + DashAlpha * 14.f;
		Camera->SetFieldOfView(FMath::FInterpTo(Camera->FieldOfView, TargetFOV, DeltaSeconds, 6.f));
	}
}

// ─── Construction du corps (26/07/2026) ──────────────────────────────────────
// Mêmes helpers que AWOTOLDemoUnit::AddPart/MakeJoint/MakeBone (composants créés
// dynamiquement via NewObject+RegisterComponent, pas en constructeur) — dupliqués ici plutôt
// que partagés : WOTOLHeroCharacter dérive de ACharacter, pas de AUnitBase/AWOTOLDemoUnit, donc
// aucun risque de toucher au système d'animation/dégâts des unités RTS déjà validé.

UStaticMeshComponent* AWOTOLHeroCharacter::AddPart(const TCHAR* MeshPath, const FVector& RelLoc,
	const FVector& RelScale, const FRotator& RelRot, const FLinearColor& Color)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
	if (!C) return nullptr;
	C->SetupAttachment(RootComponent);
	C->RegisterComponent();
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, MeshPath)) C->SetStaticMesh(M);
	C->SetRelativeLocationAndRotation(RelLoc, RelRot);
	C->SetRelativeScale3D(RelScale);

	const bool bEmissive = (Color.R > 1.2f || Color.G > 1.2f || Color.B > 1.2f);
	if (UMaterialInstanceDynamic* MID = bEmissive
			? WOTOLGlow::MakeGlow(this, Color) : WOTOLGlow::MakeMatte(this, Color))
	{
		C->SetMaterial(0, MID);
	}
	return C;
}

USceneComponent* AWOTOLHeroCharacter::MakeJoint(USceneComponent* Parent, const FVector& RelLoc)
{
	USceneComponent* J = NewObject<USceneComponent>(this);
	if (!J) return nullptr;
	J->SetupAttachment(Parent ? Parent : RootComponent.Get());
	J->RegisterComponent();
	J->SetRelativeLocation(RelLoc);
	return J;
}

UStaticMeshComponent* AWOTOLHeroCharacter::MakeBone(USceneComponent* Joint, const TCHAR* MeshPath,
	const FVector& Offset, const FVector& Scale, const FRotator& Rot, const FLinearColor& Color)
{
	if (!Joint) return nullptr;
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
	if (!C) return nullptr;
	C->SetupAttachment(Joint);
	C->RegisterComponent();
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, MeshPath)) C->SetStaticMesh(M);
	C->SetRelativeLocationAndRotation(Offset, Rot);
	C->SetRelativeScale3D(Scale);

	const bool bEmissive = (Color.R > 1.2f || Color.G > 1.2f || Color.B > 1.2f);
	if (UMaterialInstanceDynamic* MID = bEmissive
			? WOTOLGlow::MakeGlow(this, Color) : WOTOLGlow::MakeMatte(this, Color))
	{
		C->SetMaterial(0, MID);
	}
	return C;
}

// Corps articulé détaillé fidèle au Chef de la faction (Aquis pour Aquiloris, Noxar pour
// Noxéens — mêmes palettes/traits que BuildAquiKnight et le bloc Noxar de WOTOLDemoUnit.cpp).
// Un seul héros affiché pendant l'exploration -> détail poussé bien au-delà de ce qui serait
// raisonnable pour 100 unités de bataille (~40 pièces), sans risque de coût perf notable.
// Forward = +X (convention ACharacter standard, PAS le flip 180° historique de WOTOLDemoUnit).
void AWOTOLHeroCharacter::BuildHeroBody()
{
	const float H = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 2.f : 176.f;
	const float h = H / 100.f;
	const FRotator NoRot = FRotator::ZeroRotator;
	const bool bAq = (Faction != EFactionID::Noxeens);

	const FLinearColor AqArmor   (0.11f, 0.20f, 0.50f, 1.f);
	const FLinearColor AqGold    (0.95f, 0.78f, 0.25f, 1.f);
	const FLinearColor AqEyeGlow (0.35f, 0.75f, 1.80f, 1.f);
	const FLinearColor AqEnergyHi(0.55f, 1.30f, 2.60f, 1.f);
	const FLinearColor AqCrest   (0.55f, 0.78f, 1.00f, 1.f);
	const FLinearColor AqCape    (0.06f, 0.11f, 0.26f, 1.f);
	const FLinearColor NoxDark   (0.08f, 0.07f, 0.13f, 1.f);
	const FLinearColor NoxGlow   (0.30f, 0.85f, 1.80f, 1.f);
	const FLinearColor Skin      (0.18f, 0.28f, 0.52f, 1.f); // peau bleue Aquiloris

	const FLinearColor BodyCol = bAq ? AqArmor : NoxDark;
	const FLinearColor Shade   = BodyCol * 0.72f;
	const float BodyW = 0.42f;

	// ── Torse (2 étages + ceinturon) ──
	AddPart(M_CYL, FVector(0, 0, H * 0.16f), FVector(BodyW, BodyW * 0.86f, h * 0.30f), NoRot, BodyCol);
	AddPart(M_CYL, FVector(0, 0, -H * 0.02f), FVector(BodyW * 0.92f, BodyW * 0.80f, h * 0.16f), NoRot, BodyCol);
	AddPart(M_CYL, FVector(0, 0, -H * 0.10f), FVector(BodyW * 0.98f, BodyW * 0.86f, 0.03f), NoRot, Shade);

	// ── Bassin + jambes (hanche/genou articulés, pour l'animation de nage) ──
	AddPart(M_SPH, FVector(0, 0, -H * 0.14f), FVector(BodyW * 0.55f, BodyW * 0.5f, 0.22f), NoRot, BodyCol);
	JRHip  = MakeJoint(RootComponent, FVector(0, H * 0.11f, -H * 0.18f));
	JLHip  = MakeJoint(RootComponent, FVector(0, -H * 0.11f, -H * 0.18f));
	JRKnee = MakeJoint(JRHip, FVector(0, 0, -h * 0.42f));
	JLKnee = MakeJoint(JLHip, FVector(0, 0, -h * 0.42f));
	// Rotule de hanche (comble la couture visible entre le bassin et la cuisse en vue
	// rapprochée troisième personne — le héros est le SEUL modèle observé de près, contrairement
	// aux unités RTS où ce détail n'est jamais perceptible).
	MakeBone(JRHip, M_SPH, FVector::ZeroVector, FVector(0.17f, 0.17f, 0.17f), NoRot, BodyCol);
	MakeBone(JLHip, M_SPH, FVector::ZeroVector, FVector(0.17f, 0.17f, 0.17f), NoRot, BodyCol);
	MakeBone(JRHip, M_CYL, FVector(0, 0, -h * 0.20f), FVector(0.16f, 0.16f, h * 0.42f), NoRot, BodyCol);
	MakeBone(JLHip, M_CYL, FVector(0, 0, -h * 0.20f), FVector(0.16f, 0.16f, h * 0.42f), NoRot, BodyCol);
	MakeBone(JRKnee, M_SPH, FVector::ZeroVector, FVector(0.14f, 0.14f, 0.14f), NoRot, Shade);
	MakeBone(JLKnee, M_SPH, FVector::ZeroVector, FVector(0.14f, 0.14f, 0.14f), NoRot, Shade);
	MakeBone(JRKnee, M_CYL, FVector(0, 0, -h * 0.22f), FVector(0.13f, 0.13f, h * 0.44f), NoRot, BodyCol);
	MakeBone(JLKnee, M_CYL, FVector(0, 0, -h * 0.22f), FVector(0.13f, 0.13f, h * 0.44f), NoRot, BodyCol);
	MakeBone(JRKnee, M_CYL, FVector(2, 0, -h * 0.46f), FVector(0.10f, 0.10f, 0.02f), NoRot, Shade);
	MakeBone(JLKnee, M_CYL, FVector(2, 0, -h * 0.46f), FVector(0.10f, 0.10f, 0.02f), NoRot, Shade);
	MakeBone(JRKnee, M_CUBE, FVector(H * 0.05f, 0, -h * 0.50f), FVector(0.10f, 0.09f, 0.05f), NoRot, Shade);
	MakeBone(JLKnee, M_CUBE, FVector(H * 0.05f, 0, -h * 0.50f), FVector(0.10f, 0.09f, 0.05f), NoRot, Shade);

	// ── Cou + tête (visage devant, +X) ──
	AddPart(M_CYL, FVector(0, 0, H * 0.30f), FVector(0.14f, 0.14f, h * 0.06f), NoRot, Shade);
	const FLinearColor HeadCol = bAq ? Skin : NoxDark;
	AddPart(M_SPH, FVector(0, 0, H * 0.38f), FVector(0.20f, 0.20f, 0.22f), NoRot, HeadCol);
	AddPart(M_CUBE, FVector(H * 0.05f, 0, H * 0.34f), FVector(0.10f, 0.13f, 0.06f), NoRot, HeadCol);
	AddPart(M_SPH, FVector(H * 0.10f, 6, H * 0.39f), FVector(0.045f, 0.045f, 0.05f), NoRot, bAq ? AqEyeGlow : NoxGlow);
	AddPart(M_SPH, FVector(H * 0.10f, -6, H * 0.39f), FVector(0.045f, 0.045f, 0.05f), NoRot, bAq ? AqEyeGlow : NoxGlow);

	// ── Bras (épaule/coude articulés) + mains détaillées (poignet+paume+doigts+pouce) ──
	JRShoulder = MakeJoint(RootComponent, FVector(0, H * 0.19f, H * 0.24f));
	JLShoulder = MakeJoint(RootComponent, FVector(0, -H * 0.19f, H * 0.24f));
	JRElbow = MakeJoint(JRShoulder, FVector(0, 0, -h * 0.30f));
	JLElbow = MakeJoint(JLShoulder, FVector(0, 0, -h * 0.30f));
	// Rotule d'épaule (même logique que la rotule de hanche ci-dessus) : referme la couture
	// entre le torse et le bras au lieu de laisser un cylindre "flotter" au ras du buste.
	MakeBone(JRShoulder, M_SPH, FVector::ZeroVector, FVector(0.145f, 0.145f, 0.145f), NoRot, BodyCol);
	MakeBone(JLShoulder, M_SPH, FVector::ZeroVector, FVector(0.145f, 0.145f, 0.145f), NoRot, BodyCol);
	MakeBone(JRShoulder, M_CYL, FVector(0, 0, -h * 0.15f), FVector(0.13f, 0.13f, h * 0.30f), NoRot, BodyCol);
	MakeBone(JLShoulder, M_CYL, FVector(0, 0, -h * 0.15f), FVector(0.13f, 0.13f, h * 0.30f), NoRot, BodyCol);
	MakeBone(JRElbow, M_SPH, FVector::ZeroVector, FVector(0.11f, 0.11f, 0.11f), NoRot, Shade);
	MakeBone(JLElbow, M_SPH, FVector::ZeroVector, FVector(0.11f, 0.11f, 0.11f), NoRot, Shade);
	MakeBone(JRElbow, M_CYL, FVector(0, 0, -h * 0.16f), FVector(0.11f, 0.11f, h * 0.30f), NoRot, BodyCol);
	MakeBone(JLElbow, M_CYL, FVector(0, 0, -h * 0.16f), FVector(0.11f, 0.11f, h * 0.30f), NoRot, BodyCol);

	auto BuildHand = [&](USceneComponent* Elbow, float Side)
	{
		if (!Elbow) return;
		MakeBone(Elbow, M_CYL, FVector(0, 0, -h * 0.30f), FVector(0.075f, 0.075f, 0.02f), NoRot, Shade);
		MakeBone(Elbow, M_CUBE, FVector(0, 0, -h * 0.34f), FVector(0.08f, 0.10f, 0.03f), NoRot, BodyCol);
		for (int32 f = -1; f <= 1; ++f)
			MakeBone(Elbow, M_CYL, FVector(0, f * 5.f, -h * 0.40f), FVector(0.025f, 0.025f, h * 0.07f), NoRot, BodyCol);
		MakeBone(Elbow, M_CYL, FVector(0, Side * 6.f, -h * 0.33f), FVector(0.025f, 0.025f, h * 0.05f), FRotator(0, 0, Side * 40.f), BodyCol);
	};
	BuildHand(JRElbow, 1.f);
	BuildHand(JLElbow, -1.f);

	if (bAq)
	{
		// ── AQUIS/AQUIRA : crête, pauldrons, gemme losange, cape — même esprit que
		// BuildAquiKnight (WOTOLDemoUnit.cpp), adapté au forward +X de ce personnage.
		// Crête en RANGÉE avant-arrière (façon crête/mohawk, cf. references) : Y=0 pour
		// TOUS les pics (aucun étalement lateral) et aucune rotation en lacet -> évite l'effet
		// "éventail/couronne" vu de dos (retour terrain 31/07/2026 : comparaison directe avec
		// la reference du personnage a confirme cet ecart).
		for (int32 cc = 0; cc < 5; ++cc)
		{
			const float t = (cc - 2) / 2.f; // -1 (nuque) .. 1 (front)
			const float mid = 1.f - FMath::Abs(t) * 0.30f;
			AddPart(M_CONE, FVector(H * 0.02f + t * H * 0.09f, 0, H * 0.44f),
				FVector(0.06f, 0.06f, h * 0.22f * mid), FRotator(-14.f, 0, 0), AqCrest);
		}
		AddPart(M_SPH, FVector(0, H * 0.17f, H * 0.22f), FVector(0.20f, 0.20f, 0.17f), NoRot, AqGold); // pauldron D
		AddPart(M_SPH, FVector(0, -H * 0.17f, H * 0.22f), FVector(0.20f, 0.20f, 0.17f), NoRot, AqGold); // pauldron G
		AddPart(M_CONE, FVector(H * 0.145f, 0, H * 0.19f), FVector(0.08f, 0.08f, h * 0.09f), FRotator(90.f, 0, 0), AqGold);
		AddPart(M_CONE, FVector(H * 0.145f, 0, H * 0.13f), FVector(0.08f, 0.08f, h * 0.09f), FRotator(-90.f, 0, 0), AqGold);
		AddPart(M_SPH, FVector(H * 0.16f, 0, H * 0.16f), FVector(0.045f, 0.045f, 0.05f), NoRot, AqEnergyHi); // coeur lumineux
		AddPart(M_CUBE, FVector(H * 0.15f, 6, H * 0.02f), FVector(0.02f, 0.03f, h * 0.30f), NoRot, AqGold);
		AddPart(M_CUBE, FVector(H * 0.15f, -6, H * 0.02f), FVector(0.02f, 0.03f, h * 0.30f), NoRot, AqGold);
		AddPart(M_CYL, FVector(0, 0, H * 0.005f), FVector(BodyW * 0.94f, BodyW * 0.80f, 0.025f), NoRot, AqGold);
		// Cape UNIQUE centrée dans le dos (remplace les 2 pans latéraux qui débordaient sur le
		// côté au lieu de draper le dos, cf. reference) : segment haut étroit aux épaules +
		// segment bas plus large pour suggérer l'évasement d'un tissu qui tombe.
		AddPart(M_CUBE, FVector(-H * 0.12f, 0, H * 0.08f), FVector(0.03f, BodyW * 1.15f, h * 0.22f), FRotator(-6.f, 0, 0), AqCape);
		AddPart(M_CUBE, FVector(-H * 0.15f, 0, -H * 0.20f), FVector(0.03f, BodyW * 1.75f, h * 0.44f), FRotator(-11.f, 0, 0), AqCape);
	}
	else
	{
		// ── NOXAR : veines d'énergie bleues sur le torse + 2 tentacules dans le dos —
		// même esprit que le bloc Noxar (WOTOLDemoUnit.cpp), adapté au forward +X.
		AddPart(M_CONE, FVector(H * 0.18f, 0, H * 0.12f), FVector(0.05f, 0.05f, h * 0.22f), NoRot, NoxGlow);
		for (int32 v = 0; v < 6; ++v)
		{
			const float a = -1.5f + v * 0.6f;
			AddPart(M_CONE, FVector(H * 0.19f, FMath::Sin(a) * 16.f, H * (0.02f + 0.04f * v)),
				FVector(0.035f, 0.035f, h * 0.12f), FRotator(0, 0, FMath::RadiansToDegrees(a)), NoxGlow);
		}
		for (int32 side = -1; side <= 1; side += 2)
			for (int32 k = 0; k < 3; ++k)
				MakeBone(side < 0 ? JLElbow : JRElbow, M_CONE, FVector(0.05f, side * 4.f, -h * (0.04f + k * 0.05f)),
					FVector(0.045f, 0.045f, h * 0.11f), FRotator(0, 0, side * 60.f), NoxDark);
		AddPart(M_CONE, FVector(-16, 18, H * 0.32f), FVector(0.055f, 0.055f, h * 1.0f), FRotator(-42.f, 0, 42.f), NoxGlow);
		AddPart(M_CONE, FVector(-16, -18, H * 0.32f), FVector(0.055f, 0.055f, h * 1.0f), FRotator(-42.f, 0, -42.f), NoxGlow);
	}
}

// Nage animée : bras/jambes articulés (JRShoulder/JRElbow/JRHip/JRKnee + symétrique gauche)
// pivotent en cycle sinusoïdal, cadencé sur la VITESSE réelle (immobile = flotte doucement,
// rapide/sprint = brasse plus vite) — répond à la demande "animation cohérente avec le modèle"
// sans nécessiter de maillage squelettique/Animation Blueprint (aucun asset importé en greybox).
void AWOTOLHeroCharacter::AnimateSwim(float DeltaSeconds)
{
	if (!JRShoulder || !JLShoulder) return;

	const float SpeedRatio = FMath::Clamp(GetVelocity().Size() / FMath::Max(1.f, SwimSpeed), 0.f, 2.2f);
	SwimAnimTime += DeltaSeconds * FMath::Lerp(0.7f, 2.6f, FMath::Clamp(SpeedRatio, 0.f, 1.f));

	const float Cycle = FMath::Sin(SwimAnimTime * 6.f);
	const float CycleOpp = FMath::Sin(SwimAnimTime * 6.f + PI);
	const float ArmAmp = 30.f + SpeedRatio * 20.f;
	const float LegAmp = 18.f + SpeedRatio * 14.f;

	if (JRShoulder) JRShoulder->SetRelativeRotation(FRotator(Cycle * ArmAmp, 0, 0));
	if (JLShoulder) JLShoulder->SetRelativeRotation(FRotator(CycleOpp * ArmAmp, 0, 0));
	if (JRElbow)    JRElbow->SetRelativeRotation(FRotator(FMath::Abs(Cycle) * ArmAmp * 0.5f, 0, 0));
	if (JLElbow)    JLElbow->SetRelativeRotation(FRotator(FMath::Abs(CycleOpp) * ArmAmp * 0.5f, 0, 0));
	if (JRHip)      JRHip->SetRelativeRotation(FRotator(CycleOpp * LegAmp, 0, 0));
	if (JLHip)      JLHip->SetRelativeRotation(FRotator(Cycle * LegAmp, 0, 0));
}
