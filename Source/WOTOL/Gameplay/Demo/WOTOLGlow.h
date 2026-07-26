#pragma once

#include "CoreMinimal.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;

/**
 * Matériau ÉMISSIF (unlit) partagé pour le greybox.
 *
 * BasicShapeMaterial n'a qu'une couleur de BASE (albédo) : un objet peint avec
 * ne « brille » pas, il faut une lampe à côté qui éclaire le SOL. Ici on veut
 * l'inverse : que le MESH lui-même émette la lumière (projectiles, rayon laser,
 * cristal du Cristalliseur, bulbes de corail). On fabrique donc un matériau
 * unlit dont l'entrée Emissive = paramètre « Color ». Ainsi une couleur >1
 * fait rayonner l'objet (bloom) sans dépendre d'aucune lampe.
 */
namespace WOTOLGlow
{
	/** MODE FAIBLE GPU (grande bataille phase 3) : quand true, les VFX (rayons, projectiles)
	 *  n'ajoutent PAS de lampes dynamiques (très coûteuses en masse). Le MESH ÉMISSIF (+bloom)
	 *  reste actif -> le spectacle est conservé, mais on économise énormément de GPU quand des
	 *  dizaines de pouvoirs se déclenchent en même temps. Basculé par le directeur de démo. */
	extern bool bLowGpuVFX;

	/** Matériau parent émissif (construit une fois, mis en cache). */
	UMaterialInterface* GetGlowParent();

	/** MID émissif prêt à l'emploi : le mesh rayonne à la couleur HDR donnée. */
	UMaterialInstanceDynamic* MakeGlow(UObject* Outer, const FLinearColor& EmissiveHDR);

	/** Matériau parent MAT (rugueux, spéculaire ~0) : supprime l'aspect plastique/lisse
	 *  -> la roche, le sable et les unités accrochent la lumière en relief (contraste). */
	UMaterialInterface* GetMatteParent();

	/** MID mat prêt à l'emploi (couleur de base donnée, surface rugueuse non brillante). */
	UMaterialInstanceDynamic* MakeMatte(UObject* Outer, const FLinearColor& BaseColor);

	/** Matériau parent HALO : unlit ÉMISSIF + TRANSLUCIDE (paramètres « Color » et « Opacity »).
	 *  Sert à envelopper un modèle d'une aura lumineuse SANS masquer le modèle dessous
	 *  (coquille transparente colorée -> indicateur de sélection). */
	UMaterialInterface* GetHaloParent();

	/** MID halo prêt à l'emploi : coquille translucide émissive (couleur + opacité donnés). */
	UMaterialInstanceDynamic* MakeHalo(UObject* Outer, const FLinearColor& EmissiveHDR, float Opacity);

	/** Matériau parent SPRITE : unlit, MASQUÉ (alpha de la texture = découpe nette, pas de tri
	 *  de transparence à gérer) et DEUX FACES (bTwoSided — évite tout risque de face arrière
	 *  invisible si l'orientation du plan billboard n'est pas parfaite). Paramètre texture
	 *  « Texture ». Sert aux illustrations de bâtiments réelles (planches détourées) affichées
	 *  sur un plan en 3D à la place d'un kitbash de formes primitives. */
	UMaterialInterface* GetSpriteParent();

	/** MID sprite prêt à l'emploi : plan texturé avec la texture donnée. */
	UMaterialInstanceDynamic* MakeSprite(UObject* Outer, class UTexture2D* Texture);
}
