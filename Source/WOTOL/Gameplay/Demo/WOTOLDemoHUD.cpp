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

void AWOTOLDemoHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas) return;

	const float W = Canvas->SizeX;
	const float H = Canvas->SizeY;
	UWorld* World = GetWorld();

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

	// ─── 2) Timer de bataille (haut centre) ──────────────────────────────────
	if (World)
	{
		if (URTSBattleManager* RTS = World->GetSubsystem<URTSBattleManager>())
		{
			const FString T = RTS->GetFormattedTime().ToString();
			float TW, TH; GetTextSize(T, TW, TH, GEngine->GetLargeFont(), 1.4f);
			DrawText(T, FLinearColor(0,0,0,0.7f), (W - TW)*0.5f + 2.f, 10.f, GEngine->GetLargeFont(), 1.4f);
			DrawText(T, FLinearColor::White, (W - TW)*0.5f, 8.f, GEngine->GetLargeFont(), 1.4f);
		}
	}

	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;

	// ─── 3) Barre de vie du BOSS (kraken), haut, sous le timer ──────────────
	if (Demo)
	{
		if (AWOTOLDemoUnit* Boss = Cast<AWOTOLDemoUnit>(Demo->GetBoss()))
		{
			if (Boss->IsAlive())
			{
				const float Pct = FMath::Clamp(Boss->GetHealthPercent(), 0.f, 1.f);
				const int32 MaxHP = Boss->GetEffectiveMaxHealth();
				const int32 CurHP = FMath::RoundToInt(Pct * MaxHP);

				const float BarW = 420.f, BarH = 22.f;
				const float BX = (W - BarW) * 0.5f;
				const float BY = 50.f;
				DrawRect(FLinearColor(0.05f, 0.05f, 0.05f, 0.85f), BX - 2, BY - 2, BarW + 4, BarH + 4);
				DrawRect(FLinearColor(0.15f, 0.15f, 0.15f, 0.9f), BX, BY, BarW, BarH);
				DrawRect(FLinearColor(0.7f, 0.1f, 0.85f, 1.f), BX, BY, BarW * Pct, BarH); // violet kraken

				const FString Label = FString::Printf(TEXT("KRAKEN   %d / %d"), CurHP, MaxHP);
				float LW, LH; GetTextSize(Label, LW, LH, GEngine->GetLargeFont(), 1.f);
				DrawText(Label, FLinearColor::White, (W - LW)*0.5f, BY + 2.f, GEngine->GetLargeFont(), 1.f);
			}
		}

		// ─── 4) Message d'objectif + écran de fin ───────────────────────────
		if (!Demo->CurrentMessage.IsEmpty())
		{
			DrawCenteredText(Demo->CurrentMessage, H * 0.14f, FLinearColor::White, 1.2f);
		}

		if (Demo->GetPhase() == EDemoPhase::DemoEnd)
		{
			DrawCenteredText(TEXT("— FIN DE LA DEMO —"), H * 0.42f,
				FLinearColor(1.f, 0.85f, 0.2f, 1.f), 2.2f);
			DrawCenteredText(TEXT("Le conflit Aquiloris / Noxeens ne fait que commencer..."),
				H * 0.50f, FLinearColor::White, 1.1f);
		}
	}

	// ─── 5) Unités sélectionnées (bas de l'écran), groupées par type ─────────
	if (World)
	{
		if (UUnitSelectionManager* Sel = World->GetSubsystem<UUnitSelectionManager>())
		{
			TMap<FString, int32> Counts;
			for (AUnitBase* U : Sel->GetSelectedUnits())
			{
				if (!U || !U->IsAlive()) continue;
				const FString Name = (U->GetUnitData() && !U->GetUnitData()->DisplayName.IsEmpty())
					? U->GetUnitData()->DisplayName.ToString() : U->GetName();
				Counts.FindOrAdd(Name)++;
			}

			float X = 20.f;
			const float Y = H - 78.f;
			for (const TPair<FString, int32>& P : Counts)
			{
				const float BoxW = 150.f, BoxH = 58.f;
				DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.55f), X, Y, BoxW, BoxH);
				DrawRect(FLinearColor(0.3f, 0.6f, 1.f, 0.9f), X, Y, BoxW, 4.f); // liseré bleu
				const FString Line = FString::Printf(TEXT("%dx %s"), P.Value, *P.Key);
				DrawText(Line, FLinearColor::White, X + 8.f, Y + 20.f, GEngine->GetMediumFont(), 1.f);
				X += BoxW + 10.f;
				if (X > W - BoxW) break; // évite le débordement
			}
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
