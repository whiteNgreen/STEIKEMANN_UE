#pragma once

#include "Coreminimal.h"
#include "StaticVariables.h"

DECLARE_MULTICAST_DELEGATE(FHeightReached)
DECLARE_DELEGATE(FDeath)
DECLARE_DELEGATE_OneParam(FPostLockedMovement, TFunction<void()>/*lambdacall*/)
DECLARE_DELEGATE_TwoParams(FAttackContactDelegate, AActor* /*Instigator*/, AActor* /*Target*/)