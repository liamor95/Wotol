#include "WOTOLHeroCharacter.h"
#include "Gameplay/Units/HeroLoadoutDataAsset.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Core/WOTOLGameInstance.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

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

	GreyboxBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GreyboxBody"));
	GreyboxBody->SetupAttachment(RootComponent);
	GreyboxBody->SetRelativeLocation(FVector(0.f, 0.f, -12.f));
	GreyboxBody->SetRelativeScale3D(FVector(0.42f, 0.42f, 1.65f));
	GreyboxBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// Sphere est un asset moteur garanti ; étirée en Z, elle forme une silhouette/capsule.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BodyMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (BodyMesh.Succeeded()) GreyboxBody->SetStaticMesh(BodyMesh.Object);

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

	// Couleur de faction sur la silhouette de secours (bleu Aquiloris / vert Noxéen).
	if (GreyboxBody)
	{
		if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
		{
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, this);
			const FLinearColor Color = (Faction == EFactionID::Noxeens)
				? FLinearColor(0.05f, 0.65f, 0.22f, 1.f)
				: FLinearColor(0.12f, 0.48f, 0.95f, 1.f);
			MID->SetVectorParameterValue(TEXT("Color"), Color);
			GreyboxBody->SetMaterial(0, MID);
		}
	}
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
	// rester simple à comprendre : Espace = monter, Maj/Ctrl = descendre.
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

void AWOTOLHeroCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (DashCooldownRemaining > 0.f)
	{
		DashCooldownRemaining = FMath::Max(0.f, DashCooldownRemaining - DeltaSeconds);
	}

	// Inclinaison douce en virage (banking) : sensation de nage dirigée, pas un rail figé.
	// Appliquée à la CAMÉRA (bUsePawnControlRotation=false), pas au SpringArm : celui-ci a
	// bUsePawnControlRotation=true et recalcule sa rotation depuis le contrôleur chaque
	// tick, ce qui écraserait un roll posé directement dessus.
	const float TargetRoll = FMath::Clamp(-CurrentLateralInput * 18.f, -18.f, 18.f);
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
