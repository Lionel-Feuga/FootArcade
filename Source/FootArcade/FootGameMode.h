// Mode de jeu de base de FootArcade.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FootGameMode.generated.h"

/**
 * Mode de jeu de base : définit le pawn et le contrôleur par défaut.
 * Les règles du match (score, chrono, etc.) viendront dans les jalons suivants.
 */
UCLASS()
class FOOTARCADE_API AFootGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFootGameMode();
};
