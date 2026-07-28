#include "WOTOLDemoHUD.h"
#include "DemoFlowSubsystem.h"
#include "WOTOLDemoUnit.h"
#include "WOTOLCaptureObject.h"
#include "WOTOLBuildingArt.h"
#include "OceanCurrentSubsystem.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Texture2D.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Gameplay/Battle/WOTOLPlayerController_Battle.h"
#include "Gameplay/Battle/WOTOLBattleCamera.h"
#include "Gameplay/Battle/RTSBattleManager.h"
#include "Gameplay/Battle/UnitSelectionManager.h"
#include "Gameplay/Units/UnitBase.h"
#include "Core/FactionRegistrySubsystem.h"
#include "Gameplay/Units/UnitDataAsset.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Data/WOTOLTypes.h"

FBox2D AWOTOLDemoHUD::PauseButtonRect(float W, float H)
{
	return FBox2D(FVector2D(W - 58.f, 14.f), FVector2D(W - 18.f, 50.f));
}

FBox2D AWOTOLDemoHUD::SettingsButtonRect(float W, float H)
{
	// Engrenage juste à GAUCHE du bouton pause.
	return FBox2D(FVector2D(W - 106.f, 14.f), FVector2D(W - 66.f, 50.f));
}

FBox2D AWOTOLDemoHUD::MenuButtonRect(int32 Index, float W, float H)
{
	const float BW = 280.f, BH = 54.f, Gap = 18.f;
	const float X = (W - BW) * 0.5f;
	const float Y0 = H * 0.50f; // decale vers le bas (place liberee pour la rangee vitesse de jeu)
	const float Y = Y0 + Index * (BH + Gap);
	return FBox2D(FVector2D(X, Y), FVector2D(X + BW, Y + BH));
}

FBox2D AWOTOLDemoHUD::StartGameButtonRect(float W, float H)
{
	const float BW = 340.f, BH = 64.f;
	// Placé BAS (près du bord inférieur) pour ne pas cacher le titre WOTOL + sous-titre.
	const float X = (W - BW) * 0.5f, Y = H * 0.86f;
	return FBox2D(FVector2D(X, Y), FVector2D(X + BW, Y + BH));
}

FBox2D AWOTOLDemoHUD::FactionButtonRect(int32 Index, float W, float H)
{
	// LARGES et ÉTALÉS : chaque bouton fait ~40% de la largeur -> on remplit l'écran au lieu
	// de tout tasser au centre. Gauche = Aquiloris, droite = Noxéens.
	const float BW = W * 0.40f, BH = H * 0.16f, Gap = W * 0.08f;
	const float TotalW = BW * 2.f + Gap;
	const float X = (W - TotalW) * 0.5f + Index * (BW + Gap);
	const float Y = H * 0.30f;
	return FBox2D(FVector2D(X, Y), FVector2D(X + BW, Y + BH));
}

FBox2D AWOTOLDemoHUD::DifficultyButtonRect(int32 Index, float W, float H)
{
	// 3 chips LARGES étalées sur la largeur (mêmes marges que les factions).
	const float BW = W * 0.26f, BH = H * 0.10f, Gap = W * 0.035f;
	const float TotalW = BW * 3.f + Gap * 2.f;
	const float X = (W - TotalW) * 0.5f + Index * (BW + Gap);
	const float Y = H * 0.68f;
	return FBox2D(FVector2D(X, Y), FVector2D(X + BW, Y + BH));
}

FBox2D AWOTOLDemoHUD::FactionLaunchButtonRect(float W, float H)
{
	const float BW = 420.f, BH = 62.f;
	const float X = (W - BW) * 0.5f, Y = H * 0.85f;
	return FBox2D(FVector2D(X, Y), FVector2D(X + BW, Y + BH));
}

FBox2D AWOTOLDemoHUD::HeroAquilorisVariantButtonRect(int32 Index, float W, float H)
{
	const float BW = W * 0.20f, BH = H * 0.06f, Gap = W * 0.02f;
	const float TotalW = BW * 2.f + Gap;
	const float X = (W - TotalW) * 0.5f + Index * (BW + Gap);
	const float Y = H * 0.185f;
	return FBox2D(FVector2D(X, Y), FVector2D(X + BW, Y + BH));
}

FBox2D AWOTOLDemoHUD::HeroHeritageButtonRect(int32 Index, float W, float H)
{
	const float BW = W * 0.20f, BH = H * 0.10f, Gap = W * 0.02f;
	const float TotalW = BW * 4.f + Gap * 3.f;
	const float X = (W - TotalW) * 0.5f + Index * (BW + Gap);
	const float Y = H * 0.34f;
	return FBox2D(FVector2D(X, Y), FVector2D(X + BW, Y + BH));
}

FBox2D AWOTOLDemoHUD::HeroSpecialtyButtonRect(int32 Index, float W, float H)
{
	const float BW = W * 0.20f, BH = H * 0.10f, Gap = W * 0.02f;
	const float TotalW = BW * 4.f + Gap * 3.f;
	const float X = (W - TotalW) * 0.5f + Index * (BW + Gap);
	const float Y = H * 0.56f;
	return FBox2D(FVector2D(X, Y), FVector2D(X + BW, Y + BH));
}

FBox2D AWOTOLDemoHUD::HeroPortraitPrevRect(float W, float H)
{
	const float BW = 60.f, BH = 60.f;
	const float CX = W * 0.5f;
	return FBox2D(FVector2D(CX - 140.f, H * 0.66f), FVector2D(CX - 140.f + BW, H * 0.66f + BH));
}

FBox2D AWOTOLDemoHUD::HeroPortraitNextRect(float W, float H)
{
	const float BW = 60.f, BH = 60.f;
	const float CX = W * 0.5f;
	return FBox2D(FVector2D(CX + 80.f, H * 0.66f), FVector2D(CX + 80.f + BW, H * 0.66f + BH));
}

FBox2D AWOTOLDemoHUD::HeroCustomizationConfirmRect(float W, float H)
{
	const float BW = 420.f, BH = 62.f;
	const float X = (W - BW) * 0.5f, Y = H * 0.85f;
	return FBox2D(FVector2D(X, Y), FVector2D(X + BW, Y + BH));
}

FBox2D AWOTOLDemoHUD::HeroCustomizationBackRect(float W, float H)
{
	const float BW = 160.f, BH = 46.f;
	return FBox2D(FVector2D(24.f, H - BH - 24.f), FVector2D(24.f + BW, H - 24.f));
}

FBox2D AWOTOLDemoHUD::PreGameSummaryBackRect(float W, float H)
{
	const float BW = 220.f, BH = 60.f;
	return FBox2D(FVector2D(24.f, H - BH - 24.f), FVector2D(24.f + BW, H - 24.f));
}

FBox2D AWOTOLDemoHUD::PreGameSummaryLaunchRect(float W, float H)
{
	const float BW = 420.f, BH = 62.f;
	const float X = (W - BW) * 0.5f, Y = H * 0.85f;
	return FBox2D(FVector2D(X, Y), FVector2D(X + BW, Y + BH));
}

FBox2D AWOTOLDemoHUD::LaunchBattleButtonRect(float W, float H)
{
	// Petit bouton JUSTE SOUS le timer (haut centre) -> le centre de l'écran reste libre
	// pour placer les unités. Il "brille" (reflet animé de DrawButton) pour attirer l'oeil.
	const float BW = 250.f, BH = 42.f;
	const float X = (W - BW) * 0.5f, Y = 66.f;
	return FBox2D(FVector2D(X, Y), FVector2D(X + BW, Y + BH));
}

FBox2D AWOTOLDemoHUD::SummaryContinueButtonRect(float W, float H)
{
	const float BW = 420.f, BH = 62.f;
	const float X = (W - BW) * 0.5f, Y = H - 120.f;
	return FBox2D(FVector2D(X, Y), FVector2D(X + BW, Y + BH));
}

// 4 boutons finaux alignés (Rejouer / Changer de faction / Menu principal / Quitter).
static FBox2D SummaryQuadRect(int32 Index, float W, float H)
{
	const float BW = 250.f, BH = 62.f, Gap = 22.f;
	const float TotalW = BW * 4.f + Gap * 3.f;
	const float X = (W - TotalW) * 0.5f + Index * (BW + Gap), Y = H - 120.f;
	return FBox2D(FVector2D(X, Y), FVector2D(X + BW, Y + BH));
}

FBox2D AWOTOLDemoHUD::SummaryReplayButtonRect(float W, float H)        { return SummaryQuadRect(0, W, H); }
FBox2D AWOTOLDemoHUD::SummaryChangeFactionButtonRect(float W, float H) { return SummaryQuadRect(1, W, H); }
FBox2D AWOTOLDemoHUD::SummaryMenuButtonRect(float W, float H)          { return SummaryQuadRect(2, W, H); }
FBox2D AWOTOLDemoHUD::SummaryQuitButtonRect(float W, float H)          { return SummaryQuadRect(3, W, H); }

FBox2D AWOTOLDemoHUD::InterludeContinueButtonRect(float W, float H)
{
	const float BW = 460.f, BH = 64.f;
	const float X = (W - BW) * 0.5f, Y = H - 120.f;
	return FBox2D(FVector2D(X, Y), FVector2D(X + BW, Y + BH));
}

FBox2D AWOTOLDemoHUD::LayerUpButtonRect(float W, float H)
{
	const float BW = 130.f, BH = 40.f;
	const float X = W - BW - 16.f, Y = H - 150.f;
	return FBox2D(FVector2D(X, Y), FVector2D(X + BW, Y + BH));
}

FBox2D AWOTOLDemoHUD::LayerDownButtonRect(float W, float H)
{
	const float BW = 130.f, BH = 40.f;
	const float X = W - BW - 16.f, Y = H - 104.f;
	return FBox2D(FVector2D(X, Y), FVector2D(X + BW, Y + BH));
}

void AWOTOLDemoHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas) return;

	const float W = Canvas->SizeX;
	const float H = Canvas->SizeY;
	UWorld* World = GetWorld();

	UDemoFlowSubsystem* DemoFlow = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	const EDemoScreen Screen = DemoFlow ? DemoFlow->GetScreen() : EDemoScreen::Playing;

	// Les écrans plein écran doivent quand même laisser une fenêtre d'objectif se dessiner
	// AU-DESSUS. L'ancien return prématuré rendait le flux v0.8 invisible/bloqué sur Interlude.
	auto DrawModalIfNeeded = [&]()
	{
		if (DemoFlow && DemoFlow->IsObjectiveWindowOpen()) DrawObjectiveWindow(W, H, DemoFlow);
	};
	if (Screen == EDemoScreen::MainMenu)      { DrawMainMenu(W, H); DrawModalIfNeeded(); return; }
	if (Screen == EDemoScreen::FactionSelect) { DrawFactionSelect(W, H); DrawModalIfNeeded(); return; }
	if (Screen == EDemoScreen::HeroCustomization) { DrawHeroCustomization(W, H, DemoFlow); DrawModalIfNeeded(); return; }
	if (Screen == EDemoScreen::PreGameSummary) { DrawPreGameSummary(W, H, DemoFlow); DrawModalIfNeeded(); return; }
	if (Screen == EDemoScreen::Summary)       { DrawSummary(W, H, DemoFlow); DrawModalIfNeeded(); return; }
	if (Screen == EDemoScreen::Interlude)     { DrawInterlude(W, H, DemoFlow); DrawModalIfNeeded(); return; }
	if (Screen == EDemoScreen::City)          { DrawCityView(W, H, DemoFlow); DrawModalIfNeeded(); return; }
	if (Screen == EDemoScreen::Territory)     { DrawTerritoryView(W, H, DemoFlow); DrawModalIfNeeded(); return; }
	if (Screen == EDemoScreen::Skills)        { DrawSkillsView(W, H, DemoFlow); DrawModalIfNeeded(); return; }
	if (Screen == EDemoScreen::Loading)       { DrawLoadingScreen(W, H, DemoFlow); DrawModalIfNeeded(); return; }
	if (Screen == EDemoScreen::Exploration)   { DrawExplorationHUD(W, H, DemoFlow); DrawModalIfNeeded(); return; }

	// ─── 1) Boîte de sélection (rectangle de drag) ───────────────────────────
	if (AWOTOLPlayerController_Battle* PC =
			Cast<AWOTOLPlayerController_Battle>(GetOwningPlayerController()))
	{
		if (PC->IsBoxSelecting())
		{
			const FVector2D S = PC->GetBoxStart();
			const FVector2D C = PC->GetBoxCurrent();
			const float X = FMath::Min(S.X, C.X);
			const float Y = FMath::Min(S.Y, C.Y);
			const float BW = FMath::Abs(C.X - S.X);
			const float BH = FMath::Abs(C.Y - S.Y);

			DrawRect(FLinearColor(0.2f, 0.7f, 1.f, 0.12f), X, Y, BW, BH);
			const FLinearColor Border(0.4f, 0.85f, 1.f, 1.f);
			DrawLine(X, Y, X + BW, Y, Border, 1.5f);
			DrawLine(X, Y + BH, X + BW, Y + BH, Border, 1.5f);
			DrawLine(X, Y, X, Y + BH, Border, 1.5f);
			DrawLine(X + BW, Y, X + BW, Y + BH, Border, 1.5f);
		}
	}

	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;

	// ─── 2) Bandeau supérieur : timer (pilule) + objectif ────────────────────
	DrawTopBar(W, H, World, Demo);

	// Boussole de courant océanique (placement + bataille) pour anticiper la dérive.
	DrawCurrentIndicator(W, H, World);

	// ─── 3) Barre de vie du BOSS (style "boss fight") ────────────────────────
	if (Demo)
	{
		// Barre du boss : seulement EN BATAILLE (pas pendant la préparation, pour garder
		// l'écran de placement dégagé).
		if (Screen == EDemoScreen::Playing)
		{
			if (AWOTOLDemoUnit* Boss = Cast<AWOTOLDemoUnit>(Demo->GetBoss()))
			{
				DrawBossBar(W, H, Boss);
			}
		}

		// Barre de vie du BÂTIMENT à défendre — uniquement EN BATAILLE (sinon elle
		// chevauchait le texte de préparation). En prépa, l'objet n'est pas attaqué.
		if (Screen == EDemoScreen::Playing)
		{
			if (AWOTOLCaptureObject* Building = Cast<AWOTOLCaptureObject>(Demo->GetCaptureObject()))
			{
				DrawBuildingBar(W, H, Building);
			}
		}

		// Compteur d'unités COMPACT (allié vs ennemi) — savoir combien il reste en face
		// sans encombrer l'écran. Petit bandeau discret en haut, sous le timer.
		if (Screen == EDemoScreen::Playing)
		{
			if (UFactionRegistrySubsystem* Reg = World ? World->GetSubsystem<UFactionRegistrySubsystem>() : nullptr)
			{
				const EFactionID Ally  = Demo->GetPlayerFaction();
				const EFactionID Enemy = (Ally == EFactionID::Aquiloris) ? EFactionID::Noxeens : EFactionID::Aquiloris;
				auto CountAlive = [&](EFactionID F) { int32 n = 0; for (AUnitBase* U : Reg->GetUnitsForFaction(F)) if (U && U->IsAlive()) ++n; return n; };
				const int32 NA = CountAlive(Ally), NE = CountAlive(Enemy);
				const FString Txt = FString::Printf(TEXT("Allies %d    Ennemis %d"), NA, NE);
				UFont* F = GEngine ? GEngine->GetMediumFont() : nullptr;
				float TW = 0.f, TH = 0.f; GetTextSize(Txt, TW, TH, F, 1.f);
				const float BX = (W - TW) * 0.5f, BY = 92.f;
				DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.5f), BX - 14.f, BY - 4.f, TW + 28.f, TH + 8.f);
				DrawText(FString::Printf(TEXT("Allies %d"), NA), FLinearColor(0.5f, 0.85f, 1.f, 1.f), BX, BY, F, 1.f);
				float AW = 0.f, AH = 0.f; GetTextSize(FString::Printf(TEXT("Allies %d    "), NA), AW, AH, F, 1.f);
				DrawText(FString::Printf(TEXT("Ennemis %d"), NE), FLinearColor(1.f, 0.5f, 0.4f, 1.f), BX + AW, BY, F, 1.f);
			}
		}

		// Écran de fin
		if (Demo->GetPhase() == EDemoPhase::DemoEnd)
		{
			DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.45f), 0.f, H * 0.34f, W, H * 0.22f);
			if (Demo->bDemoVictory)
			{
				DrawCenteredText(TEXT("— VICTOIRE —"), H * 0.40f, FLinearColor(1.f, 0.85f, 0.2f, 1.f), 2.4f);
				DrawCenteredText(TEXT("Zone tenue. Le conflit Aquiloris / Noxeens ne fait que commencer..."),
					H * 0.48f, FLinearColor::White, 1.1f);
			}
			else
			{
				DrawCenteredText(TEXT("— DEFAITE —"), H * 0.40f, FLinearColor(1.f, 0.25f, 0.2f, 1.f), 2.4f);
				DrawCenteredText(TEXT("Vos forces sont tombees. Relancez pour reessayer."),
					H * 0.48f, FLinearColor::White, 1.1f);
			}
		}
	}

	// ─── 4b) Marqueurs de groupe sur le champ (icône + effectif + barre de vie) ──
	if (Screen == EDemoScreen::Playing || Screen == EDemoScreen::Prepare)
	{
		DrawBattlefieldMarkers(W, H, World);
	}

	// ─── 5) Barre de commandement (bas) : cartes d'unités sélectionnées ──────
	DrawCommandBar(W, H, World);
	if (Screen == EDemoScreen::Playing)
	{
		DrawAbilityStatus(W, H, World);
	}
	if (Screen == EDemoScreen::Playing || Screen == EDemoScreen::Prepare)
	{
		DrawMinimap(W, H, World);
	}

	// ─── Préparation : bandeau d'instructions + bouton "Lancer la bataille" ──
	if (Screen == EDemoScreen::Prepare)
	{
		DrawPrepareBar(W, H);
	}

	// ─── Jauge verticale SURFACE/MID/SOL + boutons de couche (nage) ──────────
	if (Screen == EDemoScreen::Playing || Screen == EDemoScreen::Prepare)
	{
		DrawVerticalLayerGauge(W, H, World);
	}
	DrawButton(LayerUpButtonRect(W, H),   TEXT("^ Monter"),    FLinearColor(0.3f, 0.7f, 1.f, 1.f), 1.f);
	DrawButton(LayerDownButtonRect(W, H), TEXT("v Descendre"), FLinearColor(0.3f, 0.7f, 1.f, 1.f), 1.f);

	// ─── 6) Boutons ENGRENAGE (réglages) + PAUSE (gel) ───────────────────────
	DrawSettingsButton(W, H);
	DrawPauseButton(W, H);
	// Menu réglages (engrenage) : voile + boutons. Gel simple (pause) : discret indicateur.
	if (AWOTOLPlayerController_Battle* PC = Cast<AWOTOLPlayerController_Battle>(GetOwningPlayerController()))
	{
		if (PC->IsSettingsOpen())
		{
			const uint8 Confirm = PC->GetPendingConfirmAction();
			if (Confirm == 1)
				DrawConfirmDialog(W, H, TEXT("Recommencer la demo depuis le debut ? Toute la progression sera perdue."),
					TEXT("OUI, RECOMMENCER"));
			else if (Confirm == 2)
				DrawConfirmDialog(W, H, TEXT("Quitter le jeu ?"), TEXT("OUI, QUITTER"));
			else if (PC->IsControlsOpen())
				DrawControlsScreen(W, H); // liste des touches
			else
				DrawPauseOverlay(W, H); // menu Reprendre / Recommencer / Quitter / Commandes
		}
		else if (PC->IsBattleFrozen())
		{
			// PAUSE simple : pas de voile (on veut voir le champ gelé + déplacer la caméra).
			DrawCenteredText(TEXT("— PAUSE —"), 60.f, FLinearColor(1.f, 0.95f, 0.6f, 1.f), 1.4f);
		}
	}

	// ─── Fenêtre d'objectif MODALE (dessinée EN DERNIER = par-dessus tout) ────
	if (DemoFlow && DemoFlow->IsObjectiveWindowOpen())
	{
		DrawObjectiveWindow(W, H, DemoFlow);
	}
}

// Rectangle du bouton « Continuer » de la fenêtre d'objectif (centré sous le corps).
FBox2D AWOTOLDemoHUD::ObjectiveContinueButtonRect(float W, float H)
{
	const float BW = 340.f, BH = 64.f;
	const float X = W * 0.5f - BW * 0.5f;
	const float Y = H * 0.5f + 70.f;
	return FBox2D(FVector2D(X, Y), FVector2D(X + BW, Y + BH));
}

FBox2D AWOTOLDemoHUD::ExplorationCrystalliserButtonRect(float W, float H)
{
	const float BW = 330.f, BH = 76.f;
	return FBox2D(FVector2D(28.f, H - BH - 92.f), FVector2D(28.f + BW, H - 92.f));
}

void AWOTOLDemoHUD::DrawObjectiveWindow(float W, float H, class UDemoFlowSubsystem* Demo)
{
	if (!Demo) return;

	// Voile sombre pour concentrer l'attention (l'action est gelée en dessous).
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.55f), 0, 0, W, H);

	const bool bFail = Demo->bObjWinIsFailure;
	const FLinearColor Accent = bFail ? FLinearColor(1.f, 0.35f, 0.30f, 1.f)
	                                  : FLinearColor(0.35f, 0.85f, 1.f, 1.f);

	// Panneau central.
	const float PW = 760.f, PH = 300.f;
	const float PX = W * 0.5f - PW * 0.5f;
	const float PY = H * 0.5f - PH * 0.5f - 20.f;
	DrawRect(FLinearColor(0.04f, 0.07f, 0.12f, 0.94f), PX, PY, PW, PH);
	// Liseré haut coloré (bleu = objectif, rouge = échec).
	DrawRect(Accent.CopyWithNewOpacity(0.9f), PX, PY, PW, 6.f);

	// Titre.
	DrawCenteredText(Demo->ObjWinTitle, PY + 34.f, Accent, 1.7f);
	// Corps (peut être multi-lignes séparées par \n).
	TArray<FString> Lines;
	Demo->ObjWinBody.ParseIntoArray(Lines, TEXT("\n"), false);
	float LineY = PY + 100.f;
	for (const FString& L : Lines)
	{
		DrawCenteredText(L, LineY, FLinearColor(0.90f, 0.94f, 1.f, 1.f), 1.1f);
		LineY += 34.f;
	}

	// Bouton « Continuer ».
	const FBox2D BR = ObjectiveContinueButtonRect(W, H);
	DrawButton(BR, Demo->ObjWinButton, Accent, 1.3f);
}

