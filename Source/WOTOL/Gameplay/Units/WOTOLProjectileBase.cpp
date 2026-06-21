#include "WOTOLProjectileBase.h"
#include "UnitBase.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

AWOTOLProjectileBase::AWOTOLProjectileBase()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	CollisionSphere->InitSphereRadius(20.f);
	CollisionSphere->SetCollisionProfileName(TEXT("Projectile"));
	RootComponent = CollisionSphere;

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	ProjectileMovement->InitialSpeed  = InitialSpeed;
	ProjectileMovement->MaxSpeed      = MaxSpeed;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale   = 0.f; // projectiles sous-marins = pas de gravité

	InitialLifeSpan = 5.f;
}

void AWOTOLProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(
		this, &AWOTOLProjectileBase::OnSphereOverlap);
}

void AWOTOLProjectileBase::InitProjectile(
	AUnitBase* InInstigator, AUnitBase* InTarget, float InDamage)
{
	InstigatorUnit = InInstigator;
	TargetUnit     = InTarget;
	Damage         = InDamage;

	// Ignorer la collision avec l'unité qui tire
	if (InInstigator)
	{
		CollisionSphere->IgnoreActorWhenMoving(InInstigator, true);
	}

	// Direction initiale vers la cible
	if (InTarget)
	{
		const FVector Dir = (InTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		ProjectileMovement->Velocity = Dir * InitialSpeed;
	}
}

void AWOTOLProjectileBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	TrackTarget();
}

void AWOTOLProjectileBase::TrackTarget()
{
	if (!TargetUnit.IsValid() || !TargetUnit->IsAlive())
	{
		SetActorTickEnabled(false);
		return;
	}

	// Légère correction de trajectoire vers la cible (homing doux)
	const FVector ToTarget = (TargetUnit->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	const FVector NewVel   = FMath::VInterpNormalRotationTo(
		ProjectileMovement->Velocity.GetSafeNormal(), ToTarget, GetWorld()->GetDeltaSeconds(), 90.f)
		* MaxSpeed;

	ProjectileMovement->Velocity = NewVel;
}

void AWOTOLProjectileBase::OnSphereOverlap(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	AUnitBase* HitUnit = Cast<AUnitBase>(OtherActor);
	if (!HitUnit || !HitUnit->IsAlive()) return;

	// Ne pas toucher les alliés
	if (InstigatorUnit.IsValid() &&
		HitUnit->GetFaction() == InstigatorUnit->GetFaction()) return;

	HitUnit->TakeDamageFromUnit(Damage, InstigatorUnit.Get());
	OnImpact(HitUnit);
	Destroy();
}
