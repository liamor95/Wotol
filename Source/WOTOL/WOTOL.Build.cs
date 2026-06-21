using UnrealBuildTool;

public class WOTOL : ModuleRules
{
	public WOTOL(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore",
			"AIModule", "NavigationSystem", "GameplayTasks",
			"UMG", "SlateCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate"
		});
	}
}
