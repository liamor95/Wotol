#include "WOTOLPlayerController_Battle.h"
#include "UnitSelectionManager.h"
#include "WOTOLBattleCamera.h"
#include "Gameplay/Units/UnitBase.h"
#include "Gameplay/Units/UnitDataAsset.h"
#include "Gameplay/Units/UnitAIStateComponent.h"
#include "Gameplay/AI/AIAdaptiveController.h"
#include "Gameplay/Demo/WOTOLDemoHUD.h"
#include "Gameplay/Demo/DemoFlowSubsystem.h"
#include "Gameplay/Demo/WOTOLDemoDirector.h"
#include "Gameplay/Demo/WOTOLDemoUnit.h"
#include "Gameplay/Demo/WOTOLCoverStructure.h"
#include "Core/WOTOLGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"

AWOTOLPlayerController_Battle::AWOTOLPlayerController_Battle()
{
	bShowMouseCursor    = true;
	bEnableClickEvents  = true;
	bEnableMouseOverEvents = true;
}

void AWOTOLPlayerController_Battle::BeginPlay()
{
	Super::BeginPlay();

	// Récupérer la faction depuis le GameInstance
	if (UWOTOLGameInstance* GI = Cast<UWOTOLGameInstance>(GetGameInstance()))
	{
		PlayerFaction = GI->GetSelectedFaction();
	}

	// La caméra est spawnée par le GameMode et placée dans le niveau.
	// Le GameMode appellera SetBattleCamera() juste avant BeginPlay.
}

void AWOTOLPlayerController_Battle::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindAction("LeftMouseButton",  IE_Pressed,  this,
		&AWOTOLPlayerController_Battle::OnLeftMousePressed);
	InputComponent->BindAction("LeftMouseButton",  IE_Released, this,
		&AWOTOLPlayerController_Battle::OnLeftMouseReleased);
	InputComponent->BindAction("RightMouseButton", IE_Pressed,  this,
		&AWOTOLPlayerController_Battle::OnRightMousePressed);
	InputComponent->BindAction("SelectAll", IE_Pressed, this,
		&AWOTOLPlayerController_Battle::OnSelectAll);

	// Bindings directs (fonctionnent SANS config Input du projet — démo jouable out-of-the-box)
	// Clic gauche : bExecuteWhenPaused pour pouvoir cliquer le menu pause.
	{
		FInputKeyBinding& BLM = InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this,
			&AWOTOLPlayerController_Battle::OnLeftMousePressed);
		BLM.bExecuteWhenPaused = true;
	}
	InputComponent->BindKey(EKeys::LeftMouseButton,  IE_Released, this,
		&AWOTOLPlayerController_Battle::OnLeftMouseReleased);
	// Clic droit : NE PAS consommer l'événement -> la caméra (pawn possédé) le reçoit
	// aussi pour tourner (rotation au clic droit + glisser).
	{
		FInputKeyBinding& RP = InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this,
			&AWOTOLPlayerController_Battle::OnRightMousePressed);
		RP.bConsumeInput = false;
		FInputKeyBinding& RR = InputComponent->BindKey(EKeys::RightMouseButton, IE_Released, this,
			&AWOTOLPlayerController_Battle::OnRightMouseReleased);
		RR.bConsumeInput = false;
	}
	InputComponent->BindKey(EKeys::LeftControl,      IE_Pressed,  this,
		&AWOTOLPlayerController_Battle::OnSelectAll);

	// Pause : Échap ou P. bExecuteWhenPaused = ces bindings marchent même en pause.
	{
		FInputKeyBinding& B1 = InputComponent->BindKey(EKeys::Escape, IE_Pressed, this,
			&AWOTOLPlayerController_Battle::TogglePause);
		B1.bExecuteWhenPaused = true;
		FInputKeyBinding& B2 = InputComponent->BindKey(EKeys::P, IE_Pressed, this,
			&AWOTOLPlayerController_Battle::TogglePause);
		B2.bExecuteWhenPaused = true;
	}
}

