using UnrealBuildTool;
public class VoidAndThunderEditorTarget : TargetRules {
 public VoidAndThunderEditorTarget(TargetInfo Target) : base(Target) {
  Type = TargetType.Editor; DefaultBuildSettings = BuildSettingsVersion.V7;
  IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
  ExtraModuleNames.AddRange(new string[] {"VoidAndThunder", "VoidAndThunderEditor"});
 }
}
