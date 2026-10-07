using UnrealBuildTool;
public class VoidAndThunder : ModuleRules {
 public VoidAndThunder(ReadOnlyTargetRules Target) : base(Target) {
  PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
  PublicDependencyModuleNames.AddRange(new string[] {"Core","CoreUObject","Engine","InputCore","AIModule","EnhancedInput","GameplayAbilities","GameplayTags","GameplayTasks","UMG","Niagara","OnlineSubsystem","OnlineSubsystemUtils","Json","JsonUtilities"});
  PrivateDependencyModuleNames.AddRange(new string[] {"Slate","SlateCore","RHI"});
 }
}
