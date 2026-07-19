#include "WOTOLPlayerController_Battle.h"
#include "UnitSelectionManager.h"
#include "WOTOLBattleCamera.h"
#include "Gameplay/Units/UnitBase.h"
#include "Gameplay/Units/UnitDataAsset.h"
#include "Gameplay/Units/UnitAIStateComponent.h"
#include "Gameplay/Units/AbilityComponent.h"
#include "Gameplay/Units/AbilityBase.h"
#include "Gameplay/AI/AIAdaptiveController.h"
#include "Gameplay/Demo/WOTOLDemoHUD.h"
#include "Gameplay/Demo/DemoFlowSubsystem.h"
#include "Gameplay/Demo/WOTOLDemoDirector.h"
#include "Gameplay/Demo/WOTOLDemoUnit.h"
#include "Gameplay/Demo/WOTOLCoverStructure.h"
#include "Gameplay/Demo/WOTOLCityBuildingProp.h"
#include "Core/WOTOLGameInstance.h"
#include "Core/FactionRegistrySubsystem.h"
#include "Components/SceneComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/GameUserSettings.h"
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

	// PAUSE « caméra libre » : le contrôleur continue de tourner (et de traiter les entrées)
	// même quand le jeu est en pause -> la caméra peut se déplacer alors que l'action est gelée.
	bShouldPerformFullTickWhenPaused = true;

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

	// Compétence de groupe : R active la capacité de chaque unité sélectionnée.
	InputComponent->BindKey(EKeys::R, IE_Pressed, this,
		&AWOTOLPlayerController_Battle::ActivateSelectionAbility);

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

void AWOTOLPlayerController_Battle::ApplyPauseState()
{
	// Le jeu est en pause si l'action est GELÉE (bouton pause) OU si le menu réglages est ouvert.
	UGameplayStatics::SetGamePaused(GetWorld(), bFrozen || bSettingsOpen);
}

void AWOTOLPlayerController_Battle::TogglePause()
{
	// Bouton pause = GEL de l'action (la caméra reste libre). Reprise = garde la position caméra.
	bFrozen = !bFrozen;
	ApplyPauseState();
}

void AWOTOLPlayerController_Battle::ToggleSettings()
{
	bSettingsOpen = !bSettingsOpen;
	if (!bSettingsOpen) bControlsOpen = false; // referme toujours sur le menu principal
	ApplyPauseState();
}

float AWOTOLPlayerController_Battle::GetMusicVolume() const
{
	if (AWOTOLDemoDirector* Dir = GetDemoDirector())
	{
		return Dir->GetMusicVolume();
	}
	return 0.7f;
}

void AWOTOLPlayerController_Battle::SetMusicVolume(float NewVolume)
{
	if (AWOTOLDemoDirector* Dir = GetDemoDirector())
	{
		Dir->SetMusicVolume(NewVolume);
	}
}

bool AWOTOLPlayerController_Battle::IsFullscreen() const
{
	if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		return Settings->GetFullscreenMode() != EWindowMode::Windowed;
	}
	return true;
}

void AWOTOLPlayerController_Battle::ToggleFullscreen()
{
	UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!Settings) return;

	const EWindowMode::Type NewMode = (Settings->GetFullscreenMode() == EWindowMode::Windowed)
		? EWindowMode::WindowedFullscreen : EWindowMode::Windowed;
	Settings->SetFullscreenMode(NewMode);
	Settings->ApplySettings(false);
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

void AWOTOLPlayerController_Battle::SelectFaction(EFactionID Faction)
{
	// La faction est seulement MÉMORISÉE ici. Le joueur peut encore changer la difficulté
	// et ses réglages ; rien ne démarre avant son clic explicite sur « Lancer la partie ».
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
}

