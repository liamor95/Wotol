#include "WOTOLCityCamera.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Components/InputComponent.h"
#include "InputCoreTypes.h"

AWOTOLCityCamera::AWOTOLCityCamera()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(Root);
	SpringArm->TargetArmLength         = 3200.f;
	SpringArm->bDoCollisionTest        = false;
	SpringArm->bUsePawnControlRotation = false;
	SpringArm->bEnableCameraLag         = true;
	SpringArm->CameraLagSpeed           = 8.f;
	SpringArm->SetRelativeRotation(FRotator(-55.f, 45.f, 0.f));

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;
	Camera->ProjectionMode = ECameraProjectionMode::Orthographic;
	Camera->OrthoWidth = 2400.f;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw   = false;
	bUseControllerRotationRoll  = false;
}

void AWOTOLCityCamera::ResetToHub(const FVector& InHubLocation)
{
	HubLocation = InHubLocation;
	SetActorLocation(InHubLocation);
	if (SpringArm) SpringArm->SetRelativeRotation(FRotator(FixedPitch, FixedYaw, 0.f));
	TargetOrthoWidth = FMath::Clamp(DefaultOrthoWidth, MinOrthoWidth, MaxOrthoWidth);
	if (Camera) Camera->OrthoWidth = TargetOrthoWidth;
	PanInput = FVector2D::ZeroVector;
	ZoomInput = 0.f;
}

void AWOTOLCityCamera::SetupPlayerInputComponent(UInputComponent* Input)
{
	Super::SetupPlayerInputComponent(Input);

	// Mêmes touches que la caméra de bataille (ZQSD/WASD/flèches), bindées DIRECTEMENT sur
	// les touches (pas besoin de config Input du projet — cohérent avec AWOTOLBattleCamera).
	Input->BindAxisKey(EKeys::Z,    this, &AWOTOLCityCamera::InputPanForward);
	Input->BindAxisKey(EKeys::W,    this, &AWOTOLCityCamera::InputPanForward);
	Input->BindAxisKey(EKeys::Up,   this, &AWOTOLCityCamera::InputPanForward);
	Input->BindAxisKey(EKeys::S,    this, &AWOTOLCityCamera::InputPanBackward);
	Input->BindAxisKey(EKeys::Down, this, &AWOTOLCityCamera::InputPanBackward);
	Input->BindAxisKey(EKeys::Q,    this, &AWOTOLCityCamera::InputPanLeft);
	Input->BindAxisKey(EKeys::A,    this, &AWOTOLCityCamera::InputPanLeft);
	Input->BindAxisKey(EKeys::Left, this, &AWOTOLCityCamera::InputPanLeft);
	Input->BindAxisKey(EKeys::D,     this, &AWOTOLCityCamera::InputPanRight);
	Input->BindAxisKey(EKeys::Right, this, &AWOTOLCityCamera::InputPanRight);
	Input->BindAxisKey(EKeys::MouseWheelAxis, this, &AWOTOLCityCamera::InputZoom);
}

void AWOTOLCityCamera::InputPanForward(float V)  { PanInput.X += V; }
void AWOTOLCityCamera::InputPanBackward(float V) { PanInput.X -= V; }
void AWOTOLCityCamera::InputPanRight(float V)    { PanInput.Y += V; }
void AWOTOLCityCamera::InputPanLeft(float V)     { PanInput.Y -= V; }
void AWOTOLCityCamera::InputZoom(float V)        { ZoomInput += V; }

void AWOTOLCityCamera::Tick(float DT)
{
	Super::Tick(DT);

	if (!PanInput.IsNearlyZero())
	{
		// Pan projeté sur le plan horizontal, aligné sur l'angle FIXE de la caméra (avancer =
		// vers le haut de l'écran, comme dans un city-builder isométrique classique).
		const FRotator YawRot(0.f, FixedYaw, 0.f);
		const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
		const FVector Right   = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
		FVector NewLoc = GetActorLocation()
			+ Forward * PanInput.X * PanSpeed * DT
			+ Right   * PanInput.Y * PanSpeed * DT;

		const float DistXY = FVector::DistXY(NewLoc, HubLocation);
		if (DistXY > PanRadius)
		{
			FVector Dir2D = NewLoc - HubLocation;
			Dir2D.Z = 0.f;
			Dir2D = Dir2D.GetSafeNormal();
			NewLoc = HubLocation + Dir2D * PanRadius + FVector(0.f, 0.f, NewLoc.Z - HubLocation.Z);
		}
		SetActorLocation(NewLoc);
	}

	if (!FMath::IsNearlyZero(ZoomInput))
	{
		TargetOrthoWidth = FMath::Clamp(TargetOrthoWidth - ZoomInput * ZoomStep,
			MinOrthoWidth, MaxOrthoWidth);
	}
	if (Camera)
	{
		Camera->OrthoWidth = FMath::FInterpTo(Camera->OrthoWidth, TargetOrthoWidth, DT, 8.f);
	}

	PanInput = FVector2D::ZeroVector;
	ZoomInput = 0.f;
}
