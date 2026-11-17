// Ballon de FootArcade.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FootBall.generated.h"

class UStaticMeshComponent;

/**
 * Ballon physique : un mesh statique racine qui simule la physique,
 * avec CCD activé pour ne pas traverser les poteaux ou les filets à haute vitesse.
 */
UCLASS()
class FOOTARCADE_API AFootBall : public AActor
{
	GENERATED_BODY()

public:
	AFootBall();

	/**
	 * Frappe le ballon.
	 * @param Direction  Direction du tir, hauteur comprise (normalisée en interne).
	 * @param Power      Multiplicateur de puissance (1 = frappe normale).
	 */
	UFUNCTION(BlueprintCallable, Category = "Foot|Ballon")
	void Kick(FVector Direction, float Power = 1.f);

	/** Accès au mesh du ballon. */
	UStaticMeshComponent* GetBallMesh() const { return BallMesh; }

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Applique masse et amortissements au corps physique. */
	void ApplyPhysicsSettings();

	/** Mesh racine qui porte la simulation physique. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Foot|Ballon")
	TObjectPtr<UStaticMeshComponent> BallMesh;

	/** Masse du ballon en kg (un vrai ballon fait ~0,43 kg ; plus lourd = plus stable en arcade). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foot|Ballon|Physique", meta = (ClampMin = "0.01"))
	float MassKg = 0.45f;

	/** Amortissement linéaire (freine le ballon qui roule / vole). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foot|Ballon|Physique", meta = (ClampMin = "0.0"))
	float LinearDamping = 0.3f;

	/** Amortissement angulaire (freine la rotation du ballon). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foot|Ballon|Physique", meta = (ClampMin = "0.0"))
	float AngularDamping = 0.6f;

	/** Vitesse donnée au ballon par une frappe de puissance 1, en cm/s. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foot|Ballon|Tir", meta = (ClampMin = "0.0"))
	float KickForce = 2200.f;
};