void AWOTOLDemoHUD::DrawButton(const FBox2D& R, const FString& Label, const FLinearColor& Tint, float TextScale)
{
	const FVector2D Sz = R.Max - R.Min;
	const float T = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	// Corps : léger dégradé vertical (haut plus clair) pour un relief "verre/eau".
	const int32 Bands = 10;
	for (int32 i = 0; i < Bands; ++i)
	{
		const float f = (float)i / (Bands - 1);
		const FLinearColor C = FMath::Lerp(FLinearColor(0.07f, 0.11f, 0.18f, 0.97f),
			FLinearColor(0.02f, 0.03f, 0.06f, 0.97f), f);
		DrawRect(C, R.Min.X, R.Min.Y + Sz.Y * f * (Bands - 1) / Bands, Sz.X, Sz.Y / Bands + 1.f);
	}
	// Liserés teintés (haut/bas) + montants
	DrawRect(Tint, R.Min.X, R.Min.Y, Sz.X, 4.f);
	DrawRect(Tint, R.Min.X, R.Max.Y - 4.f, Sz.X, 4.f);
	DrawRect(Tint * 0.7f, R.Min.X, R.Min.Y, 3.f, Sz.Y);
	DrawRect(Tint * 0.7f, R.Max.X - 3.f, R.Min.Y, 3.f, Sz.Y);
	// Reflet animé qui balaie le bouton (vie/appel au clic)
	const float SheenX = R.Min.X + (0.5f + 0.5f * FMath::Sin(T * 1.6f)) * (Sz.X - 40.f);
	DrawRect(FLinearColor(Tint.R, Tint.G, Tint.B, 0.14f), SheenX, R.Min.Y + 4.f, 40.f, Sz.Y - 8.f);

	// Libellé + ombre pour le contraste
	float TW, TH; GetTextSize(Label, TW, TH, GEngine->GetLargeFont(), TextScale);
	const float LX = R.Min.X + (Sz.X - TW) * 0.5f, LY = R.Min.Y + (Sz.Y - TH) * 0.5f;
	DrawText(Label, FLinearColor(0.f, 0.f, 0.f, 0.7f), LX + 2.f, LY + 2.f, GEngine->GetLargeFont(), TextScale);
	DrawText(Label, FLinearColor::White, LX, LY, GEngine->GetLargeFont(), TextScale);
}

// ─── Fond marin animé (dégradé + bulles + rais de lumière) ────────────────────
void AWOTOLDemoHUD::DrawUnderwaterBackground(float W, float H)
{
	const float T = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	// Bruit déterministe (mêmes reliefs à chaque frame).
	auto Rnd = [](int32 n) -> float
	{
		n = (n << 13) ^ n;
		return (float)((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 2147483647.f;
	};

	// ── 1) Dégradé de profondeur : bleu clair de surface -> abysse ─────────────
	const int32 Bands = 64;
	const FLinearColor Surface(0.06f, 0.24f, 0.40f, 1.f);
	const FLinearColor Mid    (0.02f, 0.10f, 0.20f, 1.f);
	const FLinearColor Deep   (0.004f, 0.015f, 0.05f, 1.f);
	for (int32 i = 0; i < Bands; ++i)
	{
		const float f = (float)i / (Bands - 1);
		const FLinearColor Col = (f < 0.5f)
			? FMath::Lerp(Surface, Mid, f * 2.f)
			: FMath::Lerp(Mid, Deep, (f - 0.5f) * 2.f);
		DrawRect(Col, 0.f, i * (H / Bands), W, H / Bands + 1.f);
	}

	// ── 2) Puits de lumière CENTRAL (source haut-centre) : cône lumineux ───────
	if (Canvas)
	{
		const float cx = W * 0.52f;
		for (int32 k = 0; k < 22; ++k)
		{
			const float f = (float)k / 21.f;
			const float y = f * H;
			const float halfw = FMath::Lerp(70.f, W * 0.34f, f);      // s'élargit vers le bas
			const float a = (1.f - f) * 0.10f;                        // s'estompe en descendant
			DrawRect(FLinearColor(0.55f, 0.82f, 1.f, a),
				cx - halfw, y, halfw * 2.f, H / 22.f + 1.f);
		}
		// Halo brillant à la source.
		Canvas->K2_DrawPolygon(nullptr, FVector2D(cx, H * 0.02f), FVector2D(220.f, 220.f), 24,
			FLinearColor(0.7f, 0.9f, 1.f, 0.12f));
	}

	// ── 3) Rais de lumière obliques qui dérivent (god rays) ────────────────────
	for (int32 j = 0; j < 6; ++j)
	{
		const float baseX = W * (0.30f + 0.10f * j);
		const float x = baseX + FMath::Sin(T * 0.15f + j * 1.3f) * W * 0.05f;
		DrawRect(FLinearColor(0.5f, 0.78f, 1.f, 0.03f), x, 0.f, 60.f + 26.f * j, H);
	}

	// ── 4) ASSOMBRISSEMENT LATÉRAL doux (parois lointaines) — encadre sans dentelure
	// pixelisée : simples bandes verticales dégradées vers les bords. ──
	{
		const int32 Steps = 24;
		for (int32 s = 0; s < Steps; ++s)
		{
			const float f = (float)s / (Steps - 1);           // 0 bord -> 1 centre
			const float bw = W * 0.18f * (1.f - f);           // largeur décroissante
			const float a  = (1.f - f) * 0.06f;
			DrawRect(FLinearColor(0.f, 0.02f, 0.04f, a), 0.f, 0.f, bw, H);
			DrawRect(FLinearColor(0.f, 0.02f, 0.04f, a), W - bw, 0.f, bw, H);
		}
	}

	// ── 5) FOND MARIN en vallée (bandes lisses, plus haut sur les bords) ───────
	{
		const int32 Cols = 64;
		const float colW = W / Cols + 1.f;
		for (int32 c = 0; c < Cols; ++c)
		{
			const float x = c * (W / Cols);
			const float centerBias = FMath::Abs((float)c / Cols - 0.5f) * 2.f; // 0 centre -> 1 bords
			const float farH = H * (0.08f + 0.12f * centerBias);
			DrawRect(FLinearColor(0.015f, 0.035f, 0.06f, 1.f), x, H - farH, colW, farH);
		}
	}

	// ── 6) CHEMINÉES VOLCANIQUES : seulement de fines BRAISES qui montent (pas de gros
	// halo en polygone plein, qui rendait des ovales orange opaques à l'écran). ──
	if (Canvas)
	{
		const float vents[3][2] = { {0.16f, 0.86f}, {0.30f, 0.92f}, {0.78f, 0.88f} };
		for (int32 v = 0; v < 3; ++v)
		{
			const float vx = W * vents[v][0];
			const float vy = H * vents[v][1];
			for (int32 e = 0; e < 8; ++e)
			{
				const float ephase = Rnd(v * 10 + e + 500);
				const float ey = vy - FMath::Fmod(T * (30.f + ephase * 40.f) + ephase * 300.f, 300.f);
				const float ex = vx + FMath::Sin(T * 1.5f + e) * 18.f;
				const float ea = FMath::Clamp((vy - ey) / 300.f, 0.f, 1.f);
				DrawRect(FLinearColor(1.f, 0.5f, 0.15f, (1.f - ea) * 0.5f), ex, ey, 2.5f, 2.5f);
			}
		}
	}

	// ── 7) PARTICULES en suspension (spores/plancton) qui dérivent ─────────────
	if (Canvas)
	{
		for (int32 i = 0; i < 60; ++i)
		{
			const float px = FMath::Fmod(Rnd(i + 600) * W + T * (4.f + Rnd(i + 610) * 8.f), W);
			const float py = Rnd(i + 620) * H + FMath::Sin(T * 0.4f + i) * 10.f;
			const float ps = 1.f + Rnd(i + 630) * 2.f;
			Canvas->K2_DrawPolygon(nullptr, FVector2D(px, py), FVector2D(ps, ps), 6,
				FLinearColor(0.6f, 0.85f, 1.f, 0.10f + Rnd(i + 640) * 0.10f));
		}
	}

	// ── 8) Bulles qui montent ──────────────────────────────────────────────────
	if (Canvas)
	{
		const int32 NumBubbles = 46;
		for (int32 i = 0; i < NumBubbles; ++i)
		{
			const float bx    = Rnd(i) * W + FMath::Sin(T * 0.6f + i) * 12.f;
			const float size  = 3.f + Rnd(i + 100) * 13.f;
			const float speed = 28.f + Rnd(i + 200) * 74.f;
			const float phase = Rnd(i + 300);
			const float by    = H - FMath::Fmod(T * speed + phase * (H + 140.f), H + 140.f);
			const float a     = 0.06f + Rnd(i + 400) * 0.12f;
			Canvas->K2_DrawPolygon(nullptr, FVector2D(bx, by), FVector2D(size, size), 16,
				FLinearColor(0.6f, 0.85f, 1.f, a));
			Canvas->K2_DrawPolygon(nullptr, FVector2D(bx - size * 0.3f, by - size * 0.3f),
				FVector2D(size * 0.28f, size * 0.28f), 10, FLinearColor(0.9f, 0.97f, 1.f, a * 1.4f));
		}
	}

	// ── 9) Vignette (haut + bas + coins) pour cadrer et lisibilité du texte ────
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.35f), 0.f, H * 0.74f, W, H * 0.26f);
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.22f), 0.f, 0.f, W, H * 0.10f);
}

