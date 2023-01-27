#pragma once

#include "Coreminimal.h"

DECLARE_MULTICAST_DELEGATE(FHeightReached)
DECLARE_DELEGATE(FDeath)
DECLARE_DELEGATE_OneParam(FPostLockedMovement, TFunction<void()>/*lambdacall*/)