bool AWOTOLPlayerController_Battle::HandleUIClick()
{
	FVector2D VpSize;
	if (!GetViewportSizeSafe(VpSize)) return false;

	float MX, MY;
	if (!GetMousePosition(MX, MY)) return false;
	const FVector2D M(MX, MY);

	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	const EDemoScreen Screen = Demo ? Demo->GetScreen() : EDemoScreen::Playing;

	// ── Fenêtre d'objectif MODALE (priorité absolue, sur tous les écrans) ──
	// Tant qu'elle est ouverte, elle capte TOUT clic : seul le bouton « Continuer »
	// la valide, le reste du clic est absorbé (l'action reste gelée).
	if (Demo && Demo->IsObjectiveWindowOpen())
	{
		if (AWOTOLDemoHUD::ObjectiveContinueButtonRect(VpSize.X, VpSize.Y).IsInside(M))
		{
			Demo->ConfirmObjectiveWindow();
		}
		return true;
	}

	// ── Exploration post-Kraken : inventaire -> cible lumineuse dans le monde ──
	if (Screen == EDemoScreen::Exploration)
	{
		if (AWOTOLDemoDirector* Dir = GetDemoDirector())
		{
			if (Dir->IsCrystalliserPlacementAvailable()
				&& AWOTOLDemoHUD::ExplorationCrystalliserButtonRect(VpSize.X, VpSize.Y).IsInside(M))
			{
				Dir->ArmCrystalliserPlacement();
				return true;
			}
			if (Dir->IsCrystalliserPlacementArmed())
			{
				FVector WorldPoint;
				if (GetWorldLocationOnHorizontalPlane(M,
					Dir->GetCrystalliserPlacementLocation().Z, WorldPoint))
				{
					Dir->TryPlaceCrystalliserAt(WorldPoint);
				}
				return true;
			}
		}
		return false; // hors placement, la nage et la caméra gardent les entrées
	}

	// ── Gestion du territoire : réparation, pose spatiale des tourelles, garnison ──
	if (Screen == EDemoScreen::Territory)
	{
		AWOTOLDemoDirector* Dir = GetDemoDirector();
		if (!Demo || !Dir) return true;
		if (AWOTOLDemoHUD::TerritoryRepairButtonRect(VpSize.X, VpSize.Y).IsInside(M))
		{
			Dir->RequestTerritoryRepair();
			return true;
		}
		if (AWOTOLDemoHUD::TerritoryDefenseButtonRect(VpSize.X, VpSize.Y).IsInside(M))
		{
			Dir->ArmDefensePlacement();
			return true;
		}
		const EDemoUnitCategory Cats[3] = {
			EDemoUnitCategory::Infanterie, EDemoUnitCategory::Montee,
			EDemoUnitCategory::Distance };
		for (int32 i = 0; i < 3; ++i)
		{
			if (AWOTOLDemoHUD::TerritoryGarrisonMinusRect(i, VpSize.X, VpSize.Y).IsInside(M))
			{
				Dir->RequestRemoveGarrisonByCategory(Cats[i]);
				return true;
			}
			if (AWOTOLDemoHUD::TerritoryGarrisonPlusRect(i, VpSize.X, VpSize.Y).IsInside(M))
			{
				Dir->RequestAssignGarrisonByCategory(Cats[i]);
				return true;
			}
		}
		if (AWOTOLDemoHUD::TerritoryReturnCityButtonRect(VpSize.X, VpSize.Y).IsInside(M))
		{
			Dir->ReturnToCityAfterTerritorySecured();
			return true;
		}
		if (Dir->IsDefensePlacementArmed())
		{
			float PlaneZ = 0.f;
			if (AActor* Building = Demo->GetCaptureObject())
				PlaneZ = Building->GetActorLocation().Z - 190.f;
			FVector WorldPoint;
			if (GetWorldLocationOnHorizontalPlane(M, PlaneZ, WorldPoint))
				Dir->TryPlaceDefenseAt(WorldPoint);
			return true;
		}
		return true;
	}

	// ── Vue CITÉ : cartes de production + bouton d'expédition ──
	if (Screen == EDemoScreen::City)
	{
		if (Demo)
		{
			if (Demo->GetProgress().bDefenseSystemInstalled
				&& !Demo->GetProgress().bMythicPlayable
				&& AWOTOLDemoHUD::CityFeedMythicButtonRect(VpSize.X, VpSize.Y).IsInside(M))
			{
				if (AWOTOLDemoDirector* Dir = GetDemoDirector()) Dir->FeedMythicAndContinue();
				return true;
			}
			if (Demo->IsCityBuildingPlacementArmed())
			{
				for (int32 Plot = 0; Plot < 3; ++Plot)
				{
					if (AWOTOLDemoHUD::CityBuildPlotRect(Plot, VpSize.X, VpSize.Y).IsInside(M))
					{
						if (Demo->ConstructRangedBuildingAtPlot(Plot))
						{
							Demo->SetObjective(TEXT("Batiment construit — produisez maintenant 10 unites a distance"));
						}
						return true;
					}
				}
			}
			for (int32 i = 0; i < AWOTOLDemoHUD::CityCardCount(); ++i)
			{
				const EDemoUnitCategory Cat = AWOTOLDemoHUD::CityCardCategory(i);
				// Bandeau HAUT de la carte = améliorer le bâtiment (niv. bâtiment -> niv. unités).
				if (AWOTOLDemoHUD::CityCardUpgradeRect(i, VpSize.X, VpSize.Y).IsInside(M))
				{
					Demo->SetSelectedCityCategory(Cat);
					if (Cat == EDemoUnitCategory::Distance && !Demo->IsRangedBuildingConstructed())
						Demo->ArmRangedBuildingPlacement();
					else
						Demo->UpgradeBuilding(Cat);
					return true;
				}
				// Reste de la carte = produire une unité.
				if (AWOTOLDemoHUD::CityCardRect(i, VpSize.X, VpSize.Y).IsInside(M))
				{
					Demo->SetSelectedCityCategory(Cat);
					if (Cat == EDemoUnitCategory::Distance && !Demo->IsRangedBuildingConstructed())
					{
						Demo->ArmRangedBuildingPlacement();
					}
					else if (Demo->ProduceUnit(Cat))
					{
						Demo->SetObjective(FString::Printf(TEXT("Produisez 10 unites a distance : %d / %d"),
							Demo->GetRangedProductionProgress(), Demo->RangedProductionTarget));
						if (Cat == EDemoUnitCategory::Distance
							&& Demo->IsRangedProductionObjectiveComplete())
						{
							if (AWOTOLDemoDirector* Dir = GetDemoDirector())
								Dir->NotifyRangedProductionObjectiveComplete();
						}
					}
					return true;
				}
			}
			if (AWOTOLDemoHUD::CitySkillsButtonRect(VpSize.X, VpSize.Y).IsInside(M))
			{
				Demo->SetScreen(EDemoScreen::Skills); // ouvre l'onglet compétences
				return true;
			}
			if (AWOTOLDemoHUD::CityDepartButtonRect(VpSize.X, VpSize.Y).IsInside(M))
			{
				// Part en expédition : le Director lance la défense du Cristalliseur (phase 10),
				// en déployant aussi les unités produites en cité (réserve).
				if (Demo->IsDefenseMissionReady())
				{
					if (AWOTOLDemoDirector* Dir = GetDemoDirector())
					{
						Dir->LaunchDefenseFromCity();
					}
				}
				return true;
			}

			// Aucun bouton d'UI touché : clic sur la maquette 3D isométrique elle-même ->
			// sélectionne le bâtiment visé (ouvre la fiche technique dans le HUD). Testé EN
			// DERNIER pour que les boutons 2D restent prioritaires même s'ils se superposent
			// visuellement à un bâtiment rendu derrière eux par la caméra isométrique.
			FHitResult CityHit;
			if (GetHitResultUnderCursorByChannel(
					UEngineTypes::ConvertToTraceType(ECC_WorldStatic), true, CityHit))
			{
				if (AWOTOLCityBuildingProp* Prop = Cast<AWOTOLCityBuildingProp>(CityHit.GetActor()))
				{
					Demo->SetSelectedCityCategory(Prop->Category);
					return true;
				}
			}
		}
		return true; // la cité capte tout clic
	}

	// ── Onglet COMPÉTENCES : choix de l'axe par unité + retour ──
	if (Screen == EDemoScreen::Skills)
	{
		if (Demo)
		{
			for (int32 i = 0; i < AWOTOLDemoHUD::CityCardCount(); ++i)
			{
				const EDemoUnitCategory Cat = AWOTOLDemoHUD::CityCardCategory(i);
				if (!Demo->IsCategoryUnlocked(Cat)) continue;
				for (int32 a = 0; a < 3; ++a)
				{
					if (AWOTOLDemoHUD::SkillsAxisRect(i, a, VpSize.X, VpSize.Y).IsInside(M))
					{
						Demo->SetUnitAxis(Cat, a);
						return true;
					}
				}
			}
			if (AWOTOLDemoHUD::SkillsBackButtonRect(VpSize.X, VpSize.Y).IsInside(M))
			{
				Demo->SetScreen(EDemoScreen::City);
			}
		}
		return true;
	}

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
			if (UDemoFlowSubsystem* DemoFlow = GI->GetSubsystem<UDemoFlowSubsystem>())
			{
				const EDemoDifficulty DVals[3] = { EDemoDifficulty::Facile, EDemoDifficulty::Normal, EDemoDifficulty::Difficile };
				for (int32 i = 0; i < 3; ++i)
				{
					if (AWOTOLDemoHUD::DifficultyButtonRect(i, VpSize.X, VpSize.Y).IsInside(M))
					{
						DemoFlow->SetDifficulty(DVals[i]);
						bDiffClicked = true;
						break;
					}
				}
			}
		}
		if (bDiffClicked) { return true; }

		if (AWOTOLDemoHUD::FactionButtonRect(0, VpSize.X, VpSize.Y).IsInside(M))
		{
			SelectFaction(EFactionID::Aquiloris);
		}
		else if (AWOTOLDemoHUD::FactionButtonRect(1, VpSize.X, VpSize.Y).IsInside(M))
		{
			SelectFaction(EFactionID::Noxeens);
		}
		else if (AWOTOLDemoHUD::FactionLaunchButtonRect(VpSize.X, VpSize.Y).IsInside(M)
			&& Demo && Demo->SelectedFaction != EFactionID::None)
		{
			// La faction/difficulté sont validées : on passe par la personnalisation du héros
			// avant de lancer réellement la démo (StartDemoAfterSelection).
			Demo->SetScreen(EDemoScreen::HeroCustomization);
		}
		return true;
	}

	// ── Personnalisation du héros (héritage / spécialité / portrait) ──
	if (Screen == EDemoScreen::HeroCustomization)
	{
		if (!Demo) return true;
		const EHeroHeritage HeritageVals[4] = { EHeroHeritage::Thalassi, EHeroHeritage::Givrelier, EHeroHeritage::Abysseen, EHeroHeritage::Gardien };
		for (int32 i = 0; i < 4; ++i)
		{
			if (AWOTOLDemoHUD::HeroHeritageButtonRect(i, VpSize.X, VpSize.Y).IsInside(M))
			{
				Demo->SetHeroHeritage(HeritageVals[i]);
				return true;
			}
		}
		const EHeroSpecialty SpecialtyVals[4] = { EHeroSpecialty::Thalassi, EHeroSpecialty::Guerrier, EHeroSpecialty::Mage, EHeroSpecialty::Inquisiteur };
		for (int32 i = 0; i < 4; ++i)
		{
			if (AWOTOLDemoHUD::HeroSpecialtyButtonRect(i, VpSize.X, VpSize.Y).IsInside(M))
			{
				Demo->SetHeroSpecialty(SpecialtyVals[i]);
				return true;
			}
		}
		if (AWOTOLDemoHUD::HeroPortraitPrevRect(VpSize.X, VpSize.Y).IsInside(M))
		{
			Demo->CycleHeroPortrait(-1);
		}
		else if (AWOTOLDemoHUD::HeroPortraitNextRect(VpSize.X, VpSize.Y).IsInside(M))
		{
			Demo->CycleHeroPortrait(1);
		}
		else if (AWOTOLDemoHUD::HeroCustomizationBackRect(VpSize.X, VpSize.Y).IsInside(M))
		{
			Demo->SetScreen(EDemoScreen::FactionSelect);
		}
		else if (AWOTOLDemoHUD::HeroCustomizationConfirmRect(VpSize.X, VpSize.Y).IsInside(M))
		{
			if (AWOTOLDemoDirector* Dir = GetDemoDirector()) Dir->StartDemoAfterSelection();
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
				if (AWOTOLDemoDirector* Dir = GetDemoDirector())
				{
					if (Demo->bSummaryCanReturnToCity) Dir->ReturnToCityAfterDefenseDefeat();
					else Dir->RestartDemo(false);
				}
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
				if (AWOTOLDemoDirector* Dir = GetDemoDirector()) Dir->ContinueFromBattleSummary();
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

	// ENGRENAGE (réglages) : ouvre/ferme le menu Reprendre / Recommencer / Quitter.
	if (AWOTOLDemoHUD::SettingsButtonRect(VpSize.X, VpSize.Y).IsInside(M))
	{
		ToggleSettings();
		return true;
	}

	// Bouton PAUSE = gel de l'action (icône pause <-> play). La caméra reste libre.
	if (AWOTOLDemoHUD::PauseButtonRect(VpSize.X, VpSize.Y).IsInside(M))
	{
		TogglePause();
		return true;
	}

	// Clic sur la MINIMAP : recentre la caméra sur le point monde correspondant (convention
	// du genre — Total War, Company of Heroes, Homeworld permettent tous de cliquer la
	// minimap pour s'y téléporter). Uniquement en bataille : c'est là qu'elle sert vraiment
	// à naviguer un champ plus grand que l'écran.
	if (Screen == EDemoScreen::Playing)
	{
		FVector MinimapWorldLoc;
		if (AWOTOLDemoHUD::MinimapScreenToWorld(M, VpSize.X, VpSize.Y, GetWorld(), MinimapWorldLoc))
		{
			if (BattleCamera.IsValid()) BattleCamera->FocusOn(MinimapWorldLoc);
			return true;
		}
	}

	// Menu RÉGLAGES ouvert : ses 3 boutons + les vrais réglages (volume, plein écran).
	if (bSettingsOpen)
	{
		// Sous-écran COMMANDES (liste des touches) : un seul bouton actif, RETOUR.
		if (bControlsOpen)
		{
			if (AWOTOLDemoHUD::ControlsBackButtonRect(VpSize.X, VpSize.Y).IsInside(M))
			{
				bControlsOpen = false;
			}
			return true; // tout le reste est consommé sans effet tant que les commandes s'affichent
		}
		if (AWOTOLDemoHUD::MenuButtonRect(3, VpSize.X, VpSize.Y).IsInside(M)) // Commandes
		{
			bControlsOpen = true;
			return true;
		}
		// Barre de volume musique : clic = fixe le niveau à la position horizontale cliquée.
		{
			const FBox2D VolRect = AWOTOLDemoHUD::MusicVolumeBarRect(VpSize.X, VpSize.Y);
			if (VolRect.IsInside(M))
			{
				const float NewVolume = (M.X - VolRect.Min.X) / FMath::Max(1.f, VolRect.Max.X - VolRect.Min.X);
				SetMusicVolume(FMath::Clamp(NewVolume, 0.f, 1.f));
				return true;
			}
		}
		if (AWOTOLDemoHUD::FullscreenToggleButtonRect(VpSize.X, VpSize.Y).IsInside(M))
		{
			ToggleFullscreen();
			return true;
		}
		if (AWOTOLDemoHUD::MenuButtonRect(0, VpSize.X, VpSize.Y).IsInside(M)) // Reprendre (ferme)
		{
			bSettingsOpen = false; ApplyPauseState();
			return true;
		}
		if (AWOTOLDemoHUD::MenuButtonRect(1, VpSize.X, VpSize.Y).IsInside(M)) // Recommencer
		{
			bSettingsOpen = false; UGameplayStatics::SetGamePaused(GetWorld(), false);
			UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetName()));
			return true;
		}
		if (AWOTOLDemoHUD::MenuButtonRect(2, VpSize.X, VpSize.Y).IsInside(M)) // Quitter
		{
			UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
			return true;
		}
		return true; // menu ouvert : tout clic est consommé par le menu
	}
	return false;
}