bool AWOTOLPlayerController_Battle::GetViewportSizeSafe(FVector2D& Out) const
{
	if (UWorld* W = GetWorld())
	{
		if (UGameViewportClient* VP = W->GetGameViewport())
		{
			VP->GetViewportSize(Out);
			return !Out.IsNearlyZero();
		}
	}
	return false;
}

void AWOTOLPlayerController_Battle::TogglePause()
{
	const bool bNowPaused = !UGameplayStatics::IsGamePaused(GetWorld());
	UGameplayStatics::SetGamePaused(GetWorld(), bNowPaused);
}

AWOTOLDemoDirector* AWOTOLPlayerController_Battle::GetDemoDirector() const
{
	for (TActorIterator<AWOTOLDemoDirector> It(GetWorld()); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

void AWOTOLPlayerController_Battle::ChangeLayerForSelection(float DeltaZ)
{
	UUnitSelectionManager* SelectionMgr = GetSelectionManager();
	if (!SelectionMgr) return;
	for (AUnitBase* U : SelectionMgr->GetSelectedUnits())
	{
		if (!U) continue;
		// SÉCURITÉ : on ne change JAMAIS la hauteur d'une unité ennemie, même si elle a
		// été prise par erreur dans la boîte de sélection. Le joueur ne pilote que SES unités.
		if (U->GetFaction() != PlayerFaction) continue;
		if (AWOTOLDemoUnit* DU = Cast<AWOTOLDemoUnit>(U))
		{
			// Couche visuelle : décalage 0 (fond) .. 2400 (haut). Contrôle joueur = INSTANTANÉ.
			const float NewZ = FMath::Clamp(DU->GetDesiredZ() + DeltaZ, 0.f, 2400.f);
			DU->SetDesiredZ(NewZ);
			// Un changement de couche manuel compte comme un ordre -> l'IA tactique ne
			// le réécrasera pas tout de suite.
			DU->LastPlayerOrderTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
		}
	}
}

void AWOTOLPlayerController_Battle::PickFactionAndPrepare(EFactionID Faction)
{
	// Source fiable = le subsystem (toujours présent), pas seulement le GameInstance.
	if (UDemoFlowSubsystem* Demo = GetGameInstance()
			? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr)
	{
		Demo->SetSelectedFaction(Faction);
	}
	if (UWOTOLGameInstance* GI = Cast<UWOTOLGameInstance>(GetGameInstance()))
	{
		GI->SessionConfig.SelectedFaction = Faction;
	}
	SetPlayerFaction(Faction);
	if (AWOTOLDemoDirector* Dir = GetDemoDirector())
	{
		Dir->BeginPreparation();
	}
}

bool AWOTOLPlayerController_Battle::HandleUIClick()
{
	FVector2D VpSize;
	if (!GetViewportSizeSafe(VpSize)) return false;

	float MX, MY;
	if (!GetMousePosition(MX, MY)) return false;
	const FVector2D M(MX, MY);
	const bool bPaused = UGameplayStatics::IsGamePaused(GetWorld());

	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	const EDemoScreen Screen = Demo ? Demo->GetScreen() : EDemoScreen::Playing;

	// ── Menu principal ──
	if (Screen == EDemoScreen::MainMenu)
	{
		if (Demo && AWOTOLDemoHUD::StartGameButtonRect(VpSize.X, VpSize.Y).IsInside(M))
		{
			Demo->SetScreen(EDemoScreen::FactionSelect);
		}
		return true; // tout clic est consommé par le menu
	}

	// ── Choix de faction ──
	if (Screen == EDemoScreen::FactionSelect)
	{
		// Choix de la DIFFICULTÉ (ne lance pas la démo, juste mémorisé) — vérifié en premier.
		bool bDiffClicked = false;
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UDemoFlowSubsystem* Demo = GI->GetSubsystem<UDemoFlowSubsystem>())
			{
				const EDemoDifficulty DVals[3] = { EDemoDifficulty::Facile, EDemoDifficulty::Normal, EDemoDifficulty::Difficile };
				for (int32 i = 0; i < 3; ++i)
				{
					if (AWOTOLDemoHUD::DifficultyButtonRect(i, VpSize.X, VpSize.Y).IsInside(M))
					{
						Demo->SetDifficulty(DVals[i]);
						bDiffClicked = true;
						break;
					}
				}
			}
		}
		if (bDiffClicked) { return true; }

		if (AWOTOLDemoHUD::FactionButtonRect(0, VpSize.X, VpSize.Y).IsInside(M))
		{
			PickFactionAndPrepare(EFactionID::Aquiloris);
		}
		else if (AWOTOLDemoHUD::FactionButtonRect(1, VpSize.X, VpSize.Y).IsInside(M))
		{
			PickFactionAndPrepare(EFactionID::Noxeens);
		}
		return true;
	}

	// ── Écran de RÉSUMÉ de bataille ──
	if (Screen == EDemoScreen::Summary)
	{
		if (Demo && Demo->bSummaryIsFinal)
		{
			if (AWOTOLDemoHUD::SummaryReplayButtonRect(VpSize.X, VpSize.Y).IsInside(M))
			{
				// REJOUER = relance la phase qu'on vient de terminer/perdre (pas toute la démo).
				if (AWOTOLDemoDirector* Dir = GetDemoDirector()) Dir->ReplayCurrentPhase();
			}
			else if (AWOTOLDemoHUD::SummaryChangeFactionButtonRect(VpSize.X, VpSize.Y).IsInside(M))
			{
				if (AWOTOLDemoDirector* Dir = GetDemoDirector()) Dir->RestartDemo(false);
			}
			else if (AWOTOLDemoHUD::SummaryMenuButtonRect(VpSize.X, VpSize.Y).IsInside(M))
			{
				if (AWOTOLDemoDirector* Dir = GetDemoDirector()) Dir->ReturnToMainMenu();
			}
			else if (AWOTOLDemoHUD::SummaryQuitButtonRect(VpSize.X, VpSize.Y).IsInside(M))
			{
				UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
			}
		}
		else
		{
			if (AWOTOLDemoHUD::SummaryContinueButtonRect(VpSize.X, VpSize.Y).IsInside(M))
			{
				if (AWOTOLDemoDirector* Dir = GetDemoDirector()) Dir->ShowInterlude();
			}
		}
		return true; // tout clic est consommé par l'écran de résumé
	}

	// ── Écran de TRANSITION narrative (hors-champ) ──
	if (Screen == EDemoScreen::Interlude)
	{
		if (AWOTOLDemoHUD::InterludeContinueButtonRect(VpSize.X, VpSize.Y).IsInside(M))
		{
			if (AWOTOLDemoDirector* Dir = GetDemoDirector()) Dir->ContinueToPhase2();
		}
		return true;
	}

	// ── Préparation : bouton "Lancer la bataille" ──
	if (Screen == EDemoScreen::Prepare
		&& AWOTOLDemoHUD::LaunchBattleButtonRect(VpSize.X, VpSize.Y).IsInside(M))
	{
		if (AWOTOLDemoDirector* Dir = GetDemoDirector())
		{
			Dir->StartBattleNow();
		}
		return true;
	}

	// Boutons de couche verticale (nage) : montent/descendent la sélection
	if (AWOTOLDemoHUD::LayerUpButtonRect(VpSize.X, VpSize.Y).IsInside(M))
	{
		ChangeLayerForSelection(+600.f);
		return true;
	}
	if (AWOTOLDemoHUD::LayerDownButtonRect(VpSize.X, VpSize.Y).IsInside(M))
	{
		ChangeLayerForSelection(-600.f);
		return true;
	}

	// Bouton pause (toujours actif en préparation / jeu)
	if (AWOTOLDemoHUD::PauseButtonRect(VpSize.X, VpSize.Y).IsInside(M))
	{
		TogglePause();
		return true;
	}

	if (!bPaused) return false;

	// Boutons du menu pause
	if (AWOTOLDemoHUD::MenuButtonRect(0, VpSize.X, VpSize.Y).IsInside(M)) // Reprendre
	{
		UGameplayStatics::SetGamePaused(GetWorld(), false);
		return true;
	}
	if (AWOTOLDemoHUD::MenuButtonRect(1, VpSize.X, VpSize.Y).IsInside(M)) // Recommencer
	{
		UGameplayStatics::SetGamePaused(GetWorld(), false);
		UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetName()));
		return true;
	}
	if (AWOTOLDemoHUD::MenuButtonRect(2, VpSize.X, VpSize.Y).IsInside(M)) // Quitter
	{
		UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
		return true;
	}
	return true; // en pause : tout clic est consommé par le menu
}

