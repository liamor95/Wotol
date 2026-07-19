#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Data/WOTOLTypes.h"
#include "WOTOLHeroCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class UHeroLoadoutDataAsset;

// Héros contrôlé par le joueur en mode exploration (vue troisième personne)
UCLASS()
class WOTOL_API AWOTOLHeroCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AWOTOLHeroCharacter();

	virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Hero")
	void SetLoadout(UHeroLoadoutDataAsset* Loadout);

	// ── SENSATION ACTION (distincte des phases de bataille tactique) ────────────────
	// Vitesse de nage normale / en sprint (Alt gauche maintenu).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hero|Action")
	float SwimSpeed = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hero|Action")
	float SprintSpeed = 1050.f;

	// Ruée courte (clic gauche) : impulsion instantanée dans l'axe de déplacement courant,
	// avec temps de recharge — donne un vrai geste "action" au joueur, pas juste une nage.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hero|Action")
	float DashImpulse = 2600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hero|Action")
	float DashCooldown = 2.2f;

	UFUNCTION(BlueprintPure, Category = "Hero|Action")
	bool IsDashReady() const { return DashCooldownRemaining <= 0.f; }

	UFUNCTION(BlueprintPure, Category = "Hero")
	UHeroLoadoutDataAsset* GetLoadout() const { return CurrentLoadout; }

	UFUNCTION(BlueprintPure, Category = "Hero")
	EFactionID GetFaction() const { return Faction; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera;

	// Silhouette greybox visible même sans Blueprint/mesh importé. Elle sera remplacée par
	// le véritable héros, mais rend déjà la nage testable dans un niveau procédural vide.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hero|Greybox")
	TObjectPtr<UStaticMeshComponent> GreyboxBody;

	UPROPERTY(BlueprintReadOnly, Category = "Hero")
	TObjectPtr<UHeroLoadoutDataAsset> CurrentLoadout;

	UPROPERTY(BlueprintReadOnly, Category = "Hero")
	EFactionID Faction = EFactionID::None;

	// Mouvement (nage en volume)
	void MoveForward(float Value);
	void MoveRight(float Value);
	void MoveUp(float Value); // montée / descente verticale

	void StartSprint();
	void StopSprint();
	void PerformDash();

	bool  bIsSprinting = false;
	float DashCooldownRemaining = 0.f;
	// Valeurs d'axe BRUTES du tick courant (pas un vecteur accumulé) : MoveForward et
	// MoveRight sont deux callbacks indépendants dont l'ordre d'appel n'est pas garanti ;
	// la direction de ruée est recomposée à la demande à partir de ces deux valeurs.
	float CurrentForwardInput = 0.f;
	float CurrentLateralInput = 0.f; // aussi utilisé pour l'inclinaison (banking) en Tick
	float CurrentBankRoll = 0.f;
	float BaseFOV = 90.f;

	static constexpr float DashFOVPunchDuration = 0.35f;
	float DashFOVPunchRemaining = 0.f;
};
