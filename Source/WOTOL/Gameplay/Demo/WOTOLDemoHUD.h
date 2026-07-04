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

	// Flux d'écrans
	static FBox2D StartGameButtonRect(float W, float H);      // menu principal
	static FBox2D FactionButtonRect(int32 Index, float W, float H); // 0=Aquiloris 1=Noxeens
	static FBox2D LaunchBattleButtonRect(float W, float H);   // préparation

	// Boutons de couche verticale (nage) — montent/descendent la sélection
	static FBox2D LayerUpButtonRect(float W, float H);
	static FBox2D LayerDownButtonRect(float W, float H);

	// Écran de RÉSUMÉ de bataille
	static FBox2D SummaryContinueButtonRect(float W, float H);      // phase 1 -> phase 2
	static FBox2D SummaryReplayButtonRect(float W, float H);        // final : rejouer
	static FBox2D SummaryChangeFactionButtonRect(float W, float H); // final : changer de faction
	static FBox2D SummaryQuitButtonRect(float W, float H);          // final : quitter
	static FBox2D InterludeContinueButtonRect(float W, float H);    // transition -> phase 2

private:
	void DrawCenteredText(const FString& Text, float Y, const FLinearColor& Color, float Scale);
	void DrawPauseButton(float W, float H);
	void DrawPauseOverlay(float W, float H);

	// Écrans du flux (menu / faction / préparation)
	void DrawMainMenu(float W, float H);
	void DrawFactionSelect(float W, float H);
	void DrawPrepareBar(float W, float H);
	void DrawSummary(float W, float H, class UDemoFlowSubsystem* Demo);
	void DrawInterlude(float W, float H, class UDemoFlowSubsystem* Demo);
	void DrawBuildingBar(float W, float H, class AWOTOLCaptureObject* Building);
	void DrawButton(const FBox2D& R, const FString& Label, const FLinearColor& Tint, float TextScale = 1.3f);

	// Éléments style Total War
	void DrawTopBar(float W, float H, class UWorld* World, class UDemoFlowSubsystem* Demo);
	void DrawBossBar(float W, float H, class AWOTOLDemoUnit* Boss);
	void DrawCommandBar(float W, float H, class UWorld* World);
	// Petite barre encadrée générique (fond + remplissage + cadre).
	void DrawBar(float X, float Y, float BarW, float BarH, float Pct,
		const FLinearColor& Fill, const FLinearColor& Back);
};
