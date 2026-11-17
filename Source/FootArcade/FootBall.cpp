// Ballon de FootArcade.

#include "FootBall.h"
#include "FootArcade.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AFootBall::AFootBall()
{
	PrimaryActorTick.bCanEverTick = false;

	BallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BallMesh"));
	SetRootComponent(BallMesh);

	// Mesh par défaut : la sphère de l'engine (diamètre 100 cm) ramenée à ~22 cm.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		BallMesh->SetStaticMesh(SphereMesh.Object);
		BallMesh->SetRelativeScale3D(FVector(0.22f));
	}

	// Physique : simulation, collision complète, CCD contre l'effet tunnel.
	BallMesh->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	BallMesh->SetSimulatePhysics(true);
	BallMesh->SetUseCCD(true);
	BallMesh->SetNotifyRigidBodyCollision(true);
	BallMesh->SetGenerateOverlapEvents(true);

	// Pendant la construction du CDO, le moteur physique n'est pas prêt : on écrit
	// directement dans le BodyInstance sans déclencher de recalcul de masse.
	BallMesh->BodyInstance.SetMassOverride(MassKg, true);
	BallMesh->BodyInstance.LinearDamping = LinearDamping;
	BallMesh->BodyInstance.AngularDamping = AngularDamping;
}

void AFootBall::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Les valeurs ont pu être modifiées dans un Blueprint enfant : on les réapplique.
	ApplyPhysicsSettings();
}

void AFootBall::ApplyPhysicsSettings()
{
	if (!BallMesh)
	{
		return;
	}

	BallMesh->SetMassOverrideInKg(NAME_None, MassKg, true);
	BallMesh->SetLinearDamping(LinearDamping);
	BallMesh->SetAngularDamping(AngularDamping);
}

void AFootBall::Kick(FVector Direction, float Power)
{
	if (!BallMesh || !BallMesh->IsSimulatingPhysics())
	{
		return;
	}

	// La direction est utilisée telle quelle : c'est le tireur qui décide de la hauteur.
	const FVector ShotDir = Direction.GetSafeNormal();
	if (ShotDir.IsNearlyZero())
	{
		return;
	}

	// On repart d'une vitesse nulle pour une frappe nette et prévisible (style arcade).
	BallMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
	BallMesh->AddImpulse(ShotDir * KickForce * FMath::Max(Power, 0.f), NAME_None, /*bVelChange=*/ true);

	UE_LOG(LogFootArcade, Verbose, TEXT("%s frappé : direction %s, puissance %.2f"), *GetName(), *ShotDir.ToString(), Power);
}