// Silhouettes stylisées du biome de la faction (cristaux dressés Aquiloris / croissance
// bioluminescente Noxéenne) — dessinées en fond, concentrées sur les bords/coins bas de
// l'écran pour ne jamais empiéter sur le contenu central (texte/boutons). Demande de Liamor
// le 25/07/2026 : les écrans ne doivent plus paraître vides/neutres une fois la faction
// choisie. Purement procédural (formes géométriques via Canvas, mêmes primitives que
// DrawUnderwaterBackground) — reste un habillage STYLISÉ, pas un visuel définitif : aucune
// image/texture n'est importée (impossible sans éditeur Unreal disponible ici), à remplacer
// plus tard par de vrais fonds/matériaux une fois les assets de Liamor reçus et validés.
// Hors scope démo (Thalassidra/Muréniens/Pirates Abyssaux) : pas de motif inventé pour elles.
static void DrawFactionBiomeSilhouette(UCanvas* Canvas, float W, float H, float T, EFactionID Faction)
{
	if (!Canvas || (Faction != EFactionID::Aquiloris && Faction != EFactionID::Noxeens)) return;

	auto Rnd = [](int32 n) -> float
	{
		n = (n << 13) ^ n;
		return (float)((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 2147483647.f;
	};
	const FLinearColor Primary   = FFactionColors::Get(Faction);
	const FLinearColor Secondary = FFactionColors::GetSecondary(Faction);
	const float Corners[2] = { 0.f, W };

	if (Faction == EFactionID::Aquiloris)
	{
		// Flèches de cristal dressées aux deux coins bas, en éventail vers le centre.
		for (int32 c = 0; c < 2; ++c)
		{
			const float Dir = (c == 0) ? 1.f : -1.f;
			for (int32 i = 0; i < 5; ++i)
			{
				const float BaseX  = Corners[c] + Dir * (30.f + i * 55.f);
				const float BaseY  = H * (0.97f - Rnd(c * 20 + i) * 0.05f);
				const float SpireH = H * (0.22f + Rnd(c * 20 + i + 7) * 0.20f) * (1.f - i * 0.10f);
				const float SpireW = 26.f + Rnd(c * 20 + i + 3) * 20.f;
				Canvas->K2_DrawPolygon(nullptr, FVector2D(BaseX, BaseY - SpireH * 0.5f),
					FVector2D(SpireW, SpireH * 0.5f), 3,
					FLinearColor(Primary.R, Primary.G, Primary.B, 0.14f + 0.045f * (4 - i)));
			}
		}
		// Éclats de cristal scintillants dispersés (accents pulsants).
		for (int32 i = 0; i < 14; ++i)
		{
			const float px    = Rnd(i + 900) * W;
			const float py    = H * (0.55f + Rnd(i + 910) * 0.4f);
			const float Pulse = 0.5f + 0.5f * FMath::Sin(T * 1.3f + i * 2.1f);
			Canvas->K2_DrawPolygon(nullptr, FVector2D(px, py),
				FVector2D(3.f + Pulse * 3.f, 3.f + Pulse * 3.f), 4,
				FLinearColor(Secondary.R, Secondary.G, Secondary.B, 0.09f + Pulse * 0.11f));
		}
	}
	else // Noxeens
	{
		// Amas sombres bioluminescents aux coins bas (silhouette irrégulière, triangles superposés).
		for (int32 c = 0; c < 2; ++c)
		{
			const float Dir = (c == 0) ? 1.f : -1.f;
			for (int32 i = 0; i < 6; ++i)
			{
				const float BaseX  = Corners[c] + Dir * (20.f + i * 42.f + Rnd(c * 30 + i) * 20.f);
				const float BaseY  = H * (0.99f - Rnd(c * 30 + i + 2) * 0.04f);
				const float MoundH = H * (0.10f + Rnd(c * 30 + i + 5) * 0.16f);
				Canvas->K2_DrawPolygon(nullptr, FVector2D(BaseX, BaseY - MoundH * 0.5f),
					FVector2D(20.f + Rnd(c * 30 + i + 9) * 16.f, MoundH * 0.5f), 5,
					FLinearColor(0.02f, 0.05f, 0.045f, 0.5f));
			}
		}
		// Spores bioluminescentes qui pulsent doucement, dispersées vers le haut de l'écran.
		for (int32 i = 0; i < 16; ++i)
		{
			const float px    = Rnd(i + 1000) * W;
			const float py    = H * (0.62f + Rnd(i + 1010) * 0.35f);
			const float Pulse = 0.5f + 0.5f * FMath::Sin(T * 0.9f + i * 1.7f);
			Canvas->K2_DrawPolygon(nullptr, FVector2D(px, py),
				FVector2D(2.5f + Pulse * 3.5f, 2.5f + Pulse * 3.5f), 8,
				FLinearColor(Primary.R, Primary.G, Primary.B, 0.10f + Pulse * 0.16f));
		}
		// Tentacules filiformes qui ondulent depuis le bas de l'écran (courbes en segments).
		for (int32 t = 0; t < 4; ++t)
		{
			const float StartX = W * (0.08f + t * 0.28f + Rnd(t + 50) * 0.06f);
			FVector2D Prev(StartX, H);
			const int32 Segs = 10;
			for (int32 s = 1; s <= Segs; ++s)
			{
				const float f    = (float)s / Segs;
				const float Sway = FMath::Sin(f * 3.1f + T * 0.5f + t * 1.5f) * 24.f * f;
				const FVector2D Cur(StartX + Sway, H * (1.f - f * 0.32f));
				const FLinearColor LineCol(Secondary.R, Secondary.G, Secondary.B, 0.13f * (1.f - f * 0.5f));
				Canvas->K2_DrawLine(Prev, Cur, 2.f, LineCol);
				Prev = Cur;
			}
		}
	}
}

// Lavis translucide + fond de biome dans la teinte de la faction choisie, à appeler APRÈS
// DrawUnderwaterBackground sur les écrans qui suivent le choix de faction (thème d'interface
// dynamique par faction, décision Liamor du 22/07/2026, enrichi le 25/07/2026 pour que les
// écrans ne paraissent plus vides/neutres). Priorité à la vraie image (Content/UI/
// BackgroundAquiloris.png / BackgroundNoxeens.png, fournies par Liamor) si présente ; sinon
// repli sur les silhouettes procédurales. Sans effet tant qu'aucune faction n'est choisie.
void AWOTOLDemoHUD::DrawFactionAmbientTint(float W, float H, EFactionID Faction)
{
	if (Faction == EFactionID::None) return;
	if (UTexture2D* BG = GetFactionBackground(Faction))
	{
		// Mélangé à 55% par-dessus le dégradé procédural existant (garde la vignette de
		// lisibilité de DrawUnderwaterBackground en dessous) plutôt que de le remplacer.
		DrawTexture(BG, 0.f, 0.f, W, H, 0.f, 0.f, 1.f, 1.f, FLinearColor(1.f, 1.f, 1.f, 0.55f));
	}
	else
	{
		DrawFactionBiomeSilhouette(Canvas, W, H, GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f, Faction);
	}
	const FLinearColor Tint = FFactionColors::Get(Faction);
	DrawRect(Tint.CopyWithNewOpacity(0.05f), 0.f, 0.f, W, H);
}

// Titre "LAVE" multicolore et lumineux (pour le nom du jeu) : halo chaud +
// dégradé rouge->orange->jaune + braises scintillantes. Fait "péter" le titre.
void AWOTOLDemoHUD::DrawLavaTitle(const FString& Text, float Y, float Scale)
{
	if (!Canvas) return;
	UFont* Font = GEngine->GetLargeFont();
	float TW, TH; GetTextSize(Text, TW, TH, Font, Scale);
	const float X = (Canvas->SizeX - TW) * 0.5f;
	const float T = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	// (Pas de halo en polygone plein : le Canvas ne dégrade pas l'alpha -> ça faisait un
	// gros ovale orange opaque. On s'appuie sur les passes de texte décalées ci-dessous.)

	// Contour sombre (braise éteinte) pour détourer.
	const float o = 3.f;
	const float dirs[8][2] = { {-o,0},{o,0},{0,-o},{0,o},{-o,-o},{o,-o},{-o,o},{o,o} };
	for (int32 i = 0; i < 8; ++i)
		DrawText(Text, FLinearColor(0.18f, 0.02f, 0.f, 0.95f), X + dirs[i][0], Y + dirs[i][1], Font, Scale);

	// Dégradé vertical simulé : rouge (bas) -> orange -> jaune (haut) via 3 passes
	// légèrement décalées, la plus claire au-dessus.
	DrawText(Text, FLinearColor(0.85f, 0.10f, 0.02f, 1.f), X, Y + 3.f, Font, Scale); // rouge profond (bas)
	DrawText(Text, FLinearColor(1.f, 0.42f, 0.08f, 1.f),  X, Y + 1.f, Font, Scale);  // orange
	DrawText(Text, FLinearColor(1.f, 0.82f, 0.28f, 1.f),  X, Y,       Font, Scale);  // cœur jaune

	// Braises scintillantes autour des lettres.
	auto Rnd = [](int32 n) -> float { n = (n << 13) ^ n;
		return (float)((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 2147483647.f; };
	for (int32 i = 0; i < 18; ++i)
	{
		const float ex = X + Rnd(i) * TW;
		const float ey = Y + TH * Scale * (0.1f + Rnd(i + 40) * 0.8f) - FMath::Fmod(T * 20.f + Rnd(i + 80) * 60.f, 60.f);
		const float ea = 0.4f + 0.6f * FMath::Sin(T * 3.f + i);
		Canvas->K2_DrawPolygon(nullptr, FVector2D(ex, ey), FVector2D(2.f, 2.f), 6,
			FLinearColor(1.f, 0.6f, 0.2f, FMath::Max(0.f, ea) * 0.6f));
	}
}

void AWOTOLDemoHUD::DrawGlowTitle(const FString& Text, float Y, float Scale, const FLinearColor& Color)
{
	if (!Canvas) return;
	UFont* Font = GEngine->GetLargeFont();
	float TW, TH; GetTextSize(Text, TW, TH, Font, Scale);
	const float X = (Canvas->SizeX - TW) * 0.5f;

	// Contour NET (pas de halo flou) : 8 copies noires collées à 2 px -> lettres bien
	// détourées et lisibles ; puis le texte plein par-dessus.
	const FLinearColor Outline(0.f, 0.f, 0.f, 0.9f);
	const float o = 2.f;
	const float dirs[8][2] = { {-o,0},{o,0},{0,-o},{0,o},{-o,-o},{o,-o},{-o,o},{o,o} };
	for (int32 i = 0; i < 8; ++i)
	{
		DrawText(Text, Outline, X + dirs[i][0], Y + dirs[i][1], Font, Scale);
	}
	DrawText(Text, Color, X, Y, Font, Scale);
}

UTexture2D* AWOTOLDemoHUD::GetMenuBackground()
{
	if (bMenuBgTried) return MenuBgTexture;
	bMenuBgTried = true;

	// 1) Priorité à l'asset importé dans l'éditeur (le plus propre, cuit au packaging).
	if (UTexture2D* Asset = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/MainMenuBG.MainMenuBG")))
	{
		MenuBgTexture = Asset;
		return MenuBgTexture;
	}
	// 2) Sinon, on charge le PNG DIRECTEMENT depuis le disque (pas d'import manuel requis
	//    pour la démo en éditeur). Content/UI/MainMenuBG.png.
	const FString PngPath = FPaths::ProjectContentDir() / TEXT("UI/MainMenuBG.png");
	if (FPaths::FileExists(PngPath))
	{
		if (UTexture2D* Loaded = FImageUtils::ImportFileAsTexture2D(PngPath))
		{
			MenuBgTexture = Loaded;
		}
	}
	return MenuBgTexture;
}

UTexture2D* AWOTOLDemoHUD::GetFactionBackground(EFactionID Faction)
{
	// Meme mecanisme que GetMenuBackground (PNG charge directement depuis le disque, sans
	// import manuel). Images officielles fournies par Liamor le 25/07/2026.
	if (Faction == EFactionID::Aquiloris)
	{
		if (bAquilorisBgTried) return AquilorisBgTexture;
		bAquilorisBgTried = true;
		const FString PngPath = FPaths::ProjectContentDir() / TEXT("UI/BackgroundAquiloris.png");
		if (FPaths::FileExists(PngPath)) AquilorisBgTexture = FImageUtils::ImportFileAsTexture2D(PngPath);
		return AquilorisBgTexture;
	}
	if (Faction == EFactionID::Noxeens)
	{
		if (bNoxeensBgTried) return NoxeensBgTexture;
		bNoxeensBgTried = true;
		const FString PngPath = FPaths::ProjectContentDir() / TEXT("UI/BackgroundNoxeens.png");
		if (FPaths::FileExists(PngPath)) NoxeensBgTexture = FImageUtils::ImportFileAsTexture2D(PngPath);
		return NoxeensBgTexture;
	}
	return nullptr; // Hors scope demo (Thalassidra/Mureniens/Pirates Abyssaux)
}

UTexture2D* AWOTOLDemoHUD::GetFactionEmblem(EFactionID Faction)
{
	if (Faction == EFactionID::Aquiloris)
	{
		if (bAquilorisEmblemTried) return AquilorisEmblemTexture;
		bAquilorisEmblemTried = true;
		const FString PngPath = FPaths::ProjectContentDir() / TEXT("UI/EmblemAquiloris.png");
		if (FPaths::FileExists(PngPath)) AquilorisEmblemTexture = FImageUtils::ImportFileAsTexture2D(PngPath);
		return AquilorisEmblemTexture;
	}
	if (Faction == EFactionID::Noxeens)
	{
		if (bNoxeensEmblemTried) return NoxeensEmblemTexture;
		bNoxeensEmblemTried = true;
		const FString PngPath = FPaths::ProjectContentDir() / TEXT("UI/EmblemNoxeens.png");
		if (FPaths::FileExists(PngPath)) NoxeensEmblemTexture = FImageUtils::ImportFileAsTexture2D(PngPath);
		return NoxeensEmblemTexture;
	}
	return nullptr;
}

void AWOTOLDemoHUD::DrawMainMenu(float W, float H)
{
	// IMAGE d'accueil : asset importé OU PNG chargé depuis le disque (voir GetMenuBackground).
	// Elle contient déjà le titre + le sous-titre -> on ne redessine pas le titre par-dessus.
	if (UTexture2D* BG = GetMenuBackground())
	{
		DrawTexture(BG, 0.f, 0.f, W, H, 0.f, 0.f, 1.f, 1.f);
		// Léger assombrissement en bas pour la lisibilité du bouton.
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.28f), 0.f, H * 0.80f, W, H * 0.20f);
	}
	else
	{
		DrawUnderwaterBackground(W, H);
		// Titre imposant en LAVE (rouge/orange/jaune lumineux), sous-titre chaud, puis bouton
		DrawLavaTitle(TEXT("WOTOL"), H * 0.22f, 5.0f);
		DrawCenteredText(TEXT("WAR OF THE OCEAN'S LEGACY"), H * 0.42f, FLinearColor(1.f, 0.45f, 0.35f, 1.f), 1.5f);
	}
	DrawButton(StartGameButtonRect(W, H), TEXT("COMMENCER LA DEMO"), FLinearColor(0.3f, 0.75f, 1.f, 1.f), 1.6f);
}

void AWOTOLDemoHUD::DrawFactionSelect(float W, float H)
{
	DrawUnderwaterBackground(W, H);
	DrawGlowTitle(TEXT("CHOISISSEZ VOTRE FACTION"), H * 0.13f, 2.4f, FLinearColor(0.7f, 0.9f, 1.f, 1.f));

	const float T = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	// Emblème animé au-dessus de chaque bouton (cristal Aquiloris / organisme Noxéen). Les
	// boutons étant LARGES et écartés, les emblèmes sont loin du titre centré (plus de collision).
	if (Canvas)
	{
		const FBox2D RA = FactionButtonRect(0, W, H);
		const FBox2D RN = FactionButtonRect(1, W, H);
		const float bobA = FMath::Sin(T * 1.4f) * 8.f;
		const float bobN = FMath::Sin(T * 1.4f + 1.6f) * 8.f;
		const FVector2D CA((RA.Min.X + RA.Max.X) * 0.5f, RA.Min.Y - H * 0.07f + bobA);
		const FVector2D CN((RN.Min.X + RN.Max.X) * 0.5f, RN.Min.Y - H * 0.07f + bobN);
		// Emblèmes officiels (Content/UI/EmblemAquiloris.png / EmblemNoxeens.png, fournis par
		// Liamor le 25/07/2026) si présents ; repli sur les icônes procédurales sinon.
		const float IconSize = 100.f;
		if (UTexture2D* EmblemA = GetFactionEmblem(EFactionID::Aquiloris))
		{
			DrawTexture(EmblemA, CA.X - IconSize * 0.5f, CA.Y - IconSize * 0.5f, IconSize, IconSize, 0.f, 0.f, 1.f, 1.f);
		}
		else
		{
			// Aquiloris : cristal (triangle cyan) + halo
			Canvas->K2_DrawPolygon(nullptr, CA, FVector2D(52.f, 52.f), 16, FLinearColor(0.2f, 0.6f, 1.f, 0.15f));
			Canvas->K2_DrawPolygon(nullptr, CA, FVector2D(34.f, 46.f), 3, FLinearColor(0.5f, 0.9f, 1.f, 0.95f));
		}
		if (UTexture2D* EmblemN = GetFactionEmblem(EFactionID::Noxeens))
		{
			DrawTexture(EmblemN, CN.X - IconSize * 0.5f, CN.Y - IconSize * 0.5f, IconSize, IconSize, 0.f, 0.f, 1.f, 1.f);
		}
		else
		{
			// Noxéens : organisme (hexa vert) + halo
			Canvas->K2_DrawPolygon(nullptr, CN, FVector2D(52.f, 52.f), 16, FLinearColor(0.2f, 0.9f, 0.45f, 0.15f));
			Canvas->K2_DrawPolygon(nullptr, CN, FVector2D(40.f, 40.f), 16, FLinearColor(0.3f, 0.95f, 0.5f, 0.95f));
		}
	}

	UDemoFlowSubsystem* Flow = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	const EFactionID Selected = Flow ? Flow->SelectedFaction : EFactionID::None;
	// Couleurs de faction : FFactionColors — source de vérité unique (thème d'interface par
	// faction, décision Liamor du 22/07/2026) — jamais redéfinies localement.
	DrawButton(FactionButtonRect(0, W, H),
		Selected == EFactionID::Aquiloris ? TEXT("AQUILORIS  [CHOISIE]") : TEXT("AQUILORIS"),
		FFactionColors::Get(EFactionID::Aquiloris), 1.7f);
	DrawButton(FactionButtonRect(1, W, H),
		Selected == EFactionID::Noxeens ? TEXT("NOXEENS  [CHOISIE]") : TEXT("NOXEENS"),
		FFactionColors::Get(EFactionID::Noxeens), 1.7f);
	// Description COURTE, juste sous les boutons de faction (bien au-dessus du bloc difficulté).
	DrawCenteredText(TEXT("Aquiloris : cristal-tech, coordination          Noxeens : abysses bioluminescents"),
		FactionButtonRect(0, W, H).Max.Y + H * 0.04f, FLinearColor(0.8f, 0.9f, 1.f, 0.9f), 1.0f);
	if (Selected == EFactionID::Aquiloris)
	{
		DrawCenteredText(TEXT("Aquiloris — gardiens d'Aquilor, technologie cristalline et discipline collective."),
			H * 0.545f, FFactionColors::GetSecondary(EFactionID::Aquiloris), 0.95f);
	}
	else if (Selected == EFactionID::Noxeens)
	{
		DrawCenteredText(TEXT("Noxeens — peuple des failles, puissance abyssale et bioluminescence verte."),
			H * 0.545f, FFactionColors::GetSecondary(EFactionID::Noxeens), 0.95f);
	}

	// ── DIFFICULTÉ (3 niveaux) : le joueur la choisit AVANT de cliquer sur une faction.
	// Le niveau sélectionné est mis en évidence (couleur vive) ; les autres sont grisés.
	EDemoDifficulty CurDiff = EDemoDifficulty::Normal;
	if (UWorld* Wd = GetWorld())
		if (UGameInstance* GI = Wd->GetGameInstance())
			if (UDemoFlowSubsystem* D = GI->GetSubsystem<UDemoFlowSubsystem>())
				CurDiff = D->GetDifficulty();

	DrawCenteredText(TEXT("DIFFICULTE"), DifficultyButtonRect(0, W, H).Min.Y - H * 0.055f,
		FLinearColor(0.95f, 0.85f, 0.4f, 1.f), 1.3f);
	const TCHAR* DLabels[3] = { TEXT("FACILE"), TEXT("NORMAL"), TEXT("DIFFICILE") };
	const EDemoDifficulty DVals[3] = { EDemoDifficulty::Facile, EDemoDifficulty::Normal, EDemoDifficulty::Difficile };
	for (int32 i = 0; i < 3; ++i)
	{
		const bool bSel = (CurDiff == DVals[i]);
		const FLinearColor Col = bSel ? FLinearColor(1.f, 0.85f, 0.3f, 1.f)   // sélectionné : or vif
									  : FLinearColor(0.45f, 0.5f, 0.6f, 1.f); // autre : grisé
		DrawButton(DifficultyButtonRect(i, W, H), DLabels[i], Col, bSel ? 1.3f : 1.05f);
	}

	const bool bReady = Selected != EFactionID::None;
	DrawButton(FactionLaunchButtonRect(W, H),
		bReady ? TEXT("LANCER LA PARTIE") : TEXT("CHOISISSEZ UNE FACTION"),
		bReady ? FLinearColor(1.f, 0.72f, 0.22f, 1.f) : FLinearColor(0.38f, 0.42f, 0.48f, 1.f),
		1.35f);
}

void AWOTOLDemoHUD::DrawHeroCustomization(float W, float H, UDemoFlowSubsystem* Demo)
{
	DrawUnderwaterBackground(W, H);
	if (Demo) DrawFactionAmbientTint(W, H, Demo->SelectedFaction);
	DrawGlowTitle(TEXT("PERSONNALISATION DU HEROS"), H * 0.10f, 2.0f, FLinearColor(0.7f, 0.9f, 1.f, 1.f));
	if (!Demo) return;
	const FHeroLoadout& Loadout = Demo->GetHeroLoadout();

	// Choix Aquis / Aquira — Aquiloris uniquement (demande de Liamor, 25/07/2026). Stats et
	// capacites strictement identiques (Role Chef) ; celui non choisi devient le chef de
	// faction en narration/PNJ. Les Noxeens n'ont pas ce choix (Noxar reste seul chef jouable).
	if (Demo->SelectedFaction == EFactionID::Aquiloris)
	{
		DrawCenteredText(TEXT("INCARNATION"), HeroAquilorisVariantButtonRect(0, W, H).Min.Y - H * 0.04f,
			FLinearColor(0.95f, 0.85f, 0.4f, 1.f), 1.1f);
		const TCHAR* VariantLabels[2] = { TEXT("AQUIS"), TEXT("AQUIRA") };
		for (int32 i = 0; i < 2; ++i)
		{
			const bool bSel = (Loadout.bPlayAsAquira == (i == 1));
			const FLinearColor Col = bSel ? FLinearColor(0.4f, 0.85f, 1.f, 1.f) : FLinearColor(0.4f, 0.45f, 0.52f, 1.f);
			DrawButton(HeroAquilorisVariantButtonRect(i, W, H), VariantLabels[i], Col, bSel ? 1.1f : 1.0f);
		}
		DrawCenteredText(Loadout.bPlayAsAquira
				? TEXT("Aquira, reine des Aquiloris. Aquis dirige la faction en votre absence.")
				: TEXT("Aquis, chef des Aquiloris. Aquira dirige la faction en votre absence."),
			HeroAquilorisVariantButtonRect(0, W, H).Max.Y + H * 0.02f,
			FLinearColor(0.8f, 0.9f, 1.f, 0.9f), 0.85f);
	}

	DrawCenteredText(TEXT("HERITAGE"), HeroHeritageButtonRect(0, W, H).Min.Y - H * 0.045f,
		FLinearColor(0.95f, 0.85f, 0.4f, 1.f), 1.2f);
	const TCHAR* HeritageLabels[4] = { TEXT("THALASSI"), TEXT("GIVRELERE"), TEXT("ABYSSEEN"), TEXT("GARDIEN") };
	const EHeroHeritage HeritageVals[4] = { EHeroHeritage::Thalassi, EHeroHeritage::Givrelier, EHeroHeritage::Abysseen, EHeroHeritage::Gardien };
	int32 HeritageIdx = 0;
	for (int32 i = 0; i < 4; ++i)
	{
		const bool bSel = (Loadout.Heritage == HeritageVals[i]);
		if (bSel) HeritageIdx = i;
		const FLinearColor Col = bSel ? FLinearColor(0.4f, 0.85f, 1.f, 1.f) : FLinearColor(0.4f, 0.45f, 0.52f, 1.f);
		DrawButton(HeroHeritageButtonRect(i, W, H), HeritageLabels[i], Col, bSel ? 1.15f : 1.0f);
	}
	const TCHAR* HeritageDesc[4] = {
		TEXT("Sang des courants de surface — rapide et adaptable."),
		TEXT("Glace des abysses polaires — endurance accrue."),
		TEXT("Nuit des grands fonds — camouflage et perception."),
		TEXT("Lignee protectrice — robustesse au combat.")
	};
	DrawCenteredText(HeritageDesc[HeritageIdx], HeroHeritageButtonRect(0, W, H).Max.Y + H * 0.025f,
		FLinearColor(0.8f, 0.9f, 1.f, 0.9f), 0.9f);

	DrawCenteredText(TEXT("SPECIALITE"), HeroSpecialtyButtonRect(0, W, H).Min.Y - H * 0.045f,
		FLinearColor(0.95f, 0.85f, 0.4f, 1.f), 1.2f);
	const TCHAR* SpecialtyLabels[4] = { TEXT("THALASSI"), TEXT("GUERRIER"), TEXT("MAGE"), TEXT("INQUISITEUR") };
	const EHeroSpecialty SpecialtyVals[4] = { EHeroSpecialty::Thalassi, EHeroSpecialty::Guerrier, EHeroSpecialty::Mage, EHeroSpecialty::Inquisiteur };
	int32 SpecialtyIdx = 0;
	for (int32 i = 0; i < 4; ++i)
	{
		const bool bSel = (Loadout.Specialty == SpecialtyVals[i]);
		if (bSel) SpecialtyIdx = i;
		const FLinearColor Col = bSel ? FLinearColor(1.f, 0.72f, 0.22f, 1.f) : FLinearColor(0.4f, 0.45f, 0.52f, 1.f);
		DrawButton(HeroSpecialtyButtonRect(i, W, H), SpecialtyLabels[i], Col, bSel ? 1.15f : 1.0f);
	}
	const TCHAR* SpecialtyDesc[4] = {
		TEXT("Combat polyvalent, equilibre attaque/defense."),
		TEXT("Force brute, degats de melee eleves."),
		TEXT("Maitrise des courants, degats a distance/zone."),
		TEXT("Traque et controle, cible les ennemis isoles.")
	};
	DrawCenteredText(SpecialtyDesc[SpecialtyIdx], HeroSpecialtyButtonRect(0, W, H).Max.Y + H * 0.025f,
		FLinearColor(0.8f, 0.9f, 1.f, 0.9f), 0.9f);

	// Portrait : simple index cyclable + halo teinté par la faction, en attendant de vrais
	// portraits illustrés (aucun asset de ce type n'existe encore côté Content).
	DrawCenteredText(TEXT("PORTRAIT"), H * 0.635f, FLinearColor(0.95f, 0.85f, 0.4f, 1.f), 1.1f);
	if (Canvas)
	{
		const FVector2D Center(W * 0.5f, H * 0.66f + 30.f);
		const FLinearColor FacCol = FFactionColors::Get(Demo->SelectedFaction);
		Canvas->K2_DrawPolygon(nullptr, Center, FVector2D(46.f, 46.f), 16, FLinearColor(FacCol.R, FacCol.G, FacCol.B, 0.2f));
		Canvas->K2_DrawPolygon(nullptr, Center, FVector2D(30.f, 30.f), 16, FacCol);
	}
	DrawButton(HeroPortraitPrevRect(W, H), TEXT("<"), FLinearColor(0.5f, 0.55f, 0.62f, 1.f), 1.3f);
	DrawButton(HeroPortraitNextRect(W, H), TEXT(">"), FLinearColor(0.5f, 0.55f, 0.62f, 1.f), 1.3f);
	DrawCenteredText(FString::Printf(TEXT("%d / 5"), Loadout.PortraitIndex + 1), H * 0.66f + 70.f,
		FLinearColor::White, 1.0f);

	DrawButton(HeroCustomizationBackRect(W, H), TEXT("< RETOUR"), FLinearColor(0.4f, 0.45f, 0.52f, 1.f), 1.0f);
	DrawButton(HeroCustomizationConfirmRect(W, H), TEXT("CONFIRMER LE HEROS"), FLinearColor(1.f, 0.72f, 0.22f, 1.f), 1.3f);
}

void AWOTOLDemoHUD::DrawPreGameSummary(float W, float H, UDemoFlowSubsystem* Demo)
{
	DrawUnderwaterBackground(W, H);
	if (Demo) DrawFactionAmbientTint(W, H, Demo->SelectedFaction);
	DrawGlowTitle(TEXT("RESUME DE LA PARTIE"), H * 0.10f, 2.2f, FLinearColor(0.7f, 0.9f, 1.f, 1.f));
	if (!Demo) return;

	const FHeroLoadout& Loadout = Demo->GetHeroLoadout();
	const FLinearColor Accent = FFactionColors::Get(Demo->SelectedFaction);

	const TCHAR* HeritageLabels[4] = { TEXT("Thalassi"), TEXT("Givrelere"), TEXT("Abysseen"), TEXT("Gardien") };
	const EHeroHeritage HeritageVals[4] = { EHeroHeritage::Thalassi, EHeroHeritage::Givrelier, EHeroHeritage::Abysseen, EHeroHeritage::Gardien };
	FString HeritageLabel = TEXT("?");
	for (int32 i = 0; i < 4; ++i) if (HeritageVals[i] == Loadout.Heritage) HeritageLabel = HeritageLabels[i];

	const TCHAR* SpecialtyLabels[4] = { TEXT("Thalassi"), TEXT("Guerrier"), TEXT("Mage"), TEXT("Inquisiteur") };
	const EHeroSpecialty SpecialtyVals[4] = { EHeroSpecialty::Thalassi, EHeroSpecialty::Guerrier, EHeroSpecialty::Mage, EHeroSpecialty::Inquisiteur };
	FString SpecialtyLabel = TEXT("?");
	for (int32 i = 0; i < 4; ++i) if (SpecialtyVals[i] == Loadout.Specialty) SpecialtyLabel = SpecialtyLabels[i];

	const TCHAR* DiffLabels[3] = { TEXT("Facile"), TEXT("Normal"), TEXT("Difficile") };
	const EDemoDifficulty DiffVals[3] = { EDemoDifficulty::Facile, EDemoDifficulty::Normal, EDemoDifficulty::Difficile };
	FString DiffLabel = TEXT("Normal");
	for (int32 i = 0; i < 3; ++i) if (DiffVals[i] == Demo->GetDifficulty()) DiffLabel = DiffLabels[i];

	const FBox2D Panel(FVector2D(W * 0.5f - 420.f, H * 0.22f), FVector2D(W * 0.5f + 420.f, H * 0.74f));
	DrawRect(FLinearColor(0.01f, 0.05f, 0.09f, 0.88f), Panel.Min.X, Panel.Min.Y,
		Panel.Max.X - Panel.Min.X, Panel.Max.Y - Panel.Min.Y);
	DrawLine(Panel.Min.X, Panel.Min.Y, Panel.Max.X, Panel.Min.Y, Accent, 3.f);

	// Deux colonnes, façon UI_ResumePartie.png (Faction | Difficulte, Heritage | Specialite).
	const float ColL = Panel.Min.X + 50.f, ColR = Panel.Min.X + 470.f;
	float Y = Panel.Min.Y + 40.f;
	auto Row = [&](float X, const FString& Label, const FString& Value)
	{
		DrawText(Label, FLinearColor(0.75f, 0.82f, 0.9f, 0.9f), X, Y, GEngine ? GEngine->GetSmallFont() : nullptr, 0.9f);
		DrawText(Value, FLinearColor::White, X, Y + 24.f, GEngine ? GEngine->GetMediumFont() : nullptr, 1.1f);
	};
	Row(ColL, TEXT("HEROS"), Loadout.HeroName);
	Y += 84.f;
	Row(ColL, TEXT("FACTION"), Demo->SelectedFaction == EFactionID::Noxeens ? TEXT("Noxeens") : TEXT("Aquiloris"));
	Row(ColR, TEXT("NIVEAU DE DIFFICULTE"), DiffLabel);
	Y += 84.f;
	Row(ColL, TEXT("HERITAGE"), HeritageLabel);
	Row(ColR, TEXT("SPECIALITE"), SpecialtyLabel);
	Y += 84.f;
	Row(ColL, TEXT("PORTRAIT"), FString::Printf(TEXT("%d / 5"), Loadout.PortraitIndex + 1));

	DrawButton(PreGameSummaryBackRect(W, H), TEXT("< RETOUR"), FLinearColor(0.4f, 0.45f, 0.52f, 1.f), 1.0f);
	DrawButton(PreGameSummaryLaunchRect(W, H), TEXT("LANCER LA PARTIE"), FLinearColor(1.f, 0.72f, 0.22f, 1.f), 1.35f);
}

void AWOTOLDemoHUD::DrawExplorationHUD(float W, float H, UDemoFlowSubsystem* Demo)
{
	// On garde le monde 3D visible : seulement deux bandeaux translucides, jamais le HUD RTS.
	DrawRect(FLinearColor(0.f, 0.02f, 0.06f, 0.72f), 0.f, 0.f, W, 94.f);
	DrawRect(FLinearColor(0.f, 0.02f, 0.06f, 0.68f), 0.f, H - 72.f, W, 72.f);
	DrawCenteredText(TEXT("EXPLORATION — NOUVELLE ZONE"), 20.f,
		FLinearColor(0.55f, 0.88f, 1.f, 1.f), 1.45f);
	if (Demo && !Demo->ObjectiveText.IsEmpty())
	{
		DrawCenteredText(Demo->ObjectiveText, 57.f, FLinearColor::White, 1.0f);
	}
	DrawCenteredText(TEXT("ZQSD/WASD : nager | Souris : regarder | Espace/E : monter | Maj/Ctrl : descendre | Alt : sprint | C : ruee"),
		H - 48.f, FLinearColor(0.86f, 0.93f, 1.f, 0.95f), 0.9f);

	// Après le rapport du Kraken, le Cristalliseur arrive dans un véritable inventaire de
	// bâtiment. Le joueur doit sélectionner cette carte puis cliquer la cible 3D lumineuse.
	if (Demo && Demo->GetPhase() == EDemoPhase::Capture_Zone
		&& !Demo->GetProgress().bZoneCaptured)
	{
		const bool bNox = Demo->GetPlayerFaction() == EFactionID::Noxeens;
		DrawButton(ExplorationCrystalliserButtonRect(W, H),
			bNox ? TEXT("BATIMENT : ABYSSALYSEUR") : TEXT("BATIMENT : CRISTALLISEUR"),
			FFactionColors::Get(Demo->GetPlayerFaction()), 1.05f);
		DrawText(FString::Printf(TEXT("Cristaux %d   |   Mineraux %d"),
			Demo->GetCrystals(), Demo->PlayerAbyssalMaterials),
			FLinearColor(1.f, 0.94f, 0.58f, 1.f), 38.f, H - 87.f,
			GEngine ? GEngine->GetMediumFont() : nullptr, 0.9f);
	}
}

// ─── Vue CITÉ ────────────────────────────────────────────────────────────────
int32 AWOTOLDemoHUD::CityCardCount() { return 5; }

EDemoUnitCategory AWOTOLDemoHUD::CityCardCategory(int32 Index)
{
	static const EDemoUnitCategory Cats[5] = {
		EDemoUnitCategory::Infanterie, EDemoUnitCategory::Distance,
		EDemoUnitCategory::Montee,     EDemoUnitCategory::Speciale,
		EDemoUnitCategory::Mythique };
	return Cats[FMath::Clamp(Index, 0, 4)];
}

FBox2D AWOTOLDemoHUD::CityCardRect(int32 Index, float W, float H)
{
	const int32 N = CityCardCount();
	const float CW = FMath::Min(230.f, (W * 0.82f) / N);
	const float CH = 168.f;
	const float Gap = 16.f;
	const float TotalW = N * CW + (N - 1) * Gap;
	const float StartX = (W - TotalW) * 0.5f;
	const float Y = H - CH - 46.f;
	const float X = StartX + Index * (CW + Gap);
	return FBox2D(FVector2D(X, Y), FVector2D(X + CW, Y + CH));
}

FBox2D AWOTOLDemoHUD::CityCardUpgradeRect(int32 Index, float W, float H)
{
	// Bandeau supérieur de la carte (les ~34 px du haut).
	const FBox2D R = CityCardRect(Index, W, H);
	return FBox2D(R.Min, FVector2D(R.Max.X, R.Min.Y + 34.f));
}

FBox2D AWOTOLDemoHUD::CityDepartButtonRect(float W, float H)
{
	const float BW = 340.f, BH = 60.f;
	return FBox2D(FVector2D(W - BW - 40.f, 40.f), FVector2D(W - 40.f, 40.f + BH));
}

FBox2D AWOTOLDemoHUD::CityBuildPlotRect(int32 Index, float W, float H)
{
	const float BW = FMath::Clamp(W * 0.14f, 150.f, 230.f);
	const float BH = FMath::Clamp(H * 0.11f, 82.f, 122.f);
	const float Gap = W * 0.045f;
	const float TotalW = BW * 3.f + Gap * 2.f;
	const float X = (W - TotalW) * 0.5f + FMath::Clamp(Index, 0, 2) * (BW + Gap);
	const float Y = H * 0.29f + (Index == 1 ? -28.f : 18.f);
	return FBox2D(FVector2D(X, Y), FVector2D(X + BW, Y + BH));
}

FBox2D AWOTOLDemoHUD::CityFeedMythicButtonRect(float W, float H)
{
	const float BW = 460.f, BH = 70.f;
	return FBox2D(FVector2D((W - BW) * 0.5f, H * 0.49f),
		FVector2D((W + BW) * 0.5f, H * 0.49f + BH));
}

FBox2D AWOTOLDemoHUD::TerritoryRepairButtonRect(float W, float H)
{
	return FBox2D(FVector2D(48.f, H * 0.27f), FVector2D(408.f, H * 0.27f + 66.f));
}

FBox2D AWOTOLDemoHUD::TerritoryDefenseButtonRect(float W, float H)
{
	return FBox2D(FVector2D(48.f, H * 0.40f), FVector2D(408.f, H * 0.40f + 66.f));
}

FBox2D AWOTOLDemoHUD::TerritoryGarrisonMinusRect(int32 Index, float W, float H)
{
	const float Y = H * 0.31f + FMath::Clamp(Index, 0, 2) * 72.f;
	return FBox2D(FVector2D(W - 410.f, Y), FVector2D(W - 354.f, Y + 50.f));
}

FBox2D AWOTOLDemoHUD::TerritoryGarrisonPlusRect(int32 Index, float W, float H)
{
	const float Y = H * 0.31f + FMath::Clamp(Index, 0, 2) * 72.f;
	return FBox2D(FVector2D(W - 112.f, Y), FVector2D(W - 56.f, Y + 50.f));
}

FBox2D AWOTOLDemoHUD::TerritoryReturnCityButtonRect(float W, float H)
{
	const float BW = 430.f, BH = 66.f;
	return FBox2D(FVector2D((W - BW) * 0.5f, H - 104.f),
		FVector2D((W + BW) * 0.5f, H - 104.f + BH));
}

// Nom du bâtiment producteur (Aquiloris) / générique Noxéen, par catégorie.
// Noms alignés sur la liste des 22 bâtiments canoniques (doc Drive "CLAUDE — RÉFÉRENCE ACTIVE
// WOTOL", statut canonique au 24/07/2026, confirmée par sync ChatGPT du 25/07/2026) : TOUS les
// noms ci-dessous sont désormais CERTAINS, y compris Distance/Spéciale Noxéens qui n'étaient
// jusque-là que des estimations ("Foyer des Décharges" recrute les Noxeblasts, "Faille
// Abyssale" recrute les Noxeons — confirmé, l'estimation précédente pour cette dernière était
// déjà correcte). Les 6 autres bâtiments canoniques (Noyau Cristalin/Trône des profondeurs,
// Bastion Cristallin/Enceinte Noxéenne, et les 3 bâtiments de ressources partagées par
// faction) ne correspondent à aucune catégorie de recrutement du système de cartes actuel de
// la démo (Infanterie/Distance/Montée/Spéciale/Mythique) — hors scope de cette fonction,
// documentés dans TODO_WOTOL.md.
static FString CityBuildingLabel(EFactionID Fac, EDemoUnitCategory Cat)
{
	const bool bAq = (Fac != EFactionID::Noxeens);
	switch (Cat)
	{
		case EDemoUnitCategory::Infanterie: return bAq ? TEXT("Academie") : TEXT("Fosse d'Emergence");
		case EDemoUnitCategory::Distance:   return bAq ? TEXT("Champ de Tir") : TEXT("Foyer des Decharges");
		case EDemoUnitCategory::Montee:     return bAq ? TEXT("Dome des Aquilances") : TEXT("Cavite des Mastodontes");
		case EDemoUnitCategory::Speciale:   return bAq ? TEXT("Nexus des Ombres") : TEXT("Faille Abyssale");
		case EDemoUnitCategory::Mythique:   return bAq ? TEXT("Coeur-Eclat") : TEXT("Antre du Noxedrake");
		default: return TEXT("");
	}
}

// Nom d'unité affiché sur la carte, par catégorie/faction.
static FString CityUnitLabel(EFactionID Fac, EDemoUnitCategory Cat)
{
	const bool bAq = (Fac != EFactionID::Noxeens);
	switch (Cat)
	{
		case EDemoUnitCategory::Infanterie: return bAq ? TEXT("Aquiloryons") : TEXT("Noxeflare");
		case EDemoUnitCategory::Distance:   return bAq ? TEXT("Aquispheres")  : TEXT("Noxeblast");
		case EDemoUnitCategory::Montee:     return bAq ? TEXT("Aquilances")   : TEXT("Noxebeast");
		case EDemoUnitCategory::Speciale:   return bAq ? TEXT("Aquilombres") : TEXT("Noxeons");
		case EDemoUnitCategory::Mythique:   return bAq ? TEXT("Leviaphenix"): TEXT("Noxedrake");
		default: return TEXT("");
	}
}

// Déplacée plus haut (utilisée à la fois par l'onglet Compétences et par la fiche technique
// de la cité, qui affiche désormais l'axe choisi — demande de Liamor du 26/07/2026).
static FString SkillAxisLabel(EFactionID Fac, EDemoUnitCategory Cat, int32 Axis)
{
	if (Axis == 0) return TEXT("Base");
	const bool bAq = (Fac != EFactionID::Noxeens);
	switch (Cat)
	{
		case EDemoUnitCategory::Infanterie: return bAq ? (Axis==1?TEXT("Mur amplifie"):TEXT("Double Lames"))
		                                               : (Axis==1?TEXT("Voile Profond"):TEXT("Frappe Aveugle"));
		case EDemoUnitCategory::Distance:   return bAq ? (Axis==1?TEXT("Hydrosniper"):TEXT("Hydropompe"))
		                                               : (Axis==1?TEXT("Rayon Perforant"):TEXT("Explosion Biolum."));
		case EDemoUnitCategory::Montee:     return bAq ? (Axis==1?TEXT("Percee amplifiee"):TEXT("Rempart Synth."))
		                                               : (Axis==1?TEXT("Bastion Brutal"):TEXT("Defoncement"));
		case EDemoUnitCategory::Speciale:   return bAq ? (Axis==1?TEXT("Critique Amplifie"):TEXT("Ombres Projetees"))
		                                               : (Axis==1?TEXT("Reacteur de Guerre"):TEXT("Ancrage Abyssal"));
		case EDemoUnitCategory::Mythique:   return bAq ? (Axis==1?TEXT("Rayon Stabilisateur"):TEXT("Rayon Vital"))
		                                               : (Axis==1?TEXT("Devastation Totale"):TEXT("Dominion Radieux"));
		default: return (Axis==1?TEXT("Axe 1"):TEXT("Axe 2"));
	}
}

UTexture2D* AWOTOLDemoHUD::GetTransitionBackground()
{
	if (TransitionBgTexture || bTransitionBgTried) return TransitionBgTexture;
	bTransitionBgTried = true;
	const FString PngPath = FPaths::ProjectContentDir() / TEXT("UI/WOTOL_Transition_Background.png");
	if (FPaths::FileExists(PngPath))
	{
		TransitionBgTexture = FImageUtils::ImportFileAsTexture2D(PngPath);
	}
	return TransitionBgTexture;
}

void AWOTOLDemoHUD::DrawCityView(float W, float H, UDemoFlowSubsystem* Demo)
{
	if (!Demo) return;
	const EFactionID Fac = Demo->GetPlayerFaction();

	// La cité est maintenant une VRAIE scène 3D vue depuis une caméra isométrique fixe
	// (AWOTOLCityCamera/AWOTOLCityEnvironment, possédée automatiquement en entrant sur cet
	// écran — cf. AWOTOLDemoDirector::HandleScreenChanged/PossessCityCamera) : elle est déjà
	// rendue DERRIÈRE ce Canvas. On ne dessine donc plus d'image/dégradé plein écran ici (ça
	// la masquerait entièrement) — seuls les bandeaux de chrome haut/bas restent, translucides,
	// pour garder les cartes/ressources lisibles sans cacher la maquette au centre.
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.34f), 0.f, 0.f, W, H * 0.16f);          // bandeau haut
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.46f), 0.f, H * 0.66f, W, H * 0.34f);    // bandeau bas (cartes)

	const bool bAq = (Fac != EFactionID::Noxeens);
	const FLinearColor Accent = FFactionColors::Get(Fac);
	const FString CityName = bAq ? TEXT("CITE D'AQUILOR") : TEXT("NOX CAVE");
	DrawGlowTitle(CityName, H * 0.04f, 2.2f, Accent);

	// Ressources persistantes gagnées en mission (haut-gauche). 4 ressources conformes aux
	// visuels Content/UI/Reference/Ressources : Cristaux/Biolumens (propre a la faction),
	// Mineraux Abyssaux, Biomasse, Energie Oceanique (SEULE a capacite de stockage limitee).
	const FString Res = bAq ? TEXT("Cristaux") : TEXT("Biolumens");
	DrawText(FString::Printf(TEXT("%s %d   |   Mineraux Abyssaux %d   |   Biomasse %d   |   Energie Oceanique %d/%d"),
		*Res, Demo->GetCrystals(), Demo->PlayerAbyssalMaterials, Demo->PlayerBiomass,
		Demo->PlayerOceanicEnergy, Demo->MaxOceanicEnergy),
		FLinearColor(1.f, 0.95f, 0.6f, 1.f), 44.f, 44.f, GEngine ? GEngine->GetLargeFont() : nullptr, 1.5f);
	DrawText(FString::Printf(TEXT("ARMEE : %d / %d"), Demo->GetArmyUnitCount(), Demo->GetArmyUnitCap()),
		Accent, 44.f, 84.f, GEngine ? GEngine->GetMediumFont() : nullptr, 1.15f);
	// Rappel de commandes caméra — absent jusqu'ici alors que TOUS les autres écrans pilotés
	// caméra en ont un (panneau CONTROLES en préparation, bandeau bas en exploration) ; sans
	// lui, rien n'indique qu'on peut déplacer/zoomer la caméra isométrique de cette vue.
	DrawText(TEXT("ZQSD/Fleches : deplacer la camera  |  Molette : zoom  |  Clic sur un batiment : details"),
		FLinearColor(0.75f, 0.82f, 0.9f, 0.85f), 44.f, 138.f, GEngine ? GEngine->GetSmallFont() : nullptr, 0.95f);

	// Progression persistante : visible à chaque passage en cité.
	const float ProgW = 230.f;
	DrawText(FString::Printf(TEXT("HEROS NIV. %d"), Demo->HeroLevel), FLinearColor::White,
		W - ProgW - 44.f, 116.f, nullptr, 0.9f);
	DrawBar(W - ProgW - 44.f, 138.f, ProgW, 10.f, Demo->GetHeroXPPercent(),
		FLinearColor(0.35f, 0.85f, 1.f, 1.f), FLinearColor(0.f, 0.f, 0.f, 0.7f));
	DrawText(FString::Printf(TEXT("CITE NIV. %d"), Demo->CityLevel), FLinearColor::White,
		W - ProgW - 44.f, 158.f, nullptr, 0.9f);
	DrawBar(W - ProgW - 44.f, 180.f, ProgW, 10.f, Demo->GetCityXPPercent(),
		FLinearColor(1.f, 0.75f, 0.25f, 1.f), FLinearColor(0.f, 0.f, 0.f, 0.7f));

	if (Demo->GetProgress().bDefenseSystemInstalled && !Demo->GetProgress().bMythicPlayable)
	{
		const FBox2D Feed = CityFeedMythicButtonRect(W, H);
		DrawRect(FLinearColor(0.01f, 0.04f, 0.08f, 0.88f), Feed.Min.X - 70.f,
			Feed.Min.Y - 100.f, (Feed.Max.X - Feed.Min.X) + 140.f, 190.f);
		DrawCenteredText(FString::Printf(TEXT("FAIRE GRANDIR LE %s"),
			*CityUnitLabel(Fac, EDemoUnitCategory::Mythique).ToUpper()),
			Feed.Min.Y - 75.f, Accent, 1.35f);
		DrawCenteredText(FString::Printf(TEXT("Biomasse : %d / %d"), Demo->PlayerBiomass,
			Demo->MythicGrowthBiomassGoal), Feed.Min.Y - 38.f,
			Demo->HasEnoughBiomassForMythic() ? FLinearColor(0.55f, 1.f, 0.6f, 1.f)
				: FLinearColor(1.f, 0.55f, 0.45f, 1.f), 1.05f);
		DrawButton(Feed, Demo->HasEnoughBiomassForMythic()
			? TEXT("NOURRIR ET LIBERER") : TEXT("BIOMASSE INSUFFISANTE"),
			Demo->HasEnoughBiomassForMythic() ? Accent
				: FLinearColor(0.38f, 0.40f, 0.45f, 1.f), 1.2f);
	}

	// Construction spatiale sur la vue isométrique : trois parcelles valides. Une fois posé,
	// le bâtiment reste représenté à son emplacement et sa carte devient son panneau d'action.
	if (Demo->IsCityBuildingPlacementArmed())
	{
		DrawCenteredText(TEXT("CHOISISSEZ UN EMPLACEMENT LIBRE POUR LE BATIMENT A DISTANCE"),
			H * 0.20f, FLinearColor(1.f, 0.88f, 0.35f, 1.f), 1.2f);
		for (int32 Plot = 0; Plot < 3; ++Plot)
		{
			DrawButton(CityBuildPlotRect(Plot, W, H),
				FString::Printf(TEXT("EMPLACEMENT %d"), Plot + 1), Accent, 0.95f);
		}
	}
	else if (Demo->IsRangedBuildingConstructed() && Demo->RangedBuildingPlotIndex != INDEX_NONE)
	{
		const FBox2D Plot = CityBuildPlotRect(Demo->RangedBuildingPlotIndex, W, H);
		DrawRect(FLinearColor(0.02f, 0.10f, 0.16f, 0.86f), Plot.Min.X, Plot.Min.Y,
			Plot.Max.X - Plot.Min.X, Plot.Max.Y - Plot.Min.Y);
		DrawLine(Plot.Min.X, Plot.Min.Y, Plot.Max.X, Plot.Min.Y, Accent, 4.f);
		DrawText(bAq ? TEXT("CENTRE AQUISFERES") : TEXT("FOSSE NOX BLAST"), FLinearColor::White,
			Plot.Min.X + 12.f, Plot.Min.Y + 22.f, GEngine ? GEngine->GetMediumFont() : nullptr, 1.0f);
		DrawText(TEXT("BATIMENT ACTIF — NIV. 1"), Accent,
			Plot.Min.X + 12.f, Plot.Min.Y + 56.f, nullptr, 0.9f);
	}

	const bool bStrategicAlert = Demo->GetProgress().bZoneThreatened
		|| Demo->GetProgress().bZoneLost;
	const FString Objective = Demo->bReadyForGrandBattleDeparture
		? Demo->ObjectiveText // "VOTRE CITE A GRANDI... Recrutez (X / Y)..." tenu à jour à chaque recrutement
		: bStrategicAlert
		? Demo->ObjectiveText
		: (Demo->GetProgress().bDefenseSystemInstalled && !Demo->GetProgress().bMythicPlayable
			? FString::Printf(TEXT("OBJECTIF : NOURRIR LE %s"),
				*CityUnitLabel(Fac, EDemoUnitCategory::Mythique).ToUpper())
			: (Demo->IsRangedProductionObjectiveComplete()
				? FString(TEXT("Objectif rempli — preparez vos ameliorations puis defendez la zone"))
				: FString::Printf(TEXT("OBJECTIF : PRODUIRE 10 UNITES A DISTANCE   %d / %d"),
					Demo->GetRangedProductionProgress(), Demo->RangedProductionTarget)));
	DrawCenteredText(Objective, H * 0.60f,
		Demo->bReadyForGrandBattleDeparture ? FLinearColor(0.95f, 0.75f, 0.15f, 1.f)
			: (bStrategicAlert && Demo->GetProgress().bZoneLost)
			? FLinearColor(1.f, 0.36f, 0.26f, 1.f)
			: Demo->IsRangedProductionObjectiveComplete() ? FLinearColor(0.45f, 1.f, 0.55f, 1.f)
			: FLinearColor(0.9f, 0.95f, 1.f, 0.95f), 1.1f);

	// Cartes de production (bâtiments).
	for (int32 i = 0; i < CityCardCount(); ++i)
	{
		const EDemoUnitCategory Cat = CityCardCategory(i);
		const FBox2D R = CityCardRect(i, W, H);
		const int32 Cost = Demo->GetProductionCost(Cat);
		const bool bUnlocked = Demo->IsCategoryUnlocked(Cat);
		const bool bNeedsBuilding = Cat == EDemoUnitCategory::Distance
			&& !Demo->IsRangedBuildingConstructed();
		const bool bCanBuild = bNeedsBuilding
			&& Demo->CanAffordTerritoryBuilding(Demo->RangedBuildingCrystalCost,
				Demo->RangedBuildingAbyssalMaterialCost);
		const bool bAfford = bNeedsBuilding ? bCanBuild : Demo->CanProduce(Cat);
		const FName UnitID = Demo->GetUnitID(Fac, Cat);
		const int32 InReserve = Demo->GetReserveCount(UnitID);

		// Fond de carte : vif si productible, grisé si verrouillé/insuffisant.
		const FLinearColor CardBg = !bUnlocked ? FLinearColor(0.10f, 0.10f, 0.13f, 0.85f)
			: bAfford ? FLinearColor(0.08f, 0.16f, 0.24f, 0.92f)
			          : FLinearColor(0.14f, 0.12f, 0.10f, 0.90f);
		DrawRect(CardBg, R.Min.X, R.Min.Y, R.Max.X - R.Min.X, R.Max.Y - R.Min.Y);
		const FLinearColor Border = bAfford ? Accent : FLinearColor(0.4f, 0.42f, 0.48f, 1.f);
		DrawLine(R.Min.X, R.Min.Y, R.Max.X, R.Min.Y, Border, 2.f);
		DrawLine(R.Min.X, R.Max.Y, R.Max.X, R.Max.Y, Border, 2.f);
		DrawLine(R.Min.X, R.Min.Y, R.Min.X, R.Max.Y, Border, 2.f);
		DrawLine(R.Max.X, R.Min.Y, R.Max.X, R.Max.Y, Border, 2.f);
		// Surbrillance quand ce bâtiment est sélectionné (clic 3D sur la maquette isométrique
		// OU clic sur cette carte) : cadre épais blanc en plus du cadre de couleur normal.
		if (Demo->HasCitySelection() && Demo->SelectedCityCategory == Cat)
		{
			DrawLine(R.Min.X - 3.f, R.Min.Y - 3.f, R.Max.X + 3.f, R.Min.Y - 3.f, FLinearColor::White, 3.f);
			DrawLine(R.Min.X - 3.f, R.Max.Y + 3.f, R.Max.X + 3.f, R.Max.Y + 3.f, FLinearColor::White, 3.f);
			DrawLine(R.Min.X - 3.f, R.Min.Y - 3.f, R.Min.X - 3.f, R.Max.Y + 3.f, FLinearColor::White, 3.f);
			DrawLine(R.Max.X + 3.f, R.Min.Y - 3.f, R.Max.X + 3.f, R.Max.Y + 3.f, FLinearColor::White, 3.f);
		}
		// Badge "NOUVEAU !" : marque visuellement les bâtiments tout juste débloqués par la
		// croissance de phase 3 (Spéciale/Mythique) — matérialise le déblocage sans dépendre
		// uniquement du texte de l'interlude (demande de Liamor du 26/07/2026).
		if (Demo->bReadyForGrandBattleDeparture
			&& (Cat == EDemoUnitCategory::Speciale || Cat == EDemoUnitCategory::Mythique))
		{
			const FString Badge = TEXT("NOUVEAU !");
			float BgW, BgH; GetTextSize(Badge, BgW, BgH, GEngine ? GEngine->GetSmallFont() : nullptr, 1.0f);
			DrawRect(FLinearColor(0.95f, 0.75f, 0.15f, 0.95f),
				R.Max.X - BgW - 16.f, R.Min.Y - BgH * 0.5f - 6.f, BgW + 12.f, BgH + 8.f);
			DrawText(Badge, FLinearColor(0.08f, 0.06f, 0.02f, 1.f), R.Max.X - BgW - 10.f,
				R.Min.Y - BgH * 0.5f - 2.f, GEngine ? GEngine->GetSmallFont() : nullptr, 1.0f);
		}

		const float CX = R.Min.X + 12.f;
		// Bandeau HAUT : niveau du bâtiment + bouton « Améliorer » (niv. bâtiment = niv. unités).
		const int32 BLevel = Demo->GetBuildingLevel(Cat);
		const int32 UpCost = Demo->GetBuildingUpgradeCost(Cat);
		const bool  bCanUp = Demo->CanUpgradeBuilding(Cat);
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.35f), R.Min.X, R.Min.Y, R.Max.X - R.Min.X, 34.f);
		const FString LevelLabel = bNeedsBuilding
			? FString(TEXT("A CONSTRUIRE")) : FString::Printf(TEXT("Niv.%d"), BLevel);
		DrawText(LevelLabel, Accent, CX, R.Min.Y + 8.f, nullptr, bNeedsBuilding ? 0.9f : 1.0f);
		if (bNeedsBuilding)
		{
			DrawText(TEXT("Selectionnez puis placez"), FLinearColor(1.f, 0.9f, 0.5f, 1.f),
				CX + 92.f, R.Min.Y + 8.f, nullptr, 0.78f);
		}
		else if (UpCost > 0)
		{
			DrawText(FString::Printf(TEXT("Ameliorer (%d)"), UpCost),
				bCanUp ? FLinearColor(1.f, 0.9f, 0.5f, 1.f) : FLinearColor(0.6f, 0.6f, 0.65f, 1.f),
				CX + 78.f, R.Min.Y + 8.f, nullptr, 0.95f);
		}
		else if (!bNeedsBuilding)
		{
			DrawText(TEXT("Niveau max"), FLinearColor(0.6f, 0.7f, 0.6f, 1.f), CX + 78.f, R.Min.Y + 8.f, nullptr, 0.95f);
		}
		DrawText(CityBuildingLabel(Fac, Cat), Border, CX, R.Min.Y + 40.f, GEngine ? GEngine->GetMediumFont() : nullptr, 0.9f);
		DrawText(CityUnitLabel(Fac, Cat), FLinearColor::White, CX, R.Min.Y + 66.f, GEngine ? GEngine->GetLargeFont() : nullptr, 1.1f);

		if (!bUnlocked)
		{
			DrawText(TEXT("Verrouille"), FLinearColor(0.7f, 0.7f, 0.75f, 1.f), CX, R.Min.Y + 96.f, nullptr, 1.f);
		}
		else if (bNeedsBuilding)
		{
			DrawText(FString::Printf(TEXT("Construction : %d cristaux + %d mineraux abyssaux"),
				Demo->RangedBuildingCrystalCost, Demo->RangedBuildingAbyssalMaterialCost),
				bCanBuild ? FLinearColor(1.f, 0.95f, 0.6f, 1.f) : FLinearColor(1.f, 0.55f, 0.5f, 1.f),
				CX, R.Min.Y + 96.f, nullptr, 0.82f);
			DrawText(bCanBuild ? TEXT("+ CONSTRUIRE") : TEXT("RESSOURCES INSUFFISANTES"),
				bCanBuild ? Accent : FLinearColor(0.7f, 0.5f, 0.5f, 1.f),
				CX, R.Max.Y - 28.f, GEngine ? GEngine->GetMediumFont() : nullptr, 0.9f);
		}
		else if (Cat == EDemoUnitCategory::Mythique)
		{
			// Créature unique élevée par la narration (Feed & Grow) : jamais recrutable en
			// série en cité, déjà comptée dans l'armée automatiquement.
			DrawText(TEXT("Deja dans votre armee (creature unique)"),
				FLinearColor(0.7f, 0.85f, 0.75f, 1.f), CX, R.Min.Y + 96.f, nullptr, 0.85f);
		}
		else
		{
			DrawText(FString::Printf(TEXT("Cout : %d   Reserve : %d"), Cost, InReserve),
				bAfford ? FLinearColor(1.f, 0.95f, 0.6f, 1.f) : FLinearColor(1.f, 0.55f, 0.5f, 1.f),
				CX, R.Min.Y + 96.f, nullptr, 0.95f);
			const FString Action = bAfford ? TEXT("+ PRODUIRE")
				: (Demo->GetArmyUnitCount() >= Demo->GetArmyUnitCap()
					? TEXT("PLAFOND D'ARMEE") : TEXT("INDISPONIBLE / RESERVE OBJECTIF"));
			DrawText(Action, bAfford ? Accent : FLinearColor(0.7f, 0.5f, 0.5f, 1.f),
				CX, R.Max.Y - 28.f, GEngine ? GEngine->GetMediumFont() : nullptr, 0.82f);
		}
	}

	// ─── FICHE TECHNIQUE : détail du bâtiment sélectionné (clic 3D sur la maquette
	// isométrique OU clic sur sa carte). Placée à droite, dans la bande centrale laissée
	// libre par les bandeaux haut/bas -> ne masque ni les ressources ni les cartes.
	if (Demo->HasCitySelection())
	{
		const EDemoUnitCategory SelCat = Demo->SelectedCityCategory;
		const FBox2D Panel(FVector2D(W - 380.f, H * 0.22f), FVector2D(W - 20.f, H * 0.58f));
		DrawRect(FLinearColor(0.01f, 0.05f, 0.09f, 0.90f), Panel.Min.X, Panel.Min.Y,
			Panel.Max.X - Panel.Min.X, Panel.Max.Y - Panel.Min.Y);
		DrawLine(Panel.Min.X, Panel.Min.Y, Panel.Max.X, Panel.Min.Y, Accent, 3.f);

		float Y = Panel.Min.Y + 16.f;
		// Illustration officielle réelle (même image que le plan 3D affiché dans la scène —
		// cohérence demandée par Liamor le 25/07/2026 : pas d'écran qui contredit la vue 3D).
		if (UTexture2D* Icon = WOTOLBuildingArt::GetBuildingIcon(Fac, SelCat))
		{
			const float ImgSize = 108.f;
			const float ImgX = (Panel.Min.X + Panel.Max.X) * 0.5f - ImgSize * 0.5f;
			DrawTexture(Icon, ImgX, Y, ImgSize, ImgSize, 0.f, 0.f, 1.f, 1.f);
			Y += ImgSize + 6.f;
		}
		DrawCenteredText(CityBuildingLabel(Fac, SelCat).ToUpper(), Y, Accent, 1.15f); Y += 34.f;
		DrawCenteredText(CityUnitLabel(Fac, SelCat), Y, FLinearColor::White, 1.0f); Y += 42.f;

		const bool bSelUnlocked = Demo->IsCategoryUnlocked(SelCat);
		if (!bSelUnlocked)
		{
			DrawCenteredText(TEXT("VERROUILLE"), Y, FLinearColor(0.85f, 0.4f, 0.35f, 1.f), 1.0f); Y += 30.f;
			DrawCenteredText(TEXT("Se debloque plus tard dans la demo."), Y,
				FLinearColor(0.75f, 0.78f, 0.85f, 0.9f), 0.85f);
		}
		else if (SelCat == EDemoUnitCategory::Distance && !Demo->IsRangedBuildingConstructed())
		{
			DrawCenteredText(TEXT("PAS ENCORE CONSTRUIT"), Y, FLinearColor(1.f, 0.85f, 0.4f, 1.f), 1.0f); Y += 30.f;
			DrawCenteredText(FString::Printf(TEXT("Cout : %d cristaux + %d mineraux abyssaux"),
				Demo->RangedBuildingCrystalCost, Demo->RangedBuildingAbyssalMaterialCost), Y,
				FLinearColor(0.85f, 0.9f, 1.f, 0.9f), 0.9f); Y += 26.f;
			DrawCenteredText(TEXT("Choisissez un emplacement via sa carte en bas."), Y,
				FLinearColor(0.7f, 0.75f, 0.85f, 0.85f), 0.8f);
		}
		else
		{
			// Niveau UNIQUE (1-3) : améliore à la fois attaque ET défense des unités de cette
			// catégorie (×1.0/1.15/1.30, cf. WOTOLDemoDirector::SpawnUnit) — pas deux jauges
			// séparées dans ce système, on l'affiche donc explicitement pour lever l'ambiguïté
			// (demande de detail par batiment de Liamor du 26/07/2026).
			const int32 BLvl = Demo->GetBuildingLevel(SelCat);
			DrawCenteredText(FString::Printf(TEXT("NIVEAU %d / %d  (attaque + defense)"),
				BLvl, UDemoFlowSubsystem::MaxBuildingLevel),
				Y, FLinearColor(0.95f, 0.85f, 0.4f, 1.f), 1.0f); Y += 30.f;

			// Voie tactique choisie (axe de compétence, réglable dans l'onglet COMPETENCES).
			if (SelCat != EDemoUnitCategory::Chef)
			{
				const int32 Axis = Demo->GetUnitAxis(SelCat);
				DrawCenteredText(FString::Printf(TEXT("Voie tactique : %s"),
					*SkillAxisLabel(Fac, SelCat, Axis)),
					Y, FLinearColor(0.65f, 0.9f, 1.f, 0.95f), 0.9f); Y += 28.f;
			}

			const int32 UpCost = Demo->GetBuildingUpgradeCost(SelCat);
			if (UpCost > 0)
			{
				DrawCenteredText(FString::Printf(TEXT("Amelioration : %d cristaux"), UpCost), Y,
					Demo->CanUpgradeBuilding(SelCat) ? FLinearColor(0.6f, 1.f, 0.65f, 1.f)
						: FLinearColor(1.f, 0.6f, 0.55f, 1.f), 0.9f);
			}
			else
			{
				DrawCenteredText(TEXT("Niveau maximum atteint"), Y, FLinearColor(0.7f, 0.85f, 1.f, 0.9f), 0.9f);
			}
			Y += 34.f;

			const FName SelUnitID = Demo->GetUnitID(Fac, SelCat);
			DrawCenteredText(FString::Printf(TEXT("Cout de production : %d   |   Reserve : %d"),
				Demo->GetProductionCost(SelCat), Demo->GetReserveCount(SelUnitID)), Y,
				FLinearColor(0.85f, 0.9f, 1.f, 0.9f), 0.85f);
		}
	}

	// Bouton d'expédition + bouton compétences. 3e état prioritaire : cité déjà débloquée
	// (phase 2 -> 3), le joueur embarque pour la grande bataille au lieu de repartir en défense.
	// Avertissement NON BLOQUANT (le joueur reste libre d'embarquer sous-effectif s'il le veut,
	// mais rien ne devait le prevenir avant) si l'armee recrutee est tres faible face au
	// contingent ennemi fixe (60/100 selon la faction) : recherche autonome du 26/07/2026.
	const bool bArmyLow = Demo->bReadyForGrandBattleDeparture
		&& Demo->GetArmyUnitCount() < Demo->GetArmyUnitCap() / 2;
	DrawButton(CityDepartButtonRect(W, H),
		Demo->bReadyForGrandBattleDeparture
			? (bArmyLow ? TEXT("EMBARQUER - ARMEE FAIBLE") : TEXT("EMBARQUER - GRANDE BATAILLE"))
			: Demo->IsDefenseMissionReady() ? TEXT("DEFENDRE LA ZONE") : TEXT("OBJECTIF : 10 UNITES"),
		Demo->bReadyForGrandBattleDeparture
			? (bArmyLow ? FLinearColor(0.9f, 0.35f, 0.25f, 1.f) : FLinearColor(0.95f, 0.75f, 0.15f, 1.f))
			: Demo->IsDefenseMissionReady() ? FLinearColor(1.f, 0.7f, 0.25f, 1.f)
			: FLinearColor(0.38f, 0.42f, 0.48f, 1.f), 1.15f);
	DrawButton(CitySkillsButtonRect(W, H), TEXT("COMPETENCES"),
		FLinearColor(0.6f, 0.8f, 1.f, 1.f), 1.2f);
}

