#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "WOTOLCityCamera.generated.h"

class UCameraComponent;
class USpringArmComponent;

// ─────────────────────────────────────────────────────────────────────────────
// CAMÉRA ISOMÉTRIQUE FIXE DE LA VUE CITÉ — l'angle (yaw/pitch) ne change JAMAIS (contrairement
// à AWOTOLBattleCamera, qui s'oriente librement) : seuls le PAN (ZQSD/WASD/flèches, dans le
// plan horizontal, borné autour du hub) et le ZOOM (molette, projection ORTHOGRAPHIQUE — pas
// de distorsion perspective, look "city-builder" classique) sont possibles. Possédée
// uniquement pendant EDemoScreen::City (AWOTOLDemoDirector::PossessCityCamera).
// ─────────────────────────────────────────────────────────────────────────────
UCLASS()
class WOTOL_API AWOTOLCityCamera : public APawn
{
	GENERATED_BODY()

public:
	AWOTOLCityCamera();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* Input) override;

	// Recentre le pan sur le hub et remet un zoom par défaut. Appelée à chaque entrée dans
	// la vue cité (AWOTOLDemoDirector::PossessCityCamera) — la vue repart toujours du même
	// cadrage d'ensemble, comme un retour au menu dans la plupart des city-builders.
	UFUNCTION(BlueprintCallable, Category = "City|Camera")
	void ResetToHub(const FVector& InHubLocation);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Camera")
	float FixedYaw = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Camera")
	float FixedPitch = -55.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Camera")
	float PanSpeed = 1400.f;

	// Distance maximale du pan par rapport au hub (le joueur ne peut pas sortir de la cité).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Camera")
	float PanRadius = 2300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Camera")
	float DefaultOrthoWidth = 2400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Camera")
	float MinOrthoWidth = 1100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Camera")
	float MaxOrthoWidth = 4200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Camera")
	float ZoomStep = 260.f;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UCameraComponent> Camera;

private:
	FVector   HubLocation = FVector::ZeroVector;
	FVector2D PanInput = FVector2D::ZeroVector;
	float     ZoomInput = 0.f;
	float     TargetOrthoWidth = 2400.f;

	void InputPanForward(float V);
	void InputPanBackward(float V);
	void InputPanRight(float V);
	void InputPanLeft(float V);
	void InputZoom(float V);
};
