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

/** Un foyer de flammes de l'aura, attaché à un os du squelette. */
USTRUCT(BlueprintType)
struct FOOTARCADE_API FFootAuraFlamme
{
	GENERATED_BODY()

	/** Os (ou socket) du squelette auquel les flammes sont attachées (ex. "clavicle_l", "spine_05"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aura")
	FName Os;

	/** Décalage par rapport à l'os. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aura")
	FVector Decalage = FVector::ZeroVector;

	/** Taille des flammes de ce foyer. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aura")
	float Echelle = 0.5f;
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

	/** Effet Niagara (en boucle) des flammes qui montent du corps, joué sur chaque foyer. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aura")
	TObjectPtr<UNiagaraSystem> SystemeFlammes = nullptr;

	/** Foyers de flammes répartis sur le corps (épaules, bras, dos...). Toujours orientés vers le haut. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aura")
	TArray<FFootAuraFlamme> Flammes;

	/**
	 * Paramètre utilisateur float de l'effet qui règle la taille des flammes (les effets Vefects ignorent l'échelle
	 * du composant). Chaque foyer y écrit son Echelle. Vide = on utilise l'échelle du composant.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aura")
	FName ParametreTailleFlammes = TEXT("ScaleOverall");

	/** Effet Niagara optionnel (en boucle) joué en plus, centré sur le personnage (ex. anneau aux pieds). */
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
