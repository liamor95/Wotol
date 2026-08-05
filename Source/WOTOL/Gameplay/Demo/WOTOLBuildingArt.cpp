#include "WOTOLBuildingArt.h"
#include "Engine/Texture2D.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	// Cache par chemin de fichier (une seule tentative de chargement par fichier, comme
	// AWOTOLDemoHUD::GetMenuBackground). Référence FORTE -> pas collecté par le GC.
	TMap<FString, TStrongObjectPtr<UTexture2D>> GTextureCache;
	TSet<FString> GTried;

	UTexture2D* LoadCached(const FString& FileName)
	{
		if (GTried.Contains(FileName))
		{
			const TStrongObjectPtr<UTexture2D>* Found = GTextureCache.Find(FileName);
			return Found ? Found->Get() : nullptr;
		}
		GTried.Add(FileName);

		const FString PngPath = FPaths::ProjectContentDir() / TEXT("UI") / FileName;
		if (!FPaths::FileExists(PngPath))
		{
			return nullptr;
		}
		UTexture2D* Loaded = FImageUtils::ImportFileAsTexture2D(PngPath);
		if (Loaded)
		{
			GTextureCache.Add(FileName, TStrongObjectPtr<UTexture2D>(Loaded));
		}
		return Loaded;
	}

	const TCHAR* CategorySuffix(EDemoUnitCategory Cat)
	{
		switch (Cat)
		{
			case EDemoUnitCategory::Infanterie: return TEXT("Infanterie");
			case EDemoUnitCategory::Distance:   return TEXT("Distance");
			case EDemoUnitCategory::Montee:     return TEXT("Montee");
			case EDemoUnitCategory::Speciale:   return TEXT("Speciale");
			case EDemoUnitCategory::Mythique:   return TEXT("Mythique");
			default:                            return nullptr; // Chef : pas d'image dediee
		}
	}

	const TCHAR* FactionPrefix(EFactionID Faction)
	{
		if (Faction == EFactionID::Aquiloris) return TEXT("Aquiloris");
		if (Faction == EFactionID::Noxeens)   return TEXT("Noxeens");
		return nullptr; // hors scope demo
	}
}

UTexture2D* WOTOLBuildingArt::GetBuildingIcon(EFactionID Faction, EDemoUnitCategory Category)
{
	const TCHAR* Fac = FactionPrefix(Faction);
	const TCHAR* Cat = CategorySuffix(Category);
	if (!Fac || !Cat) return nullptr;
	return LoadCached(FString::Printf(TEXT("Buildings/Building%s%s.png"), Fac, Cat));
}

UTexture2D* WOTOLBuildingArt::GetCentralBuildingIcon(EFactionID Faction)
{
	const TCHAR* Fac = FactionPrefix(Faction);
	if (!Fac) return nullptr;
	return LoadCached(FString::Printf(TEXT("Buildings/Building%sCentral.png"), Fac));
}

UTexture2D* WOTOLBuildingArt::GetDefenseBuildingIcon(EFactionID Faction)
{
	const TCHAR* Fac = FactionPrefix(Faction);
	if (!Fac) return nullptr;
	return LoadCached(FString::Printf(TEXT("Buildings/Building%sDefense.png"), Fac));
}

UTexture2D* WOTOLBuildingArt::GetSiegeBuildingIcon(EFactionID Faction)
{
	const TCHAR* Fac = FactionPrefix(Faction);
	if (!Fac) return nullptr;
	return LoadCached(FString::Printf(TEXT("Buildings/Building%sSiege.png"), Fac));
}

UTexture2D* WOTOLBuildingArt::GetTerritoryBastionIcon(EFactionID Faction)
{
	const TCHAR* Fac = FactionPrefix(Faction);
	if (!Fac) return nullptr;
	return LoadCached(FString::Printf(TEXT("Buildings/Building%sBastion.png"), Fac));
}

UTexture2D* WOTOLBuildingArt::GetCityBackdrop(EFactionID Faction)
{
	// Garde-fou du 31/07/2026 RETIRE le 02/08/2026 : Content/UI/CityBackdropNoxeens.png a été
	// remplacé par une vraie illustration de cité organique (tours/dômes bioluminescents reliés
	// par des chemins, même esprit que la planche Aquiloris), fournie par Liamor. Le fond
	// incorrect (scène de récif sans architecture) qui justifiait de désactiver l'affichage
	// côté Noxéens n'existe plus.
	const TCHAR* Fac = FactionPrefix(Faction);
	if (!Fac) return nullptr;
	return LoadCached(FString::Printf(TEXT("CityBackdrop%s.png"), Fac));
}
