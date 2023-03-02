#pragma once

#include "Coreminimal.h"
#include "StaticVariables.h"

DECLARE_MULTICAST_DELEGATE(FHeightReached)
DECLARE_DELEGATE(FDeath)
DECLARE_DELEGATE_OneParam(FPostLockedMovement, TFunction<void()> lambdacall)
DECLARE_MULTICAST_DELEGATE_OneParam(FAttackContact_Other_Delegate, AActor* Target)
//DECLARE_MULTICAST_DELEGATE(FAttackContact_Instigator_Delegate)
