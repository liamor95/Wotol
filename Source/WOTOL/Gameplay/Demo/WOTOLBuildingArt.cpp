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
	// BUG D'ASSET CONFIRME (retour terrain 31/07/2026, recherche dediee) : Content/UI/
	// CityBackdropNoxeens.png n'est PAS une illustration de cite (contrairement a
	// CityBackdropAquiloris.png, une vraie planche de cite-cristal) — c'est une scene de
	// recif/grotte bioluminescente SANS aucune architecture. Comme ce fond occupe TOUT le
	// cadre de la camera orthographique (design voulu, cf. commentaire dans
	// WOTOLCityEnvironment.cpp), la mauvaise image donnait l'impression d'un simple aplat bleu
	// plat sans cite du tout. En attendant une vraie illustration de cite Noxeens (meme esprit
	// que la planche Aquiloris, fournie par Liamor), on N'AFFICHE PAS ce fond incorrect -> la
	// vraie geometrie 3D de la cite (hub + anneau de batiments + decor organique) redevient
	// visible au premier plan, plus sobre mais correcte, plutot que masquee par une image hors
	// sujet. Retirer ce garde-fou des qu'un vrai CityBackdropNoxeens.png (illustration de cite)
	// est fourni.
	if (Faction == EFactionID::Noxeens) return nullptr;
	const TCHAR* Fac = FactionPrefix(Faction);
	if (!Fac) return nullptr;
	return LoadCached(FString::Printf(TEXT("CityBackdrop%s.png"), Fac));
}

UTexture2D* WOTOLBuildingArt::GetLoadingBackdrop(EFactionID Faction)
{
	const TCHAR* Fac = FactionPrefix(Faction);
	if (!Fac) return nullptr;
	return LoadCached(FString::Printf(TEXT("Loading%s.png"), Fac));
}
