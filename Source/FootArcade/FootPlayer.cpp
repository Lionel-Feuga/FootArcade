// Joueur de FootArcade.

#include "FootPlayer.h"
#include "FootArcade.h"
#include "FootBall.h"
#include "FootPowerComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"

AFootPlayer::AFootPlayer()
{
	// Le tick ne sert qu'à l'affichage de la visée pendant la charge.
	PrimaryActorTick.bCanEverTick = true;

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

	// Aura : attachée au personnage, éteinte tant qu'aucun pouvoir n'est actif.
	Aura = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Aura"));
	Aura->SetupAttachment(RootComponent);
	Aura->SetAutoActivate(false);
	Aura->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AFootPlayer::BeginPlay()
{
	Super::BeginPlay();

	if (ClassePouvoir)
	{
		const UFootPowerComponent* PowerCDO = ClassePouvoir->GetDefaultObject<UFootPowerComponent>();
		if (PowerCDO->GetFamille() != Famille)
		{
			UE_LOG(LogFootArcade, Warning, TEXT("%s : le pouvoir %s n'appartient pas à la famille du personnage."),
				*GetName(), *ClassePouvoir->GetName());
		}

		Pouvoir = NewObject<UFootPowerComponent>(this, ClassePouvoir, TEXT("Pouvoir"));
		Pouvoir->RegisterComponent();
	}
}

// -------------------------------------------------------------------- Déplacement / caméra

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

// -------------------------------------------------------------------- Tir

FVector AFootPlayer::GetAimDirection() const
{
	// Sans contrôleur (IA future, tests) : on tire droit devant, à ras du sol.
	FRotator Aim = Controller ? Controller->GetControlRotation() : GetActorRotation();

	// Tangage de caméra ramené dans [-180, 180], décalé puis borné.
	const float CameraPitch = FRotator::NormalizeAxis(Aim.Pitch);
	Aim.Pitch = FMath::Clamp(CameraPitch + DecalageVisee, AngleTirMin, AngleTirMax);
	Aim.Roll = 0.f;
	return Aim.Vector();
}

float AFootPlayer::GetKickCharge() const
{
	if (!bChargingKick || !GetWorld())
	{
		return 0.f;
	}
	return FMath::Clamp((GetWorld()->GetTimeSeconds() - ChargeStartTime) / TempsDeCharge, 0.f, 1.f);
}

void AFootPlayer::StartKickCharge()
{
	bChargingKick = true;
	ChargeStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
}

bool AFootPlayer::ReleaseKick()
{
	if (!bChargingKick)
	{
		return false;
	}

	const float Power = FMath::Lerp(PuissanceMin, PuissanceMax, GetKickCharge());
	bChargingKick = false;
	return TryKick(Power);
}

bool AFootPlayer::TryKick(float Power)
{
	if (!ZoneDeFrappe)
	{
		return false;
	}

	TArray<AActor*> Overlapping;
	ZoneDeFrappe->GetOverlappingActors(Overlapping, AFootBall::StaticClass());
	if (Overlapping.Num() == 0)
	{
		return false;
	}

	const FVector Direction = GetAimDirection();

	// Le joueur se tourne vers la cible au moment de la frappe.
	SetActorRotation(FRotator(0.f, Direction.Rotation().Yaw, 0.f));

	for (AActor* Actor : Overlapping)
	{
		if (AFootBall* Ball = Cast<AFootBall>(Actor))
		{
			Ball->Kick(Direction, Power);
		}
	}
	return true;
}

void AFootPlayer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

#if ENABLE_DRAW_DEBUG
	if (bAfficherVisee && bChargingKick && IsLocallyControlled())
	{
		// Flèche partant des pieds : direction = visée, longueur et couleur = charge.
		const float Charge = GetKickCharge();
		const FVector Start = ZoneDeFrappe ? ZoneDeFrappe->GetComponentLocation() : GetActorLocation();
		const FVector End = Start + GetAimDirection() * FMath::Lerp(150.f, 600.f, Charge);
		const FColor Color = FLinearColor::LerpUsingHSV(FLinearColor::Green, FLinearColor::Red, Charge).ToFColor(true);
		DrawDebugDirectionalArrow(GetWorld(), Start, End, 40.f, Color, false, -1.f, 0, 4.f);
	}
#endif
}

// -------------------------------------------------------------------- Pouvoir et aura

bool AFootPlayer::UsePower()
{
	return Pouvoir && Pouvoir->TryActivatePower();
}

const FFootAuraSettings* AFootPlayer::FindAuraSettings() const
{
	return AurasParFamille.Find(Famille);
}

FLinearColor AFootPlayer::GetCouleurFamille() const
{
	const FFootAuraSettings* Settings = FindAuraSettings();
	return Settings ? Settings->Couleur : FLinearColor::White;
}

void AFootPlayer::ActivateAura()
{
	const FFootAuraSettings* Settings = FindAuraSettings();
	if (!Aura || !Settings || !Settings->Systeme)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimerExtinctionAura);
	}

	if (Aura->GetAsset() != Settings->Systeme)
	{
		Aura->SetAsset(Settings->Systeme);
	}
	Aura->SetRelativeLocation(Settings->Decalage);
	Aura->SetRelativeScale3D(Settings->Echelle);
	for (const TPair<FName, float>& Param : Settings->ParametresFloat)
	{
		Aura->SetVariableFloat(Param.Key, Param.Value);
	}
	Aura->Activate(true);
}

void AFootPlayer::StopAura()
{
	if (!Aura)
	{
		return;
	}

	// Extinction douce : on arrête l'émission...
	Aura->Deactivate();

	// ...puis on coupe net après un court fondu, car certains effets du pack continuent d'émettre après Deactivate.
	if (UWorld* World = GetWorld())
	{
		TWeakObjectPtr<UNiagaraComponent> WeakAura = Aura;
		TWeakObjectPtr<AFootPlayer> WeakThis = this;
		World->GetTimerManager().SetTimer(TimerExtinctionAura, FTimerDelegate::CreateLambda([WeakAura, WeakThis]()
		{
			const bool bPouvoirActif = WeakThis.IsValid() && WeakThis->Pouvoir && WeakThis->Pouvoir->IsPowerActive();
			if (WeakAura.IsValid() && !bPouvoirActif)
			{
				WeakAura->DeactivateImmediate();
			}
		}), DureeFonduAura, false);
	}
}
