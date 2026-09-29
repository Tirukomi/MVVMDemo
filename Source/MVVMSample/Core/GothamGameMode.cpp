// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/GothamGameMode.h"

#include "Core/GothamCharacter.h"
#include "Core/GothamPlayerController.h"

AGothamGameMode::AGothamGameMode()
{
	DefaultPawnClass = AGothamCharacter::StaticClass();
	PlayerControllerClass = AGothamPlayerController::StaticClass();
}
