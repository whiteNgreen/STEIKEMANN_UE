#pragma once

#define DEBUG !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
#ifdef DEBUG


// Print To Screen MACRO
#define PRINT(X)				( GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Blue, FString::Printf(TEXT(X))) )
#define PRINTLONG(T, X)			( GEngine->AddOnScreenDebugMessage(-1, T,	FColor::Blue, FString::Printf(TEXT(X))) )

#define PRINTPAR(X, ...)		( GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT(X), ##__VA_ARGS__)) )
#define PRINTPARLONG(T, X, ...)	( GEngine->AddOnScreenDebugMessage(-1, T, FColor::Yellow, FString::Printf(TEXT(X), ##__VA_ARGS__)) )

#define PLOG(X) ( UE_LOG(LogTemp, Display, FString::Printf(TEXT(X))) )


// DrawDebugLine MACRO from GetActorLocation()
#define DRAWLINE(Vec, Color, T) ( DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (Vec), Color, false, T, 0, 5.f) )
#define DRAWPOINT(Vec, Color, T) ( DrawDebugPoint(GetWorld(), Vec, 20.f, Color, false, T, 0) )


/* -- timer macros --
*	TIMER t1 = TIMENOW()
*	TIME_MILLI("text", t1) */
#include <chrono>
typedef std::chrono::high_resolution_clock::time_point timer;
#define DURATION(a) std::chrono::duration_cast<std::chrono::nanoseconds>(a).count()
#define TIMENOW() std::chrono::high_resolution_clock::now()
#define TIME_MILLI(t)				PRINTPARLONG("%f milliseconds",		(float)(DURATION(TIMENOW() - t) / (1e6)))		
#define TIME_MILLI_float(t)			(float)(DURATION(TIMENOW() - t) / (1e6))		
#define TIME_SECOND(t)				PRINTPARLONG("%f seconds",			(float)(DURATION(TIMENOW() - t) / (1e9)))	
#define TIME_SECOND_float(t)		(float)(DURATION(TIMENOW() - t) / (1e9))		

constexpr auto TimeAverageStep = 10;
class TIMER
{
public:
	TIMER()		{}
	~TIMER()	{}
	void Start() {
		timer = TIMENOW();
	}
	float End() {
		float t = TIME_MILLI_float(timer);
		m_timer[count++] = t;
		if (count == TimeAverageStep) count = 0;
		for (const auto& ti : m_timer)
			average += ti;
		average /= (float)TimeAverageStep;
		return t;
	}
	float End_Average() {
		End();
		return average;
	}
	void Reset(){
		for (auto& ti : m_timer)
			ti = 0.f;
		average = 0.f;
		count = 0;
	}
	void Print(FString s) {
		FString p = s;
		p += FString::Printf(TEXT(" = %f milliseconds"), End());
		GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, p);
	}
	void Print_Average(FString s) {
		FString p = s;
		p += FString::Printf(TEXT(" = %f milliseconds"), End_Average());
		GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, p);
	}
private:
	timer timer;
	float m_timer[TimeAverageStep]{};
	int count{ 0 };
	float average{};
};
#endif // DEBUG


