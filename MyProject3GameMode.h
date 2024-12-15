// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MyProject3Character.h"
#include "GameFramework/PlayerStart.h"
#include "MyProject3GameMode.generated.h"


class AMyProject3Character;

UCLASS(minimalapi)
class AMyProject3GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMyProject3GameMode();

protected:


    UPROPERTY(EditDefaultsOnly, Category = "Player")
    TSubclassOf<AMyProject3Character> PlayerCharacterClass;

};



