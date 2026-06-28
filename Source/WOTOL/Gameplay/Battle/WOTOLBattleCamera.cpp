#include "WOTOLBattleCamera.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameViewportClient.h"

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

	// Pan
	Input->BindAxis("CameraPanForward", this, &AWOTOLBattleCamera::InputPanForward);
	Input->BindAxis("CameraPanRight",   this, &AWOTOLBattleCamera::InputPanRight);

	// Vertical (Q/E)
	Input->BindAxis("CameraVertical",   this, &AWOTOLBattleCamera::InputVertical);

	// Zoom
	Input->BindAxis("CameraZoom",       this, &AWOTOLBattleCamera::InputZoom);

	// Rotation — drag
	Input->BindAxis("MouseX",           this, &AWOTOLBattleCamera::InputMouseX);
	Input->BindAxis("MouseY",           this, &AWOTOLBattleCamera::InputMouseY);

	// Middle mouse — yaw rotation
	Input->BindAction("CameraRotateYaw",   IE_Pressed,  this, &AWOTOLBattleCamera::InputMiddleMousePressed);
	Input->BindAction("CameraRotateYaw",   IE_Released, this, &AWOTOLBattleCamera::InputMiddleMouseReleased);
}

void AWOTOLBattleCamera::Tick(float DT)
{
	Super::Tick(DT);

	TickRotation(DT);
	TickPan(DT);
	TickZoom(DT);

	if (bEdgeScrollEnabled)
	{
		TickEdgeScroll(DT);
	}

	ClampPosition();
	ApplyArmRotation();

	// Reset accumulated inputs
	PanInput      = FVector2D::ZeroVector;
	VerticalInput = 0.f;
	ZoomInput     = 0.f;
}

// ─── Input bindings ───────────────────────────────────────────────────────────

void AWOTOLBattleCamera::InputPanForward(float V)  { PanInput.X += V; }
void AWOTOLBattleCamera::InputPanRight(float V)    { PanInput.Y += V; }
void AWOTOLBattleCamera::InputVertical(float V)    { VerticalInput += V; }
void AWOTOLBattleCamera::InputZoom(float V)        { ZoomInput += V; }

void AWOTOLBattleCamera::InputMiddleMousePressed()
{
	bRotatingYaw = true;
	if (APlayerController* PC = GetOwnerPC())
	{
		PC->GetMousePosition(LastMousePos.X, LastMousePos.Y);
	}
}

void AWOTOLBattleCamera::InputMiddleMouseReleased()
{
	bRotatingYaw = false;
}

void AWOTOLBattleCamera::InputMouseX(float V)
{
	if (bRotatingYaw)   YawInput   += V;
	if (bRotatingPitch) PitchInput -= V;
}

void AWOTOLBattleCamera::InputMouseY(float V)
{
	if (bRotatingPitch) PitchInput += V;
}

// ─── Tick helpers ─────────────────────────────────────────────────────────────

void AWOTOLBattleCamera::TickPan(float DT)
{
	if (PanInput.IsNearlyZero() && FMath::IsNearlyZero(VerticalInput)) return;

	// Pan is relative to current yaw so WASD always moves in the viewed direction
	const FRotator YawRot(0.f, CurrentYaw, 0.f);
	const FVector  ForwardDir = FRotationMatrix(YawRot).GetScaledAxis(EAxis::X);
	const FVector  RightDir   = FRotationMatrix(YawRot).GetScaledAxis(EAxis::Y);

	FVector Delta = ForwardDir * PanInput.X * PanSpeed * DT
	              + RightDir   * PanInput.Y * PanSpeed * DT
	              + FVector::UpVector * VerticalInput * VerticalSpeed * DT;

	AddActorWorldOffset(Delta);
}

