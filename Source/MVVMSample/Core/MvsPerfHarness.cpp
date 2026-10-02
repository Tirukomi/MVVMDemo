// Copyright IG. All Rights Reserved.

#include "Core/MvsPerfHarness.h"

#if !UE_BUILD_SHIPPING

#include "Blueprint/UserWidget.h"
#include "Core/MvsScript.h"
#include "Core/MvsCharacter.h"
#include "Core/MvsPlayerController.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Gameplay/ForensicComponent.h"
#include "HAL/PlatformMemory.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ProfilingDebugging/MiscTrace.h"
#include "DynamicRHI.h"
#include "RenderTimer.h"
#include "Gameplay/ThreatSubsystem.h"
#include "UI/ClueEntryWidget.h"
#include "UI/Screens/ClueLogScreen.h"
#include "UI/Screens/ConfirmModalScreen.h"
#include "UI/Screens/GadgetWheelScreen.h"
#include "UI/Screens/PauseMenuScreen.h"
#include "UI/Screens/SettingsScreen.h"
#include "UI/MvsUISettings.h"
#include "UI/Layout/MvsUISubsystem.h"
#include "ViewModels/ClueViewModels.h"
#include "ViewModels/MvsViewModelSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogMvsPerf, Log, All);

namespace
{
	struct FScenario
	{
		FString Name;
		TFunction<void()> Setup;
		TFunction<void(float /*Seconds*/)> PerFrame;
		TFunction<void()> Teardown;
		/** Whether the scenario is in the state it measures (checked on the first sampled frame). A scenario whose
		 *  setup silently failed would otherwise measure nothing and look cheap: until R4, case-file-505 never
		 *  opened the case file. Null means nothing to check. */
		TFunction<bool()> Check;
	};

	struct FResult
	{
		FString Name;
		int32 Frames = 0;
		double AvgFrameMs = 0.0;
		double P95FrameMs = 0.0;
		double AvgGameMs = 0.0;
		/** GPU time of the whole frame (RHIGetGPUFrameCycles, as stat unit shows it). */
		double AvgGpuMs = 0.0;
		int32 UserWidgets = 0;
		int32 Ticking = 0;
		int32 UObjects = 0;
		double UsedMB = 0.0;
		bool bValid = true;
		/** The world was paused while sampled (the pause menu pauses it): compare with no-ui-paused, not no-ui. */
		bool bPaused = false;
	};

	constexpr float WarmupSeconds = 2.f;
	/** Sampled seconds per scenario; -MvsPerfSeconds=<s> overrides (the gate uses 5). */
	float SampleSeconds()
	{
		static const float Seconds = []
		{
			float Value = 8.f;
			FParse::Value(FCommandLine::Get(), TEXT("MvsPerfSeconds="), Value);
			return FMath::Clamp(Value, 1.f, 60.f);
		}();
		return Seconds;
	}

	class FRun : public TSharedFromThis<FRun>
	{
	public:
		TWeakObjectPtr<AMvsPlayerController> Controller;
		FString Label;
		TArray<FScenario> Scenarios;
		TArray<FResult> Results;

		int32 Index = -1;
		TArray<float> FrameMs;
		double GameMsSum = 0.0;
		double GpuMsSum = 0.0;
		int32 GameSamples = 0;
		bool bInRegion = false;
		bool bValid = true;
		bool bPaused = false;
		double StartMB = 0.0;
		/** -MvsPerfInject=<scenario>:<ms>: busy-waits that long on the game thread every frame of one scenario, so
		 *  the gate can prove it catches a real regression of a known size. */
		FString InjectScenario;
		double InjectMs = 0.0;

		static double UsedMB() { return FPlatformMemory::GetStats().UsedPhysical / (1024.0 * 1024.0); }

		static int32 CountUserWidgets(bool bTickingOnly = false)
		{
			int32 Count = 0;
			for (TObjectIterator<UUserWidget> It; It; ++It)
			{
				if (It->HasAnyFlags(RF_ClassDefaultObject))
				{
					continue;
				}
				Count += (!bTickingOnly || It->GetDesiredTickFrequency() != EWidgetTickFrequency::Never) ? 1 : 0;
			}
			return Count;
		}