bool AWOTOLPlayerController_Battle::GetWorldLocationOnHorizontalPlane(
	const FVector2D& ScreenPosition, float PlaneZ, FVector& OutLocation) const
{
	FVector RayOrigin, RayDirection;
	if (!DeprojectScreenPositionToWorld(
		ScreenPosition.X, ScreenPosition.Y, RayOrigin, RayDirection)) return false;
	if (FMath::Abs(RayDirection.Z) < KINDA_SMALL_NUMBER) return false;
	const float T = (PlaneZ - RayOrigin.Z) / RayDirection.Z;
	if (T < 0.f) return false;
	OutLocation = RayOrigin + RayDirection * T;
	return true;
}

bool AWOTOLPlayerController_Battle::HandleCommandBarClick(bool bDoubleClick)
{
	UUnitSelectionManager* Sel = GetSelectionManager();
	if (!Sel) return false;

	FVector2D Vp;
	if (!GetViewportSizeSafe(Vp)) return false;
	float MX, MY;
	if (!GetMousePosition(MX, MY)) return false;
	const FVector2D M(MX, MY);

	// Groupes du roster = TOUTES les unités vivantes du joueur (même ordre que le HUD),
	// PAS seulement la sélection -> le roster reste affiché en entier et les index de
	// cartes correspondent au rendu.
	TArray<FString> Order;
	TMap<FString, TArray<AUnitBase*>> ByName;
	AWOTOLDemoHUD::BuildRosterGroups(GetWorld(), PlayerFaction, Order, ByName);
	if (Order.Num() == 0) return false;

	const int32 MaxFit = AWOTOLDemoHUD::CommandCardMaxFit(Vp.X);
	const int32 Shown = FMath::Min(Order.Num(), MaxFit);
	for (int32 i = 0; i < Shown; ++i)
	{
		if (!AWOTOLDemoHUD::CommandCardRect(i, Vp.X, Vp.Y).IsInside(M)) continue;

		// Clic sur cette carte.
		if (!bDoubleClick) return true; // simple clic : consommé, ne change rien.

		// DOUBLE clic : ce groupe devient la sélection ACTIVE (les autres restent
		// affichés dans le roster, juste grisés) + la caméra se recule et SUIT le groupe.
		const TArray<AUnitBase*>& Grp = ByName[Order[i]];
		Sel->ClearSelection();
		bool bFirst = true;
		for (AUnitBase* U : Grp)
		{
			if (!U || !U->IsAlive()) continue;
			if (bFirst) { Sel->SelectUnit(U, PlayerFaction); bFirst = false; }
			else        Sel->AddToSelection(U, PlayerFaction);
		}
		if (BattleCamera.IsValid())
		{
			// Recul « troisième personne » (~8 m) + suivi continu du groupe.
			BattleCamera->FollowGroup(Grp, 800.f);
		}
		return true;
	}
	return false;
}

