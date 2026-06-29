#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "WOTOLDemoHUD.generated.h"

// HUD de la démo dessiné 100% en C++ (Canvas) — AUCUN widget UMG requis.
// Affiche : la boîte de sélection (rectangle de drag), le message d'objectif
// au centre-haut, et l'écran de fin de démo. Branché auto par WOTOLGameMode_Demo.
UCLASS()
class WOTOL_API AWOTOLDemoHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawCenteredText(const FString& Text, float Y, const FLinearColor& Color, float Scale);
};
