// Pouvoir bleu : super vitesse.

#pragma once

#include "CoreMinimal.h"
#include "FootPowerComponent.h"
#include "FootPower_SuperVitesse.generated.h"

/**
 * Famille Bleu (capacités physiques) : le joueur court beaucoup plus vite
 * et accélère plus fort pendant la durée du pouvoir.
 */
UCLASS(Blueprintable, ClassGroup = (Foot), meta = (BlueprintSpawnableComponent))
class FOOTARCADE_API UFootPower_SuperVitesse : public UFootPowerComponent
{
	GENERATED_BODY()

public:
	UFootPower_SuperVitesse();

protected:
	virtual void OnPowerStart() override;
	virtual void OnPowerEnd() override;

	/** Multiplicateur de la vitesse de course maximale. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foot|Pouvoir|Super vitesse", meta = (ClampMin = "1.0"))
	float MultiplicateurVitesse = 1.8f;

	/** Multiplicateur de l'accélération et du freinage (pour garder des appuis nets à haute vitesse). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foot|Pouvoir|Super vitesse", meta = (ClampMin = "1.0"))
	float MultiplicateurAcceleration = 2.5f;

private:
	float VitesseOrigine = 0.f;
	float AccelerationOrigine = 0.f;
	float FreinageOrigine = 0.f;
};
