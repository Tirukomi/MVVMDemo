// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MvsAcceptable.generated.h"

UINTERFACE(MinimalAPI)
class UMvsAcceptable : public UInterface
{
	GENERATED_BODY()
};

/**
 * A menu item the accept prompt can act on: clicking "[Enter] Select" does to the current item exactly what pressing
 * Enter on it does, by calling Accept rather than by sending a key.
 */
class IMvsAcceptable
{
	GENERATED_BODY()

public:
	virtual void Accept() = 0;
};
