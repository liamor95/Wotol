#include "WOTOLBattleCamera.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameViewportClient.h"
#include "Components/InputComponent.h"
#include "InputCoreTypes.h"

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

	// Bindings DIRECTS sur les touches (fonctionnent SANS config Input du projet)
	Input->BindAxisKey(EKeys::W,              this, &AWOTOLBattleCamera::InputPanForward);
	Input->BindAxisKey(EKeys::S,              this, &AWOTOLBattleCamera::InputPanBackward);
	Input->BindAxisKey(EKeys::D,              this, &AWOTOLBattleCamera::InputPanRight);
	Input->BindAxisKey(EKeys::A,              this, &AWOTOLBattleCamera::InputPanLeft);
	// Flèches directionnelles = même chose que WASD
	Input->BindAxisKey(EKeys::Up,             this, &AWOTOLBattleCamera::InputPanForward);
	Input->BindAxisKey(EKeys::Down,           this, &AWOTOLBattleCamera::InputPanBackward);
	Input->BindAxisKey(EKeys::Right,          this, &AWOTOLBattleCamera::InputPanRight);
	Input->BindAxisKey(EKeys::Left,           this, &AWOTOLBattleCamera::InputPanLeft);
	Input->BindAxisKey(EKeys::E,              this, &AWOTOLBattleCamera::InputVertical);
	Input->BindAxisKey(EKeys::Q,              this, &AWOTOLBattleCamera::InputVerticalDown);
	Input->BindAxisKey(EKeys::MouseWheelAxis, this, &AWOTOLBattleCamera::InputZoom);
	Input->BindAxisKey(EKeys::MouseX,         this, &AWOTOLBattleCamera::InputMouseX);
	Input->BindAxisKey(EKeys::MouseY,         this, &AWOTOLBattleCamera::InputMouseY);
	Input->BindKey(EKeys::MiddleMouseButton, IE_Pressed,  this, &AWOTOLBattleCamera::InputMiddleMousePressed);
	Input->BindKey(EKeys::MiddleMouseButton, IE_Released, this, &AWOTOLBattleCamera::InputMiddleMouseReleased);
	// CLIC DROIT maintenu + glisser = pivoter la caméra (yaw + pitch), comme dans
	// l'éditeur Unreal et beaucoup de RTS. Le clic droit BREF reste un ordre (géré
	// par le PlayerController, qui distingue tap vs drag).
	Input->BindKey(EKeys::RightMouseButton, IE_Pressed,  this, &AWOTOLBattleCamera::InputMiddleMousePressed);
	Input->BindKey(EKeys::RightMouseButton, IE_Released, this, &AWOTOLBattleCamera::InputMiddleMouseReleased);
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
void AWOTOLBattleCamera::InputPanBackward(float V) { PanInput.X -= V; }
void AWOTOLBattleCamera::InputPanRight(float V)    { PanInput.Y += V; }
void AWOTOLBattleCamera::InputPanLeft(float V)     { PanInput.Y -= V; }
void AWOTOLBattleCamera::InputVertical(float V)    { VerticalInput += V; }
void AWOTOLBattleCamera::InputVerticalDown(float V){ VerticalInput -= V; }
void AWOTOLBattleCamera::InputZoom(float V)        { ZoomInput += V; }

void AWOTOLBattleCamera::InputMiddleMousePressed()
{
	// Orbite libre : clic-milieu glissé fait tourner la vue en yaw ET en pitch
	bRotatingYaw   = true;
	bRotatingPitch = true;
	if (APlayerController* PC = GetOwnerPC())
	{
		PC->GetMousePosition(LastMousePos.X, LastMousePos.Y);
	}
}

void AWOTOLBattleCamera::InputMiddleMouseReleased()
{
	bRotatingYaw   = false;
	bRotatingPitch = false;
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

	// Zoom par CRAN (pas d'échelle par DT) : chaque cran de molette rapproche
	// d'un pas franc, proportionnel à la distance actuelle (accélère de loin).
	const float Step = ZoomSpeed * (0.6f + SpringArm->TargetArmLength / MaxArmLength);
	SpringArm->TargetArmLength = FMath::Clamp(
		SpringArm->TargetArmLength - ZoomInput * Step,
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

void AWOTOLBattleCamera::SetInitialView(FVector Focus, float Yaw, float Pitch, float ArmLength)
{
	SetActorLocation(Focus);
	CurrentYaw   = Yaw;
	CurrentPitch = FMath::Clamp(Pitch, MinPitch, MaxPitch);
	if (SpringArm)
	{
		SpringArm->TargetArmLength = FMath::Clamp(ArmLength, MinArmLength, MaxArmLength);
	}
	ApplyArmRotation();
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