void AWOTOLDemoHUD::DrawTerritoryView(float W, float H, UDemoFlowSubsystem* Demo)
{
	if (!Demo) return;
	const bool bNox = Demo->GetPlayerFaction() == EFactionID::Noxeens;
	const FLinearColor Accent = FFactionColors::Get(Demo->GetPlayerFaction());
	const FString Building = bNox ? TEXT("ABYSSALYSEUR") : TEXT("CRISTALLISEUR");
	// Noms des structures defensives alignes sur les planches Drive ajoutees le 23/07/2026 :
	// tourelle individuelle (emplacements installables) + rempart perimetrique du batiment central.
	const FString DefenseStructureName = bNox ? TEXT("OEIL BIOLUMINAL") : TEXT("TOURELLE HYDROCRISTALLINE");
	const FString RampartName = bNox ? TEXT("Entraves abyssales") : TEXT("Rempart cristallin");

	// Le monde 3D reste visible : deux panneaux latéraux encadrent le bâtiment et ses cinq
	// emplacements lumineux, au lieu de remplacer la zone par un menu abstrait.
	DrawRect(FLinearColor(0.01f, 0.03f, 0.06f, 0.88f), 24.f, 28.f, 420.f, H - 150.f);
	DrawRect(FLinearColor(0.01f, 0.03f, 0.06f, 0.88f), W - 444.f, 28.f, 420.f, H - 150.f);
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.64f), 0.f, 0.f, W, 112.f);
	DrawGlowTitle(TEXT("GESTION DU TERRITOIRE"), 24.f, 2.05f, Accent);
	DrawCenteredText(Demo->ObjectiveText, 76.f, FLinearColor(0.9f, 0.96f, 1.f, 1.f), 1.0f);

	DrawText(Building, Accent, 48.f, 138.f, GEngine ? GEngine->GetLargeFont() : nullptr, 1.3f);
	DrawBar(48.f, 180.f, 360.f, 18.f, Demo->GetTerritoryHealthPercent(),
		FLinearColor(0.30f, 0.92f, 0.48f, 1.f), FLinearColor(0.15f, 0.05f, 0.04f, 0.9f));
	DrawText(FString::Printf(TEXT("Integrite : %d %%   —   %s"),
		FMath::RoundToInt(Demo->GetTerritoryHealthPercent() * 100.f), *RampartName),
		FLinearColor::White, 48.f, 205.f, nullptr, 1.0f);
	DrawText(FString::Printf(TEXT("Cristaux %d   |   Mineraux %d"), Demo->GetCrystals(),
		Demo->PlayerAbyssalMaterials), FLinearColor(1.f, 0.90f, 0.42f, 1.f),
		48.f, 235.f, nullptr, 0.9f);

	const int32 RepairC = Demo->GetRepairCrystalCost();
	const int32 RepairM = Demo->GetRepairAbyssalMaterialCost();
	DrawButton(TerritoryRepairButtonRect(W, H), RepairC > 0
		? FString::Printf(TEXT("REPARER  —  %d C / %d M"), RepairC, RepairM)
		: FString(TEXT("BATIMENT REPARE")),
		RepairC > 0 && Demo->CanRepairTerritory() ? Accent
			: FLinearColor(0.34f, 0.38f, 0.42f, 1.f), 1.0f);

	const int32 NextDefense = Demo->InstalledDefenseCount + 1;
	const FString DefenseBuildingName = bNox ? TEXT("ENCEINTE NOXEENNE") : TEXT("BASTION CRISTALLIN");
	DrawText(FString::Printf(TEXT("%s — DEFENSES : %d / %d   —   TECHNOLOGIE NIV. %d"),
		*DefenseBuildingName, Demo->InstalledDefenseCount, Demo->GetDefenseCapacity(),
		Demo->DefenseTechnologyLevel),
		FLinearColor::White, 48.f, H * 0.365f - 28.f, nullptr, 0.92f);
	DrawButton(TerritoryDefenseButtonRect(W, H),
		Demo->CanInstallNextDefense()
			? FString::Printf(TEXT("PLACER %s  —  %d C / %d M"), *DefenseStructureName,
				Demo->DefenseInstallCrystalCost * NextDefense,
				Demo->DefenseInstallAbyssalMaterialCost * NextDefense)
			: (Demo->InstalledDefenseCount >= Demo->GetDefenseCapacity()
				? FString(TEXT("CAPACITE DE DEFENSE ATTEINTE"))
				: FString(TEXT("RESSOURCES INSUFFISANTES"))),
		Demo->CanInstallNextDefense() ? Accent : FLinearColor(0.34f, 0.38f, 0.42f, 1.f), 0.9f);
	DrawText(TEXT("Cliquez ensuite l'un des 5 emplacements lumineux."),
		FLinearColor(0.76f, 0.86f, 0.94f, 1.f), 48.f, H * 0.40f + 78.f, nullptr, 0.82f);

	const float RX = W - 420.f;
	DrawText(FString::Printf(TEXT("GARNISON : %d / %d"), Demo->GarrisonUnits,
		Demo->GetGarrisonCapacity()), Accent, RX, 138.f,
		GEngine ? GEngine->GetLargeFont() : nullptr, 1.25f);
	DrawText(TEXT("Ne compte pas dans l'armee de campagne"),
		FLinearColor(0.75f, 0.84f, 0.92f, 1.f), RX, 176.f, nullptr, 0.86f);
	const EDemoUnitCategory Cats[3] = { EDemoUnitCategory::Infanterie,
		EDemoUnitCategory::Montee, EDemoUnitCategory::Distance };
	const TCHAR* Labels[3] = { TEXT("Infanterie"), TEXT("Montees"), TEXT("Distance") };
	for (int32 i = 0; i < 3; ++i)
	{
		const FName UnitID = Demo->GetUnitID(Demo->GetPlayerFaction(), Cats[i]);
		const int32 Count = Demo->GetGarrisonCount(UnitID);
		const FBox2D Minus = TerritoryGarrisonMinusRect(i, W, H);
		const FBox2D Plus = TerritoryGarrisonPlusRect(i, W, H);
		DrawButton(Minus, TEXT("-"), Count > 0 ? Accent : FLinearColor(0.32f, 0.34f, 0.38f, 1.f), 1.2f);
		DrawButton(Plus, TEXT("+"), Demo->CanAssignGarrisonUnit()
			? Accent : FLinearColor(0.32f, 0.34f, 0.38f, 1.f), 1.2f);
		DrawText(FString::Printf(TEXT("%s     %d"), Labels[i], Count), FLinearColor::White,
			Minus.Max.X + 22.f, Minus.Min.Y + 14.f, GEngine ? GEngine->GetMediumFont() : nullptr, 1.0f);
	}

	const bool bCanReturn = RepairC <= 0 && Demo->InstalledDefenseCount > 0;
	DrawButton(TerritoryReturnCityButtonRect(W, H), bCanReturn
		? TEXT("VALIDER ET RETOURNER A LA CITE")
		: TEXT("REPAREZ ET INSTALLEZ UNE DEFENSE"),
		bCanReturn ? FLinearColor(1.f, 0.72f, 0.24f, 1.f)
			: FLinearColor(0.34f, 0.38f, 0.42f, 1.f), 1.05f);
}

