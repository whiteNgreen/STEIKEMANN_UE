#pragma once

#define DEBUG !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
#ifdef DEBUG


#define PRINT(X) ( GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Cyan, FString::Printf(TEXT(X))) )
#define PRINTLONG(X) ( GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Cyan, FString::Printf(TEXT(X))) )

#define PRINTPAR(X, ...) ( GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT(X), ##__VA_ARGS__)) )
#define PRINTPARLONG(X, ...) ( GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Yellow, FString::Printf(TEXT(X), ##__VA_ARGS__)) )





#endif // DEBUG
