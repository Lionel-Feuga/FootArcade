// Joueur de FootArcade.

#include "FootPlayer.h"
#include "FootBall.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"

AFootPlayer::AFootPlayer()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	// Le personnage s'oriente dans la direction du déplacement, pas de la caméra.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, 720.f, 0.f);
	Move->MaxWalkSpeed = 600.f;
	Move->MaxAcceleration = 2400.f;
	Move->BrakingDecelerationWalking = 2400.f;
	Move->JumpZVelocity = 500.f;
	Move->AirControl = 0.35f;
	// Force de poussée basse : le joueur ne doit pas "dribbler" le ballon par simple contact.
	Move->bEnablePhysicsInteraction = true;
	Move->PushForceFactor = 8000.f;

	// Caméra 3e personne.
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 450.f;
	CameraBoom->SocketOffset = FVector(0.f, 0.f, 80.f);
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Zone de frappe : sphère devant les pieds, en overlap sur tout sauf le canal Pawn.
	ZoneDeFrappe = CreateDefaultSubobject<USphereComponent>(TEXT("ZoneDeFrappe"));
	ZoneDeFrappe->SetupAttachment(RootComponent);
	ZoneDeFrappe->InitSphereRadius(60.f);
	ZoneDeFrappe->SetRelativeLocation(FVector(55.f, 0.f, -70.f));
	ZoneDeFrappe->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ZoneDeFrappe->SetCollisionObjectType(ECC_WorldDynamic);
	ZoneDeFrappe->SetCollisionResponseToAllChannels(ECR_Overlap);
	ZoneDeFrappe->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	ZoneDeFrappe->SetGenerateOverlapEvents(true);
	ZoneDeFrappe->SetCanEverAffectNavigation(false);
}

void AFootPlayer::DoMove(FVector2D MoveInput)
{
	if (!Controller)
	{
		return;
	}

	// Seul le lacet du contrôleur compte pour le déplacement au sol.
	const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
	const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(Forward, MoveInput.Y);
	AddMovementInput(Right, MoveInput.X);
}

void AFootPlayer::DoLook(FVector2D LookInput)
{
	if (!Controller)
	{
		return;
	}

	AddControllerYawInput(LookInput.X);
	AddControllerPitchInput(LookInput.Y);
}

bool AFootPlayer::TryKick(float Power)
{
	if (!ZoneDeFrappe)
	{
		return false;
	}

	if (Power <= 0.f)
	{
		Power = DefaultKickPower;
	}

	TArray<AActor*> Overlapping;
	ZoneDeFrappe->GetOverlappingActors(Overlapping, AFootBall::StaticClass());

	bool bKicked = false;
	for (AActor* Actor : Overlapping)
	{
		AFootBall* Ball = Cast<AFootBall>(Actor);
		if (!Ball)
		{
			continue;
		}

		// Direction : l'avant du joueur, à plat (la hauteur est gérée par le ballon).
		FVector Direction = GetActorForwardVector();
		Direction.Z = 0.f;
		Ball->Kick(Direction, Power);
		bKicked = true;
	}

	return bKicked;
}
