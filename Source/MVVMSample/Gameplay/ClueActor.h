// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Gameplay/DetectiveTypes.h"
#include "ClueActor.generated.h"

class UClueDataAsset;
class UStaticMeshComponent;

/**
 * A clue placed in the world. Invisible to normal vision; Detective Mode turns on custom depth so the
 * post-process pass can draw it (through walls) in "unscanned" or "scanned" colours via the stencil value.
 */
UCLASS()
class MVVMSAMPLE_API AClueActor : public AActor
{
	GENERATED_BODY()

public:
	AClueActor();

	/** Stencil values the Detective Mode post-process material keys off. */
	static constexpr int32 StencilUnscanned = EGothamStencil::ClueUnscanned;
	static constexpr int32 StencilScanned = EGothamStencil::ClueScanned;

	const UClueDataAsset* GetClue() const { return Clue; }
	bool IsScanned() const { return bScanned; }

	void SetDetectiveHighlight(bool bEnabled);
	void MarkScanned();

private:
	void RefreshRendering();

	UPROPERTY(EditAnywhere, Category = "Clue")
	TObjectPtr<UClueDataAsset> Clue;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Mesh;

	bool bScanned = false;
	bool bHighlighted = false;
};