bool AWOTOLPlayerController_Battle::HandleCommandBarClick(bool bDoubleClick)
{
	UUnitSelectionManager* Sel = GetSelectionManager();
	if (!Sel || !Sel->HasSelection()) return false;

	FVector2D Vp;
	if (!GetViewportSizeSafe(Vp)) return false;
	float MX, MY;
	if (!GetMousePosition(MX, MY)) return false;
	const FVector2D M(MX, MY);

	// Reconstitue l'ordre des groupes EXACTEMENT comme la barre de commandement
	// (agrégation par nom, dans l'ordre d'itération de la sélection).
	TArray<FString> Order;
	TMap<FString, TArray<AUnitBase*>> ByName;
	for (AUnitBase* U : Sel->GetSelectedUnits())
	{
		if (!U || !U->IsAlive()) continue;
		const FString Name = (U->GetUnitData() && !U->GetUnitData()->DisplayName.IsEmpty())
			? U->GetUnitData()->DisplayName.ToString() : U->GetName();
		if (!ByName.Contains(Name)) Order.Add(Name);
		ByName.FindOrAdd(Name).Add(U);
	}
	if (Order.Num() == 0) return false;

	const int32 MaxFit = AWOTOLDemoHUD::CommandCardMaxFit(Vp.X);
	const int32 Shown = FMath::Min(Order.Num(), MaxFit);
	for (int32 i = 0; i < Shown; ++i)
	{
		if (!AWOTOLDemoHUD::CommandCardRect(i, Vp.X, Vp.Y).IsInside(M)) continue;

		// Clic sur cette carte.
		if (!bDoubleClick) return true; // simple clic : consommé, ne désélectionne pas.

		// DOUBLE clic : ne garde QUE ce groupe sélectionné + zoom caméra dessus,
		// comme un double-clic sur l'unité, mais sans avoir à la chercher au sol.
		const TArray<AUnitBase*>& Grp = ByName[Order[i]];
		Sel->ClearSelection();
		for (int32 g = 0; g < Grp.Num(); ++g)
		{
			if (!Grp[g]) continue;
			if (g == 0) Sel->SelectUnit(Grp[g], PlayerFaction);
			else        Sel->AddToSelection(Grp[g], PlayerFaction);
		}
		if (BattleCamera.IsValid())
		{
			FVector C = FVector::ZeroVector; int32 N = 0;
			for (AUnitBase* U : Grp) { if (U) { C += U->GetActorLocation(); ++N; } }
			if (N > 0) BattleCamera->FocusOnUnitClose(C / N, 1000.f);
		}
		return true;
	}
	return false;
}