void AWOTOLBattleCamera::TickZoom(float DT)
{
	if (FMath::IsNearlyZero(ZoomInput) || !SpringArm) return;

	SpringArm->TargetArmLength = FMath::Clamp(
		SpringArm->TargetArmLength - ZoomInput * ZoomSpeed * DT,
		MinArmLength, MaxArmLength);
}

void AWOTOLBattleCamera::TickEdgeScroll(float DT)
{
	APlayerController* PC = GetOwnerPC();
	if (!PC || !PC->GetLocalPlayer()) return;

	UGameViewportClient* Viewport = GetWorld()->GetGameViewport();
	if (!Viewport) return;

	FVector2D ViewportSize;
	Viewport->GetViewportSize(ViewportSize);
	if (ViewportSize.IsNearlyZero()) return;

	float MouseX, MouseY;
	if (!PC->GetMousePosition(MouseX, MouseY)) return;

	const float ZoneX = ViewportSize.X * EdgeScrollZonePercent;
	const float ZoneY = ViewportSize.Y * EdgeScrollZonePercent;

	FVector2D EdgeDir = FVector2D::ZeroVector;
	if (MouseX < ZoneX)                     EdgeDir.Y = -1.f;
	else if (MouseX > ViewportSize.X - ZoneX) EdgeDir.Y =  1.f;
	if (MouseY < ZoneY)                     EdgeDir.X =  1.f;
	else if (MouseY > ViewportSize.Y - ZoneY) EdgeDir.X = -1.f;

	if (!EdgeDir.IsNearlyZero())
	{
		const FRotator YawRot(0.f, CurrentYaw, 0.f);
		const FVector  ForwardDir = FRotationMatrix(YawRot).GetScaledAxis(EAxis::X);
		const FVector  RightDir   = FRotationMatrix(YawRot).GetScaledAxis(EAxis::Y);

		AddActorWorldOffset(
			ForwardDir * EdgeDir.X * PanSpeed * DT +
			RightDir   * EdgeDir.Y * PanSpeed * DT);
	}
}

void AWOTOLBattleCamera::TickRotation(float DT)
{
	if (!FMath::IsNearlyZero(YawInput))
	{
		CurrentYaw += YawInput * RotationSpeed * DT;
		YawInput = 0.f;
	}

	if (!FMath::IsNearlyZero(PitchInput))
	{
		CurrentPitch = FMath::Clamp(
			CurrentPitch + PitchInput * RotationSpeed * DT,
			MinPitch, MaxPitch);
		PitchInput = 0.f;
	}
}

void AWOTOLBattleCamera::ClampPosition()
{
	FVector Loc = GetActorLocation();
	Loc.X = FMath::Clamp(Loc.X, BoundsMin.X, BoundsMax.X);
	Loc.Y = FMath::Clamp(Loc.Y, BoundsMin.Y, BoundsMax.Y);
	Loc.Z = FMath::Clamp(Loc.Z, BoundsMin.Z, BoundsMax.Z);
	SetActorLocation(Loc);
}

void AWOTOLBattleCamera::ApplyArmRotation()
{
	if (SpringArm)
	{
		SpringArm->SetRelativeRotation(FRotator(CurrentPitch, CurrentYaw, 0.f));
	}
}

// ─── API publique ─────────────────────────────────────────────────────────────

void AWOTOLBattleCamera::FocusOn(FVector WorldLocation)
{
	WorldLocation.Z = GetActorLocation().Z;
	SetActorLocation(WorldLocation);
	ClampPosition();
}

void AWOTOLBattleCamera::FocusOnLayer(float TargetZ)
{
	FVector Loc = GetActorLocation();
	Loc.Z = FMath::Clamp(TargetZ, BoundsMin.Z, BoundsMax.Z);
	SetActorLocation(Loc);
}

float AWOTOLBattleCamera::GetCurrentZoom() const
{
	return SpringArm ? SpringArm->TargetArmLength : 0.f;
}

APlayerController* AWOTOLBattleCamera::GetOwnerPC() const
{
	return Cast<APlayerController>(GetController());
}
