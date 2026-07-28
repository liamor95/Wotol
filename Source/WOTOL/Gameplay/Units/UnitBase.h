#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Data/WOTOLTypes.h"
#include "FormationComponent.h" // EFormationType (formations tactiques, ActiveFormationType)
#include "UnitBase.generated.h"

class UVerticalLayerComponent;
class UAbilityComponent;
class UUnitMoraleComponent;
class UUnitDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitDied,      AUnitBase*, Unit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, NewHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitSelected,  bool, bSelected);

UCLASS(Abstract)
class WOTOL_API AUnitBase : public ACharacter
{
	GENERATED_BODY()

public:
	AUnitBase();

	// ---- Data ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit")
	TObjectPtr<UUnitDataAsset> UnitData;

	UFUNCTION(BlueprintPure, Category = "Unit")
	UUnitDataAsset* GetUnitData() const { return UnitData; }

	// ---- Composants ----
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vertical")
	TObjectPtr<UVerticalLayerComponent> VerticalLayer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UAbilityComponent> AbilityComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Morale")
	TObjectPtr<UUnitMoraleComponent> MoraleComp;

	// ---- État combat ----
	UFUNCTION(BlueprintCallable, Category = "Combat")
	float GetHealthPercent() const;

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsAlive() const { return CurrentHealth > 0.f; }

	// Total de dégâts INFLIGÉS par cette unité (pour le résumé de bataille).
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float DamageDealt = 0.f;

	// Aveuglé jusqu'à ce temps (secondes de jeu) : précision quasi nulle (rate souvent).
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float BlindedUntil = 0.f;

	// RALENTI (encre du Kraken) jusqu'à ce temps : vitesse fortement réduite tant qu'on
	// est dans la flaque. Rafraîchi en continu par la zone d'encre.
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float SlowUntil = 0.f;

	// Vitesse de marche de base (mémorisée pour appliquer/retirer le ralenti).
	float BaseWalkSpeed = 0.f;

	// Multiplicateur des dégâts REÇUS (1 = normal). < 1 = avantage défensif (ex. le
	// défenseur de la phase 2 qui possède la zone/les cristaux -> encaisse moins).
	UPROPERTY(BlueprintReadWrite, Category = "Combat")
	float IncomingDamageMult = 1.f;

	// Multiplicateur des dégâts INFLIGÉS (1 = normal). > 1 = avantage offensif (ex. le
	// défenseur de la phase 2 galvanisé par sa zone/ses cristaux -> frappe plus fort).
	UPROPERTY(BlueprintReadWrite, Category = "Combat")
	float OutgoingDamageMult = 1.f;

	// Multiplicateur de dégâts d'ÉQUILIBRAGE (difficulté + camp joueur/ennemi), fixé AU SPAWN
	// et STABLE (jamais recalculé par frame -> ne se fait pas écraser par les synergies). C'est
	// le levier qui rend Facile/Difficile réels côté dégâts et équilibre la phase 3.
	UPROPERTY(BlueprintReadWrite, Category = "Combat")
	float BalanceDamageMult = 1.f;

	// Correcteur ADAPTATIF de rencontre, recalculé en temps réel par le DemoDirector selon
	// l'effectif réellement déployé, les pertes et le rythme du combat. Séparé du multiplicateur
	// de base pour ne jamais écraser les statistiques, la faction, les bâtiments ou la difficulté.
	UPROPERTY(BlueprintReadWrite, Category = "Combat")
	float AdaptiveOutgoingDamageMult = 1.f;

	UPROPERTY(BlueprintReadWrite, Category = "Combat")
	float AdaptiveIncomingDamageMult = 1.f;

	// Plancher de PV temporaire utilisé uniquement comme garde-fou de rencontre. À 0, aucun
	// changement. Le Kraken/l'unité d'ancrage ne peut pas mourir avant la plage de pertes
	// demandée ; les derniers survivants ne peuvent pas dépasser le maximum prévu.
	UPROPERTY(BlueprintReadWrite, Category = "Combat")
	float MinimumHealthFloor = 0.f;

	// Multiplicateur de dégâts issu d'une SYNERGIE de faction dynamique (1 = aucune).
	// STACKE avec OutgoingDamageMult sans l'écraser (ex. Aquiloryons protégés par un
	// bouclier + soutenus par une lance Aquilance derrière -> frappent plus fort).
	UPROPERTY(BlueprintReadWrite, Category = "Combat")
	float SynergyDamageMult = 1.f;

	// Multiplicateur du PROCHAIN coup uniquement (1 = normal), CONSOMMÉ après le coup.
	// Sert aux coups critiques ponctuels (ex. Aquilombres qui frappe dans le dos / depuis
	// la furtivité).
	UPROPERTY(BlueprintReadWrite, Category = "Combat")
	float NextHitCritMult = 1.f;

	// AURA (Léviaphénix) : buffs appliqués aux alliés proches. AuraDamageMult dope les
	// dégâts infligés ; AuraDefenseMult (<1) réduit les dégâts subis. Rafraîchis par l'aura
	// et décroissent doucement vers 1 quand l'unité sort du rayon (voir le Tick greybox).
	UPROPERTY(BlueprintReadWrite, Category = "Combat")
	float AuraDamageMult = 1.f;
	UPROPERTY(BlueprintReadWrite, Category = "Combat")
	float AuraDefenseMult = 1.f;

