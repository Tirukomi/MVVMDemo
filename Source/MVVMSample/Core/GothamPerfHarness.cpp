// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/GothamPerfHarness.h"

#if !UE_BUILD_SHIPPING

#include "Blueprint/UserWidget.h"
#include "Core/GothamScript.h"
#include "Core/GothamCharacter.h"
#include "Core/GothamPlayerController.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Gameplay/DetectiveComponent.h"
#include "HAL/PlatformMemory.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "DynamicRHI.h"
#include "RenderTimer.h"
#include "Gameplay/ThreatSubsystem.h"
#include "UI/ClueEntryWidget.h"
#include "UI/Screens/ConfirmModalScreen.h"
#include "UI/Screens/GadgetWheelScreen.h"
#include "UI/Screens/PauseMenuScreen.h"
#include "UI/GothamUISettings.h"
#include "UI/Layout/GothamUISubsystem.h"
#include "ViewModels/ClueViewModels.h"
#include "ViewModels/GothamViewModelSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogGothamPerf, Log, All);

namespace
{
	struct FScenario
	{
		FString Name;
		TFunction<void()> Setup;
		TFunction<void(float /*Seconds*/)> PerFrame;
		TFunction<void()> Teardown;
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
	};

	constexpr float WarmupSeconds = 2.f;
	/** Sampled seconds per scenario; -GothamPerfSeconds=<s> overrides (the gate uses 5). */
	float SampleSeconds()
	{
		static const float Seconds = []
		{
			float Value = 8.f;
			FParse::Value(FCommandLine::Get(), TEXT("GothamPerfSeconds="), Value);
			return FMath::Clamp(Value, 1.f, 60.f);
		}();
		return Seconds;
	}

	class FRun : public TSharedFromThis<FRun>
	{
	public:
		TWeakObjectPtr<AGothamPlayerController> Controller;
		FString Label;
		TArray<FScenario> Scenarios;
		TArray<FResult> Results;