FBox2D AWOTOLDemoHUD::CitySkillsButtonRect(float W, float H)
{
	const float BW = 220.f, BH = 60.f;
	return FBox2D(FVector2D(40.f, 40.f + 70.f), FVector2D(40.f + BW, 40.f + 70.f + BH));
}

FBox2D AWOTOLDemoHUD::SkillsAxisRect(int32 CatIndex, int32 AxisIndex, float W, float H)
{
	const float RowTop = H * 0.20f;
	const float RowH   = 92.f;
	const float BX     = W * 0.34f;      // colonne des boutons d'axe (après le nom)
	const float BW     = 210.f, BH = 66.f, Gap = 18.f;
	const float X = BX + AxisIndex * (BW + Gap);
	const float Y = RowTop + CatIndex * RowH;
	return FBox2D(FVector2D(X, Y), FVector2D(X + BW, Y + BH));
}

FBox2D AWOTOLDemoHUD::SkillsBackButtonRect(float W, float H)
{
	const float BW = 220.f, BH = 60.f;
	return FBox2D(FVector2D(40.f, H - BH - 40.f), FVector2D(40.f + BW, H - 40.f));
}

// Nom de la VOIE (axe) par faction/catégorie (0=Base, 1=Axe1, 2=Axe2) — d'après le GDD §7.
void AWOTOLDemoHUD::DrawVerticalLayerGauge(float W, float H, UWorld* World)
{
	if (!World || !Canvas) return;
	UDemoFlowSubsystem* Demo = GetGameInstance() ? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	UFactionRegistrySubsystem* Reg = World->GetSubsystem<UFactionRegistrySubsystem>();
	if (!Demo || !Reg) return;
	const EFactionID Fac = Demo->GetPlayerFaction();

	// Couche moyenne de la SÉLECTION (sinon de toute l'armée). DesiredZ : 0=SOL .. 2400=SURFACE.
	float SumZ = 0.f; int32 N = 0; int32 SumSel = 0;
	for (AUnitBase* U : Reg->GetUnitsForFaction(Fac))
	{
		AWOTOLDemoUnit* DU = Cast<AWOTOLDemoUnit>(U);
		if (!DU || !DU->IsAlive()) continue;
		if (DU->IsSelected()) { SumZ += DU->GetDesiredZ(); ++SumSel; }
	}
	if (SumSel > 0) { N = SumSel; }
	else
	{
		for (AUnitBase* U : Reg->GetUnitsForFaction(Fac))
		{
			AWOTOLDemoUnit* DU = Cast<AWOTOLDemoUnit>(U);
			if (!DU || !DU->IsAlive()) continue;
			SumZ += DU->GetDesiredZ(); ++N;
		}
	}
	if (N == 0) return;
	const float AvgZ = SumZ / N;            // 0..2400
	const float Frac = FMath::Clamp(AvgZ / 2400.f, 0.f, 1.f);

	// Barre verticale à gauche, centrée verticalement.
	const float GX = 34.f, GW = 26.f;
	const float GTop = H * 0.30f, GBot = H * 0.70f, GH = GBot - GTop;
	DrawRect(FLinearColor(0.03f, 0.06f, 0.10f, 0.75f), GX - 6.f, GTop - 30.f, GW + 12.f, GH + 60.f);
	// 3 bandes : SURFACE (haut) / MID / SOL (bas).
	const TCHAR* Labels[3] = { TEXT("SURFACE"), TEXT("MID"), TEXT("SOL") };
	for (int32 b = 0; b < 3; ++b)
	{
		const float y0 = GTop + GH * (b / 3.f);
		const float h  = GH / 3.f;
		// La bande active (contenant la couche moyenne) est mise en avant.
		const int32 ActiveBand = (Frac >= 0.66f) ? 0 : (Frac >= 0.33f ? 1 : 2);
		const bool bAct = (b == ActiveBand);
		DrawRect(bAct ? FLinearColor(0.15f, 0.45f, 0.75f, 0.55f) : FLinearColor(0.08f, 0.14f, 0.20f, 0.5f),
			GX, y0, GW, h - 2.f);
		DrawText(Labels[b], bAct ? FLinearColor(0.7f, 0.95f, 1.f, 1.f) : FLinearColor(0.5f, 0.6f, 0.7f, 1.f),
			GX + GW + 6.f, y0 + h * 0.5f - 8.f, nullptr, 0.85f);
	}
	// Curseur de la couche courante (petit repère).
	const float My = GBot - GH * Frac;
	DrawRect(FLinearColor(0.5f, 0.9f, 1.f, 1.f), GX - 4.f, My - 2.f, GW + 8.f, 4.f);
}

void AWOTOLDemoHUD::DrawSkillsView(float W, float H, UDemoFlowSubsystem* Demo)
{
	DrawUnderwaterBackground(W, H);
	if (!Demo) return;
	const EFactionID Fac = Demo->GetPlayerFaction();
	const FLinearColor Accent = FFactionColors::Get(Fac);
	DrawFactionAmbientTint(W, H, Fac);

	DrawGlowTitle(TEXT("COMPETENCES"), H * 0.06f, 2.2f, Accent);
	DrawCenteredText(TEXT("Choisissez la VOIE de chaque type d'unite (change son axe tactique)"),
		H * 0.14f, FLinearColor(0.9f, 0.95f, 1.f, 0.95f), 1.05f);

	for (int32 i = 0; i < CityCardCount(); ++i)
	{
		const EDemoUnitCategory Cat = CityCardCategory(i);
		const FBox2D R0 = SkillsAxisRect(i, 0, W, H);
		// Nom de l'unité à gauche de la ligne.
		DrawText(CityUnitLabel(Fac, Cat), FLinearColor::White, W * 0.08f, R0.Min.Y + 18.f,
			GEngine ? GEngine->GetLargeFont() : nullptr, 1.15f);
		const bool bUnlocked = Demo->IsCategoryUnlocked(Cat);
		const int32 Cur = Demo->GetUnitAxis(Cat);
		for (int32 a = 0; a < 3; ++a)
		{
			const FBox2D R = SkillsAxisRect(i, a, W, H);
			const bool bSel = (Cur == a);
			FLinearColor Tint = !bUnlocked ? FLinearColor(0.4f, 0.4f, 0.45f, 1.f)
				: bSel ? Accent : FLinearColor(0.55f, 0.6f, 0.7f, 1.f);
			DrawButton(R, SkillAxisLabel(Fac, Cat, a), Tint, bSel ? 1.05f : 0.9f);
		}
	}

	DrawButton(SkillsBackButtonRect(W, H), TEXT("RETOUR"), FLinearColor(0.8f, 0.8f, 0.4f, 1.f), 1.2f);
}