		void Begin()
		{
			GEngine->Exec(nullptr, TEXT("t.MaxFPS 0"));
			GEngine->Exec(nullptr, TEXT("r.VSync 0"));
			if (FParse::Param(FCommandLine::Get(), TEXT("MvsInvalidation")))
			{
				GEngine->Exec(nullptr, TEXT("Slate.EnableGlobalInvalidation 1"));
			}
			StartMB = UsedMB();
			// Each scenario: set up, run warm-up + sample seconds (sampling after the warm-up), finish. The script
			// holds this run; the run never holds the script, so there is no cycle.
			const TSharedRef<FRun> Self = AsShared();
			TSharedRef<FMvsScript> Script = MakeShared<FMvsScript>();
			for (int32 i = 0; i < Scenarios.Num(); ++i)
			{
				Script->Do([Self, i]() { Self->StartScenario(i); })
					.Sample(WarmupSeconds + SampleSeconds(), [Self](float Elapsed, float DeltaTime) { Self->SampleFrame(Elapsed, DeltaTime); })
					.Do([Self]() { Self->FinishScenario(); });
			}
			Script->Do([Self]() { Self->WriteReport(); }).Quit();
			Script->Start();
		}

		void StartScenario(int32 InIndex)
		{
			Index = InIndex;
			FrameMs.Reset();
			GameMsSum = 0.0;
			GpuMsSum = 0.0;
			GameSamples = 0;
			if (Scenarios.IsValidIndex(Index) && Scenarios[Index].Setup)
			{
				Scenarios[Index].Setup();
			}
		}

		void FinishScenario()
		{
			if (bInRegion)
			{
				TRACE_END_REGION(*Scenarios[Index].Name);
				bInRegion = false;
			}
			FResult Result;
			Result.Name = Scenarios[Index].Name;
			Result.Frames = FrameMs.Num();
			if (Result.Frames > 0)
			{
				double Sum = 0.0;
				for (float Ms : FrameMs) { Sum += Ms; }
				Result.AvgFrameMs = Sum / Result.Frames;
				TArray<float> Sorted = FrameMs;
				Sorted.Sort();
				Result.P95FrameMs = Sorted[FMath::Min(Sorted.Num() - 1, static_cast<int32>(Sorted.Num() * 0.95))];
			}
			Result.AvgGameMs = GameSamples > 0 ? GameMsSum / GameSamples : 0.0;
			Result.AvgGpuMs = GameSamples > 0 ? GpuMsSum / GameSamples : 0.0;
			Result.UserWidgets = CountUserWidgets();
			Result.Ticking = CountUserWidgets(true);
			Result.UObjects = GUObjectArray.GetObjectArrayNumMinusAvailable();
			Result.UsedMB = UsedMB();
			Result.bValid = bValid;
			Result.bPaused = bPaused;
			Results.Add(Result);
			UE_LOG(LogMvsPerf, Log, TEXT("%-16s frames=%d avg=%.3fms p95=%.3fms game=%.3fms widgets=%d objects=%d"),
				*Result.Name, Result.Frames, Result.AvgFrameMs, Result.P95FrameMs, Result.AvgGameMs, Result.UserWidgets, Result.UObjects);
			if (Scenarios[Index].Teardown)
			{
				Scenarios[Index].Teardown();
			}
		}

