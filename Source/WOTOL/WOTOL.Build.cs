using UnrealBuildTool;

public class WOTOL : ModuleRules
{
	public WOTOL(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Permet les includes relatifs à la racine du module (ex: "Data/WOTOLTypes.h",
		// "Gameplay/Units/UnitBase.h") depuis n'importe quel fichier du module.
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore",
			"AIModule", "NavigationSystem", "GameplayTasks",
			"UMG", "SlateCore", "Niagara"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate"
		});
	}
}
