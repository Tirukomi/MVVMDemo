// Copyright IG. All Rights Reserved.

// Functional tests: they need the running game (a world, a player, painted widgets), so they carry only the client
// context and run in Scripts/run_tests.py's game pass, never in the editor pass.

#include "Misc/AutomationTest.h"

#include "MvsMenuInputTest.h"
#include "Core/MvsPlayerController.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#if WITH_DEV_AUTOMATION_TESTS

DEFINE_LOG_CATEGORY_STATIC(LogMvsFunctional, Log, All);

namespace MvsFunctionalTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter;

	AMvsPlayerController* FindPlayer()
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			UWorld* World = Context.World();
			if (World && World->IsGameWorld())
			{
				if (AMvsPlayerController* PC = Cast<AMvsPlayerController>(GEngine->GetFirstLocalPlayerController(World)))
				{
					return PC->GetPawn() ? PC : nullptr;
				}
			}
		}
		return nullptr;
	}

	/**
	 * Waits for the level and the player, settles for SettleSeconds (the HUD's intro), runs the script Build makes, and
	 * finishes when it does. Every rule becomes a test pass (info) or error.
	 */
	class FRunScript : public IAutomationLatentCommand
	{
	public:
		using FBuild = TFunction<TSharedPtr<FMvsScript>(AMvsPlayerController*, FMvsScript::FReporter)>;

		FRunScript(FAutomationTestBase* InTest, FBuild InBuild, double InTimeoutSeconds)
			: Test(InTest), Build(MoveTemp(InBuild)), Deadline(FPlatformTime::Seconds() + InTimeoutSeconds) {}

		virtual bool Update() override
		{
			if (FPlatformTime::Seconds() > Deadline)
			{
				Test->AddError(Script ? TEXT("the script did not finish in time") : TEXT("no game world with a Mvs player (run in -game)"));
				return true;
			}
			if (!Script)
			{
				AMvsPlayerController* PC = FindPlayer();
				if (!PC)
				{
					return false;
				}
				if (ReadyAt == 0.0)
				{
					ReadyAt = FPlatformTime::Seconds();
				}
				if (FPlatformTime::Seconds() - ReadyAt < SettleSeconds)
				{
					return false;
				}
				FAutomationTestBase* T = Test;
				Script = Build(PC, [T](EMvsCheck Result, const FString& Rule)
				{
					// A known bug is recorded, not failed; a known bug that now passes fails, so it gets promoted.
					const FString Line = FString::Printf(TEXT("%s: %s"), FMvsScript::ResultLabel(Result), *Rule);
					// Also in the log: if the game hangs, the report is never written, but the log shows how far it got.
					UE_LOG(LogMvsFunctional, Display, TEXT("%s"), *Line);
					if (Result == EMvsCheck::Passed || Result == EMvsCheck::KnownBug) { T->AddInfo(Line); }
					else { T->AddError(Line); }
				});
				if (!Script)
				{
					Test->AddError(TEXT("could not build the script (no UI subsystem)"));
					return true;
				}
				Script->Start();
				return false;
			}
			if (!Script->IsFinished())
			{
				return false;
			}
			if (Script->GetPassed() + Script->GetFailed() + Script->GetKnownBugs() == 0)
			{
				Test->AddError(TEXT("the script finished without checking anything"));
			}
			return true;
		}

	private:
		static constexpr double SettleSeconds = 2.0;
		FAutomationTestBase* Test;
		FBuild Build;
		double Deadline;
		double ReadyAt = 0.0;
		TSharedPtr<FMvsScript> Script;
	};
}

// The key that opens a screen closes it, every prompt does what its key does, and an open screen restyles live.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsFunctionalMenuInputTest, "Mvs.Functional.MenuInput", MvsFunctionalTests::Flags)
bool FMvsFunctionalMenuInputTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(MvsFunctionalTests::FRunScript(this, &FMvsMenuInputTest::Build, 90.0));
	return true;
}

#endif
