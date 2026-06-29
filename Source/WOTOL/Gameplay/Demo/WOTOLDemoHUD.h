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

	// ─── Zones cliquables (source de vérité partagée HUD ↔ PlayerController) ───
	// Bouton pause (deux barres) en haut à droite.
	static FBox2D PauseButtonRect(float W, float H);
	// Boutons du menu pause (0 = Reprendre, 1 = Recommencer, 2 = Quitter).
	static FBox2D MenuButtonRect(int32 Index, float W, float H);

private:
	void DrawCenteredText(const FString& Text, float Y, const FLinearColor& Color, float Scale);
	void DrawPauseButton(float W, float H);
	void DrawPauseOverlay(float W, float H);
};
