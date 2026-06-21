#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "WOTOLBattleCamera.generated.h"

class UCameraComponent;
class USpringArmComponent;

// Caméra isométrique de la bataille tactique
// Spawné et possédé par le PlayerController_Battle au BeginPlay
UCLASS()
class WOTOL_API AWOTOLBattleCamera : public APawn
{
	GENERATED_BODY()

public:
	AWOTOLBattleCamera();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* Input) override;

	// Bornes de déplacement de la caméra dans le niveau (à configurer dans l'éditeur)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Bounds")
	FVector2D PanBoundsMin = FVector2D(-5000.f, -5000.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Bounds")
	FVector2D PanBoundsMax = FVector2D(5000.f, 5000.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Speed")
	float PanSpeed = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Speed")
	float ZoomSpeed = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom")
	float MinArmLength = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom")
	float MaxArmLength = 2500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Angle")
	float PitchAngle = -55.f;

	// Centre la caméra sur une position monde
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void FocusOn(FVector WorldLocation);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UCameraComponent> Camera;

private:
	FVector2D PanInput;
	float     ZoomInput = 0.f;

	void PanForward(float Value);
	void PanRight(float Value);
	void Zoom(float Value);
	void ClampPosition();
};
