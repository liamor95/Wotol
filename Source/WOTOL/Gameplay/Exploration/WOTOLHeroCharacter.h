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

	UFUNCTION(BlueprintCallable, Category = "Hero")
	void SetLoadout(UHeroLoadoutDataAsset* Loadout);

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
};
