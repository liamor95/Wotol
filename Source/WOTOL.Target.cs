using UnrealBuildTool;
using System.Collections.Generic;

public class WOTOLTarget : TargetRules
{
	public WOTOLTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("WOTOL");
	}
}
