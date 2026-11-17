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
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

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

void AFootPlayer::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	SetupModularMeshes();
}

void AFootPlayer::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// Aussi dans l'éditeur, pour que l'aperçu du Blueprint soit correctement posé.
	SetupModularMeshes();
}

void AFootPlayer::SetupModularMeshes()
{
	USkeletalMeshComponent* Leader = GetMesh();
	const USkeletalMesh* LeaderMesh = Leader ? Leader->GetSkeletalMeshAsset() : nullptr;
	if (!LeaderMesh)
	{
		return;
	}

	TArray<USkeletalMeshComponent*> Parts;
	GetComponents<USkeletalMeshComponent>(Parts);
	for (USkeletalMeshComponent* Part : Parts)
	{
		const USkeletalMesh* PartMesh = Part ? Part->GetSkeletalMeshAsset() : nullptr;
		if (Part == Leader || !PartMesh)
		{
			continue;
		}
		if (PartMesh->GetSkeleton() == LeaderMesh->GetSkeleton())
		{
			Part->SetLeaderPoseComponent(Leader);
			continue;
		}

		// Pièce à squelette propre (ex. cheveux) : tag "Socket:<nom>" = attachée à ce socket du Mesh principal,
		// en gardant sa transformation relative.
		for (const FName& Tag : Part->ComponentTags)
		{
			FString SocketName;
			if (Tag.ToString().Split(TEXT("Socket:"), nullptr, &SocketName) && !SocketName.IsEmpty())
			{
				if (Part->GetAttachParent() != Leader || Part->GetAttachSocketName() != FName(*SocketName))
				{
					Part->AttachToComponent(Leader, FAttachmentTransformRules::KeepRelativeTransform, FName(*SocketName));
				}
				break;
			}
		}
	}
}

void AFootPlayer::GetVisibleBodyMeshes(TArray<USkeletalMeshComponent*>& OutMeshes) const
{
	TArray<USkeletalMeshComponent*> Parts;
	GetComponents<USkeletalMeshComponent>(Parts);
	for (USkeletalMeshComponent* Part : Parts)
	{
		if (Part && Part->GetSkeletalMeshAsset() && Part->IsVisible() && !Part->bHiddenInGame)
		{
			OutMeshes.Add(Part);
		}
	}
}

