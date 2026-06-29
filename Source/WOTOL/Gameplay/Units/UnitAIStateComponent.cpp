#include "UnitAIStateComponent.h"
#include "UnitBase.h"
#include "UnitDataAsset.h"
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

		Owner->OnUnitDied.AddDynamic(this, &UUnitAIStateComponent::HandleOwnerDied);
	}
}

void UUnitAIStateComponent::HandleOwnerDied(AUnitBase* /*Unit*/)
{
	TransitionTo(EUnitAIState::Dead);
	SetAIActive(false);
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
	// Ordre de déplacement simple en cours — attendre l'arrivée
	if (bFollowingPlayerOrder) return;

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

	AUnitBase* Target = FindBestTarget();
	if (Target)
	{
		CurrentTarget = Target;
		if (IsInAttackRange(Target))
			TransitionTo(EUnitAIState::Attacking);
		else
			TransitionTo(EUnitAIState::Seeking);
		return;
	}

	// En AttackMove : continuer vers la destination
	if (bAttackMoveActive)
	{
		if (AAIAdaptiveController* AIC = GetAIController())
		{
			AIC->MoveToLocation(AttackMoveDestination, 100.f);
		}
		return;
	}

	// Tenir la position : rester strictement en formation (pas de patrouille/errance)
	if (bHoldPosition)
	{
		return;
	}

	TransitionTo(EUnitAIState::Patrolling);
}

void UUnitAIStateComponent::EvaluatePatrolling()
{
	AUnitBase* Target = FindBestTarget();
	if (Target)
	{
		CurrentTarget = Target;
		TransitionTo(EUnitAIState::Seeking);
		return;
	}

	if (HasLowHealth())
	{
		TransitionTo(EUnitAIState::Retreating);
		return;
	}

	if (AAIAdaptiveController* AIC = GetAIController())
	{
		AUnitBase* Owner = Cast<AUnitBase>(GetOwner());
		if (!Owner) return;

		const float Dist = FVector::Dist2D(Owner->GetActorLocation(), PatrolDestination);
		if (Dist < 200.f)
		{
			UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
			FNavLocation NavLoc;
			if (NavSys && NavSys->GetRandomReachablePointInRadius(SpawnLocation, 600.f, NavLoc))
			{
				PatrolDestination = NavLoc.Location;
				AIC->MoveToLocation(PatrolDestination, 100.f);
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

	AUnitBase* Target = FindBestTarget();
	if (!Target)
	{
		CurrentTarget.Reset();
		TransitionTo(EUnitAIState::Idle);
		return;
	}

	CurrentTarget = Target;

	if (IsInAttackRange(Target))
	{
		TransitionTo(EUnitAIState::Attacking);
		return;
	}

	if (AAIAdaptiveController* AIC = GetAIController())
	{
		AIC->MoveToActor(Target, 50.f);
	}
}

void UUnitAIStateComponent::EvaluateAttacking()
{
	if (HasLowHealth())
	{
		TransitionTo(EUnitAIState::Retreating);
		return;
	}

	AUnitBase* Target = FindBestTarget();
	if (!Target)
	{
		CurrentTarget.Reset();
		TransitionTo(EUnitAIState::Idle);
		return;
	}

	CurrentTarget = Target;

	if (!IsInAttackRange(Target))
	{
		// HoldPosition : pas de poursuite — attendre que l'ennemi vienne
		if (bHoldPosition)
		{
			return;
		}
		TransitionTo(EUnitAIState::Seeking);
		return;
	}

	if (AUnitBase* Owner = Cast<AUnitBase>(GetOwner()))
	{
		Owner->PerformAttack(Target);
	}
}

void UUnitAIStateComponent::EvaluateRetreating()
{
	AUnitBase* Owner = Cast<AUnitBase>(GetOwner());
	if (!Owner) return;

	if (!HasLowHealth())
	{
		bHoldPosition = false;
		TransitionTo(EUnitAIState::Idle);
		return;
	}

	if (AAIAdaptiveController* AIC = GetAIController())
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

	const bool bWasMoving = (Old == EUnitAIState::Seeking
		|| Old == EUnitAIState::Patrolling
		|| Old == EUnitAIState::Retreating);
	const bool bWillMove = (NewState == EUnitAIState::Seeking
		|| NewState == EUnitAIState::Patrolling
		|| NewState == EUnitAIState::Retreating);

	if (bWasMoving && !bWillMove)
	{
		if (AAIAdaptiveController* AIC = GetAIController())
		{
			AIC->StopMovement();
		}
	}

	if (NewState == EUnitAIState::Patrolling)
	{
		PatrolDestination = SpawnLocation;
	}

	OnAIStateChanged.Broadcast(Old, NewState);
}

AUnitBase* UUnitAIStateComponent::FindBestTarget() const
{
	// Priorité : cible imposée par ordre joueur
	if (ForceTarget.IsValid() && ForceTarget->IsAlive())
	{
		return ForceTarget.Get();
	}

	return FindNearestEnemy();
}

AUnitBase* UUnitAIStateComponent::FindNearestEnemy() const
{
	AUnitBase* Owner = Cast<AUnitBase>(GetOwner());
	if (!Owner) return nullptr;

	UFactionRegistrySubsystem* Registry =
		GetWorld()->GetSubsystem<UFactionRegistrySubsystem>();
	if (!Registry) return nullptr;

	AUnitBase* Nearest     = nullptr;
	float      NearestDist = SightRange * SightRange;

	const EFactionID OwnFaction = Owner->GetFaction();
	const FVector    OwnLoc     = Owner->GetActorLocation();

	for (uint8 i = 1; i <= static_cast<uint8>(EFactionID::PiratesAbyssaux); ++i)
	{
		const EFactionID FID = static_cast<EFactionID>(i);
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

AAIAdaptiveController* UUnitAIStateComponent::GetAIController() const
{
	if (AUnitBase* Owner = Cast<AUnitBase>(GetOwner()))
	{
		return Cast<AAIAdaptiveController>(Owner->GetController());
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
	const float Range = Owner->GetUnitData()->Stats.AttackRange * 200.f;
	// Distance BORD À BORD : on soustrait les rayons de collision des deux unités,
	// sinon une cible énorme (kraken) n'est jamais "à portée" centre-à-centre.
	const float CenterDist = FVector::Dist(Owner->GetActorLocation(), Target->GetActorLocation());
	const float EdgeDist   = CenterDist - Owner->GetSimpleCollisionRadius()
	                                    - Target->GetSimpleCollisionRadius();
	return EdgeDist <= Range;
}
