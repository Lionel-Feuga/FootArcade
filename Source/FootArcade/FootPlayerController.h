// Contrôleur joueur de FootArcade.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FootPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

/**
 * Contrôleur joueur basé sur Enhanced Input.
 * Le contexte de mapping et les actions sont à renseigner dans un Blueprint enfant.
 */
UCLASS()
class FOOTARCADE_API AFootPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	/** Contexte de mapping ajouté au joueur local au démarrage. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foot|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	/** Priorité du contexte de mapping. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foot|Input")
	int32 MappingPriority = 0;

	/** Action de déplacement (Axis2D). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foot|Input")
	TObjectPtr<UInputAction> MoveAction;

	/** Action de caméra (Axis2D). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foot|Input")
	TObjectPtr<UInputAction> LookAction;

	/** Action de frappe (Bool) : appui = charge, relâchement = tir. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foot|Input")
	TObjectPtr<UInputAction> KickAction;

	/** Action du pouvoir spécial (Bool). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foot|Input")
	TObjectPtr<UInputAction> PowerAction;

private:
	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnKickPressed(const FInputActionValue& Value);
	void OnKickReleased(const FInputActionValue& Value);
	void OnPower(const FInputActionValue& Value);
};
