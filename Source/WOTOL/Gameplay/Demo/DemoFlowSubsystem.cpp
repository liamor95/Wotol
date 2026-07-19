#include "DemoFlowSubsystem.h"
#include "Core/WOTOLGameInstance.h"

void UDemoFlowSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CurrentPhase = EDemoPhase::None;
	Progress = FDemoProgress();
}

void UDemoFlowSubsystem::SetPhase(EDemoPhase NewPhase)
{
	if (NewPhase == CurrentPhase) return;
	const EDemoPhase Previous = CurrentPhase;
	CurrentPhase = NewPhase;
	OnDemoPhaseChanged.Broadcast(NewPhase, Previous);
}

void UDemoFlowSubsystem::AdvancePhase()
{
	// Ordre linéaire de la démo
	switch (CurrentPhase)
	{
		// Canon le plus récent : après la configuration, la démo commence par l'introduction
		// puis la nage libre vers le Kraken. La cité vient après la conquête.
		case EDemoPhase::None:                 SetPhase(EDemoPhase::Exploration_Creature); break;
		case EDemoPhase::CityIntro:            SetPhase(EDemoPhase::Exploration_Creature); break;
		case EDemoPhase::Exploration_Creature: SetPhase(EDemoPhase::Battle_Creature); break;
		case EDemoPhase::Battle_Creature:      SetPhase(EDemoPhase::Capture_Zone); break;
		case EDemoPhase::Capture_Zone:         SetPhase(EDemoPhase::Mythic_Discovery); break;
		case EDemoPhase::Mythic_Discovery:     SetPhase(EDemoPhase::City_Unlock); break;
		case EDemoPhase::City_Unlock:          SetPhase(EDemoPhase::Exploration_Rival); break;
		case EDemoPhase::Exploration_Rival:    SetPhase(EDemoPhase::Battle_Rival); break;
		case EDemoPhase::Battle_Rival:         SetPhase(EDemoPhase::Repair_Zone); break;
		case EDemoPhase::Repair_Zone:          SetPhase(EDemoPhase::DemoEnd); break;
		default: break;
	}
}

void UDemoFlowSubsystem::OpenObjectiveWindow(FName StepId, const FString& Title, const FString& Body,
	const FString& ButtonLabel, bool bFailure)
{
	ObjWinStepId       = StepId;
	ObjWinTitle        = Title;
	ObjWinBody         = Body;
	ObjWinButton       = ButtonLabel.IsEmpty() ? TEXT("Continuer") : ButtonLabel;
	bObjWinIsFailure   = bFailure;
	bObjectiveWindowOpen = true;
}

void UDemoFlowSubsystem::ConfirmObjectiveWindow()
{
	if (!bObjectiveWindowOpen) return;
	const FName Step = ObjWinStepId;
	bObjectiveWindowOpen = false;
	ObjWinStepId = NAME_None;
	OnObjectiveConfirmed.Broadcast(Step);
}

EBattleType UDemoFlowSubsystem::GetCurrentBattleType() const
{
	return (CurrentPhase == EDemoPhase::Battle_Rival || CurrentPhase == EDemoPhase::Battle_Grand)
		? EBattleType::RivalDefense
		: EBattleType::CreatureEncounter;
}

EFactionID UDemoFlowSubsystem::GetPlayerFaction() const
{
	// Source FIABLE : la faction choisie stockée ici (comme ResolvePlayerFaction du
	// Director). Le repli GameInstance pouvait être périmé -> compteur allié/ennemi inversé.
	if (SelectedFaction != EFactionID::None) return SelectedFaction;
	if (const UWOTOLGameInstance* GI = Cast<UWOTOLGameInstance>(GetGameInstance()))
	{
		return GI->GetSelectedFaction();
	}
	return EFactionID::None;
}

