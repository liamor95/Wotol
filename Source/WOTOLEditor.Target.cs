using UnrealBuildTool;
using System.Collections.Generic;

public class WOTOLEditorTarget : TargetRules
{
	public WOTOLEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("WOTOL");
	}
}