void AWOTOLPlayerController_Battle::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bIsBoxSelecting)
	{
		float X, Y;
		GetMousePosition(X, Y);
		BoxSelectCurrent = FVector2D(X, Y);
	}
}

void AWOTOLPlayerController_Battle::SetBattleCamera(AWOTOLBattleCamera* Camera)
{
	BattleCamera = Camera;
	if (Camera)
	{
		Possess(Camera);
	}
}

void AWOTOLPlayerController_Battle::SetPlayerFaction(EFactionID Faction)
{
	PlayerFaction = Faction;
}

UUnitSelectionManager* AWOTOLPlayerController_Battle::GetSelectionManager() const
{
	return GetWorld()->GetSubsystem<UUnitSelectionManager>();
}

void AWOTOLPlayerController_Battle::OnLeftMousePressed()
{
	// Priorité à l'UI (bouton pause / menu). Si consommé, pas de sélection.
	if (HandleUIClick()) return;

	float X, Y;
	GetMousePosition(X, Y);
	const FVector2D Pos(X, Y);

	// ── Double-clic gauche sur une unité ALLIÉE = focus caméra dessus ──
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	const bool bDouble = (Now - LastLeftClickTime < 0.30f)
		&& FVector2D::Distance(Pos, LastLeftClickPos) < 14.f;
	LastLeftClickTime = Now;
	LastLeftClickPos  = Pos;

	// Clic sur la barre de commandement (bas-gauche) : simple clic consommé (pas de
	// désélection), double clic = sélectionne + zoome sur ce groupe d'unités.
	if (HandleCommandBarClick(bDouble)) return;

	if (bDouble)
	{
		if (AUnitBase* U = GetUnitUnderCursor())
		{
			if (U->GetFaction() == PlayerFaction && BattleCamera.IsValid())
			{
				// Zoom rapproché (~quelques mètres) centré sur l'unité
				BattleCamera->FocusOnUnitClose(U->GetActorLocation(), 800.f);
				return; // pas de nouvelle sélection sur le double-clic
			}
		}
	}

	BoxSelectStart   = Pos;
	bIsBoxSelecting  = true;
}

