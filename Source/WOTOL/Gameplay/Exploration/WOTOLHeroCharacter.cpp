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
	PrimaryActorTick.bCanEverTick = false;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength        = 400.f;
	SpringArm->bUsePawnControlRotation = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

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
	Move->MaxFlySpeed = 600.f;
	Move->MaxAcceleration = 1400.f;
	Move->BrakingDecelerationFlying = 1200.f; // dérive douce (sensation aquatique)
	Move->GravityScale = 0.f;
}

void AWOTOLHeroCharacter::BeginPlay()
{
	Super::BeginPlay();
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);

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
}

void AWOTOLHeroCharacter::MoveForward(float Value)
{
	if (Value == 0.f) return;
	const FRotator Rot(0.f, GetControlRotation().Yaw, 0.f);
	AddMovementInput(FRotationMatrix(Rot).GetUnitAxis(EAxis::X), Value);
}

void AWOTOLHeroCharacter::MoveRight(float Value)
{
	if (Value == 0.f) return;
	const FRotator Rot(0.f, GetControlRotation().Yaw, 0.f);
	AddMovementInput(FRotationMatrix(Rot).GetUnitAxis(EAxis::Y), Value);
}

void AWOTOLHeroCharacter::MoveUp(float Value)
{
	// Montée / descente verticale (nage) — indépendante de l'orientation caméra pour
	// rester simple à comprendre : Espace = monter, Maj/Ctrl = descendre.
	if (Value == 0.f) return;
	AddMovementInput(FVector::UpVector, Value);
}