void AWOTOLDemoHUD::DrawLoadingScreen(float W, float H, UDemoFlowSubsystem* Demo)
{
	DrawUnderwaterBackground(W, H);

	const EFactionID Fac = Demo ? Demo->GetPlayerFaction() : EFactionID::None;
	const bool bNox = (Fac == EFactionID::Noxeens);
	const FLinearColor Accent = (Fac == EFactionID::None)
		? FLinearColor(0.50f, 0.85f, 1.f, 1.f) // pas encore de faction choisie : bleu neutre
		: FFactionColors::Get(Fac);
	DrawFactionAmbientTint(W, H, Fac);

	// Titre : nom de faction si connue, sinon le logo du jeu (comme les maquettes).
	const FString Title = (Fac == EFactionID::None) ? TEXT("WOTOL")
		: (bNox ? TEXT("NOXEENS") : TEXT("AQUILORIS"));
	DrawGlowTitle(Title, H * 0.22f, 3.0f, Accent);
	if (Fac == EFactionID::None)
		DrawCenteredText(TEXT("WAR OF THE OCEAN'S LEGACY"), H * 0.40f, Accent.CopyWithNewOpacity(0.85f), 1.3f);

	// Texte de lore (façon maquette de chargement).
	const FString Lore = bNox
		? TEXT("Peuple des abysses, les Noxeens rodent dans la faille bioluminescente,\ntapis entre les plaques du monde, prets a jaillir de l'obscurite.")
		: TEXT("Nobles et technologues, les Aquiloris veillent depuis la cite de cristal\nd'Aquilor, gardiens de l'energie bleue des profondeurs.");
	if (Fac != EFactionID::None)
	{
		TArray<FString> Lines; Lore.ParseIntoArray(Lines, TEXT("\n"), false);
		float LY = H * 0.48f;
		for (const FString& L : Lines) { DrawCenteredText(L, LY, FLinearColor(0.88f, 0.93f, 1.f, 0.95f), 1.0f); LY += 30.f; }
	}

	// Barre de progression INDÉTERMINÉE (pas de vrai % en démo synchrone) : remplissage
	// qui va-et-vient, façon "chargement en cours".
	const float T = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	const float BarW = W * 0.42f, BarH = 16.f;
	const float BX = (W - BarW) * 0.5f, BY = H * 0.72f;
	DrawRect(FLinearColor(0.05f, 0.08f, 0.12f, 0.9f), BX, BY, BarW, BarH);
	const float Pulse = 0.5f + 0.5f * FMath::Sin(T * 2.2f);
	const float FillW = BarW * (0.25f + 0.55f * Pulse);
	DrawRect(Accent.CopyWithNewOpacity(0.85f), BX, BY, FillW, BarH);
	DrawLine(BX, BY, BX + BarW, BY, Accent, 1.5f);
	DrawLine(BX, BY + BarH, BX + BarW, BY + BarH, Accent, 1.5f);

	const FString Msg = (Demo && !Demo->CurrentMessage.IsEmpty()) ? Demo->CurrentMessage : TEXT("Chargement...");
	DrawCenteredText(Msg, BY + 34.f, FLinearColor(0.85f, 0.92f, 1.f, 1.f), 1.1f);

	// Astuce (comme les maquettes) — tourne parmi quelques conseils. Deux astuces d'INTERFACE
	// ajoutees (R + ecran Commandes) en plus des astuces de combat existantes : les tirages
	// precedents ne mentionnaient que la tactique, jamais les fonctions du HUD (moment ideal
	// pour les enseigner, entre deux phases, cf. recherche onboarding session du 19/07/2026).
	static const TCHAR* Tips[6] = {
		TEXT("Astuce : attaquez depuis une couche inferieure pour un bonus de degats ascendant."),
		TEXT("Astuce : les Aquilombres sont invisibles a l'arret — approchez pour frapper dans le dos."),
		TEXT("Astuce : gardez vos unites groupees, la coordination Aquiloris renforce le groupe."),
		TEXT("Astuce : les Noxeens sont plus puissants dans les zones bioluminescentes vertes."),
		TEXT("Astuce : la touche R active la competence des unites selectionnees."),
		TEXT("Astuce : Reglages > Commandes rappelle toutes les touches a tout moment.") };
	const int32 Idx = ((int32)(T * 0.2f)) % 6;
	DrawCenteredText(Tips[Idx], H * 0.86f, FLinearColor(0.75f, 0.85f, 0.95f, 0.9f), 0.95f);
}

void AWOTOLDemoHUD::DrawSummary(float W, float H, UDemoFlowSubsystem* Demo)
{
	DrawUnderwaterBackground(W, H);
	if (!Demo) return;
	DrawFactionAmbientTint(W, H, Demo->GetPlayerFaction());

	// Titre (or si victoire, rouge si défaite)
	const bool bWin = Demo->bSummaryVictory;
	const FLinearColor TitleCol = bWin ? FLinearColor(1.f, 0.85f, 0.2f, 1.f)
		: FLinearColor(1.f, 0.3f, 0.25f, 1.f);
	const FString Title = Demo->SummaryTitle.IsEmpty()
		? (bWin ? TEXT("VICTOIRE") : TEXT("DEFAITE")) : Demo->SummaryTitle;
	DrawGlowTitle(FString::Printf(TEXT("— %s —"), *Title), H * 0.06f, 2.8f, TitleCol);
	const int32 Dur = FMath::RoundToInt(Demo->SummaryDurationSeconds);
	// Écran FINAL (victoire totale ou défaite non récupérable — PAS l'échec de défense
	// récupérable, qui relance la boucle via "Réessayer/Retour à la cité") : message de fin
	// de démo explicite. Absent jusqu'ici (même bandeau générique que les résumés
	// intermédiaires) alors que les démos indépendantes marquent presque toutes clairement
	// ce moment (cf. recherche session du 19/07/2026 sur les conventions Steam/itch.io).
	const bool bTrueDemoEnd = Demo->bSummaryIsFinal && !Demo->bSummaryCanReturnToCity;
	const FString Sub = bTrueDemoEnd
		? FString::Printf(TEXT("FIN DE LA DEMO  —  duree totale : %d min %02d s  —  Merci d'avoir joue !"),
			Dur / 60, Dur % 60)
		: FString::Printf(TEXT("Resume de la bataille  —  duree : %d min %02d s"), Dur / 60, Dur % 60);
	// NOIR GRAS, SANS ombre : dessiné sur le disque clair -> lisible. (L'ombre de DrawCenteredText
	// rendait le noir sur noir illisible : on dessine donc en direct, plusieurs passes noires
	// legerement decalees = effet GRAS, aucune ombre coloree.)
	if (Canvas)
	{
		UFont* SF = GEngine ? GEngine->GetLargeFont() : nullptr;
		float STW = 0.f, STH = 0.f; GetTextSize(Sub, STW, STH, SF, 1.4f);
		const float SX = (Canvas->SizeX - STW) * 0.5f, SY = H * 0.16f;
		const FLinearColor Blk(0.f, 0.f, 0.f, 1.f);
		DrawText(Sub, Blk, SX - 1.f, SY, SF, 1.4f);
		DrawText(Sub, Blk, SX + 1.f, SY, SF, 1.4f);
		DrawText(Sub, Blk, SX, SY - 1.f, SF, 1.4f);
		DrawText(Sub, Blk, SX, SY + 1.f, SF, 1.4f);
		DrawText(Sub, Blk, SX, SY, SF, 1.4f);
	}

	const bool bHasRewards = Demo->LastRewardCrystals > 0
		|| Demo->LastRewardAbyssalMaterials > 0 || Demo->LastRewardBiomass > 0
		|| Demo->LastRewardOceanicEnergy > 0;
	if (bHasRewards)
	{
		DrawCenteredText(FString::Printf(TEXT(
			"RECOMPENSES : Cristaux +%d   |   Mineraux Abyssaux +%d   |   Biomasse +%d   |   Energie Oceanique +%d"),
			Demo->LastRewardCrystals, Demo->LastRewardAbyssalMaterials,
			Demo->LastRewardBiomass, Demo->LastRewardOceanicEnergy),
			H * 0.205f, FLinearColor(1.f, 0.88f, 0.35f, 1.f), 1.0f);
	}

	// Deux colonnes : pertes de TON armée (gauche) / pertes de l'ennemi (droite).
	// Hauteur bornée AU-DESSUS des boutons + interligne DYNAMIQUE -> tout tient, rien ne
	// chevauche (2 lignes compactes par unité).
	const float ColTop  = bHasRewards ? H * 0.255f : H * 0.22f;
	const float ColBot  = SummaryReplayButtonRect(W, H).Min.Y - 24.f; // au-dessus des boutons
	auto DrawColumn = [&](float X, float ColW, const FString& Header,
		const TArray<FUnitLossEntry>& Entries, const FLinearColor& Accent)
	{
		const float ColH = ColBot - ColTop;
		DrawRect(FLinearColor(0.02f, 0.04f, 0.08f, 0.9f), X, ColTop, ColW, ColH);
		DrawRect(Accent, X, ColTop, ColW, 4.f);
		DrawText(Header, Accent, X + 18.f, ColTop + 10.f, GEngine->GetMediumFont(), 1.2f);

		const float HeaderH = 40.f, TotalH = 26.f;
		const float AvailH = ColH - HeaderH - TotalH;
		const int32 N = FMath::Max(1, Entries.Num());
		const float Step = FMath::Clamp(AvailH / N, 34.f, 52.f);

		float Y = ColTop + HeaderH;
		int32 TotalLost = 0, Total = 0;
		if (Entries.Num() == 0)
		{
			DrawText(TEXT("(aucune unite)"), FLinearColor(0.7f, 0.7f, 0.7f, 1.f),
				X + 18.f, Y, GEngine->GetSmallFont(), 1.f);
		}
		for (const FUnitLossEntry& E : Entries)
		{
			TotalLost += E.Lost; Total += E.Total;
			const int32 Survived = E.Total - E.Lost;
			const FLinearColor LineCol = (E.Lost >= E.Total)
				? FLinearColor(1.f, 0.4f, 0.35f, 1.f) : FLinearColor(0.9f, 0.95f, 1.f, 1.f);
			// Ligne 1 : nom + pertes + dégâts infligés
			const FString L1 = FString::Printf(TEXT("%s  —  perdus %d/%d (surv %d)  —  degats %d"),
				*E.UnitName, E.Lost, E.Total, Survived, FMath::RoundToInt(E.DamageDealt));
			DrawText(L1, LineCol, X + 18.f, Y, GEngine->GetSmallFont(), 1.05f);
			// Ligne 2 : taux défensifs
			const FString L2 = FString::Printf(TEXT("   DEF %d%%   Parade %d%%   Esquive %d%%"),
				E.DefPct, E.BlockPct, E.DodgePct);
			DrawText(L2, FLinearColor(0.62f, 0.78f, 0.72f, 1.f), X + 18.f, Y + 16.f, GEngine->GetSmallFont(), 1.f);
			Y += Step;
		}
		// Total en bas de colonne
		const FString TotLine = FString::Printf(TEXT("TOTAL : %d pertes / %d"), TotalLost, Total);
		DrawText(TotLine, FLinearColor(1.f, 0.9f, 0.6f, 1.f), X + 18.f, ColBot - TotalH + 2.f,
			GEngine->GetMediumFont(), 1.f);
	};

	const float ColW = FMath::Min(460.f, (W - 120.f) * 0.5f);
	const float GapC = 40.f;
	const float LeftX = (W - (ColW * 2.f + GapC)) * 0.5f;
	DrawColumn(LeftX, ColW, TEXT("VOS PERTES"), Demo->PlayerLosses, FLinearColor(0.3f, 0.7f, 1.f, 1.f));
	DrawColumn(LeftX + ColW + GapC, ColW, TEXT("PERTES ENNEMIES"), Demo->EnemyLosses,
		FLinearColor(0.9f, 0.35f, 0.9f, 1.f));

	// Boutons selon le contexte
	if (Demo->bSummaryIsFinal)
	{
		// « Rejouer » relance la MÊME phase (celle qu'on vient de finir/perdre), pas la démo.
		DrawButton(SummaryReplayButtonRect(W, H), Demo->bSummaryCanReturnToCity
			? TEXT("REJOUER LA DEFENSE") : TEXT("REJOUER LA PHASE"),
			FLinearColor(0.3f, 0.7f, 1.f, 1.f), 1.15f);
		DrawButton(SummaryChangeFactionButtonRect(W, H), Demo->bSummaryCanReturnToCity
			? TEXT("RETOUR A LA CITE") : TEXT("CHANGER DE FACTION"),
			FLinearColor(0.3f, 0.9f, 0.5f, 1.f), 1.05f);
		DrawButton(SummaryMenuButtonRect(W, H), TEXT("MENU PRINCIPAL"),
			FLinearColor(0.9f, 0.8f, 0.35f, 1.f), 1.1f);
		DrawButton(SummaryQuitButtonRect(W, H), TEXT("QUITTER"),
			FLinearColor(1.f, 0.45f, 0.35f, 1.f), 1.15f);
	}
	else
	{
		DrawButton(SummaryContinueButtonRect(W, H),
			Demo->SummaryContinueLabel.IsEmpty()
				? FString(TEXT("CONTINUER")) : Demo->SummaryContinueLabel,
			FLinearColor(1.f, 0.7f, 0.2f, 1.f), 1.4f);
	}
}

void AWOTOLDemoHUD::DrawCurrentIndicator(float W, float H, UWorld* World)
{
	if (!World || !Canvas) return;
	UOceanCurrentSubsystem* Cur = World->GetSubsystem<UOceanCurrentSubsystem>();
	if (!Cur || !Cur->IsActive()) return;

	// Angle du courant RELATIF à la caméra (boussole écran).
	float CamYaw = 0.f;
	if (APlayerController* PC = GetOwningPlayerController())
		if (PC->PlayerCameraManager) CamYaw = PC->PlayerCameraManager->GetCameraRotation().Yaw;
	const float WorldYaw = Cur->GetDirection().Rotation().Yaw;
	const float Ang = FMath::DegreesToRadians(WorldYaw - CamYaw);
	const FVector2D Dir(FMath::Sin(Ang), -FMath::Cos(Ang)); // haut écran = avant caméra

	// Panneau en haut à droite, DÉCALÉ SOUS la rangée des boutons (pause + engrenage) pour ne
	// pas les chevaucher.
	const float BX = W - 168.f, BY = 60.f, BW = 150.f, BH = 58.f;
	DrawRect(FLinearColor(0.02f, 0.05f, 0.09f, 0.82f), BX, BY, BW, BH);
	DrawRect(FLinearColor(0.25f, 0.7f, 1.f, 0.9f), BX, BY, BW, 3.f);
	DrawText(TEXT("COURANT"), FLinearColor(0.7f, 0.9f, 1.f, 1.f), BX + 10.f, BY + 8.f, GEngine->GetSmallFont(), 1.f);

	// Flèche
	const FVector2D C(BX + 34.f, BY + 36.f);
	const float Len = 20.f;
	const FVector2D Tip = C + Dir * Len;
	const FVector2D Tail = C - Dir * Len;
	const FLinearColor Arr(0.4f, 0.85f, 1.f, 1.f);
	DrawLine(Tail.X, Tail.Y, Tip.X, Tip.Y, Arr, 3.f);
	const FVector2D Perp(-Dir.Y, Dir.X);
	DrawLine(Tip.X, Tip.Y, (Tip - Dir * 8.f + Perp * 6.f).X, (Tip - Dir * 8.f + Perp * 6.f).Y, Arr, 3.f);
	DrawLine(Tip.X, Tip.Y, (Tip - Dir * 8.f - Perp * 6.f).X, (Tip - Dir * 8.f - Perp * 6.f).Y, Arr, 3.f);

	// Intensité (couches hautes)
	const float S = Cur->GetStrength();
	const TCHAR* Lbl = (S > 110.f) ? TEXT("Fort") : (S > 85.f) ? TEXT("Moyen") : TEXT("Faible");
	DrawText(FString::Printf(TEXT("%s"), Lbl), FLinearColor(0.85f, 0.95f, 1.f, 1.f),
		BX + 66.f, BY + 30.f, GEngine->GetSmallFont(), 1.f);
	DrawText(TEXT("couches hautes"), FLinearColor(0.7f, 0.8f, 0.9f, 1.f),
		BX + 66.f, BY + 44.f, GEngine->GetSmallFont(), 0.8f);
}

void AWOTOLDemoHUD::DrawInterlude(float W, float H, UDemoFlowSubsystem* Demo)
{
	if (UTexture2D* BG = GetTransitionBackground())
	{
		DrawTexture(BG, 0.f, 0.f, W, H, 0.f, 0.f, 1.f, 1.f);
		// Voile central seulement : conserve les coraux/cristaux latéraux et garantit la lecture.
		DrawRect(FLinearColor(0.f, 0.025f, 0.07f, 0.36f), W * 0.17f, H * 0.02f,
			W * 0.66f, H * 0.90f);
	}
	else
	{
		DrawUnderwaterBackground(W, H);
	}
	if (Demo) DrawFactionAmbientTint(W, H, Demo->GetPlayerFaction());
	DrawGlowTitle(TEXT("— ENTRE DEUX BATAILLES —"), H * 0.06f, 1.9f, FLinearColor(0.6f, 0.9f, 1.f, 1.f));

	// Texte narratif (multi-lignes) — réparti sur la hauteur disponible AU-DESSUS du bouton
	// (départ haut + interligne serré) pour que la dernière ligne ne soit jamais masquée.
	if (Demo && !Demo->InterludeText.IsEmpty())
	{
		TArray<FString> Lines;
		Demo->InterludeText.ParseIntoArray(Lines, TEXT("\n"), false);
		const float ButtonTop = InterludeContinueButtonRect(W, H).Min.Y;
		const float StartY = H * 0.17f;
		const float AvailH = ButtonTop - 24.f - StartY;
		const float Step   = (Lines.Num() > 1)
			? FMath::Clamp(AvailH / Lines.Num(), 34.f, 50.f) : 42.f;
		float Y = StartY;
		// Texte PLUS GRAND + OMBRE NOIRE derrière + couleur CHAUDE -> bien plus lisible sur le
		// fond bleu (avant : petit, bleu clair sur bleu = quasi invisible et "triste").
		const float Sc = 1.35f;
		for (const FString& Line : Lines)
		{
			DrawCenteredText(Line, Y + 2.f, FLinearColor(0.f, 0.f, 0.f, 0.9f), Sc);        // ombre
			DrawCenteredText(Line, Y, FLinearColor(1.f, 0.92f, 0.62f, 1.f), Sc);           // texte doré chaud
			Y += Step;
		}
	}

	DrawButton(InterludeContinueButtonRect(W, H), TEXT("CONTINUER — Preparer la defense"),
		FLinearColor(1.f, 0.7f, 0.2f, 1.f), 1.4f);
}

void AWOTOLDemoHUD::DrawBuildingBar(float W, float H, AWOTOLCaptureObject* Building)
{
	if (!Building) return;
	const float Pct = Building->GetHealthPercent();
	const int32 MaxHP = FMath::RoundToInt(Building->MaxHealth);
	const int32 CurHP = FMath::RoundToInt(Pct * MaxHP);

	// Barre SLIM tout en haut (nom + PV DANS la barre) -> ne barre plus le milieu de l'écran.
	const float BarW = 440.f, BarH = 16.f;
	const float BX = (W - BarW) * 0.5f, BY = 52.f;

	const FLinearColor Fac = FFactionColors::Get(Building->OwnerFaction);
	DrawRect(Fac, BX - 3, BY - 3, BarW + 6, 2.f);
	const FLinearColor Fill = FMath::Lerp(FLinearColor(0.85f, 0.15f, 0.12f, 1.f),
		FLinearColor(0.25f, 0.8f, 0.35f, 1.f), Pct);
	DrawBar(BX, BY, BarW, BarH, Pct, Fill, FLinearColor(0.08f, 0.08f, 0.10f, 0.92f));

	auto Shadowed = [&](const FString& S, float TX, float TY, UFont* F, float Sc, const FLinearColor& C)
	{
		DrawText(S, FLinearColor(0.f, 0.f, 0.f, 0.8f), TX + 1.f, TY + 1.f, F, Sc);
		DrawText(S, C, TX, TY, F, Sc);
	};
	Shadowed(Building->GetDisplayName().ToString(), BX + 8.f, BY + 1.f, GEngine->GetMediumFont(), 0.9f, FLinearColor::White);
	const FString HP = FString::Printf(TEXT("%d / %d"), CurHP, MaxHP);
	float HW, HH; GetTextSize(HP, HW, HH, GEngine->GetSmallFont(), 1.f);
	Shadowed(HP, BX + BarW - HW - 8.f, BY + 2.f, GEngine->GetSmallFont(), 1.f, FLinearColor::White);
}

void AWOTOLDemoHUD::DrawPrepareBar(float W, float H)
{
	// (Les instructions détaillées sont dans les 2 fenêtres ci-dessous — plus de texte
	// centré qui chevauchait la barre du bâtiment / les unités.)

	// ── Fenêtre TUTO "CONTROLES" (à GAUCHE) — souris + clavier caméra. Disparait au combat. ──
	{
		const float PW = 400.f, PH = 196.f;
		const float PX = 16.f, PY = H - 386.f;
		DrawRect(FLinearColor(0.02f, 0.05f, 0.09f, 0.9f), PX, PY, PW, PH);
		DrawRect(FLinearColor(0.30f, 0.7f, 1.f, 1.f), PX, PY, PW, 4.f);

		DrawText(TEXT("CONTROLES"), FLinearColor(0.6f, 0.9f, 1.f, 1.f),
			PX + 14.f, PY + 12.f, GEngine->GetMediumFont(), 1.1f);
		const TCHAR* Lines[7] = {
			TEXT("Clic GAUCHE : selectionner (glisser = boite)"),
			TEXT("Clic DROIT : deplacer / attaquer"),
			TEXT("Double-clic : zoom sur une unite"),
			TEXT("CTRL : selectionner toute l'armee"),
			TEXT("Molette : zoom  |  Clic droit maintenu : pivoter"),
			TEXT("Camera : Z avancer / S reculer / Q gauche / D droite"),
			TEXT("Camera : E monter / Espace descendre")
		};
		float Y = PY + 40.f;
		for (const TCHAR* L : Lines)
		{
			DrawText(L, FLinearColor(0.9f, 0.95f, 1.f, 1.f), PX + 14.f, Y, GEngine->GetSmallFont(), 1.05f);
			Y += 21.f;
		}
	}

	// ── Fenêtre d'INFO / mini-tuto sur la VERTICALITÉ (au-dessus des boutons Monter/Descendre) ──
	{
		const float PW = 380.f, PH = 150.f;
		const float PX = W - PW - 16.f, PY = H - 340.f;
		DrawRect(FLinearColor(0.02f, 0.05f, 0.09f, 0.9f), PX, PY, PW, PH);
		DrawRect(FLinearColor(0.30f, 0.7f, 1.f, 1.f), PX, PY, PW, 4.f); // liseré haut

		DrawText(TEXT("HAUTEUR / VERTICALITE"), FLinearColor(0.6f, 0.9f, 1.f, 1.f),
			PX + 14.f, PY + 12.f, GEngine->GetMediumFont(), 1.1f);
		const TCHAR* Lines[5] = {
			TEXT("Vos unites peuvent combattre sur"),
			TEXT("plusieurs HAUTEURS (4 couches)."),
			TEXT("Selectionnez un groupe, puis les"),
			TEXT("boutons MONTER / DESCENDRE (a droite)."),
			TEXT("Valable au placement ET en bataille.")
		};
		float Y = PY + 40.f;
		for (const TCHAR* L : Lines)
		{
			DrawText(L, FLinearColor(0.9f, 0.95f, 1.f, 1.f), PX + 14.f, Y, GEngine->GetSmallFont(), 1.05f);
			Y += 21.f;
		}
	}

	DrawButton(LaunchBattleButtonRect(W, H), TEXT("LANCER LA BATAILLE"),
		FLinearColor(1.f, 0.7f, 0.2f, 1.f), 1.0f);
}

void AWOTOLDemoHUD::DrawPauseButton(float W, float H)
{
	const FBox2D R = PauseButtonRect(W, H);
	const FVector2D Sz = R.Max - R.Min;
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.45f), R.Min.X, R.Min.Y, Sz.X, Sz.Y);

	// GELÉ -> icône PLAY (triangle) pour REPRENDRE ; sinon icône PAUSE (deux barres).
	bool bFrozen = false;
	if (AWOTOLPlayerController_Battle* PC = Cast<AWOTOLPlayerController_Battle>(GetOwningPlayerController()))
		bFrozen = PC->IsBattleFrozen();

	if (bFrozen)
	{
		// Icône PLAY (triangle pointant à droite) tramée en bandes horizontales.
		const float TriH = Sz.Y * 0.5f, TriW = Sz.X * 0.34f;
		const float LX = R.Min.X + Sz.X * 0.34f, CY = R.Min.Y + Sz.Y * 0.5f;
		const int32 Rows = 9;
		for (int32 r = 0; r <= Rows; ++r)
		{
			const float t = (float)r / Rows;                 // 0 (haut) .. 1 (bas)
			const float y = CY - TriH * 0.5f + t * TriH;
			const float w = TriW * (1.f - FMath::Abs(t - 0.5f) * 2.f); // large au centre, pointe aux extrêmes
			DrawRect(FLinearColor::White, LX, y - 1.f, w, 3.f);
		}
	}
	else
	{
		const float BarW = 7.f, BarH = Sz.Y * 0.55f;
		const float BarY = R.Min.Y + (Sz.Y - BarH) * 0.5f;
		DrawRect(FLinearColor::White, R.Min.X + Sz.X * 0.30f - BarW * 0.5f, BarY, BarW, BarH);
		DrawRect(FLinearColor::White, R.Min.X + Sz.X * 0.70f - BarW * 0.5f, BarY, BarW, BarH);
	}
}

