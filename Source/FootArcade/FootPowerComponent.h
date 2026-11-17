// Base des pouvoirs spéciaux.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FootPowerTypes.h"
#include "FootPowerComponent.generated.h"

class AFootPlayer;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFootPowerStateChanged, bool, bActif);

/**
 * Pouvoir spécial d'un joueur : activation, durée, temps de recharge et aura.
 * Les pouvoirs concrets dérivent de cette classe et surchargent OnPowerStart / OnPowerEnd.
 */
UCLASS(Abstract, Blueprintable, ClassGroup = (Foot))
class FOOTARCADE_API UFootPowerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFootPowerComponent();

	/** Tente d'activer le pouvoir. Échoue s'il est déjà actif ou en recharge. */
	UFUNCTION(BlueprintCallable, Category = "Foot|Pouvoir")
	bool TryActivatePower();

	/** Arrête le pouvoir avant la fin de sa durée. */
	UFUNCTION(BlueprintCallable, Category = "Foot|Pouvoir")
	void StopPower();

	UFUNCTION(BlueprintPure, Category = "Foot|Pouvoir")
	bool IsPowerActive() const { return bActif; }

	/** Temps de recharge restant en secondes (0 = prêt). */
	UFUNCTION(BlueprintPure, Category = "Foot|Pouvoir")
	float GetCooldownRemaining() const;

	UFUNCTION(BlueprintPure, Category = "Foot|Pouvoir")
	EFootPowerFamily GetFamille() const { return Famille; }

	/** Appelé quand le pouvoir démarre (true) ou s'arrête (false). Pour l'UI et les effets. */
	UPROPERTY(BlueprintAssignable, Category = "Foot|Pouvoir")
	FFootPowerStateChanged OnPowerStateChanged;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Effet du pouvoir au démarrage. */
	virtual void OnPowerStart() {}

	/** Retire l'effet du pouvoir. */
	virtual void OnPowerEnd() {}

	/** Événements Blueprint, pour ajouter sons, caméra, etc. sans C++. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Foot|Pouvoir", meta = (DisplayName = "Pouvoir démarré"))
	void K2_OnPowerStart();

	UFUNCTION(BlueprintImplementableEvent, Category = "Foot|Pouvoir", meta = (DisplayName = "Pouvoir terminé"))
	void K2_OnPowerEnd();

	AFootPlayer* GetFootPlayer() const;

	/** Famille (couleur) à laquelle appartient ce pouvoir. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foot|Pouvoir")
	EFootPowerFamily Famille = EFootPowerFamily::Aucune;

	/** Nom affiché du pouvoir. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foot|Pouvoir")
	FText NomAffiche;

	/** Durée d'effet en secondes. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foot|Pouvoir", meta = (ClampMin = "0.1"))
	float Duree = 3.f;

	/** Temps de recharge en secondes, compté à partir de la fin de l'effet. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foot|Pouvoir", meta = (ClampMin = "0.0"))
	float Recharge = 6.f;

private:
	void EndPowerInternal();

	bool bActif = false;
	float RechargeFinTemps = 0.f;
	FTimerHandle TimerDuree;
};
