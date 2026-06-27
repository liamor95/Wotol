#include "UnitAIStateComponent.h"
#include "UnitBase.h"
#include "Gameplay/AI/AIAdaptiveController.h"
#include "Core/FactionRegistrySubsystem.h"
#include "AIController.h"
#include "NavigationSystem.h"

UUnitAIStateComponent::UUnitAIStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUnitAIStateComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AUnitBase* Owner = Cast<AUnitBase>(GetOwner()))
	{
		SpawnLocation = Owner->GetActorLocation();

		// Se désactiver quand l'unité meurt
		Owner->OnUnitDied.AddWeakLambda(this, [this](AUnitBase*)
		{
			TransitionTo(EUnitAIState::Dead);
			SetAIActive(false);
		});
	}
}

void UUnitAIStateComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AITickHandle);
	}
	Super::EndPlay(Reason);
}

void UUnitAIStateComponent::SetAIActive(bool bActive)
{
	bAIActive = bActive;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AITickHandle);

		if (bActive && CurrentState != EUnitAIState::Dead)
		{
			World->GetTimerManager().SetTimer(
				AITickHandle, this, &UUnitAIStateComponent::AITick,
				TickInterval, true, 0.f);
		}
	}
}

void UUnitAIStateComponent::AITick()
{
	switch (CurrentState)
	{
		case EUnitAIState::Idle:        EvaluateIdle();       break;
		case EUnitAIState::Patrolling:  EvaluatePatrolling(); break;
		case EUnitAIState::Seeking:     EvaluateSeeking();    break;
		case EUnitAIState::Attacking:   EvaluateAttacking();  break;
		case EUnitAIState::Retreating:  EvaluateRetreating(); break;
		default: break;
	}
}

void UUnitAIStateComponent::EvaluateIdle()
{
	if (HasLowHealth())
	{
		TransitionTo(EUnitAIState::Retreating);
		return;
	}

	AUnitBase* Enemy = FindNearestEnemy();
	if (Enemy)
	{
		CurrentTarget = Enemy;
		if (IsInAttackRange(Enemy))
			TransitionTo(EUnitAIState::Attacking);
		else
			TransitionTo(EUnitAIState::Seeking);
		return;
	}

	// Aucun ennemi — patrol après 2 évaluations
	TransitionTo(EUnitAIState::Patrolling);
}

void UUnitAIStateComponent::EvaluatePatrolling()
{
	// Un ennemi est-il apparu ?
	AUnitBase* Enemy = FindNearestEnemy();
	if (Enemy)
	{
		CurrentTarget = Enemy;
		TransitionTo(EUnitAIState::Seeking);
		return;
	}

	if (HasLowHealth())
	{
		TransitionTo(EUnitAIState::Retreating);
		return;
	}

	// Nouvelle destination de patrol si on est arrivé
	if (UAIAdaptiveController* AIC = GetAIController())
	{
		AUnitBase* Owner = Cast<AUnitBase>(GetOwner());
		if (Owner)
		{
			const float Dist = FVector::Dist2D(Owner->GetActorLocation(), PatrolDestination);
			if (Dist < 200.f)
			{
				// Nouvelle destination aléatoire dans un rayon de 600 unités
				UNavigationSystemV1* NavSys =
					UNavigationSystemV1::GetCurrent(GetWorld());
				FNavLocation NavLoc;
				if (NavSys && NavSys->GetRandomReachablePointInRadius(
						SpawnLocation, 600.f, NavLoc))
				{
					PatrolDestination = NavLoc.Location;
					AIC->MoveToLocation(PatrolDestination, 100.f);
				}
			}
		}
	}
}

void UUnitAIStateComponent::EvaluateSeeking()
{
	if (HasLowHealth())
	{
		TransitionTo(EUnitAIState::Retreating);
		return;
	}

	if (!CurrentTarget.IsValid() || !CurrentTarget->IsAlive())
	{
		CurrentTarget = FindNearestEnemy();
		if (!CurrentTarget.IsValid())
		{
			TransitionTo(EUnitAIState::Idle);
			return;
		}
	}

	if (IsInAttackRange(CurrentTarget.Get()))
	{
		TransitionTo(EUnitAIState::Attacking);
		return;
	}

	// Continuer à se déplacer vers la cible
	if (UAIAdaptiveController* AIC = GetAIController())
	{
		AIC->MoveToActor(CurrentTarget.Get(), 50.f);
	}
}

