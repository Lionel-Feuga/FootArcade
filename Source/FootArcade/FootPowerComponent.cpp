// Base des pouvoirs spéciaux.

#include "FootPowerComponent.h"
#include "FootArcade.h"
#include "FootPlayer.h"
#include "Engine/World.h"
#include "TimerManager.h"

UFootPowerComponent::UFootPowerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

AFootPlayer* UFootPowerComponent::GetFootPlayer() const
{
	return Cast<AFootPlayer>(GetOwner());
}

float UFootPowerComponent::GetCooldownRemaining() const
{
	const UWorld* World = GetWorld();
	return World ? FMath::Max(0.f, RechargeFinTemps - World->GetTimeSeconds()) : 0.f;
}

bool UFootPowerComponent::TryActivatePower()
{
	UWorld* World = GetWorld();
	if (!World || bActif || GetCooldownRemaining() > 0.f)
	{
		return false;
	}

	bActif = true;
	OnPowerStart();
	if (AFootPlayer* Player = GetFootPlayer())
	{
		Player->ActivateAura();
	}
	K2_OnPowerStart();
	OnPowerStateChanged.Broadcast(true);

	World->GetTimerManager().SetTimer(TimerDuree, this, &UFootPowerComponent::EndPowerInternal, Duree, false);
	UE_LOG(LogFootArcade, Log, TEXT("%s : pouvoir %s activé (%.1fs)"), *GetNameSafe(GetOwner()), *GetName(), Duree);
	return true;
}

void UFootPowerComponent::StopPower()
{
	if (bActif)
	{
		EndPowerInternal();
	}
}

void UFootPowerComponent::EndPowerInternal()
{
	if (!bActif)
	{
		return;
	}

	bActif = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimerDuree);
		RechargeFinTemps = World->GetTimeSeconds() + Recharge;
	}

	OnPowerEnd();
	if (AFootPlayer* Player = GetFootPlayer())
	{
		Player->StopAura();
	}
	K2_OnPowerEnd();
	OnPowerStateChanged.Broadcast(false);
}

void UFootPowerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopPower();
	Super::EndPlay(EndPlayReason);
}
