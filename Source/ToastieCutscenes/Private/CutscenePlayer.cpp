#include "CutscenePlayer.h"

// Sets default values
ACutscenePlayer::ACutscenePlayer()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	Scene = nullptr;
	IdCounter = 0;
}

// Called when the game starts or when spawned
void ACutscenePlayer::BeginPlay()
{
	Super::BeginPlay();

	if (Scene)
	{
		Process.EndIndex = Scene->Commands.Num() - 1;
		Process.FetchCommands(*this);
	}
	else
	{
		// No scene was set, delete this actor
		Destroy();
	}
}

void ACutscenePlayer::FinishCommand(const int32 Id)
{
	// Each Id should only be used once
	// For safety's sake, run through each active process anyways
	// and finish any command with a matching Id
	ForEachProcess([Id](FAProcess& CurrentProcess)
	{
		for (auto& Command : CurrentProcess.ActiveCommands)
		{
			if (Command.Id == Id)
				Command.State = FAProcess::ECommandStates::Finished;
		}
	});
}

void ACutscenePlayer::FinishPlayerChoice(const int32 Id, const FToastieCutsceneOption& Option)
{
	if (Option.Label.Equals(TEXT("Exit")))
	{
		ForEachProcess([Id](FAProcess& CurrentProcess)
		{
			for (auto& Command : CurrentProcess.ActiveCommands)
			{
				if (Command.Id == Id)
					Command.State = FAProcess::ECommandStates::Finished;
			}
			CurrentProcess.CurrentIndex = INT32_MAX;
		});
	}
	else
	{
		if (auto LabelIndex = FindLabel(Option.Label);
			Scene && Scene->Commands.IsValidIndex(LabelIndex))
		{
			ForEachProcess([Id, LabelIndex](FAProcess& CurrentProcess)
			{
				for (auto& Command : CurrentProcess.ActiveCommands)
				{
					if (Command.Id == Id)
					{
						Command.State = FAProcess::ECommandStates::Finished;
						CurrentProcess.CurrentIndex = LabelIndex + 1;
					}
				}
			});
		}
	}
}

// Called every frame
void ACutscenePlayer::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	Process.Tick(DeltaTime, *this);
	
	if (Process.IsFinished(*this))
	{
		Destroy();
	}
}

void ACutscenePlayer::FAProcess::Tick(const float DeltaTime, ACutscenePlayer& CutscenePlayer)
{
	if (Delay > 0.0f)
	{
		Delay -= DeltaTime;
		return;
	}

	// Tick children
	bool bAtLeastOneBlockingChild = false;
	for (auto& Child : Children)
	{
		Child.Tick(DeltaTime, CutscenePlayer);
		bAtLeastOneBlockingChild |= Child.bBlocking;
	}

	// Clear finished children
	struct FRemoveFinishedChildren
	{
		const ACutscenePlayer& CutscenePlayer;
		bool operator()(const FAProcess& Process) const
		{
			return Process.IsFinished(CutscenePlayer);
		}
	};
	Children.RemoveAll(FRemoveFinishedChildren { CutscenePlayer });

	if (!Children.IsEmpty() && bAtLeastOneBlockingChild)
	{
		return;
	}

	// Tick active commands
	for (auto& [Id, Index, DelayedTimeRemaining, State, bCommandIsBlocking] : ActiveCommands)
	{
		if (State == ECommandStates::Delayed)
		{
			DelayedTimeRemaining -= DeltaTime;
			if (DelayedTimeRemaining <= 0.0f)
			{
				State = ECommandStates::Queued;
			}
		}

		if (State == ECommandStates::Queued)
		{
			const auto Result = CutscenePlayer.ExecuteCommand(Index, Id);
			State = Result == ECutscenePlayerExecuteResult::Finished ?
				ECommandStates::Finished : ECommandStates::Running;
		}
	}

	// Clear finished commands
	struct FRemoveFinishedCommands
	{
		bool operator()(const FCommandState& Command) const
		{
			return Command.State == ECommandStates::Finished;
		}
	};
	ActiveCommands.RemoveAll(FRemoveFinishedCommands());

	// If all current commands have finished,
	// fetch new commands
	if (IsFinishedCurrentActions())
	{
		FetchCommands(CutscenePlayer);
	}
}

bool ACutscenePlayer::FAProcess::IsFinishedCurrentActions() const
{
	for (const auto& ActiveCommand : ActiveCommands)
	{
		if (ActiveCommand.bBlocking &&
			ActiveCommand.State != ECommandStates::Finished)
		{
			return false;
		}
	}
	return true;
}

bool ACutscenePlayer::FAProcess::IsFinished(const ACutscenePlayer& CutscenePlayer) const
{
	return !CutscenePlayer.Scene || (ActiveCommands.IsEmpty() && Children.IsEmpty());
}

