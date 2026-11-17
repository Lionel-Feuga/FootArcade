// Contrôleur joueur de FootArcade.

#include "FootPlayerController.h"
#include "FootArcade.h"
#include "FootPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"

void AFootPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Ajoute le contexte de mapping uniquement pour un joueur local.
	if (IsLocalPlayerController() && DefaultMappingContext)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, MappingPriority);
		}
	}
}

void AFootPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EIC)
	{
		UE_LOG(LogFootArcade, Error, TEXT("%s : l'InputComponent n'est pas un EnhancedInputComponent (vérifier les Project Settings > Input)."), *GetName());
		return;
	}

	if (MoveAction)
	{
		EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFootPlayerController::OnMove);
	}
	if (LookAction)
	{
		EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AFootPlayerController::OnLook);
	}
	if (KickAction)
	{
		EIC->BindAction(KickAction, ETriggerEvent::Started, this, &AFootPlayerController::OnKickPressed);
		EIC->BindAction(KickAction, ETriggerEvent::Completed, this, &AFootPlayerController::OnKickReleased);
		EIC->BindAction(KickAction, ETriggerEvent::Canceled, this, &AFootPlayerController::OnKickReleased);
	}
	if (PowerAction)
	{
		EIC->BindAction(PowerAction, ETriggerEvent::Started, this, &AFootPlayerController::OnPower);
	}
}

void AFootPlayerController::OnMove(const FInputActionValue& Value)
{
	if (AFootPlayer* FootPawn = GetPawn<AFootPlayer>())
	{
		FootPawn->DoMove(Value.Get<FVector2D>());
	}
}

void AFootPlayerController::OnLook(const FInputActionValue& Value)
{
	if (AFootPlayer* FootPawn = GetPawn<AFootPlayer>())
	{
		FootPawn->DoLook(Value.Get<FVector2D>());
	}
}

void AFootPlayerController::OnKickPressed(const FInputActionValue& Value)
{
	if (AFootPlayer* FootPawn = GetPawn<AFootPlayer>())
	{
		FootPawn->StartKickCharge();
	}
}

void AFootPlayerController::OnKickReleased(const FInputActionValue& Value)
{
	if (AFootPlayer* FootPawn = GetPawn<AFootPlayer>())
	{
		FootPawn->ReleaseKick();
	}
}

void AFootPlayerController::OnPower(const FInputActionValue& Value)
{
	if (AFootPlayer* FootPawn = GetPawn<AFootPlayer>())
	{
		FootPawn->UsePower();
	}
}
