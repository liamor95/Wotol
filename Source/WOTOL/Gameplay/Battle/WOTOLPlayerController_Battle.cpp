#include "WOTOLPlayerController_Battle.h"
#include "UnitSelectionManager.h"
#include "WOTOLBattleCamera.h"
#include "Gameplay/Units/UnitBase.h"
#include "Gameplay/Units/UnitAIStateComponent.h"
#include "Gameplay/AI/AIAdaptiveController.h"
#include "Gameplay/Demo/WOTOLDemoHUD.h"
#include "Gameplay/Demo/DemoFlowSubsystem.h"
#include "Gameplay/Demo/WOTOLDemoDirector.h"
#include "Gameplay/Demo/WOTOLDemoUnit.h"
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
				if (AWOTOLDemoDirector* Dir = GetDemoDirector()) Dir->RestartDemo(true);
			}
			else if (AWOTOLDemoHUD::SummaryChangeFactionButtonRect(VpSize.X, VpSize.Y).IsInside(M))
			{
				if (AWOTOLDemoDirector* Dir = GetDemoDirector()) Dir->RestartDemo(false);
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
	FVector    TargetLocation = FVector::ZeroVector;
	if (!TargetUnit)
	{
		GetGroundLocationUnderCursor(TargetLocation);
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
		return Cast<AUnitBase>(Hit.GetActor());
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
		for (AUnitBase* Unit : Sel)
		{
			if (!Unit || !Unit->IsAlive()) continue;
			if (AAIAdaptiveController* AIC = Cast<AAIAdaptiveController>(Unit->GetController()))
			{
				// Décalage conservé autour de la cible -> elles encerclent au lieu de s'empiler.
				FVector Offset = Unit->GetActorLocation() - Centroid; Offset.Z = 0.f;
				AIC->ActivateRTSBehavior();
				AIC->IssueOrder_AttackMove(TargetLoc + Offset.GetClampedToMaxSize(400.f));
			}
			if (UUnitAIStateComponent* St = Unit->FindComponentByClass<UUnitAIStateComponent>())
				St->SightRange = 60000.f;
		}
		return;
	}

	// ── Déplacement en CONSERVANT LA FORMATION ──
	// Chaque unité garde son décalage par rapport au centre du groupe ; la ligne
	// entière se déplace au point cliqué sans se disperser ni se déformer.
	FVector Centroid = FVector::ZeroVector;
	int32 Count = 0;
	for (AUnitBase* Unit : Sel)
	{
		if (!Unit || !Unit->IsAlive()) continue;
		Centroid += Unit->GetActorLocation();
		++Count;
	}
	if (Count == 0) return;
	Centroid /= Count;

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

	for (AUnitBase* Unit : Sel)
	{
		if (!Unit || !Unit->IsAlive()) continue;
		AAIAdaptiveController* AIC = Cast<AAIAdaptiveController>(Unit->GetController());
		if (!AIC) continue;

		// Décalage horizontal conservé ; hauteur (couche verticale) inchangée.
		FVector Offset = Unit->GetActorLocation() - Centroid;
		Offset.Z = 0.f;
		FVector Dest(TargetLocation.X + Offset.X, TargetLocation.Y + Offset.Y,
			Unit->GetActorLocation().Z);
		if (bClamp) Dest.X = FMath::Min(Dest.X, BoundaryX); // pas au-delà de sa zone
		// En PRÉPARATION : simple placement (pas de combat). EN BATAILLE : attack-move
		// -> l'unité se repositionne MAIS continue d'engager l'ennemi (reste autonome et
		// efficace, ne devient pas passive après un ordre de déplacement).
		if (bClamp)
		{
			AIC->IssueOrder_Move(Dest);
		}
		else
		{
			AIC->ActivateRTSBehavior();
			AIC->IssueOrder_AttackMove(Dest);
			if (UUnitAIStateComponent* St = Unit->FindComponentByClass<UUnitAIStateComponent>())
				St->SightRange = 60000.f;
		}
	}
}
