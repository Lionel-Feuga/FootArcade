// Pouvoir bleu : super vitesse.

#include "FootPower_SuperVitesse.h"
#include "FootPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"

UFootPower_SuperVitesse::UFootPower_SuperVitesse()
{
	Famille = EFootPowerFamily::Bleu;
	NomAffiche = NSLOCTEXT("FootArcade", "SuperVitesse", "Super vitesse");
	Duree = 3.f;
	Recharge = 6.f;
}

void UFootPower_SuperVitesse::OnPowerStart()
{
	AFootPlayer* Player = GetFootPlayer();
	UCharacterMovementComponent* Move = Player ? Player->GetCharacterMovement() : nullptr;
	if (!Move)
	{
		return;
	}

	VitesseOrigine = Move->MaxWalkSpeed;
	AccelerationOrigine = Move->MaxAcceleration;
	FreinageOrigine = Move->BrakingDecelerationWalking;

	Move->MaxWalkSpeed = VitesseOrigine * MultiplicateurVitesse;
	Move->MaxAcceleration = AccelerationOrigine * MultiplicateurAcceleration;
	Move->BrakingDecelerationWalking = FreinageOrigine * MultiplicateurAcceleration;
}

void UFootPower_SuperVitesse::OnPowerEnd()
{
	AFootPlayer* Player = GetFootPlayer();
	UCharacterMovementComponent* Move = Player ? Player->GetCharacterMovement() : nullptr;
	if (!Move || VitesseOrigine <= 0.f)
	{
		return;
	}

	Move->MaxWalkSpeed = VitesseOrigine;
	Move->MaxAcceleration = AccelerationOrigine;
	Move->BrakingDecelerationWalking = FreinageOrigine;
}
