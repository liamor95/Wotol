#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Data/WOTOLTypes.h"
#include "AIAdaptiveController.generated.h"

class UUnitAIStateComponent;
class AUnitBase;

// Ordres RTS qu'un joueur ou un supérieur peut envoyer à une unité
UENUM(BlueprintType)
enum class ERTSOrder : uint8
{
	None,
	Move,           // Déplacement simple vers un point
	AttackMove,     // Avancer et engager tous les ennemis en chemin
	AttackTarget,   // Focaliser une cible précise
	HoldPosition,   // Tenir la position — pas de poursuite
	UseAbility,     // Activer une compétence
	ChangeLayer,    // Changer de couche verticale (WOTOL)
	Retreat         // Retraite vers le point de spawn
};

// Contrôleur IA RTS — inspiration Total War / Bannerlord
// - L'IA tourne EN CONTINU pendant toute la phase Tactical (pas de tours)
// - Reçoit des ordres du PlayerController (move, attack, ability, layer change)
// - Adapte son comportement selon le profil comportemental du joueur
// - PAS d'abonnement à un TacticalPhaseManager (système tour par tour supprimé)
UCLASS()
class WOTOL_API UAIAdaptiveController : public AAIController
{
	GENERATED_BODY()

public:
	UAIAdaptiveController();

	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	// ─── Activation RTS ───────────────────────────────────────────────────────

	// Appelé par URTSBattleManager au démarrage du combat — active l'IA immédiatement
	UFUNCTION(BlueprintCallable, Category = "AI|RTS")
	void ActivateRTSBehavior();

	// Désactive l'IA (fin de bataille, mort de l'unité)
	UFUNCTION(BlueprintCallable, Category = "AI|RTS")
	void DeactivateRTSBehavior();

	// ─── Ordres RTS (reçus du PlayerController ou d'un commandant IA) ─────────

	// Déplacer vers un point (clic droit sur terrain)
	UFUNCTION(BlueprintCallable, Category = "AI|Orders")
	void IssueOrder_Move(FVector TargetLocation);

	// Avancer et attaquer tout ennemi en chemin (shift+clic droit ou A+clic)
	UFUNCTION(BlueprintCallable, Category = "AI|Orders")
	void IssueOrder_AttackMove(FVector TargetLocation);

	// Focaliser une cible unique (clic droit sur unité ennemie)
	UFUNCTION(BlueprintCallable, Category = "AI|Orders")
	void IssueOrder_AttackTarget(AUnitBase* Target);

	// Tenir la position — l'unité attaque à portée mais ne poursuit pas
	UFUNCTION(BlueprintCallable, Category = "AI|Orders")
	void IssueOrder_HoldPosition();

	// Activer une compétence (touche 1/2/3 du joueur)
	UFUNCTION(BlueprintCallable, Category = "AI|Orders")
	void IssueOrder_UseAbility(int32 AbilityIndex, FVector TargetLocation, AUnitBase* TargetUnit);

	// Changer de couche verticale — mécanique propre à WOTOL
	UFUNCTION(BlueprintCallable, Category = "AI|Orders")
	void IssueOrder_ChangeLayer(EVerticalLayer NewLayer);

	// Retraite tactique vers le spawn
	UFUNCTION(BlueprintCallable, Category = "AI|Orders")
	void IssueOrder_Retreat();

	// ─── Adaptation comportementale ───────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category = "AI|Adaptation")
	void SetControlledFaction(EFactionID InFaction);

	UFUNCTION(BlueprintCallable, Category = "AI|Adaptation")
	void UpdatePlayerProfile(const FPlayerBehaviorProfile& Profile);

	// ─── Lecture état ─────────────────────────────────────────────────────────

	UFUNCTION(BlueprintPure, Category = "AI")
	ERTSOrder GetCurrentOrder() const { return CurrentOrder; }

	UFUNCTION(BlueprintPure, Category = "AI")
	bool IsAIActive() const { return bAIActive; }

	// Paramètres calculés dynamiquement depuis le profil joueur
	UPROPERTY(BlueprintReadOnly, Category = "AI")
	float ComputedAggressionLevel = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "AI")
	float ComputedCautionLevel = 0.5f;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "AI")
	EFactionID ControlledFaction = EFactionID::None;

private:
	void AdaptToPlayerProfile(const FPlayerBehaviorProfile& Profile);
	void SetAIStateActive(bool bActive);
	UUnitAIStateComponent* GetStateComponent() const;

	ERTSOrder CurrentOrder = ERTSOrder::None;
	bool      bAIActive    = false;
};
