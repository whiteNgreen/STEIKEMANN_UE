#pragma once

#define DEBUG !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
#ifdef DEBUG


// Print To Screen MACRO
#define PRINT(X) ( GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Cyan, FString::Printf(TEXT(X))) )
#define PRINTLONG(X) ( GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Cyan, FString::Printf(TEXT(X))) )

#define PRINTPAR(X, ...) ( GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT(X), ##__VA_ARGS__)) )
#define PRINTPARLONG(X, ...) ( GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Yellow, FString::Printf(TEXT(X), ##__VA_ARGS__)) )

#define PLOG(X) ( UE_LOG(LogTemp, Display, FString::Printf(TEXT(X))) )


// DrawDebugLine MACRO
#define DLINE_1(A)		( DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + A, FColor::Red, false, 0.f, 0, 6.f) )
#define DLINE_2(A,B)	( DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + A, FColor::Red, false,	  B, 0, 6.f) )
#define DLINE_3(A,B,C)	( DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + A,			  C, false,	  B, 0, 6.f) )


/* -- timer macros --
*	TIMER t1 = TIMENOW()
*	TIME_MILLI("text", t1) */
#include <chrono>

typedef std::chrono::high_resolution_clock::time_point TIMER;
#define DURATION(a) std::chrono::duration_cast<std::chrono::nanoseconds>(a).count()
#define TIMENOW() std::chrono::high_resolution_clock::now()
#define TIME_NANO(x, t)		E_TRACE(std::string(x) + "{0} nanoseconds",				DURATION(TIMENOW() - t))				
#define TIME_MICRO(x, t)	E_TRACE(std::string(x) + "{0} microseconds",	(float)(DURATION(TIMENOW() - t) / (1e3)))		
#define TIME_MILLI(x, t)	E_TRACE(std::string(x) + "{0} milliseconds",	(float)(DURATION(TIMENOW() - t) / (1e6)))		
#define TIME_SECOND(x, t)	E_TRACE(std::string(x) + "{0} seconds",			(float)(DURATION(TIMENOW() - t) / (1e9)))	

#endif // DEBUG