	// FORMATION TACTIQUE (26/07/2026, UFormationComponent branché) : type de formation assigné
	// au dernier ordre de groupe du joueur (Aucune par défaut -> aucun changement de
	// comportement pour qui n'utilise jamais les formations). FormationOrderDest est
	// l'emplacement assigné dans cette formation ; le bonus de DEF n'est appliqué QUE quand
	// l'unité est arrivée à cet emplacement (voir WOTOLDemoUnit::Tick) — comme dans les jeux de
	// référence, rompre les rangs fait perdre le bonus. Indépendant de FormationGroupId/
	// FormationSlot (cohésion PASSIVE hors combat, système différent, non modifié).
	UPROPERTY(BlueprintReadOnly, Category = "Formation")
	EFormationType ActiveFormationType = EFormationType::None;
	FVector FormationOrderDest = FVector::ZeroVector;
	// Multiplicateur de DEF de formation (même convention qu'AuraDefenseMult : <1 réduit les
	// dégâts subis, rafraîchi tant que l'unité est en position, décroît vers 1 sinon).
	UPROPERTY(BlueprintReadWrite, Category = "Formation")
	float FormationDefenseMult = 1.f;

	// ── Rythme de bataille (demo) — batailles plus LONGUES et sous-marines ──
	// Multiplicateur GLOBAL de dégâts (< 1 = combats plus longs, plus d'échanges).
	static float GlobalDamageScale;
	// Multiplicateur GLOBAL de vitesse de déplacement (< 1 = frottement de l'eau).
	static float GlobalSpeedScale;

	UFUNCTION(BlueprintPure, Category = "Combat")
	EFactionID GetFaction() const { return Faction; }

	// Inflige des dégâts ; valeur négative = soin.
	// VIRTUEL : certaines unités (ex. Aquis) interceptent les dégâts entrants (parade/
	// absorption par la lame photonique) avant d'appliquer le calcul de base.
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual float TakeDamageFromUnit(float Damage, AUnitBase* InstigatorUnit);

	// Vrai si l'unité est CACHÉE aux yeux des ennemis (ex. Aquilombres furtive) : l'IA
	// hostile ne la prend PAS pour cible. Le joueur, lui, continue de la voir (fantôme).
	UFUNCTION(BlueprintPure, Category = "Combat")
	virtual bool IsHiddenFromEnemies() const { return false; }

	// Déclenche une attaque vers la cible (appelé par l'IA ou le joueur)
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void PerformAttack(AUnitBase* Target);

	// Composant auquel accrocher les textes flottants (dégâts / esquive / parade) afin
	// qu'ils SUIVENT l'unité à sa hauteur de couche. Par défaut = RootComponent ; les
	// unités greybox renvoient leur VisualRoot (position visuelle réelle en hauteur).
	virtual class USceneComponent* GetFloatingTextAnchor() const { return RootComponent; }

	// Joue un système Niagara (référence SOFT, ex. asset Fab assigné sur la fiche d'unité) à un
	// endroit donné, s'il est assigné. No-op si le slot est vide -> le greybox reste par défaut.
	void PlayVFX(const TSoftObjectPtr<class UNiagaraSystem>& VFX, const FVector& Loc,
		const FRotator& Rot = FRotator::ZeroRotator);

	// Ancre d'affichage des DÉGÂTS/critiques/esquives. Par défaut = ancre de texte flottant.
	// Surchargée pour le Kraken (colosse) -> renvoie l'étiquette nom/PV, sinon les chiffres
	// apparaissaient au PIED du modèle géant, invisibles.
	virtual class USceneComponent* GetDamageTextAnchor() const { return GetFloatingTextAnchor(); }

	// ---- Sélection (joueur) ----
	UFUNCTION(BlueprintCallable, Category = "Selection")
	void SetSelected(bool bNewSelected);

	UFUNCTION(BlueprintPure, Category = "Selection")
	bool IsSelected() const { return bSelected; }

	// ---- Délégués ----
	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnUnitDied OnUnitDied;

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Selection")
	FOnUnitSelected OnUnitSelected;

	// Événement Blueprint pour l'animation d'attaque / FX
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
	void OnAttackPerformed(AUnitBase* Target);

	// Hook C++ déclenché à CHAQUE coup réellement porté (après le cooldown) : sert à ne
	// jouer l'animation d'attaque qu'au MOMENT du coup (pas pendant tout l'état Attacking).
	virtual void OnAttackAnimTrigger() {}

	// Événement Blueprint pour l'indicateur de sélection (décal, cercle, etc.)
	UFUNCTION(BlueprintImplementableEvent, Category = "Selection")
	void OnSelectionChanged(bool bNewSelected);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float CurrentHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit")
	EFactionID Faction = EFactionID::None;

private:
	void InitFromDataAsset();
	void Die();
	void SpawnProjectileToward(AUnitBase* Target, float OverrideDamage);

	float LastAttackTime  = -9999.f;
	bool  bSelected       = false;
};
