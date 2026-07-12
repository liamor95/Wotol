#include "WOTOLBeam.h"
#include "Gameplay/Units/UnitBase.h"
#include "Core/FactionRegistrySubsystem.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/PointLightComponent.h"
#include "WOTOLGlow.h"

AWOTOLBeam::AWOTOLBeam()
{
	PrimaryActorTick.bCanEverTick = true;
	Pivot = CreateDefaultSubobject<USceneComponent>(TEXT("Pivot"));
	RootComponent = Pivot;
	Beam = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Beam"));
	Beam->SetupAttachment(Pivot);
	Beam->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Beam->SetCanEverAffectNavigation(false);
	// Lumière FLUO accrochée au rayon (éclaire la scène à la couleur de l'attaque).
	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(Pivot);
	Glow->SetCastShadows(false);
	// Lampes DISCRÈTES : le trait lui-même est émissif (il brille sur toute sa longueur) ;
	// ces lampes ne font qu'un léger halo, elles n'inondent plus le sol.
	Glow->SetAttenuationRadius(260.f);
	Glow->SetIntensity(1500.f);
	Glow2 = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow2"));
	Glow2->SetupAttachment(Pivot);
	Glow2->SetCastShadows(false);
	Glow2->SetAttenuationRadius(260.f);
	Glow2->SetIntensity(1500.f);
}

AWOTOLBeam* AWOTOLBeam::Fire(UWorld* World, const FVector& Origin, float YawStart, float YawEnd,
	float Length, const FLinearColor& Color, AUnitBase* Caster, float SweepDamage, float Pitch, float Thickness)
{
	if (!World) return nullptr;
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AWOTOLBeam* B = World->SpawnActor<AWOTOLBeam>(AWOTOLBeam::StaticClass(), Origin, FRotator::ZeroRotator, P);
	if (!B) return nullptr;

	B->OriginLoc = Origin;
	B->Yaw0 = YawStart; B->Yaw1 = YawEnd; B->PitchAngle = Pitch; B->Len = Length;
	B->CasterUnit = Caster; B->Damage = SweepDamage;

	UStaticMesh* Cyl = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cyl) B->Beam->SetStaticMesh(Cyl);
	// Cylindre couché le long de +X (le mesh est vertical à la base -> pitch +90),
	// fin et long = trait laser. Ancré à l'origine, s'étend vers l'avant.
	B->Beam->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	B->Beam->SetRelativeScale3D(FVector(0.10f * Thickness, 0.10f * Thickness, Length / 100.f));
	B->Beam->SetRelativeLocation(FVector(Length * 0.5f, 0.f, 0.f));
	// Trait ÉMISSIF sur TOUTE sa longueur : c'est le rayon qui rayonne (couleur >1),
	// pas le sol. Matériau unlit -> le cylindre entier brille fluo à l'écran.
	if (UMaterialInstanceDynamic* MID = WOTOLGlow::MakeGlow(B,
			FLinearColor(Color.R * 3.0f + 0.3f, Color.G * 3.0f + 0.3f, Color.B * 3.0f + 0.3f, 1.f)))
	{
		B->Beam->SetMaterial(0, MID);
		B->BeamMID = MID;
	}
	B->Pivot->SetWorldRotation(FRotator(Pitch, YawStart, 0.f));
	// DEUX lampes fluo réparties sur le rayon -> TOUT le trait est illuminé (pas juste
	// le milieu), à la couleur de l'attaque (vert pour Noxar, cyan pour Aquis).
	const FLinearColor LCol(FMath::Min(1.f, Color.R + 0.25f),
		FMath::Min(1.f, Color.G + 0.25f), FMath::Min(1.f, Color.B + 0.25f));
	const float LRad = FMath::Clamp(Length * 0.55f, 500.f, 1400.f);
	if (B->Glow)  { B->Glow->SetLightColor(LCol);  B->Glow->SetRelativeLocation(FVector(Length * 0.28f, 0.f, 0.f));  B->Glow->SetAttenuationRadius(LRad); }
	if (B->Glow2) { B->Glow2->SetLightColor(LCol); B->Glow2->SetRelativeLocation(FVector(Length * 0.72f, 0.f, 0.f)); B->Glow2->SetAttenuationRadius(LRad); }
	return B;
}

void AWOTOLBeam::Tick(float Dt)
{
	Super::Tick(Dt);
	Life += Dt;
	const float a = FMath::Clamp(Life / Duration, 0.f, 1.f);

	// Balayage : interpole le yaw du départ vers l'arrivée. Le pitch (visée verticale
	// vers une couche différente) est conservé pendant tout le rayon.
	const float Yaw = FMath::Lerp(Yaw0, Yaw1, a);
	Pivot->SetWorldRotation(FRotator(PitchAngle, Yaw, 0.f));

	// Léger fondu en fin de vie.
	if (BeamMID) BeamMID->SetScalarParameterValue(TEXT("Opacity"), 1.f - a);

	// Dégâts de BALAYAGE : les unités ennemies sur la ligne du rayon (une fois chacune).
	UWorld* W = GetWorld();
	if (W && Damage > 0.f && CasterUnit.IsValid())
	{
		const FVector Dir = FRotator(0.f, Yaw, 0.f).Vector();
		if (UFactionRegistrySubsystem* Reg = W->GetSubsystem<UFactionRegistrySubsystem>())
		{
			const EFactionID Enemy = (CasterUnit->GetFaction() == EFactionID::Aquiloris)
				? EFactionID::Noxeens : EFactionID::Aquiloris;
			for (AUnitBase* U : Reg->GetUnitsForFaction(Enemy))
			{
				if (!U || !U->IsAlive() || AlreadyHit.Contains(U)) continue;
				FVector To = U->GetActorLocation() - OriginLoc; To.Z = 0.f;
				const float Along = FVector::DotProduct(To, Dir);
				if (Along < 0.f || Along > Len) continue;                 // dans la portée du rayon
				const FVector Perp = To - Dir * Along;
				if (Perp.SizeSquared() > 160.f * 160.f) continue;          // proche de la ligne
				U->TakeDamageFromUnit(Damage, CasterUnit.Get());
				AlreadyHit.Add(U);
			}
		}
	}

	if (Life >= Duration) Destroy();
}
