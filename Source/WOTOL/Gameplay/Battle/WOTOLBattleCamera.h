#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "WOTOLBattleCamera.generated.h"

class UCameraComponent;
class USpringArmComponent;
class AUnitBase;

// Caméra de bataille libre 3D — Total War / Bannerlord style
// Sous-marin → verticalité complète, 4 couches de profondeur navigables
//
// Contrôles :
//   WASD / flèches      → pan horizontal
//   Q / E               → descente / montée verticale (profondeur)
//   Molette             → zoom avant/arrière
//   Clic milieu + drag  → rotation Yaw (orbite horizontale)
//   Clic droit + drag   → rotation Pitch (inclinaison)
//   F                   → focus sur sélection
//   Bord écran          → edge scrolling (optionnel, activable)
UCLASS()
class WOTOL_API AWOTOLBattleCamera : public APawn
{
	GENERATED_BODY()

public:
	AWOTOLBattleCamera();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* Input) override;

	// ─── Limites de la carte ──────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Bounds")
	// Z borné AU-DESSUS DU SOL de l'arène (top du sol ≈ 0) : la caméra peut descendre au ras
	// du sol mais JAMAIS passer sous la carte (le pivot descendait à -14000 -> vue « sous la map »
	// en contre-plongée, défaut visuel). La verticalité de la greybox est VISUELLE (les unités
	// montent depuis le sol), donc la caméra n'a jamais besoin d'aller sous le plateau.
	FVector BoundsMin = FVector(-8000.f, -8000.f, 20.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Bounds")
	FVector BoundsMax = FVector(8000.f, 8000.f, 3400.f);    // assez haut pour SUIVRE une unité
	                                                        // montée jusqu'à la couche max (~2400)

	// ─── Vitesses ─────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Speed")
	float PanSpeed = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Speed")
	float VerticalSpeed = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Speed")
	float RotationSpeed = 120.f;   // degrés/s (clic milieu + drag)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Speed")
	float ZoomSpeed = 650.f;   // zoom molette réactif (par cran)

	// ─── Zoom ─────────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom")
	float MinArmLength = 300.f;    // très proche des unités

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom")
	float MaxArmLength = 6000.f;   // vue globale du champ de bataille

	// ─── Angles ───────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Angle")
	float DefaultPitch = -50.f;    // vue 3/4 vers le bas au départ

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Angle")
	float MinPitch = -89.f;        // vue quasi verticale

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Angle")
	float MaxPitch = -10.f;        // vue quasi horizontale (cinématique)

	// ─── Edge scrolling ───────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|EdgeScroll")
	bool bEdgeScrollEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|EdgeScroll")
	float EdgeScrollZonePercent = 0.03f; // 3% du bord écran

	// ─── API publique ─────────────────────────────────────────────────────────

	// Centre la caméra sur un point monde (conserve la hauteur courante)
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void FocusOn(FVector WorldLocation);

	// Centre ET zoome de près sur une unité (double-clic) — pivot sur l'unité,
	// distance de bras réduite pour la voir à ~quelques mètres.
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void FocusOnUnitClose(FVector WorldLocation, float ArmLength);

	// Vue initiale : place le pivot sur Focus, oriente la caméra (yaw/pitch) et
	// règle la distance de zoom. Utilisé au lancement pour cadrer l'armée du joueur.
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void SetInitialView(FVector Focus, float Yaw, float Pitch, float ArmLength);

	// SUIVI (troisième personne) : la caméra se recule (ArmLength) et SUIT en continu
	// le centre du groupe d'unités sans avoir à bouger la souris. Rotation/zoom restent
	// libres pendant le suivi. Le suivi se coupe si l'on pan manuellement (WASD) ou via
	// StopFollow() (clic sur le terrain).
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void FollowGroup(const TArray<AUnitBase*>& Units, float ArmLength);

	UFUNCTION(BlueprintCallable, Category = "Camera")
	void StopFollow();

	UFUNCTION(BlueprintPure, Category = "Camera")
	bool IsFollowing() const { return bFollowing; }

	// Aller directement à une couche verticale (Épipélagique = 0, Hadal = -12000)
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void FocusOnLayer(float TargetZ);

	UFUNCTION(BlueprintPure, Category = "Camera")
	float GetCurrentZoom() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UCameraComponent> Camera;

private:
	// Inputs accumulés ce frame
	FVector2D PanInput      = FVector2D::ZeroVector;
	float     VerticalInput = 0.f;
	float     ZoomInput     = 0.f;
	float     YawInput      = 0.f;
	float     PitchInput    = 0.f;

	// État rotation par drag
	bool      bRotatingYaw   = false; // bouton milieu
	bool      bRotatingPitch = false; // bouton droit (optionnel)
	FVector2D LastMousePos;

	// ─── Suivi de groupe (troisième personne) ──────────────────────────────────
	bool bFollowing = false;
	TArray<TWeakObjectPtr<AUnitBase>> FollowUnits;
	void TickFollow(float DT);

	// Rotation courante du bras (indépendante du contrôleur)
	float CurrentYaw   = -45.f;
	float CurrentPitch = -50.f;

	// ─── Bindings ─────────────────────────────────────────────────────────────
	void InputPanForward(float V);
	void InputPanBackward(float V);
	void InputPanRight(float V);
	void InputPanLeft(float V);
	void InputVertical(float V);
	void InputVerticalDown(float V);
	void InputZoom(float V);
	void InputMiddleMousePressed();
	void InputMiddleMouseReleased();
	void InputMouseX(float V);
	void InputMouseY(float V);

	// ─── Update ───────────────────────────────────────────────────────────────
	void TickPan(float DT);
	void TickZoom(float DT);
	void TickEdgeScroll(float DT);
	void TickRotation(float DT);
	void ClampPosition();
	void ApplyArmRotation();

	APlayerController* GetOwnerPC() const;
};