void AWOTOLPlayerController_Battle::OnLeftMouseReleased()
{
	if (!bIsBoxSelecting) return;
	bIsBoxSelecting = false;

	UUnitSelectionManager* SelectionMgr = GetSelectionManager();
	if (!SelectionMgr) return;

	const float DragDist = FVector2D::Distance(BoxSelectStart, BoxSelectCurrent);

	if (DragDist >= BoxSelectDragThreshold)
	{
		// Boîte de sélection
		const bool bAdditive = IsInputKeyDown(EKeys::LeftShift);
		if (!bAdditive) SelectionMgr->ClearSelection();
		SelectionMgr->BoxSelect(this, BoxSelectStart, BoxSelectCurrent, PlayerFaction);
	}
	else
	{
		// Clic simple
		AUnitBase* HitUnit = GetUnitUnderCursor();
		const bool bAdditive = IsInputKeyDown(EKeys::LeftShift);

		if (HitUnit)
		{
			if (bAdditive)
				SelectionMgr->AddToSelection(HitUnit, PlayerFaction);
			else
				SelectionMgr->SelectUnit(HitUnit, PlayerFaction);
		}
		else if (!bAdditive)
		{
			SelectionMgr->ClearSelection();
		}
	}
}

void AWOTOLPlayerController_Battle::OnRightMousePressed()
{
	// On NOTE seulement la position : on décidera au relâchement si c'était
	// un ordre (clic bref) ou une rotation caméra (glisser). La rotation est
	// gérée en parallèle par AWOTOLBattleCamera (clic droit maintenu).
	float X, Y;
	GetMousePosition(X, Y);
	RightPressPos = FVector2D(X, Y);
	bRightDown    = true;
}

