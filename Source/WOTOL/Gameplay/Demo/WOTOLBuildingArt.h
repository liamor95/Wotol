#pragma once

#include "CoreMinimal.h"
#include "Data/WOTOLTypes.h"
#include "DemoFlowSubsystem.h" // EDemoUnitCategory

class UTexture2D;

// ─────────────────────────────────────────────────────────────────────────────
// CHARGEUR PARTAGÉ DES ILLUSTRATIONS DE BÂTIMENTS RÉELLES (planches officielles de Liamor,
// recadrées + détourées, 25/07/2026). Même mécanisme que AWOTOLDemoHUD::GetMenuBackground
// (PNG chargé directement depuis Content/UI/, sans import manuel dans l'éditeur), mais
// centralisé ici pour être réutilisable à la fois par le HUD 2D (fiche technique) et les
// acteurs 3D (AWOTOLCityBuildingProp, AWOTOLCityEnvironment). Hors scope démo (Thalassidra/
// Muréniens/Pirates Abyssaux) : retourne toujours nullptr pour elles, aucun fichier attendu.
// ─────────────────────────────────────────────────────────────────────────────
namespace WOTOLBuildingArt
{
	// Bâtiment de recrutement par catégorie (Infanterie/Distance/Montée/Spéciale/Mythique).
	// Category::Chef n'a pas d'image dédiée (pas de carte de recrutement pour le Chef).
	UTexture2D* GetBuildingIcon(EFactionID Faction, EDemoUnitCategory Category);

	// Bâtiment central (Cristalliseur / Abysalyseur).
	UTexture2D* GetCentralBuildingIcon(EFactionID Faction);

	// Structure défensive individuelle (Tourelle hydrocristalline / Œil bioluminal).
	UTexture2D* GetDefenseBuildingIcon(EFactionID Faction);

	// Fond de cité grand format (vue Cité 3D uniquement).
	UTexture2D* GetCityBackdrop(EFactionID Faction);
}