FName UDemoFlowSubsystem::GetUnitID(EFactionID Faction, EDemoUnitCategory Category) const
{
	// Noms canoniques du GDD/code (décision : on garde les noms existants)
	if (Faction == EFactionID::Noxeens)
	{
		switch (Category)
		{
			case EDemoUnitCategory::Chef:       return TEXT("Noxar");
			case EDemoUnitCategory::Infanterie: return TEXT("Noxeflare");
			case EDemoUnitCategory::Montee:     return TEXT("Noxebeast");
			case EDemoUnitCategory::Distance:   return TEXT("Noxeblast");
			case EDemoUnitCategory::Speciale:   return TEXT("Noxeons");
			case EDemoUnitCategory::Mythique:   return TEXT("Noxedrake");
		}
	}
	else // Aquiloris par défaut
	{
		switch (Category)
		{
			case EDemoUnitCategory::Chef:       return TEXT("Aquis");
			case EDemoUnitCategory::Infanterie: return TEXT("Aquiloryons");
			case EDemoUnitCategory::Montee:     return TEXT("Aquilances");
			case EDemoUnitCategory::Distance:   return TEXT("Aquipheres");
			case EDemoUnitCategory::Speciale:   return TEXT("Aquilombres");
			case EDemoUnitCategory::Mythique:   return TEXT("Leviaphenix");
		}
	}
	return NAME_None;
}

bool UDemoFlowSubsystem::IsCategoryUnlocked(EDemoUnitCategory Category) const
{
	switch (Category)
	{
		case EDemoUnitCategory::Chef:
		case EDemoUnitCategory::Infanterie:
		case EDemoUnitCategory::Montee:
			return true;                       // jouables dès le début
		case EDemoUnitCategory::Distance:
			return Progress.bRangedUnlocked;   // débloquée après la 1ère victoire
		case EDemoUnitCategory::Speciale:
		case EDemoUnitCategory::Mythique:
			return Progress.bAllUnlocked;      // débloquées en phase 3 (grande bataille)
	}
	return false;
}

void UDemoFlowSubsystem::UnlockRangedUnit()       { Progress.bRangedUnlocked = true; }
void UDemoFlowSubsystem::UnlockAll()
{
	Progress.bRangedUnlocked   = true;
	Progress.bMythicDiscovered = true;
	Progress.bAllUnlocked      = true;
}
void UDemoFlowSubsystem::DiscoverMythic()
{
	Progress.bMythicDiscovered = true;
	Progress.bMythicBuildingUnlocked = true;
}
void UDemoFlowSubsystem::ActivateRecruitBuilding() { Progress.bRecruitBuildingActive = true; }
void UDemoFlowSubsystem::MarkZoneCaptured()        { Progress.bZoneCaptured = true; }
void UDemoFlowSubsystem::MarkZoneDamaged()         { Progress.bZoneDamaged = true; }
void UDemoFlowSubsystem::MarkZoneRepaired()        { Progress.bZoneRepaired = true; }

void UDemoFlowSubsystem::GrantMissionRewards(int32 Crystals, int32 AbyssalMaterials,
	int32 Biomass, int32 Food)
{
	LastRewardCrystals          = FMath::Max(0, Crystals);
	LastRewardAbyssalMaterials  = FMath::Max(0, AbyssalMaterials);
	LastRewardBiomass           = FMath::Max(0, Biomass);
	LastRewardFood              = FMath::Max(0, Food);

	PlayerCrystals          += LastRewardCrystals;
	PlayerAbyssalMaterials  += LastRewardAbyssalMaterials;
	PlayerBiomass           += LastRewardBiomass;
	PlayerFood              += LastRewardFood;
}

bool UDemoFlowSubsystem::CanAffordTerritoryBuilding(int32 CrystalCost,
	int32 AbyssalMaterialCost) const
{
	return CrystalCost >= 0 && AbyssalMaterialCost >= 0
		&& PlayerCrystals >= CrystalCost
		&& PlayerAbyssalMaterials >= AbyssalMaterialCost;
}

bool UDemoFlowSubsystem::SpendTerritoryBuildingCost(int32 CrystalCost,
	int32 AbyssalMaterialCost)
{
	if (!CanAffordTerritoryBuilding(CrystalCost, AbyssalMaterialCost)) return false;
	PlayerCrystals -= CrystalCost;
	PlayerAbyssalMaterials -= AbyssalMaterialCost;
	return true;
}

// ─── ÉCONOMIE DE LA CITÉ ─────────────────────────────────────────────────────
int32 UDemoFlowSubsystem::GetProductionCost(EDemoUnitCategory Category) const
{
	// Coûts inspirés des coûts d'unité du GDD (échelle réduite pour la démo).
	switch (Category)
	{
		case EDemoUnitCategory::Infanterie: return 130;
		case EDemoUnitCategory::Distance:   return 140;
		case EDemoUnitCategory::Montee:     return 180;
		case EDemoUnitCategory::Speciale:   return 160;
		case EDemoUnitCategory::Mythique:   return 400;
		case EDemoUnitCategory::Chef:       return 0;   // le chef n'est pas produit en série
	}
	return 0;
}