void UUnitAIStateComponent::EvaluateAttacking()
{
	if (HasLowHealth())
	{
		TransitionTo(EUnitAIState::Retreating);
		return;
	}

	if (!CurrentTarget.IsValid() || !CurrentTarget->IsAlive())
	{
		CurrentTarget = FindNearestEnemy();
		if (!CurrentTarget.IsValid())
		{
			TransitionTo(EUnitAIState::Idle);
			return;
		}
	}

	if (!IsInAttackRange(CurrentTarget.Get()))
	{
		TransitionTo(EUnitAIState::Seeking);
		return;
	}

	// Déclencher l'attaque via l'unité propriétaire
	if (AUnitBase* Owner = Cast<AUnitBase>(GetOwner()))
	{
		Owner->PerformAttack(CurrentTarget.Get());
	}
}

void UUnitAIStateComponent::EvaluateRetreating()
{
	AUnitBase* Owner = Cast<AUnitBase>(GetOwner());
	if (!Owner) return;

	// PV remontés ? Reprendre le combat
	if (!HasLowHealth())
	{
		TransitionTo(EUnitAIState::Idle);
		return;
	}

	// Se déplacer vers le point de spawn (zone arrière = sécurité)
	if (UAIAdaptiveController* AIC = GetAIController())
	{
		const float DistToSpawn = FVector::Dist2D(Owner->GetActorLocation(), SpawnLocation);
		if (DistToSpawn > 200.f)
		{
			AIC->MoveToLocation(SpawnLocation, 100.f);
		}
	}
}

void UUnitAIStateComponent::TransitionTo(EUnitAIState NewState)
{
	if (NewState == CurrentState) return;

	const EUnitAIState Old = CurrentState;
	CurrentState = NewState;

	// Stop navigation si on sort de Seeking/Patrolling
	if ((Old == EUnitAIState::Seeking || Old == EUnitAIState::Patrolling
		|| Old == EUnitAIState::Retreating)
		&& NewState != EUnitAIState::Seeking
		&& NewState != EUnitAIState::Patrolling
		&& NewState != EUnitAIState::Retreating)
	{
		if (UAIAdaptiveController* AIC = GetAIController())
		{
			AIC->StopMovement();
		}
	}

	// Initialisation de patrol
	if (NewState == EUnitAIState::Patrolling)
	{
		PatrolDestination = SpawnLocation;
	}

	OnAIStateChanged.Broadcast(Old, NewState);
}

AUnitBase* UUnitAIStateComponent::FindNearestEnemy() const
{
	AUnitBase* Owner = Cast<AUnitBase>(GetOwner());
	if (!Owner) return nullptr;

	UFactionRegistrySubsystem* Registry =
		GetWorld()->GetSubsystem<UFactionRegistrySubsystem>();
	if (!Registry) return nullptr;

	AUnitBase* Nearest    = nullptr;
	float      NearestDist = SightRange * SightRange;

	// Parcourir toutes les factions sauf la nôtre
	const EFactionID OwnFaction = Owner->GetFaction();
	const FVector    OwnLoc     = Owner->GetActorLocation();

	for (uint8 i = 1; i <= (uint8)EFactionID::PiratesAbyssaux; ++i)
	{
		const EFactionID FID = (EFactionID)i;
		if (FID == OwnFaction) continue;

		for (AUnitBase* Enemy : Registry->GetUnitsForFaction(FID))
		{
			if (!Enemy || !Enemy->IsAlive()) continue;
			const float Dist = FVector::DistSquared(OwnLoc, Enemy->GetActorLocation());
			if (Dist < NearestDist)
			{
				NearestDist = Dist;
				Nearest     = Enemy;
			}
		}
	}

	return Nearest;
}

UAIAdaptiveController* UUnitAIStateComponent::GetAIController() const
{
	if (AUnitBase* Owner = Cast<AUnitBase>(GetOwner()))
	{
		return Cast<UAIAdaptiveController>(Owner->GetController());
	}
	return nullptr;
}

bool UUnitAIStateComponent::HasLowHealth() const
{
	AUnitBase* Owner = Cast<AUnitBase>(GetOwner());
	return Owner && Owner->GetHealthPercent() < RetreatHealthRatio;
}

bool UUnitAIStateComponent::IsInAttackRange(AUnitBase* Target) const
{
	if (!Target) return false;
	AUnitBase* Owner = Cast<AUnitBase>(GetOwner());
	if (!Owner || !Owner->GetUnitData()) return false;

	// AttackRange est en "cases hex" — 1 case ≈ 200 UE units
	const float RangeSq = FMath::Square(Owner->GetUnitData()->Stats.AttackRange * 200.f);
	return FVector::DistSquared(Owner->GetActorLocation(), Target->GetActorLocation()) <= RangeSq;
}