// Engrenage (réglages) : disque + dents (petits rectangles autour) + moyeu sombre.
void AWOTOLDemoHUD::DrawSettingsButton(float W, float H)
{
	const FBox2D R = SettingsButtonRect(W, H);
	const FVector2D Sz = R.Max - R.Min;
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.45f), R.Min.X, R.Min.Y, Sz.X, Sz.Y);
	if (!Canvas) return;
	const FVector2D C(R.Min.X + Sz.X * 0.5f, R.Min.Y + Sz.Y * 0.5f);
	const float Rad = Sz.Y * 0.26f;
	// dents
	for (int32 i = 0; i < 8; ++i)
	{
		const float a = 2.f * PI * i / 8.f;
		const FVector2D P(C.X + FMath::Cos(a) * Rad, C.Y + FMath::Sin(a) * Rad);
		DrawRect(FLinearColor::White, P.X - 2.5f, P.Y - 2.5f, 5.f, 5.f);
	}
	Canvas->K2_DrawPolygon(nullptr, C, FVector2D(Rad, Rad), 12, FLinearColor(0.85f, 0.9f, 1.f, 1.f)); // disque
	Canvas->K2_DrawPolygon(nullptr, C, FVector2D(Rad * 0.42f, Rad * 0.42f), 10, FLinearColor(0.05f, 0.07f, 0.12f, 1.f)); // moyeu
}

FBox2D AWOTOLDemoHUD::MusicVolumeBarRect(float W, float H)
{
	const float BW = 280.f, BH = 26.f;
	const float X = (W - BW) * 0.5f;
	const float Y = H * 0.315f;
	return FBox2D(FVector2D(X, Y), FVector2D(X + BW, Y + BH));
}

FBox2D AWOTOLDemoHUD::FullscreenToggleButtonRect(float W, float H)
{
	const FBox2D VolRect = MusicVolumeBarRect(W, H);
	const float BW = 280.f, BH = 36.f;
	const float X = (W - BW) * 0.5f;
	const float Y = VolRect.Max.Y + 14.f;
	return FBox2D(FVector2D(X, Y), FVector2D(X + BW, Y + BH));
}

FBox2D AWOTOLDemoHUD::GameSpeedButtonRect(int32 Index, float W, float H)
{
	const FBox2D FsRect = FullscreenToggleButtonRect(W, H);
	const float BW = 88.f, BH = 32.f, Gap = 12.f;
	const float TotalW = BW * 3.f + Gap * 2.f;
	const float X = (W - TotalW) * 0.5f + Index * (BW + Gap);
	const float Y = FsRect.Max.Y + 34.f; // sous le libelle "Vitesse de jeu"
	return FBox2D(FVector2D(X, Y), FVector2D(X + BW, Y + BH));
}

FBox2D AWOTOLDemoHUD::ControlsBackButtonRect(float W, float H)
{
	const float BW = 220.f, BH = 60.f;
	return FBox2D(FVector2D(40.f, H - BH - 40.f), FVector2D(40.f + BW, H - 40.f));
}

FBox2D AWOTOLDemoHUD::ConfirmYesButtonRect(float W, float H)
{
	const float BW = 220.f, BH = 60.f, Gap = 24.f;
	const float X = (W - (BW * 2.f + Gap)) * 0.5f;
	return FBox2D(FVector2D(X, H * 0.55f), FVector2D(X + BW, H * 0.55f + BH));
}

FBox2D AWOTOLDemoHUD::ConfirmNoButtonRect(float W, float H)
{
	const FBox2D Yes = ConfirmYesButtonRect(W, H);
	const float BW = Yes.Max.X - Yes.Min.X, Gap = 24.f;
	return FBox2D(FVector2D(Yes.Max.X + Gap, Yes.Min.Y), FVector2D(Yes.Max.X + Gap + BW, Yes.Max.Y));
}

void AWOTOLDemoHUD::DrawPauseOverlay(float W, float H)
{
	// Voile sombre plein écran
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), 0.f, 0.f, W, H);
	DrawCenteredText(TEXT("REGLAGES"), H * 0.20f, FLinearColor::White, 2.6f);

	// ── Réglages RÉELS (absents jusqu'ici : l'écran ne contenait que Reprendre/Recommencer/
	// Quitter, aucun réglage audio/affichage) ──
	AWOTOLPlayerController_Battle* PC = Cast<AWOTOLPlayerController_Battle>(GetOwningPlayerController());

	// Volume musique : barre cliquable (clic = fixe le niveau à cette position).
	{
		const FBox2D VolRect = MusicVolumeBarRect(W, H);
		const float Vol = PC ? PC->GetMusicVolume() : 0.7f;
		float LW, LH; GetTextSize(TEXT("Musique"), LW, LH, GEngine->GetSmallFont(), 1.f);
		DrawText(TEXT("Musique"), FLinearColor(0.85f, 0.9f, 1.f, 1.f),
			VolRect.Min.X, VolRect.Min.Y - LH - 4.f, GEngine->GetSmallFont(), 1.f);
		DrawBar(VolRect.Min.X, VolRect.Min.Y, VolRect.Max.X - VolRect.Min.X, VolRect.Max.Y - VolRect.Min.Y,
			Vol, FLinearColor(0.35f, 0.7f, 1.f, 1.f), FLinearColor(0.10f, 0.10f, 0.12f, 0.9f));
		const FString PctTxt = FString::Printf(TEXT("%d %%"), FMath::RoundToInt(Vol * 100.f));
		float PW2, PH2; GetTextSize(PctTxt, PW2, PH2, GEngine->GetSmallFont(), 0.9f);
		DrawText(PctTxt, FLinearColor::White, VolRect.Max.X - PW2 - 6.f,
			VolRect.Min.Y + (VolRect.Max.Y - VolRect.Min.Y - PH2) * 0.5f, GEngine->GetSmallFont(), 0.9f);
	}

	// Plein écran / fenêtré.
	{
		const FBox2D FsRect = FullscreenToggleButtonRect(W, H);
		const FVector2D Sz = FsRect.Max - FsRect.Min;
		const bool bFs = PC ? PC->IsFullscreen() : true;
		DrawRect(FLinearColor(0.10f, 0.16f, 0.28f, 0.95f), FsRect.Min.X, FsRect.Min.Y, Sz.X, Sz.Y);
		DrawRect(FLinearColor(0.3f, 0.6f, 1.f, 1.f), FsRect.Min.X, FsRect.Min.Y, Sz.X, 3.f);
		const FString FsLabel = bFs ? TEXT("Affichage : Plein ecran") : TEXT("Affichage : Fenetre");
		float FW, FH; GetTextSize(FsLabel, FW, FH, GEngine->GetMediumFont(), 1.0f);
		DrawText(FsLabel, FLinearColor::White, FsRect.Min.X + (Sz.X - FW) * 0.5f,
			FsRect.Min.Y + (Sz.Y - FH) * 0.5f, GEngine->GetMediumFont(), 1.0f);
	}

	// Vitesse de jeu : x1 / x1.5 / x2 (demande de Liamor du 26/07/2026). Dilate le temps
	// moteur ; ne touche pas au rendu de l'UI (dessinee en temps reel de toute facon).
	{
		const float Speed = PC ? PC->GetGameSpeed() : 1.f;
		const FBox2D FirstChip = GameSpeedButtonRect(0, W, H);
		float LW, LH; GetTextSize(TEXT("Vitesse de jeu"), LW, LH, GEngine->GetSmallFont(), 1.f);
		DrawText(TEXT("Vitesse de jeu"), FLinearColor(0.85f, 0.9f, 1.f, 1.f),
			(W - LW) * 0.5f, FirstChip.Min.Y - LH - 6.f, GEngine->GetSmallFont(), 1.f);

		const TCHAR* SpeedLabels[3] = { TEXT("x1"), TEXT("x1.5"), TEXT("x2") };
		const float SpeedVals[3] = { 1.f, 1.5f, 2.f };
		for (int32 i = 0; i < 3; ++i)
		{
			const FBox2D R = GameSpeedButtonRect(i, W, H);
			const FVector2D Sz = R.Max - R.Min;
			const bool bSel = FMath::IsNearlyEqual(Speed, SpeedVals[i], 0.01f);
			DrawRect(bSel ? FLinearColor(0.25f, 0.55f, 0.95f, 0.95f) : FLinearColor(0.10f, 0.16f, 0.28f, 0.9f),
				R.Min.X, R.Min.Y, Sz.X, Sz.Y);
			float TW, TH; GetTextSize(SpeedLabels[i], TW, TH, GEngine->GetSmallFont(), 1.0f);
			DrawText(SpeedLabels[i], FLinearColor::White, R.Min.X + (Sz.X - TW) * 0.5f,
				R.Min.Y + (Sz.Y - TH) * 0.5f, GEngine->GetSmallFont(), 1.0f);
		}
	}

	const TCHAR* Labels[4] = { TEXT("Reprendre"), TEXT("Recommencer"), TEXT("Quitter"), TEXT("Commandes") };
	for (int32 i = 0; i < 4; ++i)
	{
		const FBox2D R = MenuButtonRect(i, W, H);
		const FVector2D Sz = R.Max - R.Min;
		DrawRect(FLinearColor(0.10f, 0.16f, 0.28f, 0.95f), R.Min.X, R.Min.Y, Sz.X, Sz.Y);
		DrawRect(FLinearColor(0.3f, 0.6f, 1.f, 1.f), R.Min.X, R.Min.Y, Sz.X, 3.f); // liseré
		float TW, TH; GetTextSize(Labels[i], TW, TH, GEngine->GetLargeFont(), 1.3f);
		DrawText(Labels[i], FLinearColor::White, R.Min.X + (Sz.X - TW) * 0.5f,
			R.Min.Y + (Sz.Y - TH) * 0.5f, GEngine->GetLargeFont(), 1.3f);
	}
}

void AWOTOLDemoHUD::DrawControlsScreen(float W, float H)
{
	// Voile sombre plein écran, même habillage que le menu réglages.
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.75f), 0.f, 0.f, W, H);
	DrawCenteredText(TEXT("COMMANDES"), H * 0.08f, FLinearColor::White, 2.4f);

	struct FControlLine { const TCHAR* Key; const TCHAR* Desc; };
	struct FControlSection { const TCHAR* Title; TArray<FControlLine> Lines; };

	const TArray<FControlSection> Sections = {
		{ TEXT("BATAILLE"), {
			{ TEXT("Clic gauche"), TEXT("Selectionner / glisser pour selectionner un groupe") },
			{ TEXT("Clic droit"), TEXT("Ordre (deplacement ou attaque) / maintenu = tourner la camera") },
			{ TEXT("Molette"), TEXT("Zoom") },
			{ TEXT("ZQSD / WASD / fleches"), TEXT("Deplacer la camera") },
			{ TEXT("Ctrl"), TEXT("Selectionner toute l'armee") },
			{ TEXT("R"), TEXT("Activer la competence des unites selectionnees") },
			{ TEXT("Echap / P"), TEXT("Pause") },
		}},
		{ TEXT("EXPLORATION (nage libre)"), {
			{ TEXT("ZQSD / WASD"), TEXT("Nager") },
			{ TEXT("Souris"), TEXT("Regarder") },
			{ TEXT("Espace / E"), TEXT("Monter") },
			{ TEXT("Maj / Ctrl"), TEXT("Descendre") },
			{ TEXT("Alt"), TEXT("Sprint (maintenu)") },
			{ TEXT("C"), TEXT("Ruee (courte impulsion, a recharge)") },
		}},
		{ TEXT("VUE CITE"), {
			{ TEXT("ZQSD / WASD / fleches"), TEXT("Deplacer la camera isometrique") },
			{ TEXT("Molette"), TEXT("Zoom") },
			{ TEXT("Clic sur un batiment"), TEXT("Ouvrir sa fiche technique") },
		}},
	};

	float Y = H * 0.16f;
	const float ColKeyX = W * 0.5f - 360.f;
	const float ColDescX = W * 0.5f - 130.f;
	for (const FControlSection& Sec : Sections)
	{
		DrawText(Sec.Title, FLinearColor(0.95f, 0.85f, 0.4f, 1.f), ColKeyX, Y,
			GEngine ? GEngine->GetMediumFont() : nullptr, 1.05f);
		Y += 34.f;
		for (const FControlLine& Line : Sec.Lines)
		{
			DrawText(Line.Key, FLinearColor(0.55f, 0.85f, 1.f, 1.f), ColKeyX, Y, nullptr, 0.95f);
			DrawText(Line.Desc, FLinearColor(0.9f, 0.92f, 0.96f, 0.95f), ColDescX, Y, nullptr, 0.9f);
			Y += 27.f;
		}
		Y += 16.f;
	}

	DrawButton(ControlsBackButtonRect(W, H), TEXT("RETOUR"), FLinearColor(0.6f, 0.8f, 1.f, 1.f), 1.2f);
}

void AWOTOLDemoHUD::DrawConfirmDialog(float W, float H, const FString& Message, const FString& ConfirmLabel)
{
	// Voile plus sombre que le menu réglages (attire l'oeil sur une décision destructive).
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.8f), 0.f, 0.f, W, H);
	DrawCenteredText(Message, H * 0.44f, FLinearColor(1.f, 0.85f, 0.35f, 1.f), 1.6f);
	DrawButton(ConfirmYesButtonRect(W, H), ConfirmLabel, FLinearColor(1.f, 0.45f, 0.35f, 1.f), 1.15f);
	DrawButton(ConfirmNoButtonRect(W, H), TEXT("ANNULER"), FLinearColor(0.5f, 0.55f, 0.62f, 1.f), 1.15f);
}

void AWOTOLDemoHUD::DrawBar(float X, float Y, float BarW, float BarH, float Pct,
	const FLinearColor& Fill, const FLinearColor& Back)
{
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.85f), X - 2, Y - 2, BarW + 4, BarH + 4); // cadre
	DrawRect(Back, X, Y, BarW, BarH);
	DrawRect(Fill, X, Y, BarW * FMath::Clamp(Pct, 0.f, 1.f), BarH);
}

void AWOTOLDemoHUD::DrawTopBar(float W, float H, UWorld* World, UDemoFlowSubsystem* Demo)
{
	// Pilule de timer (haut centre)
	if (World)
	{
		if (URTSBattleManager* RTS = World->GetSubsystem<URTSBattleManager>())
		{
			const FString T = RTS->GetFormattedTime().ToString();
			float TW, TH; GetTextSize(T, TW, TH, GEngine->GetLargeFont(), 1.5f);
			const float PadX = 26.f, PillW = TW + PadX * 2.f, PillH = TH + 12.f;
			const float PX = (W - PillW) * 0.5f, PY = 6.f;
			DrawRect(FLinearColor(0.03f, 0.06f, 0.10f, 0.92f), PX, PY, PillW, PillH);
			DrawRect(FLinearColor(0.30f, 0.62f, 1.f, 1.f), PX, PY, PillW, 3.f);          // liseré haut
			DrawRect(FLinearColor(0.30f, 0.62f, 1.f, 1.f), PX, PY + PillH - 3.f, PillW, 3.f);
			DrawText(T, FLinearColor::White, (W - TW) * 0.5f, PY + 6.f, GEngine->GetLargeFont(), 1.5f);
		}
	}

	// ── Fenêtre d'OBJECTIFS (toujours en HAUT À GAUCHE, jamais cachée par la barre du
	//    Kraken qui est centrée). Empile l'objectif permanent + le dernier message. ──
	if (Demo && (!Demo->ObjectiveText.IsEmpty() || !Demo->CurrentMessage.IsEmpty()))
	{
		const float PanelX = 14.f, PanelY = 12.f, PadX = 12.f, PadY = 10.f;
		// Mesure la largeur nécessaire (la plus longue des deux lignes)
		const FString ObjLine = Demo->ObjectiveText.IsEmpty()
			? FString() : (FString(TEXT("OBJECTIF : ")) + Demo->ObjectiveText);
		float OW = 0.f, OH = 0.f, MW = 0.f, MH = 0.f;
		if (!ObjLine.IsEmpty())            GetTextSize(ObjLine, OW, OH, GEngine->GetMediumFont(), 1.f);
		if (!Demo->CurrentMessage.IsEmpty()) GetTextSize(Demo->CurrentMessage, MW, MH, GEngine->GetMediumFont(), 0.95f);

		const float MaxLineW = FMath::Max(OW, MW);
		const float PanelW = FMath::Min(W * 0.5f, MaxLineW + PadX * 2.f);
		float LineH = 0.f;
		if (!ObjLine.IsEmpty())            LineH += OH + 4.f;
		if (!Demo->CurrentMessage.IsEmpty()) LineH += MH + 4.f;
		const float PanelH = LineH + PadY * 2.f;

		DrawRect(FLinearColor(0.02f, 0.04f, 0.07f, 0.82f), PanelX, PanelY, PanelW, PanelH);
		DrawRect(FLinearColor(0.95f, 0.78f, 0.25f, 0.95f), PanelX, PanelY, 4.f, PanelH); // liseré or

		float TextY = PanelY + PadY;
		if (!ObjLine.IsEmpty())
		{
			DrawText(ObjLine, FLinearColor(1.f, 0.92f, 0.6f, 1.f), PanelX + PadX, TextY,
				GEngine->GetMediumFont(), 1.f);
			TextY += OH + 4.f;
		}
		if (!Demo->CurrentMessage.IsEmpty())
		{
			DrawText(Demo->CurrentMessage, FLinearColor(0.9f, 0.95f, 1.f, 1.f), PanelX + PadX, TextY,
				GEngine->GetMediumFont(), 0.95f);
		}
	}
}

void AWOTOLDemoHUD::DrawBossBar(float W, float H, AWOTOLDemoUnit* Boss)
{
	if (!Boss || !Boss->IsAlive()) return;

	const float Pct = Boss->GetEffectiveHealthPercent();
	const int32 MaxHP = Boss->GetEffectiveMaxHealth();
	const int32 CurHP = FMath::RoundToInt(Pct * MaxHP);

	// Barre SLIM tout en haut (nom à gauche / PV à droite DANS la barre) -> ne barre plus
	// le milieu de l'écran, l'action reste dégagée.
	const float BarW = 440.f, BarH = 16.f;
	const float BX = (W - BarW) * 0.5f, BY = 52.f;

	DrawRect(FLinearColor(0.20f, 0.85f, 1.f, 0.9f), BX - 3, BY - 3, BarW + 6, 2.f);
	DrawBar(BX, BY, BarW, BarH, Pct,
		FLinearColor(0.62f, 0.12f, 0.85f, 1.f), FLinearColor(0.10f, 0.05f, 0.14f, 0.92f));

	// Libellés avec ombre (contraste) directement sur la barre
	auto Shadowed = [&](const FString& S, float TX, float TY, UFont* F, float Sc, const FLinearColor& C)
	{
		DrawText(S, FLinearColor(0.f, 0.f, 0.f, 0.8f), TX + 1.f, TY + 1.f, F, Sc);
		DrawText(S, C, TX, TY, F, Sc);
	};
	Shadowed(TEXT("KRAKEN"), BX + 8.f, BY + 1.f, GEngine->GetMediumFont(), 0.9f, FLinearColor(0.95f, 0.85f, 1.f, 1.f));
	const FString HP = FString::Printf(TEXT("%d / %d"), CurHP, MaxHP);
	float HW, HH; GetTextSize(HP, HW, HH, GEngine->GetSmallFont(), 1.f);
	Shadowed(HP, BX + BarW - HW - 8.f, BY + 2.f, GEngine->GetSmallFont(), 1.f, FLinearColor::White);
}

// Constantes de disposition PARTAGÉES entre le rendu et le hit-test (double-clic).
// Cartes RÉDUITES (roster compact) -> prend moins de place en bas, surtout en phase 3
// où de nombreux groupes s'affichent. On garde PV + effectif + icône.
static constexpr float kCardW = 120.f, kCardH = 62.f, kGap = 6.f, kPad = 8.f, kBandX = 12.f;

int32 AWOTOLDemoHUD::CommandCardMaxFit(float W)
{
	return FMath::Max(1, (int32)((W - 180.f) / (kCardW + kGap)));
}

FBox2D AWOTOLDemoHUD::CommandCardRect(int32 Index, float W, float H)
{
	const float BandH = kCardH + 12.f;
	const float BandY = H - BandH;
	const float Y = BandY + (BandH - kCardH) * 0.5f;
	const float X = kBandX + kPad + Index * (kCardW + kGap);
	return FBox2D(FVector2D(X, Y), FVector2D(X + kCardW, Y + kCardH));
}

// Construit les groupes du roster : TOUTES les unités vivantes de la faction joueur,
// agrégées par nom ET PAR GRADE (AWOTOLDemoUnit::GradeLevel), dans l'ordre du registre.
// Deux grades du même type ne sont JAMAIS fusionnés dans le même groupe -> chaque grade a sa
// propre carte, sélectionnable/déplaçable indépendamment (demande de Liamor du 26/07/2026,
// façon Total War Warhammer III : un même type d'unité à des rangs différents = des cartes
// distinctes). Partagé HUD (rendu) ↔ PlayerController (clic) : les deux restent synchronisés.
void AWOTOLDemoHUD::BuildRosterGroups(UWorld* World, EFactionID Faction,
	TArray<FString>& OutOrder, TMap<FString, TArray<AUnitBase*>>& OutByName)
{
	OutOrder.Reset();
	OutByName.Reset();
	if (!World) return;
	UFactionRegistrySubsystem* Reg = World->GetSubsystem<UFactionRegistrySubsystem>();
	if (!Reg) return;
	for (AUnitBase* U : Reg->GetUnitsForFaction(Faction))
	{
		if (!U || !U->IsAlive()) continue;
		const FString BaseName = (U->GetUnitData() && !U->GetUnitData()->DisplayName.IsEmpty())
			? U->GetUnitData()->DisplayName.ToString() : U->GetName();
		const int32 Grade = Cast<AWOTOLDemoUnit>(U) ? Cast<AWOTOLDemoUnit>(U)->GradeLevel : 1;
		// Suffixe COURT (les cartes du bandeau de commandement sont petites, ~120px) : "N2"
		// plutot que "Niveau 2" pour limiter le risque de debordement du texte sur la carte.
		const FString Name = FString::Printf(TEXT("%s N%d"), *BaseName, Grade);
		if (!OutByName.Contains(Name)) OutOrder.Add(Name);
		OutByName.FindOrAdd(Name).Add(U);
	}
}