void AWOTOLPlayerController_Battle::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// La fenêtre d'objectif MODALE gèle l'action (la caméra reste libre, comme la pause).
	// On synchronise chaque frame : ouverte -> pause ; fermée -> on rend la main à l'état
	// pause/réglages normal.
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UDemoFlowSubsystem* Demo = GI->GetSubsystem<UDemoFlowSubsystem>())
		{
			const bool bWin = Demo->IsObjectiveWindowOpen();
			if (bWin != bObjectivePausedLast)
			{
				bObjectivePausedLast = bWin;
				UGameplayStatics::SetGamePaused(GetWorld(), bWin || bFrozen || bSettingsOpen);
			}
		}
	}

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
				// Recul « troisième personne » + suivi continu de cette unité.
				BattleCamera->FollowGroup(TArray<AUnitBase*>{ U }, 800.f);
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
			// Clic sur le TERRAIN (aucune unité) : on lâche le ciblage/suivi caméra et
			// on revient à l'armée ENTIÈRE sélectionnée (tout le roster redevient actif).
			if (BattleCamera.IsValid()) BattleCamera->StopFollow();
			SelectionMgr->SelectAllOfFaction(PlayerFaction);
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
	// Shift + clic droit sur le SOL = ATTACK-MOVE (avancer en engageant l'ennemi rencontré).
	const bool bAttackMove = IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);
	IssueCommandToSelection(TargetUnit, TargetLocation, bAttackMove);
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
	if (GetHitResultUnderCursorByChannel(
			UEngineTypes::ConvertToTraceType(ECC_Visibility), true, Hit))
	{
		if (AUnitBase* U = Cast<AUnitBase>(Hit.GetActor())) return U;
	}

	// ── SÉLECTION PAR POSITION VISUELLE (robuste) ──
	// Les unités en HAUTEUR (ou le Kraken) ont leur capsule au SOL mais leur MODÈLE 3D en l'air
	// -> les traces physiques ratent le mesh flottant et le clic devenait un ordre de
	// deplacement. Ici on teste le RAYON de la camera contre le CENTRE VISUEL de chaque unite
	// -> cliquer sur le modele (meme haut) cible bien l'unite.
	FVector RayO, RayD;
	if (const_cast<AWOTOLPlayerController_Battle*>(this)->DeprojectMousePositionToWorld(RayO, RayD))
	{
		UWorld* W = GetWorld();
		UFactionRegistrySubsystem* Reg = W ? W->GetSubsystem<UFactionRegistrySubsystem>() : nullptr;
		if (Reg)
		{
			AUnitBase* Best = nullptr; float BestT = TNumericLimits<float>::Max();
			const EFactionID Facs[2] = { EFactionID::Aquiloris, EFactionID::Noxeens };
			for (EFactionID F : Facs)
				for (AUnitBase* U : Reg->GetUnitsForFaction(F))
				{
					if (!U || !U->IsAlive()) continue;
					USceneComponent* A = U->GetFloatingTextAnchor();
					const FVector C = A ? A->GetComponentLocation() : U->GetActorLocation();
					const float T = FVector::DotProduct(C - RayO, RayD);
					if (T < 0.f) continue;                          // derriere la camera
					const FVector Closest = RayO + RayD * T;
					// Rayon de selection = taille visuelle (boss plus large).
					const float PickR = (Cast<AWOTOLDemoUnit>(U) && Cast<AWOTOLDemoUnit>(U)->bIsBoss) ? 420.f : 150.f;
					if (FVector::Dist(Closest, C) <= PickR && T < BestT) { BestT = T; Best = U; }
				}
			if (Best) return Best;
		}
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
	AUnitBase* TargetUnit, FVector TargetLocation, bool bAttackMoveToGround)
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
		if (AWOTOLDemoUnit* TDU = Cast<AWOTOLDemoUnit>(TargetUnit))
		{
			TargetLayer = TDU->GetDesiredZ();
			// KRAKEN : sa capsule de déplacement est à la BASE, mais son corps (modèle 3D) monte
			// haut. On vise le CORPS -> les unités à courte portée GRIMPENT jusqu'à lui pour
			// pouvoir le toucher (au lieu de rester en bas sans l'atteindre). Celles qui ne
			// peuvent pas monter aussi haut s'arrêtent à leur couche max (clamp dans SetDesiredZ).
			if (TDU->bIsBoss || TDU->bCreatureBrain)
			{
				FVector BOri, BExt; TargetUnit->GetActorBounds(true, BOri, BExt);
				TargetLayer += BExt.Z * 0.7f;
			}
		}
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

		if (bAttackMoveToGround)
		{
			// ATTACK-MOVE (Shift+clic droit) : l'unité avance vers le point MAIS engage tout
			// ennemi rencontré en chemin (auto-ciblage du plus proche). Accessibilité « barrière
			// basse » : pas de micro, l'unité se défend et nettoie la route toute seule.
			if (UUnitAIStateComponent* St = Unit->FindComponentByClass<UUnitAIStateComponent>())
				St->SightRange = 60000.f; // voit large -> engage l'ennemi le plus proche en route
			AIC->ActivateRTSBehavior();
			AIC->IssueOrder_AttackMove(Dest);
		}
		else
		{
			// DÉPLACEMENT PUR : l'unité va DIRECTEMENT au point demandé, sans s'arrêter pour
			// engager l'ennemi (l'ordre du joueur PRIME sur le comportement auto). Elle y va
			// même en prenant des dégâts. Pour attaquer : clic droit sur un ENNEMI, ou Shift+clic
			// droit sur le sol (attack-move).
			AIC->IssueOrder_Move(Dest);
		}
	}
}

