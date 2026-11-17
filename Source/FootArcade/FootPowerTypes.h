// Types partagés du système de pouvoirs.

#pragma once

#include "CoreMinimal.h"
#include "FootPowerTypes.generated.h"

class UNiagaraSystem;
class UMaterialInterface;

/**
 * Famille de pouvoirs d'un personnage, identifiée par sa couleur.
 * Chaque personnage appartient à une seule famille et n'a accès qu'aux pouvoirs de celle-ci.
 */
UENUM(BlueprintType)
enum class EFootPowerFamily : uint8
{
	Aucune UMETA(DisplayName = "Aucune"),
	Bleu   UMETA(DisplayName = "Bleu - Capacités physiques"),
};

/** Réglages de l'aura affichée pendant qu'un pouvoir est actif. */
USTRUCT(BlueprintType)
struct FOOTARCADE_API FFootAuraSettings
{
	GENERATED_BODY()

	/** Couleur de la famille : teinte l'aura de contour (paramètre "Couleur") et sert au HUD. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aura")
	FLinearColor Couleur = FLinearColor::White;

	/**
	 * Matériau d'aura de contour, appliqué en surimpression (overlay) sur tous les meshes visibles du personnage.
	 * Paramètres attendus : "Couleur" (vecteur) et "Intensite" (scalaire 0-1, animé pour le fondu).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aura")
	TObjectPtr<UMaterialInterface> MateriauContour = nullptr;

	/** Effet Niagara optionnel (en boucle) joué en plus du contour, par ex. des flammes aux pieds. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aura")
	TObjectPtr<UNiagaraSystem> Systeme = nullptr;

	/** Position de l'aura par rapport au centre de la capsule (par défaut : aux pieds). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aura")
	FVector Decalage = FVector(0.f, 0.f, -90.f);

	/** Échelle appliquée au composant de l'aura. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aura")
	FVector Echelle = FVector(1.f);

	/** Paramètres utilisateur float du système Niagara à surcharger (ex. "ScaleOverall", "OverallSpeed"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aura")
	TMap<FName, float> ParametresFloat;
};