void AWOTOLDemoHUD::DrawCommandBar(float W, float H, UWorld* World)
{
	if (!World) return;
	UUnitSelectionManager* Sel = World->GetSubsystem<UUnitSelectionManager>();

	// Faction du joueur (source : subsystem de démo).
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	const EFactionID PlayerFac = Demo ? Demo->GetPlayerFaction() : EFactionID::None;
	if (PlayerFac == EFactionID::None) return;

	// Roster = TOUTES les unités vivantes du joueur (pas seulement la sélection) ->
	// le roster reste complet même quand un seul groupe est actif.
	TArray<FString> Order;
	TMap<FString, TArray<AUnitBase*>> ByName;
	BuildRosterGroups(World, PlayerFac, Order, ByName);
	if (Order.Num() == 0) return;

	// Ensemble des unités ACTUELLEMENT sélectionnées (= groupe actif) -> sert à
	// surligner les cartes actives et à griser les autres (toujours affichées).
	TSet<AUnitBase*> SelSet;
	if (Sel) for (AUnitBase* U : Sel->GetSelectedUnits()) if (U) SelSet.Add(U);

	// Agrégats par groupe (dans l'ordre du roster).
	struct FGroup { FString Name; int32 Count = 0; float HpSum = 0.f;
		int32 HpCur = 0; int32 HpMax = 0; EFactionID Fac = EFactionID::None;
		EUnitRole Role = EUnitRole::Infanterie; bool bActive = false; };
	TArray<FGroup> Groups;
	Groups.Reserve(Order.Num());
	for (const FString& Name : Order)
	{
		FGroup G; G.Name = Name; G.Fac = PlayerFac;
		for (AUnitBase* U : ByName[Name])
		{
			if (!U) continue;
			G.Count++;
			if (G.Role == EUnitRole::Infanterie && U->GetUnitData()) G.Role = U->GetUnitData()->Role;
			if (SelSet.Contains(U)) G.bActive = true;
			float Pct; int32 UMax;
			if (AWOTOLDemoUnit* DU = Cast<AWOTOLDemoUnit>(U))
			{
				Pct  = DU->GetEffectiveHealthPercent();
				UMax = DU->GetEffectiveMaxHealth();
			}
			else
			{
				Pct  = U->GetHealthPercent();
				UMax = (U->GetUnitData()) ? U->GetUnitData()->Stats.MaxHealth : 100;
			}
			G.HpSum += Pct;
			G.HpMax += UMax;
			G.HpCur += FMath::RoundToInt(Pct * UMax);
		}
		if (G.Count > 0) Groups.Add(MoveTemp(G));
	}
	if (Groups.Num() == 0) return;

	// Icône DISTINCTE par type d'unité (forme différente selon le rôle).
	auto DrawUnitIcon = [&](float CX, float CY, float R, EUnitRole IconRole, const FLinearColor& Fac)
	{
		if (!Canvas) return;
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.55f), CX - R - 2.f, CY - R - 2.f, (R + 2.f) * 2.f, (R + 2.f) * 2.f);
		const FLinearColor Fill(FMath::Min(1.f, Fac.R + 0.15f), FMath::Min(1.f, Fac.G + 0.15f),
			FMath::Min(1.f, Fac.B + 0.15f), 1.f);
		switch (IconRole)
		{
			case EUnitRole::Chef:      Canvas->K2_DrawPolygon(nullptr, FVector2D(CX, CY), FVector2D(R, R), 3, Fill); break; // triangle
			case EUnitRole::Mythique:  Canvas->K2_DrawPolygon(nullptr, FVector2D(CX, CY), FVector2D(R * 1.1f, R * 1.1f), 6, Fill); break; // hexa
			case EUnitRole::Montee:    Canvas->K2_DrawPolygon(nullptr, FVector2D(CX, CY), FVector2D(R, R), 5, Fill); break; // penta
			case EUnitRole::Distance:  Canvas->K2_DrawPolygon(nullptr, FVector2D(CX, CY), FVector2D(R, R), 4, Fill); break; // losange
			case EUnitRole::Speciale:  Canvas->K2_DrawPolygon(nullptr, FVector2D(CX, CY), FVector2D(R, R), 8, Fill); break; // octogone
			default:                   DrawRect(Fill, CX - R * 0.8f, CY - R * 0.8f, R * 1.6f, R * 1.6f); break;             // carré (infanterie)
		}
	};

	// ── Bandeau ADAPTATIF, COMPACT : largeur = nombre de cartes affichées ──
	const float CardW = kCardW, CardH = kCardH, Gap = kGap, Pad = kPad;
	const int32 MaxFit = CommandCardMaxFit(W); // laisse la place aux boutons de couche (droite)
	const int32 Shown = FMath::Min(Groups.Num(), MaxFit);
	const float BandH = CardH + 10.f;
	const float BandY = H - BandH;
	const float BandW = Shown * (CardW + Gap) - Gap + Pad * 2.f;
	const float BandX = kBandX;
	DrawRect(FLinearColor(0.02f, 0.03f, 0.05f, 0.78f), BandX, BandY, BandW, BandH);
	DrawRect(FLinearColor(0.30f, 0.62f, 1.f, 0.7f), BandX, BandY, BandW, 2.f); // liseré haut

	float X = BandX + Pad;
	const float Y = BandY + (BandH - CardH) * 0.5f;
	for (int32 gi = 0; gi < Shown; ++gi)
	{
		const FGroup& G = Groups[gi];
		const FLinearColor Fac = FFactionColors::Get(G.Fac);
		// Groupe ACTIF (sélectionné) = fond plus clair + cadre lumineux ; inactif = grisé.
		const float Dim = G.bActive ? 1.f : 0.55f;
		DrawRect(FLinearColor(G.bActive ? 0.06f : 0.f, G.bActive ? 0.08f : 0.f,
			G.bActive ? 0.14f : 0.f, G.bActive ? 0.82f : 0.6f), X, Y, CardW, CardH);
		DrawRect(FLinearColor(Fac.R, Fac.G, Fac.B, Dim), X, Y, CardW, 3.f); // liseré faction
		if (G.bActive)
		{
			// Cadre de sélection (4 bords cyan) autour de la carte active.
			const FLinearColor Sel2(0.45f, 0.85f, 1.f, 1.f);
			DrawLine(X, Y, X + CardW, Y, Sel2, 2.f);
			DrawLine(X, Y + CardH, X + CardW, Y + CardH, Sel2, 2.f);
			DrawLine(X, Y, X, Y + CardH, Sel2, 2.f);
			DrawLine(X + CardW, Y, X + CardW, Y + CardH, Sel2, 2.f);
		}
		DrawUnitIcon(X + 20.f, Y + 22.f, 13.f, G.Role, FLinearColor(Fac.R * Dim, Fac.G * Dim, Fac.B * Dim, 1.f));
		// Nom (compact) + effectif
		const FLinearColor NameCol(Dim, Dim, Dim, 1.f);
		DrawText(G.Name, NameCol, X + 38.f, Y + 6.f, GEngine->GetSmallFont(), 1.f);
		DrawText(FString::Printf(TEXT("x%d"), G.Count), FLinearColor(1.f * Dim, 0.9f * Dim, 0.5f * Dim, 1.f),
			X + 38.f, Y + 22.f, GEngine->GetSmallFont(), 1.1f);
		// PV totaux du groupe (compact) au-dessus de la barre
		const FString HpTxt = FString::Printf(TEXT("%d/%d"), G.HpCur, G.HpMax);
		DrawText(HpTxt, FLinearColor(0.85f * Dim, 0.95f * Dim, 0.85f * Dim, 1.f), X + 6.f, Y + CardH - 24.f,
			GEngine->GetSmallFont(), 0.9f);
		// Mini-barre de vie moyenne du groupe
		const float AvgHp = (G.Count > 0) ? G.HpSum / G.Count : 0.f;
		const FLinearColor HpCol = FMath::Lerp(FLinearColor(0.8f, 0.1f, 0.1f, 1.f),
			FLinearColor(0.2f, 0.85f, 0.2f, 1.f), AvgHp);
		DrawBar(X + 6.f, Y + CardH - 10.f, CardW - 12.f, 6.f, AvgHp, HpCol,
			FLinearColor(0.12f, 0.12f, 0.12f, 0.9f));

		X += CardW + Gap;
	}
}

// Petit panneau (au-dessus de la barre de commandement, coin bas-droit) affichant l'état
// de la compétence (touche R) de l'unité primaire sélectionnée : "Prête" ou "Recharge : Xs".
// Purement informatif (pas de bouton cliquable) -> aucun risque de conflit avec le reste du HUD.
void AWOTOLDemoHUD::DrawAbilityStatus(float W, float H, class UWorld* World)
{
	AWOTOLPlayerController_Battle* PC = Cast<AWOTOLPlayerController_Battle>(GetOwningPlayerController());
	if (!PC) return;

	FText AbilityName; float Remaining = 0.f, Max = 1.f;
	if (!PC->GetPrimarySelectionAbilityStatus(AbilityName, Remaining, Max)) return;

	const bool  bReady = Remaining <= 0.f;
	const float PW = 260.f, PH = 56.f;
	const float PX = W - PW - 16.f, PY = H - 210.f;

	DrawRect(FLinearColor(0.02f, 0.05f, 0.09f, 0.85f), PX, PY, PW, PH);
	DrawRect(bReady ? FLinearColor(0.25f, 0.9f, 0.4f, 1.f) : FLinearColor(0.6f, 0.6f, 0.62f, 1.f),
		PX, PY, PW, 3.f);

	DrawText(AbilityName.ToString(), FLinearColor(0.9f, 0.95f, 1.f, 1.f),
		PX + 12.f, PY + 8.f, GEngine->GetSmallFont(), 1.0f);

	const FString StatusTxt = bReady
		? TEXT("Prete (R)")
		: FString::Printf(TEXT("Recharge : %ds"), FMath::CeilToInt(Remaining));
	DrawText(StatusTxt, bReady ? FLinearColor(0.4f, 1.f, 0.55f, 1.f) : FLinearColor(0.8f, 0.8f, 0.82f, 1.f),
		PX + 12.f, PY + 28.f, GEngine->GetMediumFont(), 1.0f);

	if (!bReady)
	{
		const float Pct = FMath::Clamp(1.f - Remaining / FMath::Max(0.01f, Max), 0.f, 1.f);
		DrawBar(PX + 12.f, PY + PH - 10.f, PW - 24.f, 5.f, Pct,
			FLinearColor(0.5f, 0.75f, 1.f, 1.f), FLinearColor(0.12f, 0.12f, 0.12f, 0.9f));
	}
}

FBox2D AWOTOLDemoHUD::MinimapRect(float W, float H)
{
	const float MW = 220.f, MH = 220.f;
	// Sous les boutons pause/réglages (qui occupent Y 14-50 en haut-droit) -> pas de chevauchement.
	return FBox2D(FVector2D(W - MW - 16.f, 58.f), FVector2D(W - 16.f, 58.f + MH));
}

// Bornes du monde (centre + étendue) utilisées par la minimap — extrait en fonction PARTAGÉE
// entre le dessin (DrawMinimap) et le clic (PlayerController, pour convertir la position
// cliquée en point monde et recentrer la caméra dessus). Une seule source de vérité -> les
// deux restent forcément synchronisés (même principe que BuildRosterGroups pour le roster).
bool AWOTOLDemoHUD::GetMinimapWorldFrame(UWorld* World, FVector2D& OutCenter, float& OutSpan)
{
	if (!World) return false;
	UFactionRegistrySubsystem* Reg = World->GetSubsystem<UFactionRegistrySubsystem>();
	UGameInstance* GI = World->GetGameInstance();
	UDemoFlowSubsystem* Demo = GI ? GI->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Reg || !Demo) return false;

	const EFactionID PlayerFac = Demo->GetPlayerFaction();
	if (PlayerFac == EFactionID::None) return false;
	const EFactionID RivalFac = (PlayerFac == EFactionID::Noxeens) ? EFactionID::Aquiloris : EFactionID::Noxeens;

	const TArray<AUnitBase*> PlayerUnits = Reg->GetUnitsForFaction(PlayerFac);
	const TArray<AUnitBase*> RivalUnits  = Reg->GetUnitsForFaction(RivalFac);
	if (PlayerUnits.Num() == 0 && RivalUnits.Num() == 0) return false;
	AActor* Building = Demo->GetCaptureObject();

	FVector2D MinP(TNumericLimits<float>::Max(), TNumericLimits<float>::Max());
	FVector2D MaxP(-TNumericLimits<float>::Max(), -TNumericLimits<float>::Max());
	auto Accumulate = [&](const FVector& Loc)
	{
		MinP.X = FMath::Min(MinP.X, Loc.X); MaxP.X = FMath::Max(MaxP.X, Loc.X);
		MinP.Y = FMath::Min(MinP.Y, Loc.Y); MaxP.Y = FMath::Max(MaxP.Y, Loc.Y);
	};
	for (AUnitBase* U : PlayerUnits) if (U && U->IsAlive()) Accumulate(U->GetActorLocation());
	for (AUnitBase* U : RivalUnits)  if (U && U->IsAlive()) Accumulate(U->GetActorLocation());
	if (Building) Accumulate(Building->GetActorLocation());
	if (MinP.X > MaxP.X) return false; // rien de vivant à afficher

	// Marge (30%) + étendue minimum pour éviter une minimap dégénérée (tout aligné/collé).
	const float SpanX = FMath::Max(1000.f, MaxP.X - MinP.X);
	const float SpanY = FMath::Max(1000.f, MaxP.Y - MinP.Y);
	OutSpan = FMath::Max(SpanX, SpanY) * 1.3f;
	OutCenter = FVector2D((MinP.X + MaxP.X) * 0.5f, (MinP.Y + MaxP.Y) * 0.5f);
	return true;
}

// Convertit un CLIC (position écran) sur le panneau minimap en position MONDE (plan
// horizontal, Z=0 — la caméra de bataille ignore Z dans FocusOn). Renvoie faux si le clic
// tombe hors du panneau ou si la trame n'a pas pu être calculée (rien de vivant à l'écran).
bool AWOTOLDemoHUD::MinimapScreenToWorld(const FVector2D& ScreenPos, float W, float H,
	UWorld* World, FVector& OutWorldLoc)
{
	const FBox2D Rect = MinimapRect(W, H);
	if (!Rect.IsInside(ScreenPos)) return false;

	FVector2D Center; float Span;
	if (!GetMinimapWorldFrame(World, Center, Span)) return false;

	const FVector2D PanelSize = Rect.Max - Rect.Min;
	const float NX = (ScreenPos.X - Rect.Min.X) / PanelSize.X - 0.5f;
	const float NY = (ScreenPos.Y - Rect.Min.Y) / PanelSize.Y - 0.5f;
	OutWorldLoc = FVector(Center.X + NX * Span, Center.Y + NY * Span, 0.f);
	return true;
}

// Minimap SCHÉMATIQUE (pas une capture caméra 3D — cohérent avec le reste du HUD 100% Canvas,
// aucun asset/materiel supplémentaire requis) : calcule les bornes du monde à partir des unités
// vivantes + du bâtiment de capture, projette chaque acteur en point coloré. Absente jusqu'ici
// alors que toutes les références du genre (Total War, Company of Heroes, Homeworld) ET tes
// propres maquettes (UI_HUD_Complet) en montrent une.
void AWOTOLDemoHUD::DrawMinimap(float W, float H, UWorld* World)
{
	FVector2D WorldCenter; float Span;
	if (!GetMinimapWorldFrame(World, WorldCenter, Span)) return;
	if (!World) return;

	UFactionRegistrySubsystem* Reg = World->GetSubsystem<UFactionRegistrySubsystem>();
	UDemoFlowSubsystem* Demo = GetGameInstance() ? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Reg || !Demo) return;
	const EFactionID PlayerFac = Demo->GetPlayerFaction();
	const EFactionID RivalFac = (PlayerFac == EFactionID::Noxeens) ? EFactionID::Aquiloris : EFactionID::Noxeens;
	const TArray<AUnitBase*> PlayerUnits = Reg->GetUnitsForFaction(PlayerFac);
	const TArray<AUnitBase*> RivalUnits  = Reg->GetUnitsForFaction(RivalFac);
	AActor* Building = Demo->GetCaptureObject();

	const FBox2D Rect = MinimapRect(W, H);
	const FVector2D PanelSize = Rect.Max - Rect.Min;

	DrawRect(FLinearColor(0.02f, 0.05f, 0.09f, 0.85f), Rect.Min.X, Rect.Min.Y, PanelSize.X, PanelSize.Y);
	DrawRect(FLinearColor(0.30f, 0.62f, 1.f, 0.8f), Rect.Min.X, Rect.Min.Y, PanelSize.X, 2.f);

	auto ToScreen = [&](const FVector& WorldLoc) -> FVector2D
	{
		const float NX = (WorldLoc.X - WorldCenter.X) / Span + 0.5f;
		const float NY = (WorldLoc.Y - WorldCenter.Y) / Span + 0.5f;
		return FVector2D(Rect.Min.X + NX * PanelSize.X, Rect.Min.Y + NY * PanelSize.Y);
	};

	auto DrawDot = [&](const FVector& WorldLoc, const FLinearColor& Col, float Size)
	{
		const FVector2D P = ToScreen(WorldLoc);
		if (P.X < Rect.Min.X || P.X > Rect.Max.X || P.Y < Rect.Min.Y || P.Y > Rect.Max.Y) return;
		DrawRect(Col, P.X - Size * 0.5f, P.Y - Size * 0.5f, Size, Size);
	};

	const FLinearColor PlayerCol = FFactionColors::Get(PlayerFac);
	const FLinearColor RivalCol  = FFactionColors::Get(RivalFac);

	for (AUnitBase* U : RivalUnits)
	{
		if (!U || !U->IsAlive()) continue;
		const AWOTOLDemoUnit* DU = Cast<AWOTOLDemoUnit>(U);
		DrawDot(U->GetActorLocation(), RivalCol, (DU && DU->bIsBoss) ? 10.f : 4.f);
	}
	for (AUnitBase* U : PlayerUnits)
	{
		if (!U || !U->IsAlive()) continue;
		DrawDot(U->GetActorLocation(), PlayerCol, 4.f);
	}
	if (Building)
	{
		DrawDot(Building->GetActorLocation(), FLinearColor(1.f, 0.85f, 0.2f, 1.f), 8.f);
	}

	// Repère caméra ("vous regardez ici") : croix fine qui traverse le panneau.
	if (AWOTOLPlayerController_Battle* PC = Cast<AWOTOLPlayerController_Battle>(GetOwningPlayerController()))
	{
		if (AWOTOLBattleCamera* Cam = PC->GetBattleCamera())
		{
			const FVector2D P = ToScreen(Cam->GetActorLocation());
			DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.6f), P.X - 1.f, Rect.Min.Y, 2.f, PanelSize.Y);
			DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.6f), Rect.Min.X, P.Y - 1.f, PanelSize.X, 2.f);
		}
	}
}

// Icône distincte par rôle (même formes que le roster) — repère visuel du type d'unité.
void AWOTOLDemoHUD::DrawRoleIcon(float CX, float CY, float R, EUnitRole IconRole, const FLinearColor& Fac)
{
	if (!Canvas) return;
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.55f), CX - R - 2.f, CY - R - 2.f, (R + 2.f) * 2.f, (R + 2.f) * 2.f);
	const FLinearColor Fill(FMath::Min(1.f, Fac.R + 0.15f), FMath::Min(1.f, Fac.G + 0.15f),
		FMath::Min(1.f, Fac.B + 0.15f), 1.f);
	switch (IconRole)
	{
		case EUnitRole::Chef:      Canvas->K2_DrawPolygon(nullptr, FVector2D(CX, CY), FVector2D(R, R), 3, Fill); break;
		case EUnitRole::Mythique:  Canvas->K2_DrawPolygon(nullptr, FVector2D(CX, CY), FVector2D(R * 1.1f, R * 1.1f), 6, Fill); break;
		case EUnitRole::Montee:    Canvas->K2_DrawPolygon(nullptr, FVector2D(CX, CY), FVector2D(R, R), 5, Fill); break;
		case EUnitRole::Distance:  Canvas->K2_DrawPolygon(nullptr, FVector2D(CX, CY), FVector2D(R, R), 4, Fill); break;
		case EUnitRole::Speciale:  Canvas->K2_DrawPolygon(nullptr, FVector2D(CX, CY), FVector2D(R, R), 8, Fill); break;
		default:                   DrawRect(Fill, CX - R * 0.8f, CY - R * 0.8f, R * 1.6f, R * 1.6f); break;
	}
}

// MARQUEURS DE GROUPE (Total War) : pour chaque groupe (représentant) et chaque unité isolée,
// on projette sa position à l'écran et on dessine UN repère compact (icône + effectif + barre
// de vie). Les unités couvertes par un représentant ne dessinent RIEN. -> écran lisible.
void AWOTOLDemoHUD::DrawBattlefieldMarkers(float W, float H, UWorld* World)
{
	if (!World) return;
	UFactionRegistrySubsystem* Reg = World->GetSubsystem<UFactionRegistrySubsystem>();
	if (!Reg) return;

	const EFactionID Facs[2] = { EFactionID::Aquiloris, EFactionID::Noxeens };
	for (const EFactionID F : Facs)
	{
		const FLinearColor Fac = FFactionColors::Get(F);
		for (AUnitBase* U : Reg->GetUnitsForFaction(F))
		{
			AWOTOLDemoUnit* D = Cast<AWOTOLDemoUnit>(U);
			if (!D || !D->IsAlive()) continue;
			if (D->bIsBoss || D->bCreatureBrain) continue; // le boss garde son grand nom 3D
			// UN marqueur par GROUPE (le représentant central) ET un pour chaque unité SOLO/
			// isolée (Chef, mythique…). On masque seulement les unités COUVERTES par un
			// représentant -> chaque TYPE d'unité a au moins un indicateur, sans surcharge.
			if (D->IsTagSuppressed()) continue;

			// Position écran (au-dessus du modèle, en tenant compte de la couche/hauteur).
			USceneComponent* A = D->GetFloatingTextAnchor();
			const FVector WLoc = (A ? A->GetComponentLocation() : D->GetActorLocation()) + FVector(0, 0, 120.f);
			const FVector SP = Project(WLoc);
			if (SP.Z <= 0.f) continue;                     // derrière la caméra
			if (SP.X < -40.f || SP.X > W + 40.f || SP.Y < -40.f || SP.Y > H + 40.f) continue;

			const bool bGroup = D->IsTagRep() && D->GetTagCount() >= 2;
			const int32 Cur = bGroup ? D->GetTagCur() : FMath::RoundToInt(D->GetHealthPercent() * D->GetEffectiveMaxHealth());
			const int32 Mx  = bGroup ? D->GetTagMax() : D->GetEffectiveMaxHealth();
			const float Pct = (Mx > 0) ? (float)Cur / (float)Mx : 0.f;
			const EUnitRole URole = D->GetUnitData() ? D->GetUnitData()->Role : EUnitRole::Infanterie;

			const float IconR = bGroup ? 9.f : 6.f;
			DrawRoleIcon(SP.X, SP.Y, IconR, URole, Fac);
			// Effectif à droite de l'icône (groupes seulement).
			if (bGroup)
				DrawText(FString::Printf(TEXT("x%d"), D->GetTagCount()), FLinearColor::White,
					SP.X + IconR + 4.f, SP.Y - 8.f, GEngine->GetSmallFont(), 1.f);
			// Petite barre de vie sous l'icône.
			const float BarW = bGroup ? 34.f : 22.f, BarH = 4.f;
			const FLinearColor HpCol = FMath::Lerp(FLinearColor(0.85f, 0.12f, 0.12f, 1.f),
				FLinearColor(0.2f, 0.9f, 0.25f, 1.f), Pct);
			DrawBar(SP.X - BarW * 0.5f, SP.Y + IconR + 4.f, BarW, BarH, Pct, HpCol,
				FLinearColor(0.05f, 0.05f, 0.05f, 0.85f));
		}
	}
}

void AWOTOLDemoHUD::DrawCenteredText(const FString& Text, float Y,
	const FLinearColor& Color, float Scale)
{
	if (!Canvas) return;
	UFont* Font = GEngine ? GEngine->GetLargeFont() : nullptr;

	float TW = 0.f, TH = 0.f;
	GetTextSize(Text, TW, TH, Font, Scale);
	const float X = (Canvas->SizeX - TW) * 0.5f;

	DrawText(Text, FLinearColor(0.f, 0.f, 0.f, 0.7f), X + 2.f, Y + 2.f, Font, Scale);
	DrawText(Text, Color, X, Y, Font, Scale);
}
