// Mode de jeu de base de FootArcade.

#include "FootGameMode.h"
#include "FootPlayer.h"
#include "FootPlayerController.h"

AFootGameMode::AFootGameMode()
{
	// Classes par défaut ; à surcharger par des Blueprints enfants (BP_FootGameMode) si besoin.
	DefaultPawnClass = AFootPlayer::StaticClass();
	PlayerControllerClass = AFootPlayerController::StaticClass();
}