void AFootPlayer::ApplyContourOverlay(UMaterialInterface* Overlay)
{
	TArray<USkeletalMeshComponent*> Parts;
	GetVisibleBodyMeshes(Parts);
	for (USkeletalMeshComponent* Part : Parts)
	{
		Part->SetOverlayMaterial(Overlay);
	}
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

	// Fondu de l'aura de contour.
	if (ContourMID && ContourIntensite != ContourCible)
	{
		ContourIntensite = FMath::FInterpConstantTo(ContourIntensite, ContourCible, DeltaSeconds, 1.f / DureeFonduContour);
		ContourMID->SetScalarParameterValue(TEXT("Intensite"), ContourIntensite);
		if (ContourIntensite <= 0.f)
		{
			ApplyContourOverlay(nullptr);
		}
	}

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
	if (!Settings)
	{
		return;
	}

	// Aura de contour : surimpression sur tout le corps, teintée à la couleur de la famille.
	if (Settings->MateriauContour)
	{
		if (!ContourMID || ContourMID->Parent != Settings->MateriauContour)
		{
			ContourMID = UMaterialInstanceDynamic::Create(Settings->MateriauContour, this);
			ContourIntensite = 0.f;
		}
		ContourMID->SetVectorParameterValue(TEXT("Couleur"), Settings->Couleur);
		ContourMID->SetScalarParameterValue(TEXT("Intensite"), ContourIntensite);
		ContourCible = 1.f;
		ApplyContourOverlay(ContourMID);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimerExtinctionAura);
	}

	// Flammes qui montent du corps.
	ActivateFlammes(*Settings);

	// Effet Niagara optionnel centré sur le personnage.
	if (!Aura || !Settings->Systeme)
	{
		return;
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

void AFootPlayer::ActivateFlammes(const FFootAuraSettings& Settings)
{
	USkeletalMeshComponent* Corps = GetMesh();
	if (!Settings.SystemeFlammes || !Corps)
	{
		return;
	}

	// Les foyers sont créés une seule fois, puis simplement réactivés.
	if (FoyersFlammes.Num() != Settings.Flammes.Num())
	{
		for (UNiagaraComponent* Foyer : FoyersFlammes)
		{
			if (Foyer)
			{
				Foyer->DestroyComponent();
			}
		}
		FoyersFlammes.Reset();

		for (const FFootAuraFlamme& Flamme : Settings.Flammes)
		{
			UNiagaraComponent* Foyer = NewObject<UNiagaraComponent>(this);
			Foyer->SetAutoActivate(false);
			Foyer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Foyer->SetupAttachment(Corps, Flamme.Os);
			// Rotation absolue : les flammes montent toujours vers le ciel, quelle que soit la pose de l'os.
			Foyer->SetUsingAbsoluteRotation(true);
			Foyer->RegisterComponent();
			Foyer->SetRelativeLocation(Flamme.Decalage);
			Foyer->SetWorldRotation(FRotator::ZeroRotator);
			Foyer->SetRelativeScale3D(FVector(Flamme.Echelle));
			FoyersFlammes.Add(Foyer);
		}
	}

	for (int32 Index = 0; Index < FoyersFlammes.Num(); ++Index)
	{
		UNiagaraComponent* Foyer = FoyersFlammes[Index];
		if (!Foyer)
		{
			continue;
		}
		if (Foyer->GetAsset() != Settings.SystemeFlammes)
		{
			Foyer->SetAsset(Settings.SystemeFlammes);
		}
		for (const TPair<FName, float>& Param : Settings.ParametresFloat)
		{
			Foyer->SetVariableFloat(Param.Key, Param.Value);
		}
		if (!Settings.ParametreTailleFlammes.IsNone() && Settings.Flammes.IsValidIndex(Index))
		{
			Foyer->SetRelativeScale3D(FVector::OneVector);
			Foyer->SetVariableFloat(Settings.ParametreTailleFlammes, Settings.Flammes[Index].Echelle);
		}
		Foyer->Activate(true);
	}
}

void AFootPlayer::StopAura()
{
	// Le contour s'efface dans Tick, puis la surimpression est retirée.
	ContourCible = 0.f;

	// Extinction douce : on arrête l'émission des flammes et de l'effet optionnel...
	TArray<TWeakObjectPtr<UNiagaraComponent>> AEteindre;
	for (UNiagaraComponent* Foyer : FoyersFlammes)
	{
		if (Foyer)
		{
			Foyer->Deactivate();
			AEteindre.Add(Foyer);
		}
	}
	if (Aura && Aura->GetAsset())
	{
		Aura->Deactivate();
		AEteindre.Add(Aura);
	}

	// ...puis on coupe net après un court fondu, car certains effets du pack continuent d'émettre après Deactivate.
	if (UWorld* World = GetWorld())
	{
		TWeakObjectPtr<AFootPlayer> WeakThis = this;
		World->GetTimerManager().SetTimer(TimerExtinctionAura, FTimerDelegate::CreateLambda([AEteindre, WeakThis]()
		{
			const bool bPouvoirActif = WeakThis.IsValid() && WeakThis->Pouvoir && WeakThis->Pouvoir->IsPowerActive();
			if (bPouvoirActif)
			{
				return;
			}
			for (const TWeakObjectPtr<UNiagaraComponent>& Comp : AEteindre)
			{
				if (Comp.IsValid())
				{
					Comp->DeactivateImmediate();
				}
			}
		}), DureeFonduAura, false);
	}
}
