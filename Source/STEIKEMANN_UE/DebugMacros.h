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

//#define DRAWLINE_XXX(x,A,B,C,FUNC,...) FUNC
//#define DRAWLINE(...)    DRAWLINE_XXX(,##__VA_ARGS__, \
//									DRAWLINE_3(##__VA_ARGS__), \
//									DRAWLINE_2(##__VA_ARGS__), \
//									DRAWLINE_1(##__VA_ARGS__), \
//									DRAWLINE_0(##__VA_ARGS__)  \
//									)	
//
//#define THIRD_ARGUMENT(A,B,C,...) C
//#define COUNT_ARGUMENTS(...) THIRD_ARGUMENT(dummy, ##__VA_ARGS__, C, B, A)

//#define GET_3RD_ARG(x,A,B,C,D, ...) D
//#define DRAWLINE_MACRO_CHOOSER(...) ( GET_3RD_ARG(##__VA_ARGS__, DRAWLINE_3, DRAWLINE_2, DRAWLINE_1, ##__VA_ARGS__ ) )
//#define DRAWLINE(...) DRAWLINE_MACRO_CHOOSER(##__VA_ARGS__)(##__VA_ARGS__)

#endif // DEBUG
