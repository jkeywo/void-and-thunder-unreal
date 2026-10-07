using UnrealBuildTool;
public class VoidAndThunderEditor : ModuleRules {
 public VoidAndThunderEditor(ReadOnlyTargetRules Target) : base(Target) {
  PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
  PublicDependencyModuleNames.AddRange(new string[] {"Core","CoreUObject","Engine","VoidAndThunder","UnrealEd","AssetRegistry","Json","JsonUtilities","EnhancedInput","InputCore","AssetTools","UMG","UMGEditor","KismetCompiler","Niagara","GameplayTags","MeshDescription","StaticMeshDescription"});
 }
}