		int32 Index = -1;
		TArray<float> FrameMs;
		double GameMsSum = 0.0;
		double GpuMsSum = 0.0;
		int32 GameSamples = 0;
		double StartMB = 0.0;
		/** -GothamPerfInject=<scenario>:<ms>: busy-waits that long on the game thread every frame of one scenario, so
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
			if (FParse::Param(FCommandLine::Get(), TEXT("GothamInvalidation")))
			{
				GEngine->Exec(nullptr, TEXT("Slate.EnableGlobalInvalidation 1"));
			}
			StartMB = UsedMB();
			// Each scenario: set up, run warm-up + sample seconds (sampling after the warm-up), finish. The script
			// holds this run; the run never holds the script, so there is no cycle.
			const TSharedRef<FRun> Self = AsShared();
			TSharedRef<FGothamScript> Script = MakeShared<FGothamScript>();
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
			Results.Add(Result);
			UE_LOG(LogGothamPerf, Log, TEXT("%-16s frames=%d avg=%.3fms p95=%.3fms game=%.3fms widgets=%d objects=%d"),
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
			Md += TEXT("| Scenario | Frames | Avg frame (ms) | P95 frame (ms) | Avg game thread (ms) | UUserWidgets | ticking | UObjects | Used MB | Avg GPU (ms) |\n|---|---|---|---|---|---|---|---|---|---|\n");
			for (const FResult& R : Results)
			{
				Md += FString::Printf(TEXT("| %s | %d | %.3f | %.3f | %.3f | %d | %d | %d | %.0f | %.3f |\n"),
					*R.Name, R.Frames, R.AvgFrameMs, R.P95FrameMs, R.AvgGameMs, R.UserWidgets, R.Ticking, R.UObjects, R.UsedMB, R.AvgGpuMs);
			}
			const IConsoleVariable* Invalidation = IConsoleManager::Get().FindConsoleVariable(TEXT("Slate.EnableGlobalInvalidation"));
			Md += FString::Printf(TEXT("\nSlate.EnableGlobalInvalidation = %d\n"), Invalidation ? Invalidation->GetInt() : -1);
			const FString Path = FPaths::ProjectSavedDir() / TEXT("Perf") / (Label + TEXT(".md"));
			FFileHelper::SaveStringToFile(Md, *Path);
			UE_LOG(LogGothamPerf, Log, TEXT("Report written to %s"), *Path);
		}
	};
}

void FGothamPerfHarness::Start(AGothamPlayerController* Controller, const FString& Label)
{
	ULocalPlayer* LocalPlayer = Controller ? Controller->GetLocalPlayer() : nullptr;
	UGothamUISubsystem* UI = LocalPlayer ? LocalPlayer->GetSubsystem<UGothamUISubsystem>() : nullptr;
	UGothamViewModelSubsystem* ViewModels = LocalPlayer ? LocalPlayer->GetSubsystem<UGothamViewModelSubsystem>() : nullptr;
	if (!UI || !ViewModels)
	{
		return;
	}

	TSharedRef<FRun> Run = MakeShared<FRun>();
	Run->Controller = Controller;
	Run->Label = Label;
	FString Inject;
	if (FParse::Value(FCommandLine::Get(), TEXT("GothamPerfInject="), Inject))
	{
		FString Ms;
		if (Inject.Split(TEXT(":"), &Run->InjectScenario, &Ms))
		{
			Run->InjectMs = FCString::Atod(*Ms);
			UE_LOG(LogGothamPerf, Log, TEXT("Injecting %.3f ms per frame into '%s' (gate self-test)"), Run->InjectMs, *Run->InjectScenario);
		}
	}

	const TWeakObjectPtr<AGothamPlayerController> WeakPC(Controller);
	const TWeakObjectPtr<UGothamUISubsystem> WeakUI(UI);
	const TWeakObjectPtr<UGothamViewModelSubsystem> WeakVMs(ViewModels);
	auto Hero = [WeakPC]() { return WeakPC.IsValid() ? Cast<AGothamCharacter>(WeakPC->GetPawn()) : nullptr; };

	// 0. Reference: the same world with the whole UI layer hidden, so the other rows can be read as UI cost.
	Run->Scenarios.Add({ TEXT("no-ui"),
		[WeakUI]() { if (WeakUI.IsValid()) { WeakUI->SetLayoutVisible(false); } },
		nullptr,
		[WeakUI]() { if (WeakUI.IsValid()) { WeakUI->SetLayoutVisible(true); } } });

	// 1. The HUD sitting idle in combat.
	Run->Scenarios.Add({ TEXT("hud-idle"), nullptr, nullptr, nullptr });

	// 2. HUD while the combo meter and gadget cooldowns animate every frame.
	Run->Scenarios.Add({ TEXT("hud-animating"),
		nullptr,
		[Hero](float Seconds)
		{
			if (AGothamCharacter* H = Hero())
			{
				if (FMath::Fmod(Seconds, 0.4f) < 0.02f) { H->Attack(); }
				if (FMath::Fmod(Seconds, 3.2f) < 0.02f) { H->UseGadget(0); H->UseGadget(1); H->UseGadget(2); }
			}
		},
		nullptr });

	// 3. Detective Mode fully on (post-process + overlay material + objective tracker).
	Run->Scenarios.Add({ TEXT("detective"),
		[Hero]() { if (AGothamCharacter* H = Hero()) { H->ToggleDetective(); } },
		nullptr,
		[Hero]() { if (AGothamCharacter* H = Hero()) { H->ToggleDetective(); } } });

	// 4. Gadget wheel open with a hovered segment sweeping around.
	Run->Scenarios.Add({ TEXT("gadget-wheel"),
		[WeakUI]() { if (WeakUI.IsValid()) { WeakUI->OpenGadgetWheel(); } },
		[](float Seconds)
		{
			// Sweep the stick around so the hovered segment (and its animation) keeps changing.
			for (TObjectIterator<UGadgetWheelScreen> It; It; ++It)
			{
				It->SetStickInput(FVector2D(FMath::Sin(Seconds * 3.f), FMath::Cos(Seconds * 3.f)));
			}
		},
		[WeakUI]() { if (WeakUI.IsValid()) { WeakUI->PopTopScreen(); } } });

	// 5. Case file with 505 clues, scrolled continuously (worst case for the pooled list).
	Run->Scenarios.Add({ TEXT("case-file-505"),
		[WeakUI, WeakVMs]()
		{
			if (WeakVMs.IsValid()) { WeakVMs->AddDebugClues(500); }
			if (WeakUI.IsValid()) { WeakUI->ToggleClueLog(); }
		},
		[](float Seconds)
		{
			// Scroll back and forth across the whole board (offset is in rows of 4 tiles); the pool must keep rebinding tiles.
			for (TObjectIterator<UGothamClueTileView> It; It; ++It)
			{
				It->SetScrollOffset(62.f + 60.f * FMath::Sin(Seconds * 2.f));
			}
		},
		[WeakUI, WeakVMs]()
		{
			if (WeakUI.IsValid()) { WeakUI->PopTopScreen(); }
			// Later scenarios (and the HUD's objective) must see the level's real clues again, not 505.
			if (UClueListViewModel* Clues = WeakVMs.IsValid() ? WeakVMs->GetClues() : nullptr)
			{
				Clues->SetEntries(Clues->GetEntries().FilterByPredicate([](const UClueEntryViewModel* Entry) { return Entry && !Entry->IsDebug(); }));
			}
		} });

	// 6. Settings screen open (rows, scroll box, buttons).
	Run->Scenarios.Add({ TEXT("settings"),
		[WeakUI]() { if (WeakUI.IsValid()) { WeakUI->PushScreen(EGothamUILayer::Menu, GetDefault<UGothamUISettings>()->SettingsScreenClass); } },
		nullptr,
		[WeakUI]() { if (WeakUI.IsValid()) { WeakUI->PopTopScreen(); } } });

	// 7. The quit confirmation over pause: a modal over a menu, the one case where two background blurs are on screen.
	Run->Scenarios.Add({ TEXT("pause-quit"),
		[WeakUI]() { if (WeakUI.IsValid()) { WeakUI->TogglePauseMenu(); } },
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
		} });

	// 8. Combat: two thugs telegraphing on a loop (one counter prompt in view, one edge arrow), combo milestones.
	// Outside this scenario the attack director is off, so the other rows measure the same idle thugs.
	const TWeakObjectPtr<UGothamThreatSubsystem> WeakThreats(Controller->GetWorld()->GetSubsystem<UGothamThreatSubsystem>());
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
			if (AGothamCharacter* H = Hero())
			{
				if (FMath::Fmod(Seconds, 0.25f) < 0.02f) { H->Attack(); }
			}
		},
		nullptr });

	UE_LOG(LogGothamPerf, Log, TEXT("Perf run '%s' starting (%d scenarios)"), *Label, Run->Scenarios.Num());
	Run->Begin();
}

#endif // !UE_BUILD_SHIPPING
