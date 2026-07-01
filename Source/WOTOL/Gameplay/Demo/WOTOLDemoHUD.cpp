#include "WOTOLDemoHUD.h"
#include "DemoFlowSubsystem.h"
#include "WOTOLDemoUnit.h"
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
	const float BW = 360.f, BH = 62.f;
	const float X = (W - BW) * 0.5f, Y = H - 150.f;
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

	// ─── 3) Barre de vie du BOSS (style "boss fight") ────────────────────────
	if (Demo)
	{
		if (AWOTOLDemoUnit* Boss = Cast<AWOTOLDemoUnit>(Demo->GetBoss()))
		{
			DrawBossBar(W, H, Boss);
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
	DrawRect(FLinearColor(0.04f, 0.06f, 0.10f, 0.96f), R.Min.X, R.Min.Y, Sz.X, Sz.Y);
	DrawRect(Tint, R.Min.X, R.Min.Y, Sz.X, 4.f);                          // liseré haut
	DrawRect(Tint, R.Min.X, R.Max.Y - 4.f, Sz.X, 4.f);                    // liseré bas
	float TW, TH; GetTextSize(Label, TW, TH, GEngine->GetLargeFont(), TextScale);
	DrawText(Label, FLinearColor::White, R.Min.X + (Sz.X - TW) * 0.5f,
		R.Min.Y + (Sz.Y - TH) * 0.5f, GEngine->GetLargeFont(), TextScale);
}

void AWOTOLDemoHUD::DrawMainMenu(float W, float H)
{
	DrawRect(FLinearColor(0.01f, 0.03f, 0.06f, 1.f), 0.f, 0.f, W, H); // fond bleu nuit
	DrawCenteredText(TEXT("W O T O L"), H * 0.26f, FLinearColor(0.5f, 0.85f, 1.f, 1.f), 3.4f);
	DrawCenteredText(TEXT("War of the Ocean's Legacy"), H * 0.37f, FLinearColor(0.8f, 0.9f, 1.f, 1.f), 1.2f);
	DrawButton(StartGameButtonRect(W, H), TEXT("Commencer la demo"), FLinearColor(0.3f, 0.7f, 1.f, 1.f), 1.5f);
}

void AWOTOLDemoHUD::DrawFactionSelect(float W, float H)
{
	DrawRect(FLinearColor(0.01f, 0.03f, 0.06f, 1.f), 0.f, 0.f, W, H);
	DrawCenteredText(TEXT("Choisissez votre faction"), H * 0.30f, FLinearColor::White, 2.0f);
	DrawButton(FactionButtonRect(0, W, H), TEXT("AQUILORIS"), FLinearColor(0.25f, 0.55f, 1.f, 1.f), 1.6f);
	DrawButton(FactionButtonRect(1, W, H), TEXT("NOXEENS"), FLinearColor(0.25f, 0.9f, 0.45f, 1.f), 1.6f);
}

void AWOTOLDemoHUD::DrawPrepareBar(float W, float H)
{
	DrawCenteredText(TEXT("PREPARATION — placez vos unites (clic gauche: selection, clic droit: deplacer)"),
		H * 0.20f, FLinearColor(0.9f, 0.95f, 1.f, 1.f), 1.0f);
	DrawButton(LaunchBattleButtonRect(W, H), TEXT("LANCER LA BATAILLE"),
		FLinearColor(1.f, 0.7f, 0.2f, 1.f), 1.5f);
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

	// Bandeau d'objectif (sous le timer)
	if (Demo && !Demo->CurrentMessage.IsEmpty())
	{
		const FString& Msg = Demo->CurrentMessage;
		float MW, MH; GetTextSize(Msg, MW, MH, GEngine->GetLargeFont(), 1.15f);
		const float BandW = FMath::Min(W - 40.f, MW + 60.f);
		const float BX = (W - BandW) * 0.5f, BY = H * 0.12f;
		DrawRect(FLinearColor(0.02f, 0.04f, 0.07f, 0.78f), BX, BY, BandW, MH + 14.f);
		DrawRect(FLinearColor(0.95f, 0.78f, 0.25f, 0.9f), BX, BY, 4.f, MH + 14.f);       // liseré or gauche
		DrawText(Msg, FLinearColor::White, BX + 30.f, BY + 7.f, GEngine->GetLargeFont(), 1.15f);
	}

	// Objectif courant (permanent, en haut à gauche)
	if (Demo && !Demo->ObjectiveText.IsEmpty())
	{
		const FString Line = FString(TEXT("OBJECTIF : ")) + Demo->ObjectiveText;
		float OW, OH; GetTextSize(Line, OW, OH, GEngine->GetMediumFont(), 1.f);
		DrawRect(FLinearColor(0.02f, 0.04f, 0.07f, 0.72f), 14.f, 12.f, OW + 24.f, OH + 12.f);
		DrawRect(FLinearColor(0.95f, 0.78f, 0.25f, 0.9f), 14.f, 12.f, 4.f, OH + 12.f);
		DrawText(Line, FLinearColor(1.f, 0.92f, 0.6f, 1.f), 26.f, 18.f, GEngine->GetMediumFont(), 1.f);
	}
}

void AWOTOLDemoHUD::DrawBossBar(float W, float H, AWOTOLDemoUnit* Boss)
{
	if (!Boss || !Boss->IsAlive()) return;

	const float Pct = Boss->GetEffectiveHealthPercent();
	const int32 MaxHP = Boss->GetEffectiveMaxHealth();
	const int32 CurHP = FMath::RoundToInt(Pct * MaxHP);

	const float BarW = 520.f, BarH = 26.f;
	const float BX = (W - BarW) * 0.5f, BY = 56.f;

	// Liseré "boss" violet/cyan
	DrawRect(FLinearColor(0.20f, 0.85f, 1.f, 0.9f), BX - 4, BY - 4, BarW + 8, 3.f);
	DrawBar(BX, BY, BarW, BarH, Pct,
		FLinearColor(0.62f, 0.12f, 0.85f, 1.f), FLinearColor(0.10f, 0.05f, 0.14f, 0.92f));

	const FString Name = TEXT("KRAKEN");
	float NW, NH; GetTextSize(Name, NW, NH, GEngine->GetLargeFont(), 1.2f);
	DrawText(Name, FLinearColor(0.95f, 0.85f, 1.f, 1.f), (W - NW) * 0.5f, BY - 22.f, GEngine->GetLargeFont(), 1.2f);

	const FString HP = FString::Printf(TEXT("%d / %d"), CurHP, MaxHP);
	float HW, HH; GetTextSize(HP, HW, HH, GEngine->GetMediumFont(), 1.f);
	DrawText(HP, FLinearColor::White, (W - HW) * 0.5f, BY + 4.f, GEngine->GetMediumFont(), 1.f);
}

void AWOTOLDemoHUD::DrawCommandBar(float W, float H, UWorld* World)
{
	if (!World) return;
	UUnitSelectionManager* Sel = World->GetSubsystem<UUnitSelectionManager>();
	if (!Sel) return;

	// Agrège les unités sélectionnées par nom (groupe)
	struct FGroup { FString Name; int32 Count = 0; float HpSum = 0.f; EFactionID Fac = EFactionID::None; };
	TArray<FGroup> Groups;
	TMap<FString, int32> Index;
	for (AUnitBase* U : Sel->GetSelectedUnits())
	{
		if (!U || !U->IsAlive()) continue;
		const FString Name = (U->GetUnitData() && !U->GetUnitData()->DisplayName.IsEmpty())
			? U->GetUnitData()->DisplayName.ToString() : U->GetName();
		int32* Found = Index.Find(Name);
		FGroup& G = Found ? Groups[*Found]
			: Groups[Index.Add(Name, Groups.Add(FGroup{ Name, 0, 0.f, U->GetFaction() }))];
		G.Count++;
		G.HpSum += U->GetHealthPercent();
	}
	if (Groups.Num() == 0) return;

	// Bandeau de fond plein largeur
	const float BandH = 96.f;
	const float BandY = H - BandH;
	DrawRect(FLinearColor(0.02f, 0.03f, 0.05f, 0.72f), 0.f, BandY, W, BandH);
	DrawRect(FLinearColor(0.30f, 0.62f, 1.f, 0.7f), 0.f, BandY, W, 3.f); // liseré haut

	// Cartes d'unités
	const float CardW = 158.f, CardH = 76.f, Gap = 10.f;
	float X = 16.f;
	const float Y = BandY + (BandH - CardH) * 0.5f;
	for (const FGroup& G : Groups)
	{
		const FLinearColor Fac = FFactionColors::Get(G.Fac);
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), X, Y, CardW, CardH);
		DrawRect(Fac, X, Y, CardW, 4.f);                       // liseré couleur de faction
		// Icône (carré teinté)
		DrawRect(Fac * 0.7f, X + 8.f, Y + 12.f, 40.f, 40.f);
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.5f), X + 8.f, Y + 12.f, 40.f, 4.f);
		// Nom + nombre
		DrawText(G.Name, FLinearColor::White, X + 56.f, Y + 10.f, GEngine->GetMediumFont(), 1.f);
		DrawText(FString::Printf(TEXT("x%d"), G.Count), FLinearColor(1.f, 0.9f, 0.5f, 1.f),
			X + 56.f, Y + 30.f, GEngine->GetMediumFont(), 1.2f);
		// Mini-barre de vie moyenne du groupe
		const float AvgHp = (G.Count > 0) ? G.HpSum / G.Count : 0.f;
		const FLinearColor HpCol = FMath::Lerp(FLinearColor(0.8f, 0.1f, 0.1f, 1.f),
			FLinearColor(0.2f, 0.85f, 0.2f, 1.f), AvgHp);
		DrawBar(X + 8.f, Y + CardH - 14.f, CardW - 16.f, 8.f, AvgHp, HpCol,
			FLinearColor(0.12f, 0.12f, 0.12f, 0.9f));

		X += CardW + Gap;
		if (X > W - CardW - 70.f) break; // garde la place pour le bouton pause
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
