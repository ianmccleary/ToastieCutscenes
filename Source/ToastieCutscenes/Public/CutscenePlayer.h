#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ToastieCutsceneAsset.h"
#include "CutscenePlayer.generated.h"

UENUM(BlueprintType)
enum class ECutscenePlayerExecuteResult : uint8
{
	Finished		UMETA(DisplayName = "Finished"),
	InProgress		UMETA(DisplayName = "In Progress")
};

UCLASS()
class TOASTIECUTSCENES_API ACutscenePlayer : public AActor
{
	GENERATED_BODY()
	
public:
	// Sets default values for this actor's properties
	ACutscenePlayer();

protected:
	
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	UFUNCTION(BlueprintImplementableEvent)
	bool RequirementsAreMet(const TArray<FToastieCutsceneReq>& Reqs) const;

	UFUNCTION(BlueprintImplementableEvent)
	ECutscenePlayerExecuteResult ExecuteSay(const int32 Id, const FToastieCutsceneSay& Data);

	UFUNCTION(BlueprintImplementableEvent)
	ECutscenePlayerExecuteResult ExecuteEnablePlayerControl(const int32 Id, const FToastieCutsceneEnablePlayerControl& Data);

	UFUNCTION(BlueprintImplementableEvent)
	ECutscenePlayerExecuteResult ExecuteDisablePlayerControl(const int32 Id, const FToastieCutsceneDisablePlayerControl& Data);

	UFUNCTION(BlueprintImplementableEvent)
	ECutscenePlayerExecuteResult ExecuteWait(const int32 Id, const FToastieCutsceneWait& Data);

	UFUNCTION(BlueprintImplementableEvent)
	ECutscenePlayerExecuteResult ExecuteLookAt(const int32 Id, const FToastieCutsceneLookAt& Data);

	UFUNCTION(BlueprintImplementableEvent)
	ECutscenePlayerExecuteResult ExecutePlayerChoice(const int32 Id, const TArray<FToastieCutsceneOption>& Data);

	UFUNCTION(BlueprintCallable)
	void FinishCommand(const int32 Id);

	UFUNCTION(BlueprintCallable)
	void FinishPlayerChoice(const int32 Id, const FToastieCutsceneOption& Option);
	
	UPROPERTY(BlueprintReadOnly, meta=(ExposeOnSpawn))
	UToastieCutsceneAsset* Scene;

public:	
	// Called every frame
	virtual void Tick(const float DeltaTime) override;
	
private:

	int32 FindLabel(const FString& Label) const;
	
	template<typename T>
	bool TryExecuteCommand(
		const int32 Id,
		const FInstancedStruct& Data,
		ECutscenePlayerExecuteResult (ACutscenePlayer::*Function)(const int32, const T&),
		ECutscenePlayerExecuteResult& Result)
	{
		const T* Ptr = Data.GetPtr<T>();
		if (Ptr)
		{
			Result = (this->*Function)(Id, *Ptr);
			return true;
		}
		return false;
	}

	ECutscenePlayerExecuteResult ExecuteCommand(const int32 Index, const int32 Id);

	class FAProcess
	{
	public:
		bool IsFinishedCurrentActions() const;
		bool IsFinished(const ACutscenePlayer& CutscenePlayer) const;
		
		void FetchCommands(ACutscenePlayer& CutscenePlayer);
		void Tick(const float DeltaTime, ACutscenePlayer& CutscenePlayer);

		enum class ECommandStates : uint8
		{
			Delayed,
			Queued,
			Running,
			Finished
		};
	
		struct FCommandState
		{
			int32 Id;
			int32 Index;
			float DelayedTimeRemaining;
			ECommandStates State;
			bool bBlocking;
		};

		TArray<FAProcess> Children;
		TArray<FCommandState> ActiveCommands;
		int32 EndIndex = 0;
		int32 CurrentIndex = 0;
		float Delay = 0.0f;
		bool bConcurrent = false;
		bool bBlocking = true;
	};

	FAProcess Process;
	int32 IdCounter;

	template<typename F>
	void ForEachProcessImpl(F Functor, FAProcess& CurrentProcess)
	{
		Functor(CurrentProcess);
		for (auto& ChildProcess : Process.Children)
		{
			ForEachProcessImpl(Functor, ChildProcess);
		}
	}

	template<typename F>
	void ForEachProcess(F Functor)
	{
		ForEachProcessImpl(Functor, Process);
	}
};