void AWOTOLPlayerController_Battle::OnRightMouseReleased()
{
	if (!bRightDown) return;
	bRightDown = false;

	// Si la souris a bougé au-delà du seuil → c'était une rotation caméra : pas d'ordre.
	float X, Y;
	GetMousePosition(X, Y);
	if (FVector2D::Distance(RightPressPos, FVector2D(X, Y)) >= BoxSelectDragThreshold) return;

	// Clic droit BREF = ordre aux unités sélectionnées
	UUnitSelectionManager* SelectionMgr = GetSelectionManager();
	if (!SelectionMgr || !SelectionMgr->HasSelection()) return;

	AUnitBase* TargetUnit = GetUnitUnderCursor();
	// Clic droit sur une STRUCTURE de décor -> les unités l'attaquent jusqu'à destruction.
	if (!TargetUnit)
	{
		FHitResult Hit;
		if (GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(ECC_WorldStatic), true, Hit))
		{
			if (AWOTOLCoverStructure* Cover = Cast<AWOTOLCoverStructure>(Hit.GetActor()))
			{
				if (!Cover->bIndestructible && !Cover->IsDestroyed())
				{
					if (UUnitSelectionManager* Sel = GetSelectionManager())
						for (AUnitBase* U : Sel->GetSelectedUnits())
							if (AWOTOLDemoUnit* DU = Cast<AWOTOLDemoUnit>(U))
								DU->OrderAttackCover(Cover);
					return;
				}
			}
		}
	}
	FVector    TargetLocation = FVector::ZeroVector;
	if (!TargetUnit)
	{
		// Si on ne trouve AUCUN point de sol valide sous le curseur, on n'ordonne RIEN (plutôt
		// que d'envoyer les unités au centre de l'arène par défaut).
		if (!GetGroundLocationUnderCursor(TargetLocation)) return;
	}
	IssueCommandToSelection(TargetUnit, TargetLocation);
}

void AWOTOLPlayerController_Battle::OnSelectAll()
{
	if (UUnitSelectionManager* SelectionMgr = GetSelectionManager())
	{
		SelectionMgr->SelectAllOfFaction(PlayerFaction);
	}
}

AUnitBase* AWOTOLPlayerController_Battle::GetUnitUnderCursor() const
{
	FHitResult Hit;
	if (GetHitResultUnderCursorByChannel(
			UEngineTypes::ConvertToTraceType(ECC_Pawn), true, Hit))
	{
		if (AUnitBase* U = Cast<AUnitBase>(Hit.GetActor())) return U;
	}
	// SECOURS : le Kraken FLOTTE en hauteur (mesh visuel décalé de sa capsule) -> le trace Pawn
	// pouvait rater son corps -> le clic était traité comme un ordre de déplacement au lieu
	// d'une attaque. On retente sur la VISIBILITÉ et on accepte toute unité touchée.
	if (GetHitResultUnderCursorByChannel(
			UEngineTypes::ConvertToTraceType(ECC_Visibility), true, Hit))
	{
		if (AUnitBase* U = Cast<AUnitBase>(Hit.GetActor())) return U;
	}
	return nullptr;
}

bool AWOTOLPlayerController_Battle::GetGroundLocationUnderCursor(FVector& OutLocation) const
{
	FHitResult Hit;
	if (GetHitResultUnderCursorByChannel(
			UEngineTypes::ConvertToTraceType(ECC_WorldStatic), true, Hit))
	{
		OutLocation = Hit.Location;
		return true;
	}
	// SECOURS : le trace physique peut RATER (curseur au-dessus de l'eau/du vide, ou sol non
	// bloquant) -> avant, OutLocation restait (0,0,0) = CENTRE de l'arène et les unités
	// partaient droit vers le milieu/le camp adverse (« à l'opposé »). On intersecte donc le
	// rayon de la caméra avec un PLAN HORIZONTAL au niveau du sol -> point cliqué correct partout.
	FVector RayOrigin, RayDir;
	if (DeprojectMousePositionToWorld(RayOrigin, RayDir) && !FMath::IsNearlyZero(RayDir.Z))
	{
		const float PlaneZ = 100.f; // niveau du sol de l'arène (GroundZ des unités)
		const float T = (PlaneZ - RayOrigin.Z) / RayDir.Z;
		if (T > 0.f)
		{
			OutLocation = RayOrigin + RayDir * T;
			return true;
		}
	}
	return false;
}