		void SampleFrame(float Elapsed, float DeltaTime)
		{
			if (!Controller.IsValid() || !Scenarios.IsValidIndex(Index))
			{
				return;
			}
			if (Scenarios[Index].PerFrame)
			{
				Scenarios[Index].PerFrame(Elapsed);
			}
			if (InjectMs > 0.0 && Scenarios[Index].Name == InjectScenario)
			{
				const double Until = FPlatformTime::Seconds() + InjectMs / 1000.0;
				while (FPlatformTime::Seconds() < Until) {}
			}
			if (Elapsed > WarmupSeconds)
			{
				if (!bInRegion)
				{
					bValid = !Scenarios[Index].Check || Scenarios[Index].Check();
					bPaused = Controller->GetWorld() && Controller->GetWorld()->IsPaused();
					if (!bValid)
					{
						UE_LOG(LogMvsPerf, Error, TEXT("Scenario %s is not in the state it measures; its numbers are invalid."), *Scenarios[Index].Name);
					}
					// One Insights timing region per scenario's sampled frames: a trace taken with -trace=default
					// can then be split per scenario (Scripts/ProfileUI.ps1).
					TRACE_BEGIN_REGION(*Scenarios[Index].Name);
					bInRegion = true;
				}
				FrameMs.Add(DeltaTime * 1000.f);
				GameMsSum += FPlatformTime::ToMilliseconds(GGameThreadTime);
				GpuMsSum += FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());
				++GameSamples;
			}
		}

		void WriteReport()
		{
			FString Md = FString::Printf(TEXT("# UI performance run: %s\n\nResolution %ux%u, uncapped, %.0fs warm-up + %.0fs sampled per scenario. Start memory %.0f MB.\n\n"),
				*Label, GSystemResolution.ResX, GSystemResolution.ResY, WarmupSeconds, SampleSeconds(), StartMB);
			// Columns are only ever appended: G5 reads the first five, so reports from older builds stay comparable.
			Md += TEXT("| Scenario | Frames | Avg frame (ms) | P95 frame (ms) | Avg game thread (ms) | UUserWidgets | ticking | UObjects | Used MB | Avg GPU (ms) | Valid | Paused |\n|---|---|---|---|---|---|---|---|---|---|---|---|\n");
			for (const FResult& R : Results)
			{
				Md += FString::Printf(TEXT("| %s | %d | %.3f | %.3f | %.3f | %d | %d | %d | %.0f | %.3f | %s | %s |\n"),
					*R.Name, R.Frames, R.AvgFrameMs, R.P95FrameMs, R.AvgGameMs, R.UserWidgets, R.Ticking, R.UObjects, R.UsedMB, R.AvgGpuMs,
					R.bValid ? TEXT("yes") : TEXT("no"), R.bPaused ? TEXT("yes") : TEXT("no"));
			}
			const IConsoleVariable* Invalidation = IConsoleManager::Get().FindConsoleVariable(TEXT("Slate.EnableGlobalInvalidation"));
			Md += FString::Printf(TEXT("\nSlate.EnableGlobalInvalidation = %d\n"), Invalidation ? Invalidation->GetInt() : -1);
			const FString Path = FPaths::ProjectSavedDir() / TEXT("Perf") / (Label + TEXT(".md"));
			FFileHelper::SaveStringToFile(Md, *Path);
			UE_LOG(LogMvsPerf, Log, TEXT("Report written to %s"), *Path);
		}
	};
}

