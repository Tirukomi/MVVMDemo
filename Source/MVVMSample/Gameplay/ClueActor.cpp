// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/ClueActor.h"

#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AClueActor::AClueActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetRelativeScale3D(FVector(0.35f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (Sphere.Succeeded())
	{
		Mesh->SetStaticMesh(Sphere.Object);
	}

	// Hidden from the normal view; only the detective post-process pass draws it, via custom depth.
	Mesh->SetVisibility(false);
	Mesh->SetRenderCustomDepth(false);
	Mesh->SetCustomDepthStencilValue(StencilUnscanned);
}

void AClueActor::SetDetectiveHighlight(bool bEnabled)
{
	bHighlighted = bEnabled;
	RefreshRendering();
}

void AClueActor::MarkScanned()
{
	bScanned = true;
	RefreshRendering();
}

void AClueActor::RefreshRendering()
{
	Mesh->SetCustomDepthStencilValue(bScanned ? StencilScanned : StencilUnscanned);
	Mesh->SetRenderCustomDepth(bHighlighted);
	// Custom depth still draws hidden-in-game primitives only when they are visible; keep it visible while highlighted.
	Mesh->SetVisibility(bHighlighted);
}
