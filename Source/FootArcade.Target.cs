// Cible "jeu" du projet FootArcade.

using UnrealBuildTool;
using System.Collections.Generic;

public class FootArcadeTarget : TargetRules
{
	public FootArcadeTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		// MSVC 14.51+ ne compile pas UE 5.8 (hash_map manquant dans nvtesslib) :
		// on force la famille 14.50 (VS 2026 18.0). La syntaxe "<14.51" (lue dans
		// MicrosoftPlatformSDK.TryGetToolChainDir) prend la plus récente version < 14.51,
		// alors que "14.50" seul ne correspond à aucune version exacte installée.
		WindowsPlatform.CompilerVersion = "<14.51";

		ExtraModuleNames.Add("FootArcade");
	}
}