void FMvsPerfHarness::Start(AMvsPlayerController* Controller, const FString& Label)
{
	ULocalPlayer* LocalPlayer = Controller ? Controller->GetLocalPlayer() : nullptr;
	UMvsUISubsystem* UI = LocalPlayer ? LocalPlayer->GetSubsystem<UMvsUISubsystem>() : nullptr;
	UMvsViewModelSubsystem* ViewModels = LocalPlayer ? LocalPlayer->GetSubsystem<UMvsViewModelSubsystem>() : nullptr;
	if (!UI || !ViewModels)
	{
		return;
	}

	TSharedRef<FRun> Run = MakeShared<FRun>();
	Run->Controller = Controller;
	Run->Label = Label;
	FString Inject;
	if (FParse::Value(FCommandLine::Get(), TEXT("MvsPerfInject="), Inject))
	{
		FString Ms;
		if (Inject.Split(TEXT(":"), &Run->InjectScenario, &Ms))
		{
			Run->InjectMs = FCString::Atod(*Ms);
			UE_LOG(LogMvsPerf, Log, TEXT("Injecting %.3f ms per frame into '%s' (gate self-test)"), Run->InjectMs, *Run->InjectScenario);
		}
	}

	const TWeakObjectPtr<AMvsPlayerController> WeakPC(Controller);
	const TWeakObjectPtr<UMvsUISubsystem> WeakUI(UI);
	const TWeakObjectPtr<UMvsViewModelSubsystem> WeakVMs(ViewModels);
	auto Hero = [WeakPC]() { return WeakPC.IsValid() ? Cast<AMvsCharacter>(WeakPC->GetPawn()) : nullptr; };
	// What each scenario must be showing while it is sampled.
	auto Showing = [WeakUI](EMvsUILayer Layer, const UClass* ScreenClass) -> TFunction<bool()>
	{
		return [WeakUI, Layer, ScreenClass]()
		{
			const UCommonActivatableWidget* Screen = WeakUI.IsValid() ? WeakUI->GetActiveScreen(Layer) : nullptr;
			return Screen && Screen->IsA(ScreenClass);
		};
	};
	const TFunction<bool()> HudShowing = Showing(EMvsUILayer::Game, UCommonActivatableWidget::StaticClass());

	// 0. Reference: the same world with the whole UI layer hidden, so the other rows can be read as UI cost.
	Run->Scenarios.Add({ TEXT("no-ui"),
		[WeakUI]() { if (WeakUI.IsValid()) { WeakUI->SetLayoutVisible(false); } },
		nullptr,
		[WeakUI]() { if (WeakUI.IsValid()) { WeakUI->SetLayoutVisible(true); } } });

	// 1. The HUD sitting idle in combat.
	Run->Scenarios.Add({ TEXT("hud-idle"), nullptr, nullptr, nullptr, HudShowing });

	// 2. HUD while the combo meter and gadget cooldowns animate every frame.
	Run->Scenarios.Add({ TEXT("hud-animating"),
		nullptr,
		[Hero](float Seconds)
		{
			if (AMvsCharacter* H = Hero())
			{
				if (FMath::Fmod(Seconds, 0.4f) < 0.02f) { H->Attack(); }
				if (FMath::Fmod(Seconds, 3.2f) < 0.02f) { H->UseGadget(0); H->UseGadget(1); H->UseGadget(2); }
			}
		},
		nullptr, HudShowing });

	// 3. Forensic Mode fully on (post-process + overlay material + objective tracker).
	Run->Scenarios.Add({ TEXT("forensic"),
		[Hero]() { if (AMvsCharacter* H = Hero()) { H->ToggleForensic(); } },
		nullptr,
		[Hero]() { if (AMvsCharacter* H = Hero()) { H->ToggleForensic(); } },
		[Hero]() { const AMvsCharacter* H = Hero(); return H && H->GetForensicComponent() && H->GetForensicComponent()->IsActive(); } });

	// 4. Gadget wheel open with a hovered segment sweeping around.
	Run->Scenarios.Add({ TEXT("gadget-wheel"),
		[WeakUI]() { if (WeakUI.IsValid()) { WeakUI->HandleShortcut(TEXT("GadgetWheel")); } },
		[](float Seconds)
		{
			// Sweep the stick around so the hovered segment (and its animation) keeps changing.
			for (TObjectIterator<UGadgetWheelScreen> It; It; ++It)
			{
				It->SetStickInput(FVector2D(FMath::Sin(Seconds * 3.f), FMath::Cos(Seconds * 3.f)));
			}
		},
		[WeakUI]() { if (WeakUI.IsValid()) { WeakUI->PopTopScreen(); } },
		Showing(EMvsUILayer::GameMenu, UGadgetWheelScreen::StaticClass()) });

	// The case file with 505 clues (the level's five plus 500 fake ones), for the two case-file scenarios.
	const TFunction<void()> OpenCaseFile = [WeakUI, WeakVMs]()
	{
		if (WeakVMs.IsValid()) { WeakVMs->AddDebugClues(500); }
		// Pushed, not toggled: right after the other case-file scenario, its screen is still on the stack for its outro,
		// and the key would close that instead of opening one (as with pause-quit, found by the Valid check).
		if (WeakUI.IsValid()) { WeakUI->PushScreen(EMvsUILayer::Menu, GetDefault<UMvsUISettings>()->ClueLogClass); }
	};
	const TFunction<void()> CloseCaseFile = [WeakUI, WeakVMs]()
	{
		if (WeakUI.IsValid()) { WeakUI->PopTopScreen(); }
		// Later scenarios (and the HUD's objective) must see the level's real clues again, not 505.
		if (UClueListViewModel* Clues = WeakVMs.IsValid() ? WeakVMs->GetClues() : nullptr)
		{
			Clues->SetEntries(Clues->GetEntries().FilterByPredicate([](const UClueEntryViewModel* Entry) { return Entry && !Entry->IsDebug(); }));
		}
	};
	const TFunction<bool()> CaseFileFull = [IsCaseFile = Showing(EMvsUILayer::Menu, UClueLogScreen::StaticClass())]()
	{
		// The case file, with the 505 entries and tiles on screen.
		for (TObjectIterator<UMvsClueTileView> It; It && IsCaseFile(); ++It)
		{
			if (!It->HasAnyFlags(RF_ClassDefaultObject) && It->GetNumItems() >= 500 && It->GetDisplayedEntryWidgets().Num() > 0)
			{
				return true;
			}
		}
		return false;
	};

	// 5. Case file scrolled continuously, at up to 120 rows a second: the worst case for the pooled list. The tile view
	// re-adds every visible tile on each frame its offset moves, so this is the per-frame cost of any scrolling.
	Run->Scenarios.Add({ TEXT("case-file-505"),
		OpenCaseFile,
		[](float Seconds)
		{
			// Scroll back and forth across the whole board (offset is in rows of 4 tiles); the pool must keep rebinding tiles.
			for (TObjectIterator<UMvsClueTileView> It; It; ++It)
			{
				It->SetScrollOffset(62.f + 60.f * FMath::Sin(Seconds * 2.f));
			}
		},
		CloseCaseFile,
		CaseFileFull });

	// 5b. The same case file browsed the way a player does: one row every 0.15 s (a held d-pad's repeat), down 30 rows
	// and back, through the list's own navigation (it selects the tile and scrolls it into view). Most frames are still
	// between steps, so this is what browsing costs on average, next to the worst case above.
	Run->Scenarios.Add({ TEXT("case-file-browse"),
		OpenCaseFile,
		[LastRow = MakeShared<int32>(INDEX_NONE)](float Seconds)
		{
			constexpr int32 Rows = 30;
			const int32 Step = FMath::FloorToInt(Seconds / 0.15f) % (2 * Rows);
			const int32 Row = Step < Rows ? Step : 2 * Rows - Step;
			if (Row == *LastRow)
			{
				return;
			}
			*LastRow = Row;
			for (TObjectIterator<UMvsClueTileView> It; It; ++It)
			{
				if (!It->HasAnyFlags(RF_ClassDefaultObject))
				{
					It->NavigateToIndex(Row * 4);
				}
			}
		},
		CloseCaseFile,
		CaseFileFull });

	// 6. Settings screen open (rows, scroll box, buttons).
	Run->Scenarios.Add({ TEXT("settings"),
		[WeakUI]() { if (WeakUI.IsValid()) { WeakUI->PushScreen(EMvsUILayer::Menu, GetDefault<UMvsUISettings>()->SettingsScreenClass); } },
		nullptr,
		[WeakUI]() { if (WeakUI.IsValid()) { WeakUI->PopTopScreen(); } },
		Showing(EMvsUILayer::Menu, USettingsScreen::StaticClass()) });

	// 7. The quit confirmation over pause: a modal over a menu, the one case where two background blurs are on screen.
	Run->Scenarios.Add({ TEXT("pause-quit"),
		// Pushed, not toggled: settings' teardown leaves it on the stack for its outro, and a toggle would pop it again
		// instead of opening pause (found by the Valid check in review S0; the scenario never showed the modal).
		[WeakUI]() { if (WeakUI.IsValid()) { WeakUI->PushScreen(EMvsUILayer::Menu, GetDefault<UMvsUISettings>()->PauseMenuClass); } },
		[](float)
		{
			// Once pause is up, ask to quit the way the menu does (only once: the modal then stays open).
			for (TObjectIterator<UConfirmModalScreen> It; It; ++It)
			{
				if (!It->HasAnyFlags(RF_ClassDefaultObject) && It->IsActivated())
				{
					return;
				}
			}
			for (TObjectIterator<UPauseMenuScreen> It; It; ++It)
			{
				if (!It->HasAnyFlags(RF_ClassDefaultObject) && It->IsActivated())
				{
					It->RequestQuit();
				}
			}
		},
		[]()
		{
			// The modal answers "No" as it closes; then pause closes and unpauses.
			for (TObjectIterator<UConfirmModalScreen> It; It; ++It)
			{
				if (!It->HasAnyFlags(RF_ClassDefaultObject) && It->IsActivated()) { It->DeactivateWidget(); }
			}
			for (TObjectIterator<UPauseMenuScreen> It; It; ++It)
			{
				if (!It->HasAnyFlags(RF_ClassDefaultObject) && It->IsActivated()) { It->DeactivateWidget(); }
			}
		},
		Showing(EMvsUILayer::Modal, UConfirmModalScreen::StaticClass()) });

	// 8. Combat: two thugs telegraphing on a loop (one counter prompt in view, one edge arrow), combo milestones.
	// Outside this scenario the attack director is off, so the other rows measure the same idle thugs.
	const TWeakObjectPtr<UMvsThreatSubsystem> WeakThreats(Controller->GetWorld()->GetSubsystem<UMvsThreatSubsystem>());
	if (WeakThreats.IsValid())
	{
		WeakThreats->SetDirectorEnabled(false);
	}
	Run->Scenarios.Add({ TEXT("combat"),
		nullptr,
		[WeakThreats, Hero](float Seconds)
		{
			if (FMath::Fmod(Seconds, 2.f) < 0.02f && WeakThreats.IsValid())
			{
				WeakThreats->ForceWarningOnVisible();
				WeakThreats->ForceWarningBehind();
			}
			if (AMvsCharacter* H = Hero())
			{
				if (FMath::Fmod(Seconds, 0.25f) < 0.02f) { H->Attack(); }
			}
		},
		nullptr, HudShowing });

	// 9. The no-ui reference with the world paused, for scenarios that pause it (pause-quit). A paused world does far less
	// game-thread work, so against no-ui those scenarios would read as negative UI cost. Last, so the scenarios before it
	// run at the same point of the session as in builds without it (G5 compares with those).
	Run->Scenarios.Add({ TEXT("no-ui-paused"),
		[WeakUI, WeakPC]()
		{
			if (WeakUI.IsValid()) { WeakUI->SetLayoutVisible(false); }
			if (WeakPC.IsValid()) { UGameplayStatics::SetGamePaused(WeakPC.Get(), true); }
		},
		nullptr,
		[WeakUI, WeakPC]()
		{
			if (WeakPC.IsValid()) { UGameplayStatics::SetGamePaused(WeakPC.Get(), false); }
			if (WeakUI.IsValid()) { WeakUI->SetLayoutVisible(true); }
		},
		[WeakPC]() { return WeakPC.IsValid() && WeakPC->GetWorld() && WeakPC->GetWorld()->IsPaused(); } });

	UE_LOG(LogMvsPerf, Log, TEXT("Perf run '%s' starting (%d scenarios)"), *Label, Run->Scenarios.Num());
	Run->Begin();
}

#endif // !UE_BUILD_SHIPPING
