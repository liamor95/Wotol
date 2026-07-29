#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Data/WOTOLTypes.h"
#include "WOTOLHeroCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class USceneComponent;
class UHeroLoadoutDataAsset;
class AWOTOLDemoUnit;

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

	// ── Zoom caméra (molette), même convention que AWOTOLBattleCamera : clic par cran,
	// accélère plus on est loin, borné [MinArmLength, MaxArmLength] — demande explicite de
	// Liamor du 26/07/2026 ("zoom et dézoom jusqu'à un certain maximum").
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom")
	float MinArmLength = 180.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom")
	float MaxArmLength = 900.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom")
	float ZoomSpeed = 60.f;

	// ── Mode ACTION (26/07/2026, demande de Liamor) : attaque de base (touche F/Entrée,
	// réutilise l'action "Interact" déjà déclarée mais jamais branchée) + compétence (touche R,
	// même touche que l'activation de compétence en bataille RTS — cohérence d'entrée entre
	// les deux modes). Feedback visuel uniquement (texte flottant) : ne modifie PAS les PV
	// réels du Kraken avant la bataille RTS qui suit (calibrage des dégâts déjà fait pour
	// cette bataille, on ne veut pas le fausser depuis un geste d'exploration). ──
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hero|Action")
	float AttackRange = 320.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hero|Action")
	float AttackCooldown = 0.6f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hero|Action")
	float AbilityCooldown = 4.f;

	UFUNCTION(BlueprintPure, Category = "Hero|Action")
	bool IsAttackReady() const { return AttackCooldownRemaining <= 0.f; }
	UFUNCTION(BlueprintPure, Category = "Hero|Action")
	bool IsAbilityReady() const { return AbilityCooldownRemaining <= 0.f; }

	UFUNCTION(BlueprintPure, Category = "Hero")
	UHeroLoadoutDataAsset* GetLoadout() const { return CurrentLoadout; }

	UFUNCTION(BlueprintPure, Category = "Hero")
	EFactionID GetFaction() const { return Faction; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera;

	void InputZoom(float V);
	void TickZoom(float DeltaSeconds);
	float ZoomInput = 0.f;

	void PerformAttack();
	void PerformAbility();
	void TryHitFeedback(float Range, const TCHAR* Label);
	float AttackCooldownRemaining  = 0.f;
	float AbilityCooldownRemaining = 0.f;
	float AttackLungeRemaining     = 0.f; // petite impulsion avant, feedback physique du coup

	// ── Corps détaillé par faction (26/07/2026) : remplace la silhouette greybox unique.
	// Un seul héros à afficher pendant l'exploration (pas d'armée, pas le Kraken en phase
	// active) -> aucune contrainte de perf comme pour les 100 unités de la grande bataille,
	// donc détail poussé au maximum, fidèle au Chef de la faction (palette/traits identiques
	// à BuildAquiKnight pour Aquiloris, au bloc Noxar pour Noxéens, cf. WOTOLDemoUnit.cpp). ──
	void BuildHeroBody();
	void AnimateSwim(float DeltaSeconds);

	UStaticMeshComponent* AddPart(const TCHAR* MeshPath, const FVector& RelLoc,
		const FVector& RelScale, const FRotator& RelRot, const FLinearColor& Color);
	USceneComponent* MakeJoint(USceneComponent* Parent, const FVector& RelLoc);
	UStaticMeshComponent* MakeBone(USceneComponent* Joint, const TCHAR* MeshPath,
		const FVector& Offset, const FVector& Scale, const FRotator& Rot, const FLinearColor& Color);

	UPROPERTY() TObjectPtr<USceneComponent> JRShoulder;
	UPROPERTY() TObjectPtr<USceneComponent> JRElbow;
	UPROPERTY() TObjectPtr<USceneComponent> JLShoulder;
	UPROPERTY() TObjectPtr<USceneComponent> JLElbow;
	UPROPERTY() TObjectPtr<USceneComponent> JRHip;
	UPROPERTY() TObjectPtr<USceneComponent> JRKnee;
	UPROPERTY() TObjectPtr<USceneComponent> JLHip;
	UPROPERTY() TObjectPtr<USceneComponent> JLKnee;
	float SwimAnimTime = 0.f;

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
