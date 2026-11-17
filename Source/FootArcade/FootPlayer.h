// Joueur de FootArcade.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FootPowerTypes.h"
#include "FootPlayer.generated.h"

class USpringArmComponent;
class UCameraComponent;
class USphereComponent;
class UNiagaraComponent;
class UFootPowerComponent;

/**
 * Personnage joueur : caméra à la 3e personne sur spring arm, zone de frappe devant les pieds,
 * tir visé à la caméra avec puissance chargée, et un pouvoir spécial lié à sa famille (couleur).
 */
UCLASS()
class FOOTARCADE_API AFootPlayer : public ACharacter
{
	GENERATED_BODY()

public:
	AFootPlayer();

	virtual void Tick(float DeltaSeconds) override;

	// ---------------------------------------------------------------- Déplacement / caméra

	/** Déplacement dans le plan, relatif à l'orientation de la caméra (X = droite, Y = avant). */
	UFUNCTION(BlueprintCallable, Category = "Foot|Joueur")
	void DoMove(FVector2D MoveInput);

	/** Rotation de la caméra (X = lacet, Y = tangage). */
	UFUNCTION(BlueprintCallable, Category = "Foot|Joueur")
	void DoLook(FVector2D LookInput);

	// ---------------------------------------------------------------- Tir

	/** Début d'appui sur Tir : commence à charger la puissance. */
	UFUNCTION(BlueprintCallable, Category = "Foot|Tir")
	void StartKickCharge();

	/** Relâchement de Tir : frappe avec la puissance chargée. @return true si un ballon a été frappé. */
	UFUNCTION(BlueprintCallable, Category = "Foot|Tir")
	bool ReleaseKick();

	/**
	 * Frappe immédiatement les ballons présents dans la zone de frappe, dans la direction visée.
	 * @param Power  Multiplicateur de puissance (1 = frappe normale).
	 */
	UFUNCTION(BlueprintCallable, Category = "Foot|Tir")
	bool TryKick(float Power = 1.f);

	/** Direction de tir actuelle : lacet de la caméra, hauteur selon le tangage de la caméra. */
	UFUNCTION(BlueprintPure, Category = "Foot|Tir")
	FVector GetAimDirection() const;

	/** Charge du tir en cours entre 0 et 1 (0 si pas de charge). */
	UFUNCTION(BlueprintPure, Category = "Foot|Tir")
	float GetKickCharge() const;

	UFUNCTION(BlueprintPure, Category = "Foot|Tir")
	bool IsChargingKick() const { return bChargingKick; }

	// ---------------------------------------------------------------- Pouvoir et aura

	/** Active le pouvoir du joueur. */
	UFUNCTION(BlueprintCallable, Category = "Foot|Pouvoir")
	bool UsePower();

	UFUNCTION(BlueprintPure, Category = "Foot|Pouvoir")
	UFootPowerComponent* GetPower() const { return Pouvoir; }

	UFUNCTION(BlueprintPure, Category = "Foot|Pouvoir")
	EFootPowerFamily GetFamille() const { return Famille; }

	/** Couleur de la famille du joueur (pour l'UI). */
	UFUNCTION(BlueprintPure, Category = "Foot|Pouvoir")
	FLinearColor GetCouleurFamille() const;

	/** Affiche l'aura de la famille du joueur. */
	UFUNCTION(BlueprintCallable, Category = "Foot|Pouvoir")
	void ActivateAura();

	/** Cache l'aura. */
	UFUNCTION(BlueprintCallable, Category = "Foot|Pouvoir")
	void StopAura();

	USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	USphereComponent* GetZoneDeFrappe() const { return ZoneDeFrappe; }
	UNiagaraComponent* GetAura() const { return Aura; }

protected:
	virtual void BeginPlay() override;

	const FFootAuraSettings* FindAuraSettings() const;

	/** Bras de caméra derrière le joueur. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Foot|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	/** Caméra qui suit le joueur. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Foot|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	/** Zone devant les pieds dans laquelle le ballon peut être frappé (ignore le canal Pawn). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Foot|Frappe")
	TObjectPtr<USphereComponent> ZoneDeFrappe;

	/** Effet d'aura, éteint par défaut, allumé pendant un pouvoir. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Foot|Pouvoir")
	TObjectPtr<UNiagaraComponent> Aura;

	// ---------------------------------------------------------------- Réglages tir

	/** Puissance d'une frappe sans charge (tapotement = passe). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foot|Tir", meta = (ClampMin = "0.0"))
	float PuissanceMin = 0.4f;

	/** Puissance d'une frappe complètement chargée. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foot|Tir", meta = (ClampMin = "0.0"))
	float PuissanceMax = 1.4f;

	/** Temps pour charger complètement le tir, en secondes. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foot|Tir", meta = (ClampMin = "0.05"))
	float TempsDeCharge = 0.8f;

	/** Décalage ajouté au tangage de la caméra : avec la caméra légèrement plongeante, le tir reste à ras du sol. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foot|Tir")
	float DecalageVisee = 12.f;

	/** Angle de tir minimal au-dessus de l'horizontale, en degrés. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foot|Tir", meta = (ClampMin = "-10.0", ClampMax = "80.0"))
	float AngleTirMin = 2.f;

	/** Angle de tir maximal (lob), en degrés. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foot|Tir", meta = (ClampMin = "0.0", ClampMax = "85.0"))
	float AngleTirMax = 45.f;

	/** Affiche une flèche de visée pendant la charge (aide de prototypage, absente des builds Shipping). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foot|Tir")
	bool bAfficherVisee = true;

	// ---------------------------------------------------------------- Réglages pouvoir

	/** Famille (couleur) du personnage : détermine les pouvoirs auxquels il a accès. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foot|Pouvoir")
	EFootPowerFamily Famille = EFootPowerFamily::Aucune;

	/** Pouvoir du personnage ; doit appartenir à la même famille. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foot|Pouvoir")
	TSubclassOf<UFootPowerComponent> ClassePouvoir;

	/** Aura et couleur de chaque famille. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foot|Pouvoir")
	TMap<EFootPowerFamily, FFootAuraSettings> AurasParFamille;

	/** Durée du fondu de l'aura à la fin d'un pouvoir avant extinction complète, en secondes. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foot|Pouvoir", meta = (ClampMin = "0.0"))
	float DureeFonduAura = 0.4f;

	/** Instance du pouvoir créée au début de la partie. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Foot|Pouvoir")
	TObjectPtr<UFootPowerComponent> Pouvoir;

private:
	bool bChargingKick = false;
	float ChargeStartTime = 0.f;
	FTimerHandle TimerExtinctionAura;
};