// Touche R : chaque unité sélectionnée avec une compétence prête l'active sur une cible
// À PORTÉE. Priorité au CIBLAGE MANUEL : si le curseur survole un ennemi vivant, TOUTES
// les unités sélectionnées (dont il est à portée) frappent CETTE cible — permet de
// concentrer les compétences sur une cible précise (ex. tir laser qui dépasse l'ennemi
// le plus proche pour abattre une unité plus importante derrière) plutôt que de toujours
// taper le plus proche. Sans cible sous le curseur (ou hors de portée pour une unité
// donnée), repli automatique sur l'ennemi vivant le plus proche de CETTE unité.
// Une unité sans cible valide à portée ou en recharge est silencieusement ignorée.
void AWOTOLPlayerController_Battle::ActivateSelectionAbility()
{
	UUnitSelectionManager* SelectionMgr = GetSelectionManager();
	if (!SelectionMgr) return;

	UFactionRegistrySubsystem* Reg = GetWorld() ? GetWorld()->GetSubsystem<UFactionRegistrySubsystem>() : nullptr;
	if (!Reg) return;

	const EFactionID RivalFaction = (PlayerFaction == EFactionID::Noxeens)
		? EFactionID::Aquiloris : EFactionID::Noxeens;
	const TArray<AUnitBase*> Enemies = Reg->GetUnitsForFaction(RivalFaction);

	// Ciblage manuel : ennemi vivant sous le curseur, s'il y en a un (focus fire).
	AUnitBase* ManualTarget = GetUnitUnderCursor();
	if (ManualTarget && (!ManualTarget->IsAlive() || ManualTarget->GetFaction() != RivalFaction))
	{
		ManualTarget = nullptr; // sous le curseur : allié, cadavre, ou décor -> pas une cible valide
	}

	for (AUnitBase* Unit : SelectionMgr->GetSelectedUnits())
	{
		if (!Unit || !Unit->IsAlive() || !Unit->AbilityComp) continue;
		UAbilityBase* Ability = Unit->AbilityComp->GetAbilityByIndex(0);
		if (!Ability || Ability->IsOnCooldown()) continue;

		const FVector From = Unit->GetActorLocation();
		const float RangeSq = FMath::Square(Ability->Range);

		AUnitBase* BestTarget = nullptr;
		if (ManualTarget && FVector::DistSquared(From, ManualTarget->GetActorLocation()) <= RangeSq)
		{
			BestTarget = ManualTarget;
		}
		else
		{
			float BestDistSq = RangeSq;
			for (AUnitBase* Enemy : Enemies)
			{
				if (!Enemy || !Enemy->IsAlive()) continue;
				const float DistSq = FVector::DistSquared(From, Enemy->GetActorLocation());
				if (DistSq < BestDistSq) { BestDistSq = DistSq; BestTarget = Enemy; }
			}
		}
		if (!BestTarget) continue; // rien à portée -> ne consomme pas la recharge pour rien

		Unit->AbilityComp->ActivateAbilityByIndex(0, BestTarget->GetActorLocation(), BestTarget);
	}
}

// Lu par le HUD (barre de commandement) pour afficher l'état "Prête (R)" / "Recharge : Xs"
// de la compétence de la PREMIÈRE unité sélectionnée. Renvoie faux si rien n'est
// sélectionné ou si l'unité primaire n'a pas de compétence assignée.
bool AWOTOLPlayerController_Battle::GetPrimarySelectionAbilityStatus(
	FText& OutName, float& OutCooldownRemaining, float& OutCooldownMax) const
{
	UUnitSelectionManager* SelectionMgr = GetSelectionManager();
	if (!SelectionMgr) return false;

	const TArray<AUnitBase*>& Sel = SelectionMgr->GetSelectedUnits();
	if (Sel.Num() == 0 || !Sel[0] || !Sel[0]->AbilityComp) return false;

	UAbilityBase* Ability = Sel[0]->AbilityComp->GetAbilityByIndex(0);
	if (!Ability) return false;

	OutName              = Ability->DisplayName;
	OutCooldownRemaining = Ability->GetCooldownRemaining();
	OutCooldownMax       = Ability->Cooldown;
	return true;
}
