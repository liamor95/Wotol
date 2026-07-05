#include "WOTOLDemoHUD.h"
#include "DemoFlowSubsystem.h"
#include "WOTOLDemoUnit.h"
#include "WOTOLCaptureObject.h"
#include "OceanCurrentSubsystem.h"
#include "Camera/PlayerCameraManager.h"
#include "Gameplay/Battle/WOTOLPlayerController_Battle.h"
#include "Gameplay/Battle/RTSBattleManager.h"
#include "Gameplay/Battle/UnitSelectionManager.h"
#include "Gameplay/Units/UnitBase.h"
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
	const float X = (W - BW) * 0.5f, Y = H * 0.55f;
	return FBox2D(FVector2D(X, Y), FVector2D(X + BW, Y + BH));
}

FBox2D AWOTOLDemoHUD::FactionButtonRect(int32 Index, float W, float H)
{
	const float BW = 320.f, BH = 90.f, Gap = 40.f;
	const float TotalW = BW * 2.f + Gap;
	const float X = (W - TotalW) * 0.5f + Index * (BW + Gap);
	const float Y = H * 0.48f;
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

// 3 boutons finaux alignés (Rejouer / Changer de faction / Quitter).
static FBox2D SummaryTripleRect(int32 Index, float W, float H)
{
	const float BW = 300.f, BH = 62.f, Gap = 30.f;
	const float TotalW = BW * 3.f + Gap * 2.f;
	const float X = (W - TotalW) * 0.5f + Index * (BW + Gap), Y = H - 120.f;
	return FBox2D(FVector2D(X, Y), FVector2D(X + BW, Y + BH));
}

FBox2D AWOTOLDemoHUD::SummaryReplayButtonRect(float W, float H)        { return SummaryTripleRect(0, W, H); }
FBox2D AWOTOLDemoHUD::SummaryChangeFactionButtonRect(float W, float H) { return SummaryTripleRect(1, W, H); }
FBox2D AWOTOLDemoHUD::SummaryQuitButtonRect(float W, float H)          { return SummaryTripleRect(2, W, H); }

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

	// Dégradé de profondeur : surface (haut) plus claire -> abysse (bas) sombre.
	const int32 Bands = 48;
	const FLinearColor Surface(0.05f, 0.20f, 0.34f, 1.f);
	const FLinearColor Deep   (0.005f, 0.02f, 0.06f, 1.f);
	const float BandH = H / Bands + 1.f;
	for (int32 i = 0; i < Bands; ++i)
	{
		const float f = (float)i / (Bands - 1);
		DrawRect(FMath::Lerp(Surface, Deep, f), 0.f, i * (H / Bands), W, BandH);
	}

	// Rais de lumière obliques qui dérivent lentement (god rays).
	for (int32 j = 0; j < 5; ++j)
	{
		const float baseX = W * (0.12f + 0.18f * j);
		const float x = baseX + FMath::Sin(T * 0.15f + j * 1.3f) * W * 0.04f;
		DrawRect(FLinearColor(0.4f, 0.7f, 1.f, 0.035f), x, 0.f, 90.f + 20.f * j, H);
	}

	// Bulles qui montent (cercles semi-transparents), déterministes par indice.
	auto Rnd = [](int32 n) -> float
	{
		n = (n << 13) ^ n;
		return (float)((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 2147483647.f;
	};
	if (Canvas)
	{
		const int32 NumBubbles = 46;
		for (int32 i = 0; i < NumBubbles; ++i)
		{
			const float bx    = Rnd(i) * W + FMath::Sin(T * 0.6f + i) * 12.f; // léger zig-zag
			const float size  = 3.f + Rnd(i + 100) * 13.f;
			const float speed = 28.f + Rnd(i + 200) * 74.f;
			const float phase = Rnd(i + 300);
			const float by    = H - FMath::Fmod(T * speed + phase * (H + 140.f), H + 140.f);
			const float a     = 0.06f + Rnd(i + 400) * 0.12f;
			Canvas->K2_DrawPolygon(nullptr, FVector2D(bx, by), FVector2D(size, size), 16,
				FLinearColor(0.6f, 0.85f, 1.f, a));
			// petit reflet clair sur la bulle
			Canvas->K2_DrawPolygon(nullptr, FVector2D(bx - size * 0.3f, by - size * 0.3f),
				FVector2D(size * 0.28f, size * 0.28f), 10, FLinearColor(0.9f, 0.97f, 1.f, a * 1.4f));
		}
	}

	// Vignette basse (assombrit le bas pour la lisibilité du texte).
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.35f), 0.f, H * 0.72f, W, H * 0.28f);
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

void AWOTOLDemoHUD::DrawMainMenu(float W, float H)
{
	DrawUnderwaterBackground(W, H);
	// Titre imposant + halo pulsant, sous-titre, puis bouton
	DrawGlowTitle(TEXT("W O T O L"), H * 0.24f, 4.4f, FLinearColor(0.5f, 0.88f, 1.f, 1.f));
	DrawCenteredText(TEXT("War of the Ocean's Legacy"), H * 0.40f, FLinearColor(0.85f, 0.93f, 1.f, 1.f), 1.4f);
	DrawButton(StartGameButtonRect(W, H), TEXT("COMMENCER LA DEMO"), FLinearColor(0.3f, 0.75f, 1.f, 1.f), 1.6f);
}

void AWOTOLDemoHUD::DrawFactionSelect(float W, float H)
{
	DrawUnderwaterBackground(W, H);
	DrawGlowTitle(TEXT("CHOISISSEZ VOTRE FACTION"), H * 0.16f, 2.2f, FLinearColor(0.7f, 0.9f, 1.f, 1.f));

	const float T = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	// Emblème animé au-dessus de chaque bouton (cristal Aquiloris / organisme Noxéen).
	if (Canvas)
	{
		const FBox2D RA = FactionButtonRect(0, W, H);
		const FBox2D RN = FactionButtonRect(1, W, H);
		const float bobA = FMath::Sin(T * 1.4f) * 8.f;
		const float bobN = FMath::Sin(T * 1.4f + 1.6f) * 8.f;
		const FVector2D CA((RA.Min.X + RA.Max.X) * 0.5f, RA.Min.Y - 70.f + bobA);
		const FVector2D CN((RN.Min.X + RN.Max.X) * 0.5f, RN.Min.Y - 70.f + bobN);
		// Aquiloris : cristal (triangle cyan) + halo
		Canvas->K2_DrawPolygon(nullptr, CA, FVector2D(52.f, 52.f), 16, FLinearColor(0.2f, 0.6f, 1.f, 0.15f));
		Canvas->K2_DrawPolygon(nullptr, CA, FVector2D(34.f, 46.f), 3, FLinearColor(0.5f, 0.9f, 1.f, 0.95f));
		// Noxéens : organisme (hexa vert) + halo
		Canvas->K2_DrawPolygon(nullptr, CN, FVector2D(52.f, 52.f), 16, FLinearColor(0.2f, 0.9f, 0.45f, 0.15f));
		Canvas->K2_DrawPolygon(nullptr, CN, FVector2D(40.f, 40.f), 16, FLinearColor(0.3f, 0.95f, 0.5f, 0.95f));
	}

	DrawButton(FactionButtonRect(0, W, H), TEXT("AQUILORIS"), FLinearColor(0.3f, 0.6f, 1.f, 1.f), 1.7f);
	DrawButton(FactionButtonRect(1, W, H), TEXT("NOXEENS"), FLinearColor(0.3f, 0.95f, 0.5f, 1.f), 1.7f);
	DrawCenteredText(TEXT("Aquiloris : cristal-tech, coordination   —   Noxeens : abysses bioluminescents"),
		FactionButtonRect(0, W, H).Max.Y + 40.f, FLinearColor(0.8f, 0.9f, 1.f, 0.9f), 1.0f);
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
	DrawCenteredText(TEXT("Resume de la bataille"), H * 0.17f, FLinearColor(0.8f, 0.9f, 1.f, 1.f), 1.1f);

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
		DrawButton(SummaryReplayButtonRect(W, H), TEXT("REJOUER"),
			FLinearColor(0.3f, 0.7f, 1.f, 1.f), 1.3f);
		DrawButton(SummaryChangeFactionButtonRect(W, H), TEXT("CHANGER DE FACTION"),
			FLinearColor(0.3f, 0.9f, 0.5f, 1.f), 1.2f);
		DrawButton(SummaryQuitButtonRect(W, H), TEXT("QUITTER"),
			FLinearColor(1.f, 0.45f, 0.35f, 1.f), 1.3f);
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
			? FMath::Clamp(AvailH / Lines.Num(), 24.f, 34.f) : 30.f;
		float Y = StartY;
		for (const FString& Line : Lines)
		{
			DrawCenteredText(Line, Y, FLinearColor(0.92f, 0.96f, 1.f, 1.f), 1.0f);
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

void AWOTOLDemoHUD::DrawCommandBar(float W, float H, UWorld* World)
{
	if (!World) return;
	UUnitSelectionManager* Sel = World->GetSubsystem<UUnitSelectionManager>();
	if (!Sel) return;

	// Agrège les unités sélectionnées par nom (groupe) + PV totaux du groupe
	struct FGroup { FString Name; int32 Count = 0; float HpSum = 0.f;
		int32 HpCur = 0; int32 HpMax = 0; EFactionID Fac = EFactionID::None;
		EUnitRole Role = EUnitRole::Infanterie; };
	TArray<FGroup> Groups;
	TMap<FString, int32> Index;
	for (AUnitBase* U : Sel->GetSelectedUnits())
	{
		if (!U || !U->IsAlive()) continue;
		const FString Name = (U->GetUnitData() && !U->GetUnitData()->DisplayName.IsEmpty())
			? U->GetUnitData()->DisplayName.ToString() : U->GetName();
		const EUnitRole URole = U->GetUnitData() ? U->GetUnitData()->Role : EUnitRole::Infanterie;
		int32* Found = Index.Find(Name);
		FGroup& G = Found ? Groups[*Found]
			: Groups[Index.Add(Name, Groups.Add(FGroup{ Name, 0, 0.f, 0, 0, U->GetFaction(), URole }))];
		G.Count++;
		// PV EFFECTIFS (multiplicateur de PV inclus) -> le courant ne dépasse plus le max.
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
	if (Groups.Num() == 0) return;

	// Icône DISTINCTE par type d'unité (forme différente selon le rôle) — pour
	// reconnaître l'unité d'un coup d'œil, pas seulement au nom.
	auto DrawUnitIcon = [&](float CX, float CY, float R, EUnitRole IconRole, const FLinearColor& Fac)
	{
		if (!Canvas) return;
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.55f), CX - R - 3.f, CY - R - 3.f, (R + 3.f) * 2.f, (R + 3.f) * 2.f);
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

	// ── Bandeau ADAPTATIF : sa largeur = nombre de cartes affichées (pas plein écran) ──
	const float CardW = 172.f, CardH = 90.f, Gap = 10.f, Pad = 12.f;
	const int32 MaxFit = FMath::Max(1, (int32)((W - 180.f) / (CardW + Gap))); // laisse la place aux boutons de couche (droite)
	const int32 Shown = FMath::Min(Groups.Num(), MaxFit);
	const float BandH = CardH + 12.f;
	const float BandY = H - BandH;
	const float BandW = Shown * (CardW + Gap) - Gap + Pad * 2.f;
	const float BandX = 12.f;
	DrawRect(FLinearColor(0.02f, 0.03f, 0.05f, 0.78f), BandX, BandY, BandW, BandH);
	DrawRect(FLinearColor(0.30f, 0.62f, 1.f, 0.7f), BandX, BandY, BandW, 3.f); // liseré haut

	float X = BandX + Pad;
	const float Y = BandY + (BandH - CardH) * 0.5f;
	for (int32 gi = 0; gi < Shown; ++gi)
	{
		const FGroup& G = Groups[gi];
		const FLinearColor Fac = FFactionColors::Get(G.Fac);
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), X, Y, CardW, CardH);
		DrawRect(Fac, X, Y, CardW, 4.f);                       // liseré couleur de faction
		// Icône distincte par rôle
		DrawUnitIcon(X + 30.f, Y + 34.f, 20.f, G.Role, Fac);
		// Nom + nombre
		DrawText(G.Name, FLinearColor::White, X + 58.f, Y + 10.f, GEngine->GetMediumFont(), 1.f);
		DrawText(FString::Printf(TEXT("x%d"), G.Count), FLinearColor(1.f, 0.9f, 0.5f, 1.f),
			X + 58.f, Y + 30.f, GEngine->GetMediumFont(), 1.2f);
		// PV TOTAUX du groupe (restant / total), au-dessus de la barre
		const FString HpTxt = FString::Printf(TEXT("PV %d / %d"), G.HpCur, G.HpMax);
		DrawText(HpTxt, FLinearColor(0.85f, 0.95f, 0.85f, 1.f), X + 8.f, Y + CardH - 30.f,
			GEngine->GetSmallFont(), 1.f);
		// Mini-barre de vie moyenne du groupe
		const float AvgHp = (G.Count > 0) ? G.HpSum / G.Count : 0.f;
		const FLinearColor HpCol = FMath::Lerp(FLinearColor(0.8f, 0.1f, 0.1f, 1.f),
			FLinearColor(0.2f, 0.85f, 0.2f, 1.f), AvgHp);
		DrawBar(X + 8.f, Y + CardH - 14.f, CardW - 16.f, 8.f, AvgHp, HpCol,
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
