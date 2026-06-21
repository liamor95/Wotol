#include "WOTOLHeroCharacter.h"
#include "Gameplay/Units/HeroLoadoutDataAsset.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Core/WOTOLGameInstance.h"

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

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->MaxWalkSpeed = 400.f;
}

void AWOTOLHeroCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (UWOTOLGameInstance* GI = Cast<UWOTOLGameInstance>(GetGameInstance()))
	{
		Faction = GI->SelectedFaction;
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
