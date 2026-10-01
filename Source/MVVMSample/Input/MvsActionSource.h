// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MvsActionSource.generated.h"

class UInputAction;

UINTERFACE(MinimalAPI)
class UMvsActionSource : public UInterface
{
	GENERATED_BODY()
};

/**
 * Whatever owns the gameplay input actions (the player controller). UI asks it for an action by name, to show its key
 * or bind to it, without knowing the concrete controller class.
 */
class MVVMSAMPLE_API IMvsActionSource
{
	GENERATED_BODY()

public:
	/** A gameplay action by table name ("Attack", "Gadget1", "Pause"...; Input/MvsActionTable), or null. */
	virtual const UInputAction* FindAction(FName Name) const = 0;

	/** FindAction on Source if it implements this interface (a player controller, usually), else null. */
	static const UInputAction* Find(const UObject* Source, FName Name)
	{
		const IMvsActionSource* Actions = Cast<IMvsActionSource>(Source);
		return Actions ? Actions->FindAction(Name) : nullptr;
	}
};
