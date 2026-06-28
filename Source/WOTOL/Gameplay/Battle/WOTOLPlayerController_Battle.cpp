#include "WOTOLPlayerController_Battle.h"
#include "UnitSelectionManager.h"
#include "WOTOLBattleCamera.h"
#include "Gameplay/Units/UnitBase.h"
#include "Gameplay/AI/AIAdaptiveController.h"
#include "Core/WOTOLGameInstance.h"

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
	float X, Y;
	GetMousePosition(X, Y);
	BoxSelectStart   = FVector2D(X, Y);
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
	UUnitSelectionManager* SelectionMgr = GetSelectionManager();
	if (!SelectionMgr || !SelectionMgr->HasSelection()) return;

	AUnitBase* TargetUnit = GetUnitUnderCursor();
	FVector    TargetLocation;

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

	for (AUnitBase* Unit : SelectionMgr->GetSelectedUnits())
	{
		if (!Unit || !Unit->IsAlive()) continue;

		AAIAdaptiveController* AIC = Cast<AAIAdaptiveController>(Unit->GetController());
		if (!AIC) continue;

		if (TargetUnit && TargetUnit->GetFaction() != PlayerFaction)
		{
			// Ordre d'attaque
			AIC->IssueOrder_AttackTarget(TargetUnit);
		}
		else
		{
			// Ordre de déplacement avec décalage en formation
			const int32 Idx   = SelectionMgr->GetSelectedUnits().Find(Unit);
			const float Angle = (float)Idx * (2.f * PI / FMath::Max(1,
				SelectionMgr->GetSelectionCount()));
			const FVector Offset(FMath::Cos(Angle) * 150.f,
			                     FMath::Sin(Angle) * 150.f, 0.f);
			AIC->IssueOrder_Move(TargetLocation + Offset);
		}
	}
}
