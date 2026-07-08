#include "WOTOLBeam.h"
#include "Gameplay/Units/UnitBase.h"
#include "Core/FactionRegistrySubsystem.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/PointLightComponent.h"

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
	Glow->SetAttenuationRadius(700.f);
	Glow->SetIntensity(9000.f);
	Glow2 = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow2"));
	Glow2->SetupAttachment(Pivot);
	Glow2->SetCastShadows(false);
	Glow2->SetAttenuationRadius(700.f);
	Glow2->SetIntensity(9000.f);
}

AWOTOLBeam* AWOTOLBeam::Fire(UWorld* World, const FVector& Origin, float YawStart, float YawEnd,
	float Length, const FLinearColor& Color, AUnitBase* Caster, float SweepDamage)
{
	if (!World) return nullptr;
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AWOTOLBeam* B = World->SpawnActor<AWOTOLBeam>(AWOTOLBeam::StaticClass(), Origin, FRotator::ZeroRotator, P);
	if (!B) return nullptr;

	B->OriginLoc = Origin;
	B->Yaw0 = YawStart; B->Yaw1 = YawEnd; B->Len = Length;
	B->CasterUnit = Caster; B->Damage = SweepDamage;

	UStaticMesh* Cyl = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (Cyl) B->Beam->SetStaticMesh(Cyl);
	// Cylindre couché le long de +X (le mesh est vertical à la base -> pitch +90),
	// fin et long = trait laser. Ancré à l'origine, s'étend vers l'avant.
	B->Beam->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	B->Beam->SetRelativeScale3D(FVector(0.10f, 0.10f, Length / 100.f));
	B->Beam->SetRelativeLocation(FVector(Length * 0.5f, 0.f, 0.f));
	if (Base)
		if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, B))
		{
			// Couleur SURVOLTÉE (>1) -> le trait paraît lumineux fluo sur toute sa longueur.
			const FLinearColor Bright(Color.R * 2.2f + 0.2f, Color.G * 2.2f + 0.2f, Color.B * 2.2f + 0.2f, 1.f);
			MID->SetVectorParameterValue(TEXT("Color"), Bright);
			B->Beam->SetMaterial(0, MID);
			B->BeamMID = MID;
		}
	B->Pivot->SetWorldRotation(FRotator(0.f, YawStart, 0.f));
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

	// Balayage : interpole le yaw du départ vers l'arrivée.
	const float Yaw = FMath::Lerp(Yaw0, Yaw1, a);
	Pivot->SetWorldRotation(FRotator(0.f, Yaw, 0.f));

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
