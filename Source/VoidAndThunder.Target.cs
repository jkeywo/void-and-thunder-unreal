using UnrealBuildTool;
public class VoidAndThunderTarget : TargetRules {
 public VoidAndThunderTarget(TargetInfo Target) : base(Target) {
  Type = TargetType.Game; DefaultBuildSettings = BuildSettingsVersion.V7;
  IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
  ExtraModuleNames.Add("VoidAndThunder");
 }
}