bool UDemoFlowSubsystem::CanProduce(EDemoUnitCategory Category) const
{
	const int32 Cost = GetProductionCost(Category);
	return Cost > 0 && IsCategoryUnlocked(Category) && PlayerCrystals >= Cost;
}

bool UDemoFlowSubsystem::ProduceUnit(EDemoUnitCategory Category)
{
	if (!CanProduce(Category)) return false;
	const int32 Cost = GetProductionCost(Category);
	const FName UnitID = GetUnitID(GetPlayerFaction(), Category);
	if (UnitID.IsNone()) return false;
	PlayerCrystals -= Cost;
	ReserveUnits.FindOrAdd(UnitID) += 1;
	return true;
}

int32 UDemoFlowSubsystem::GetReserveCount(FName UnitID) const
{
	const int32* Found = ReserveUnits.Find(UnitID);
	return Found ? *Found : 0;
}

void UDemoFlowSubsystem::DrainReserve(TMap<FName, int32>& OutUnits)
{
	OutUnits = ReserveUnits;
	ReserveUnits.Empty();
}

// ─── PROGRESSION DES BÂTIMENTS ───────────────────────────────────────────────
int32 UDemoFlowSubsystem::GetBuildingLevel(EDemoUnitCategory Category) const
{
	const int32* Found = BuildingLevels.Find(Category);
	return Found ? *Found : 1; // niveau 1 par défaut
}

int32 UDemoFlowSubsystem::GetBuildingUpgradeCost(EDemoUnitCategory Category) const
{
	const int32 Level = GetBuildingLevel(Category);
	if (Level >= MaxBuildingLevel) return 0; // déjà au max
	// Coût croissant : 250 pour lvl 1->2, 500 pour lvl 2->3.
	return 250 * Level;
}

bool UDemoFlowSubsystem::CanUpgradeBuilding(EDemoUnitCategory Category) const
{
	const int32 Cost = GetBuildingUpgradeCost(Category);
	return Cost > 0 && PlayerCrystals >= Cost;
}

bool UDemoFlowSubsystem::UpgradeBuilding(EDemoUnitCategory Category)
{
	if (!CanUpgradeBuilding(Category)) return false;
	const int32 Cost = GetBuildingUpgradeCost(Category);
	PlayerCrystals -= Cost;
	BuildingLevels.FindOrAdd(Category) = GetBuildingLevel(Category) + 1;
	return true;
}

// ─── AXE / VOIE par type d'unité ─────────────────────────────────────────────
int32 UDemoFlowSubsystem::GetUnitAxis(EDemoUnitCategory Category) const
{
	const int32* Found = UnitAxes.Find(Category);
	return Found ? *Found : 0; // base par défaut
}

void UDemoFlowSubsystem::SetUnitAxis(EDemoUnitCategory Category, int32 Axis)
{
	UnitAxes.FindOrAdd(Category) = FMath::Clamp(Axis, 0, 2);
}

EDemoUnitCategory UDemoFlowSubsystem::GetCategoryForUnit(FName UnitID)
{
	if (UnitID == TEXT("Aquis")       || UnitID == TEXT("Noxar"))     return EDemoUnitCategory::Chef;
	if (UnitID == TEXT("Aquiloryons") || UnitID == TEXT("Noxeflare")) return EDemoUnitCategory::Infanterie;
	if (UnitID == TEXT("Aquilances")  || UnitID == TEXT("Noxebeast")) return EDemoUnitCategory::Montee;
	if (UnitID == TEXT("Aquipheres")  || UnitID == TEXT("Noxeblast")) return EDemoUnitCategory::Distance;
	if (UnitID == TEXT("Aquilombres") || UnitID == TEXT("Noxeons"))   return EDemoUnitCategory::Speciale;
	if (UnitID == TEXT("Leviaphenix") || UnitID == TEXT("Noxedrake")) return EDemoUnitCategory::Mythique;
	return EDemoUnitCategory::Infanterie;
}