void ACutscenePlayer::FAProcess::FetchCommands(ACutscenePlayer& CutscenePlayer)
{
	while (CutscenePlayer.Scene
		&& CutscenePlayer.Scene->Commands.IsValidIndex(CurrentIndex)
		&& CurrentIndex <= EndIndex)
	{
		if (const auto CommandPtr = CutscenePlayer.Scene->Commands[CurrentIndex].GetPtr<FToastieCutsceneCommandBase>();
			CommandPtr)
		{

			const auto bRequirementsMet = CutscenePlayer.RequirementsAreMet(CommandPtr->Requirements);
			const auto BlockPtr = CutscenePlayer.Scene->Commands[CurrentIndex].GetPtr<FToastieCutsceneBlock>();
			
			if (BlockPtr && BlockPtr->Type != EToastieCutsceneBlockType::PlayerChoice)
			{
				if (bRequirementsMet)
				{
					FAProcess ChildProcess;
					ChildProcess.CurrentIndex = CurrentIndex + 1;
					ChildProcess.EndIndex = CurrentIndex + BlockPtr->CommandCount;
					ChildProcess.bConcurrent = BlockPtr->Type == EToastieCutsceneBlockType::Concurrent;
					ChildProcess.Delay = BlockPtr->Delay;
					ChildProcess.bBlocking = !BlockPtr->bDoNotBlock;
					ChildProcess.FetchCommands(CutscenePlayer);
					Children.Add(ChildProcess);
				}
				
				CurrentIndex += 1 + BlockPtr->CommandCount;
			}
			else
			{
				if (bRequirementsMet)
				{
					// Add this command to the active command list
					FCommandState State;
					State.Id = ++CutscenePlayer.IdCounter;
					State.Index = CurrentIndex;
					State.DelayedTimeRemaining = CommandPtr->Delay;
					State.State = CommandPtr->Delay > 0.0f ? ECommandStates::Delayed : ECommandStates::Queued;
					State.bBlocking = !CommandPtr->bDoNotBlock;

					ActiveCommands.Add(State);
				}

				CurrentIndex += 1 + (BlockPtr ? BlockPtr->CommandCount : 0);
			}
			
			if (bConcurrent || CommandPtr->bDoNotBlock)
				continue;

			// All possible commands have been added to the queue
			// Let them tick and finish before fetching more commands
			return;
		}
		
		// Invalid command, ignored
		++CurrentIndex;
	}
}

ECutscenePlayerExecuteResult ACutscenePlayer::ExecuteCommand(const int32 Index, const int32 Id)
{
	if (!Scene || !Scene->Commands.IsValidIndex(Index))
		return ECutscenePlayerExecuteResult::Finished;

	const auto& Data = Scene->Commands[Index];
	auto Result = ECutscenePlayerExecuteResult::Finished;

	if (const auto ExitData = Data.GetPtr<FToastieCutsceneExit>();
		ExitData)
	{
		ForEachProcess([](FAProcess& CurrentProcess)
		{
			CurrentProcess.CurrentIndex = INT32_MAX;
		});
		return Result;
	}

	if (const auto PlayerChoiceData = Data.GetPtr<FToastieCutsceneBlock>();
		PlayerChoiceData && PlayerChoiceData->Type == EToastieCutsceneBlockType::PlayerChoice)
	{
		TArray<FToastieCutsceneOption> Options;
		for (int i = 1; i <= PlayerChoiceData->CommandCount; ++i)
		{
			if (const auto OptionPtr = Scene->Commands[Index + i].GetPtr<FToastieCutsceneOption>();
				OptionPtr && RequirementsAreMet(OptionPtr->Requirements))
			{
				Options.Add(*OptionPtr);
			}
		}
		Result = ExecutePlayerChoice(Id, Options);
		return Result;
	}
	
	if (TryExecuteCommand(Id, Data, &ACutscenePlayer::ExecuteSay, Result))
		return Result;

	if (TryExecuteCommand(Id, Data, &ACutscenePlayer::ExecuteEnablePlayerControl, Result))
		return Result;

	if (TryExecuteCommand(Id, Data, &ACutscenePlayer::ExecuteDisablePlayerControl, Result))
		return Result;

	if (TryExecuteCommand(Id, Data, &ACutscenePlayer::ExecuteWait, Result))
		return Result;

	if (TryExecuteCommand(Id, Data, &ACutscenePlayer::ExecuteLookAt, Result))
		return Result;
	
	return ECutscenePlayerExecuteResult::Finished;
}

int32 ACutscenePlayer::FindLabel(const FString& Label) const
{
	if (Scene)
	{
		for (int32 i = 0; i < Scene->Commands.Num(); ++i)
		{
			const auto& StructInstance = Scene->Commands[i];
			if (const auto LabelPtr = StructInstance.GetPtr<FToastieCutsceneLabel>();
				LabelPtr && LabelPtr->Label.Equals(Label))
			{
				return i;
			}
		}
	}
	return -1;
}
