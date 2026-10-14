using UnrealBuildTool;
public class VeilFallG0 : ModuleRules { public VeilFallG0(ReadOnlyTargetRules Target):base(Target) { PCHUsage=PCHUsageMode.UseExplicitOrSharedPCHs; PublicDependencyModuleNames.AddRange(new[]{"Core","CoreUObject","Engine","InputCore","EnhancedInput","UMG","AIModule","NavigationSystem","GameplayTasks"}); } }