void AWOTOLPlayerController_Battle::IssueCommandToSelection(
	AUnitBase* TargetUnit, FVector TargetLocation)
{
	UUnitSelectionManager* SelectionMgr = GetSelectionManager();
	if (!SelectionMgr) return;

	const TArray<AUnitBase*>& Sel = SelectionMgr->GetSelectedUnits();

	// ATTAQUE (clic droit sur un ennemi) : on envoie le groupe EN ATTACK-MOVE vers la
	// zone de la cible plutôt que de verrouiller TOUTES les unités sur une seule cible.
	// -> elles avancent en formation, engagent l'ennemi le plus proche et continuent après
	// (exactement comme l'IA autonome, qui était plus efficace que l'ancien "tout le monde
	// tape le même"). On garde donc l'efficacité de l'autonomie tout en dirigeant l'assaut.
	if (TargetUnit && TargetUnit->GetFaction() != PlayerFaction)
	{
		FVector Centroid = FVector::ZeroVector;
		int32 Cnt = 0;
		for (AUnitBase* Unit : Sel)
		{
			if (!Unit || !Unit->IsAlive()) continue;
			Centroid += Unit->GetActorLocation(); ++Cnt;
		}
		if (Cnt > 0) Centroid /= Cnt;
		const FVector TargetLoc = TargetUnit->GetActorLocation();
		// Couche VERTICALE de la cible : les unités qui peuvent nager montent/descendent à
		// SA hauteur pour l'attaquer là où elle se trouve (ex. Aquisphères en l'air), au lieu
		// de rester au sol sous elle. La cible précise est VERROUILLÉE (ForceTarget).
		float TargetLayer = 0.f;
		if (AWOTOLDemoUnit* TDU = Cast<AWOTOLDemoUnit>(TargetUnit)) TargetLayer = TDU->GetDesiredZ();
		for (AUnitBase* Unit : Sel)
		{
			if (!Unit || !Unit->IsAlive()) continue;
			if (AWOTOLDemoUnit* DU = Cast<AWOTOLDemoUnit>(Unit))
			{
				DU->OrderAttackCover(nullptr); // annule attaque décor + stamp
				// Monte/descend à la couche de la cible (si l'unité sait changer de couche).
				const bool bCanLayer = DU->GetUnitData() ? DU->GetUnitData()->Stats.bCanChangeLayer : true;
				if (bCanLayer) DU->SetDesiredZ(TargetLayer);
			}
			if (AAIAdaptiveController* AIC = Cast<AAIAdaptiveController>(Unit->GetController()))
			{
				// Décalage conservé autour de la cible -> elles encerclent au lieu de s'empiler.
				FVector Offset = Unit->GetActorLocation() - Centroid; Offset.Z = 0.f;
				AIC->ActivateRTSBehavior();
				AIC->IssueOrder_AttackMove(TargetLoc + Offset.GetClampedToMaxSize(400.f));
			}
			if (UUnitAIStateComponent* St = Unit->FindComponentByClass<UUnitAIStateComponent>())
			{
				St->SightRange = 60000.f;
				St->ForceTarget = TargetUnit; // VERROUILLE l'unité cliquée (pas "le plus proche")
			}
		}
		return;
	}

	// ── Déplacement avec REGROUPEMENT au point cliqué ──
	// On NE conserve PAS l'écartement d'origine (qui pouvait être très large et empêchait
	// les unités d'arriver au point) : on génère une formation COMPACTE centrée sur le point
	// cliqué, en grille serrée orientée dans le sens du déplacement. Les unités convergent
	// donc au point précis tout en gardant un espacement propre (elles ne s'empilent pas).
	TArray<AUnitBase*> Movers;
	FVector Centroid = FVector::ZeroVector;
	for (AUnitBase* Unit : Sel)
	{
		if (!Unit || !Unit->IsAlive()) continue;
		Movers.Add(Unit);
		Centroid += Unit->GetActorLocation();
	}
	const int32 Count = Movers.Num();
	if (Count == 0) return;
	Centroid /= Count;

	// Repère de la formation : Fwd = sens de marche (centre -> point cliqué).
	FVector Fwd = (TargetLocation - Centroid).GetSafeNormal2D();
	if (Fwd.IsNearlyZero()) Fwd = FVector(1.f, 0.f, 0.f);
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Fwd).GetSafeNormal();
	const float Spacing = 165.f;
	const int32 Cols = FMath::Max(1, FMath::CeilToInt(FMath::Sqrt((float)Count)));

	// Génère les emplacements (rangées derrière le point, centrées latéralement).
	TArray<FVector> Slots; Slots.Reserve(Count);
	for (int32 i = 0; i < Count; ++i)
	{
		const int32 Row = i / Cols;
		const int32 ColIdx = i % Cols;
		const int32 RowCount = FMath::Min(Cols, Count - Row * Cols);
		const float LateralX = (ColIdx - (RowCount - 1) * 0.5f) * Spacing;
		const float BackY = -(float)Row * Spacing; // rangées vers l'arrière
		Slots.Add(TargetLocation + Right * LateralX + Fwd * BackY);
	}

	// Affectation gloutonne : chaque emplacement prend l'unité NON assignée la plus proche
	// (limite les croisements, chacun va au slot le plus naturel).
	TArray<bool> Used; Used.Init(false, Count);
	TArray<int32> SlotOfUnit; SlotOfUnit.Init(-1, Count);
	for (int32 s = 0; s < Slots.Num(); ++s)
	{
		int32 Best = -1; float BestD = TNumericLimits<float>::Max();
		for (int32 u = 0; u < Count; ++u)
		{
			if (Used[u]) continue;
			const float D = FVector::DistSquared2D(Movers[u]->GetActorLocation(), Slots[s]);
			if (D < BestD) { BestD = D; Best = u; }
		}
		if (Best >= 0) { Used[Best] = true; SlotOfUnit[Best] = s; }
	}

	// En PRÉPARATION : on ne peut pas placer au-delà de sa zone (premier tiers).
	bool bClamp = false;
	float BoundaryX = 0.f;
	if (UDemoFlowSubsystem* Demo = GetGameInstance()
			? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr)
	{
		if (Demo->GetScreen() == EDemoScreen::Prepare)
		{
			if (AWOTOLDemoDirector* Dir = GetDemoDirector())
			{
				bClamp = true;
				BoundaryX = Dir->GetPlacementBoundaryWorldX();
			}
		}
	}

	for (int32 u = 0; u < Count; ++u)
	{
		AUnitBase* Unit = Movers[u];
		if (AWOTOLDemoUnit* DU = Cast<AWOTOLDemoUnit>(Unit)) DU->OrderAttackCover(nullptr); // annule attaque décor + stamp
		// Ordre de DÉPLACEMENT pur -> on lève le verrouillage de cible (sinon l'unité
		// repartirait attaquer l'ennemi verrouillé au lieu d'aller au point demandé).
		if (UUnitAIStateComponent* St = Unit->FindComponentByClass<UUnitAIStateComponent>())
			St->ForceTarget = nullptr;
		AAIAdaptiveController* AIC = Cast<AAIAdaptiveController>(Unit->GetController());
		if (!AIC) continue;

		// Emplacement compact assigné dans la formation regroupée ; hauteur inchangée.
		const int32 s = SlotOfUnit[u];
		const FVector Slot = (s >= 0) ? Slots[s] : TargetLocation;
		FVector Dest(Slot.X, Slot.Y, Unit->GetActorLocation().Z);
		if (bClamp) Dest.X = FMath::Min(Dest.X, BoundaryX); // pas au-delà de sa zone
		// DÉPLACEMENT PUR : l'unité va DIRECTEMENT au point demandé, sans s'arrêter pour
		// engager l'ennemi (l'ordre du joueur PRIME sur le comportement auto). Elle y va
		// même en prenant des dégâts. Pour attaquer, clic droit sur un ENNEMI.
		AIC->IssueOrder_Move(Dest);
	}
}
