#include "WOTOLBattleCamera.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"

AWOTOLBattleCamera::AWOTOLBattleCamera()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(Root);
	SpringArm->TargetArmLength         = 1200.f;
	SpringArm->bDoCollisionTest        = false;
	SpringArm->bUsePawnControlRotation = false;
	SpringArm->SetRelativeRotation(FRotator(PitchAngle, -45.f, 0.f));

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw   = false;
	bUseControllerRotationRoll  = false;
}

void AWOTOLBattleCamera::SetupPlayerInputComponent(UInputComponent* Input)
{
	Super::SetupPlayerInputComponent(Input);

	Input->BindAxis("CameraPanForward", this, &AWOTOLBattleCamera::PanForward);
	Input->BindAxis("CameraPanRight",   this, &AWOTOLBattleCamera::PanRight);
	Input->BindAxis("CameraZoom",       this, &AWOTOLBattleCamera::Zoom);
}

void AWOTOLBattleCamera::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!PanInput.IsZero())
	{
		const FVector Delta(PanInput.X * PanSpeed * DeltaSeconds,
		                    PanInput.Y * PanSpeed * DeltaSeconds, 0.f);
		AddActorWorldOffset(Delta);
		ClampPosition();
		PanInput = FVector2D::ZeroVector;
	}

	if (ZoomInput != 0.f && SpringArm)
	{
		SpringArm->TargetArmLength = FMath::Clamp(
			SpringArm->TargetArmLength + ZoomInput * ZoomSpeed * DeltaSeconds,
			MinArmLength, MaxArmLength);
		ZoomInput = 0.f;
	}
}

void AWOTOLBattleCamera::FocusOn(FVector WorldLocation)
{
	WorldLocation.Z = GetActorLocation().Z;
	SetActorLocation(WorldLocation);
	ClampPosition();
}

void AWOTOLBattleCamera::PanForward(float Value) { PanInput.X += Value; }
void AWOTOLBattleCamera::PanRight(float Value)   { PanInput.Y += Value; }
void AWOTOLBattleCamera::Zoom(float Value)        { ZoomInput  += Value; }

void AWOTOLBattleCamera::ClampPosition()
{
	FVector Loc = GetActorLocation();
	Loc.X = FMath::Clamp(Loc.X, PanBoundsMin.X, PanBoundsMax.X);
	Loc.Y = FMath::Clamp(Loc.Y, PanBoundsMin.Y, PanBoundsMax.Y);
	SetActorLocation(Loc);
}
