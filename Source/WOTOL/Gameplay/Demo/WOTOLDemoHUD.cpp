#include "WOTOLDemoHUD.h"
#include "DemoFlowSubsystem.h"
#include "WOTOLDemoUnit.h"
#include "WOTOLCaptureObject.h"
#include "OceanCurrentSubsystem.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Texture2D.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Gameplay/Battle/WOTOLPlayerController_Battle.h"
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

FBox2D AWOTOLDemoHUD::MenuButtonRect(int32 Index, float W, float H)
{
	const float BW = 280.f, BH = 54.f, Gap = 18.f;
	const float X = (W - BW) * 0.5f;
	const float Y0 = H * 0.42f;
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

	// ─── Écrans avant-jeu (menu / faction) : on dessine UNIQUEMENT l'écran ───
	if (Screen == EDemoScreen::MainMenu)   { DrawMainMenu(W, H);      return; }
	if (Screen == EDemoScreen::FactionSelect) { DrawFactionSelect(W, H); return; }
	if (Screen == EDemoScreen::Summary)    { DrawSummary(W, H, DemoFlow); return; }
	if (Screen == EDemoScreen::Interlude)  { DrawInterlude(W, H, DemoFlow); return; }

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

	// ─── 5) Barre de commandement (bas) : cartes d'unités sélectionnées ──────
	DrawCommandBar(W, H, World);

	// ─── Préparation : bandeau d'instructions + bouton "Lancer la bataille" ──
	if (Screen == EDemoScreen::Prepare)
	{
		DrawPrepareBar(W, H);
	}

	// ─── Boutons de couche verticale (nage) ─────────────────────────────────
	DrawButton(LayerUpButtonRect(W, H),   TEXT("^ Monter"),    FLinearColor(0.3f, 0.7f, 1.f, 1.f), 1.f);
	DrawButton(LayerDownButtonRect(W, H), TEXT("v Descendre"), FLinearColor(0.3f, 0.7f, 1.f, 1.f), 1.f);

	// ─── 6) Bouton pause + voile du menu pause ───────────────────────────────
	DrawPauseButton(W, H);
	if (UGameplayStatics::IsGamePaused(World))
	{
		DrawPauseOverlay(W, H);
	}
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
		// Aquiloris : cristal (triangle cyan) + halo
		Canvas->K2_DrawPolygon(nullptr, CA, FVector2D(52.f, 52.f), 16, FLinearColor(0.2f, 0.6f, 1.f, 0.15f));
		Canvas->K2_DrawPolygon(nullptr, CA, FVector2D(34.f, 46.f), 3, FLinearColor(0.5f, 0.9f, 1.f, 0.95f));
		// Noxéens : organisme (hexa vert) + halo
		Canvas->K2_DrawPolygon(nullptr, CN, FVector2D(52.f, 52.f), 16, FLinearColor(0.2f, 0.9f, 0.45f, 0.15f));
		Canvas->K2_DrawPolygon(nullptr, CN, FVector2D(40.f, 40.f), 16, FLinearColor(0.3f, 0.95f, 0.5f, 0.95f));
	}

	DrawButton(FactionButtonRect(0, W, H), TEXT("AQUILORIS"), FLinearColor(0.3f, 0.6f, 1.f, 1.f), 1.7f);
	DrawButton(FactionButtonRect(1, W, H), TEXT("NOXEENS"), FLinearColor(0.3f, 0.95f, 0.5f, 1.f), 1.7f);
	// Description COURTE, juste sous les boutons de faction (bien au-dessus du bloc difficulté).
	DrawCenteredText(TEXT("Aquiloris : cristal-tech, coordination          Noxeens : abysses bioluminescents"),
		FactionButtonRect(0, W, H).Max.Y + H * 0.04f, FLinearColor(0.8f, 0.9f, 1.f, 0.9f), 1.0f);

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
}

void AWOTOLDemoHUD::DrawSummary(float W, float H, UDemoFlowSubsystem* Demo)
{
	DrawUnderwaterBackground(W, H);
	if (!Demo) return;

	// Titre (or si victoire, rouge si défaite)
	const bool bWin = Demo->bSummaryVictory;
	const FLinearColor TitleCol = bWin ? FLinearColor(1.f, 0.85f, 0.2f, 1.f)
		: FLinearColor(1.f, 0.3f, 0.25f, 1.f);
	const FString Title = Demo->SummaryTitle.IsEmpty()
		? (bWin ? TEXT("VICTOIRE") : TEXT("DEFAITE")) : Demo->SummaryTitle;
	DrawGlowTitle(FString::Printf(TEXT("— %s —"), *Title), H * 0.06f, 2.8f, TitleCol);
	const int32 Dur = FMath::RoundToInt(Demo->SummaryDurationSeconds);
	const FString Sub = FString::Printf(TEXT("Resume de la bataille  —  duree : %d min %02d s"), Dur / 60, Dur % 60);
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

	// Deux colonnes : pertes de TON armée (gauche) / pertes de l'ennemi (droite).
	// Hauteur bornée AU-DESSUS des boutons + interligne DYNAMIQUE -> tout tient, rien ne
	// chevauche (2 lignes compactes par unité).
	const float ColTop  = H * 0.22f;
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
		DrawButton(SummaryReplayButtonRect(W, H), TEXT("REJOUER LA PHASE"),
			FLinearColor(0.3f, 0.7f, 1.f, 1.f), 1.15f);
		DrawButton(SummaryChangeFactionButtonRect(W, H), TEXT("CHANGER DE FACTION"),
			FLinearColor(0.3f, 0.9f, 0.5f, 1.f), 1.05f);
		DrawButton(SummaryMenuButtonRect(W, H), TEXT("MENU PRINCIPAL"),
			FLinearColor(0.9f, 0.8f, 0.35f, 1.f), 1.1f);
		DrawButton(SummaryQuitButtonRect(W, H), TEXT("QUITTER"),
			FLinearColor(1.f, 0.45f, 0.35f, 1.f), 1.15f);
	}
	else
	{
		DrawButton(SummaryContinueButtonRect(W, H), TEXT("CONTINUER — Defense de la zone"),
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

	// Panneau en haut à droite (à gauche du bouton pause).
	const float BX = W - 210.f, BY = 8.f, BW = 150.f, BH = 58.f;
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
	DrawUnderwaterBackground(W, H);
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
	// deux barres ‖
	const float BarW = 7.f, BarH = Sz.Y * 0.55f;
	const float BarY = R.Min.Y + (Sz.Y - BarH) * 0.5f;
	DrawRect(FLinearColor::White, R.Min.X + Sz.X * 0.30f - BarW * 0.5f, BarY, BarW, BarH);
	DrawRect(FLinearColor::White, R.Min.X + Sz.X * 0.70f - BarW * 0.5f, BarY, BarW, BarH);
}

void AWOTOLDemoHUD::DrawPauseOverlay(float W, float H)
{
	// Voile sombre plein écran
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), 0.f, 0.f, W, H);
	DrawCenteredText(TEXT("PAUSE"), H * 0.28f, FLinearColor::White, 2.6f);

	const TCHAR* Labels[3] = { TEXT("Reprendre"), TEXT("Recommencer"), TEXT("Quitter") };
	for (int32 i = 0; i < 3; ++i)
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
// agrégées par nom, dans l'ordre du registre. Partagé HUD (rendu) ↔ PlayerController (clic).
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
		const FString Name = (U->GetUnitData() && !U->GetUnitData()->DisplayName.IsEmpty())
			? U->GetUnitData()->DisplayName.ToString() : U->GetName();
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
