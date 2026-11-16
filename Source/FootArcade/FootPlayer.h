// Joueur de FootArcade.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FootPlayer.generated.h"

class USpringArmComponent;
class UCameraComponent;
class USphereComponent;

/**
 * Personnage joueur : caméra à la 3e personne sur spring arm,
 * et une zone de frappe sphérique devant les pieds pour toucher le ballon.
 */
UCLASS()
class FOOTARCADE_API AFootPlayer : public ACharacter
{
	GENERATED_BODY()

public:
	AFootPlayer();

	/** Déplacement dans le plan, relatif à l'orientation de la caméra (X = droite, Y = avant). */
	UFUNCTION(BlueprintCallable, Category = "Foot|Joueur")
	void DoMove(FVector2D MoveInput);

	/** Rotation de la caméra (X = lacet, Y = tangage). */
	UFUNCTION(BlueprintCallable, Category = "Foot|Joueur")
	void DoLook(FVector2D LookInput);

	/**
	 * Cherche les ballons qui chevauchent la zone de frappe et les frappe.
	 * @return true si au moins un ballon a été frappé.
	 */
	UFUNCTION(BlueprintCallable, Category = "Foot|Joueur")
	bool TryKick(float Power = 1.f);

	USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	USphereComponent* GetZoneDeFrappe() const { return ZoneDeFrappe; }

protected:
	/** Bras de caméra derrière le joueur. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Foot|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	/** Caméra qui suit le joueur. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Foot|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	/** Zone devant les pieds dans laquelle le ballon peut être frappé (ignore le canal Pawn). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Foot|Frappe")
	TObjectPtr<USphereComponent> ZoneDeFrappe;

	/** Puissance utilisée quand aucune puissance n'est précisée par l'entrée. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foot|Frappe", meta = (ClampMin = "0.0"))
	float DefaultKickPower = 1.f;
};
