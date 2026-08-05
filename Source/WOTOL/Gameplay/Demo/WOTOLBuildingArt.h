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

	// Bâtiment-siège (Noyau Cristallin / Trône des profondeurs). Aquiloris : planche pas
	// encore reçue de Liamor -> retourne nullptr (repli sur l'ancien comportement, pas de
	// régression). Noxeens : "Trône des profondeurs" disponible depuis le 02/08/2026.
	UTexture2D* GetSiegeBuildingIcon(EFactionID Faction);

	// Bâtiment de défense TERRITORIALE principal (Bastion Cristallin / Enceinte Noxéenne),
	// affiché dans la gestion du territoire — distinct de la tourelle individuelle
	// installable ci-dessus (GetDefenseBuildingIcon).
	UTexture2D* GetTerritoryBastionIcon(EFactionID Faction);

	// Fond de cité grand format (vue Cité 3D uniquement).
	UTexture2D* GetCityBackdrop(EFactionID Faction);

	// Fond de l'écran de CHARGEMENT (DrawLoadingScreen), affiché à chaque retour à la cité.
	// EFactionID::None (avant tout choix de faction) -> nullptr, aucune image attendue.
	UTexture2D* GetLoadingBackdrop(EFactionID Faction);
}
