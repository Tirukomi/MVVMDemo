// Copyright IG. All Rights Reserved.

#include "Core/MvsDevAids.h"

#if !UE_BUILD_SHIPPING

#include "Accessibility/MvsSettingsSubsystem.h"
#include "Core/MvsCharacter.h"
#include "Core/MvsPerfHarness.h"
#include "Core/MvsScript.h"
#include "Core/MvsPlayerController.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "EnhancedInputSubsystems.h"
#include "Gameplay/ClueActor.h"
#include "Gameplay/ComboComponent.h"
#include "Gameplay/ForensicComponent.h"
#include "Gameplay/HealthComponent.h"
#include "Gameplay/ThreatSubsystem.h"
#include "UI/ClueEntryWidget.h"
#include "UI/MvsUISettings.h"
#include "UI/Layout/MvsUISubsystem.h"
#include "UI/Screens/GadgetWheelScreen.h"
#include "UI/Screens/PauseMenuScreen.h"
#include "UnrealClient.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "ViewModels/ClueViewModels.h"
#include "ViewModels/MvsViewModelSubsystem.h"
#include "ViewModels/ObjectivesViewModel.h"
#include "ViewModels/SettingsViewModel.h"

DEFINE_LOG_CATEGORY_STATIC(LogMvsDevAids, Log, All);

namespace MvsDevAidsPrivate
{
	using FWeakPC = TWeakObjectPtr<AMvsPlayerController>;

	/** Every flag, read once. */
	struct FFlags
	{
		bool bQuit = false;
		bool bPause = false;
		bool bWheel = false;
		bool bForensic = false;
		int32 StressCount = 0;
		bool bClueLog = false;
		bool bSettings = false;
		bool bControls = false;
		bool bCycleLanguage = false;
		bool bRebindDemo = false;
		bool bReveal = false;
		bool bAnalyse = false;
		bool bHudDemo = false;
		bool bCombatDemo = false;
		bool bPlainShot = false;

		FFlags()
		{
			const TCHAR* Cmd = FCommandLine::Get();
			bQuit = FParse::Param(Cmd, TEXT("MvsOpenQuit"));
			bPause = FParse::Param(Cmd, TEXT("MvsOpenPause")) || bQuit;
			bWheel = FParse::Param(Cmd, TEXT("MvsOpenWheel"));
			bForensic = FParse::Param(Cmd, TEXT("MvsForensic"));
			FParse::Value(Cmd, TEXT("MvsClueLog="), StressCount);
			// "-MvsClueLog" and "-MvsClueLog=N" both open the case file.
			bClueLog = FParse::Param(Cmd, TEXT("MvsClueLog")) || StressCount > 0;
			bSettings = FParse::Param(Cmd, TEXT("MvsOpenSettings"));
			bControls = FParse::Param(Cmd, TEXT("MvsOpenControls"));
			bCycleLanguage = FParse::Param(Cmd, TEXT("MvsCycleLanguage"));
			bRebindDemo = FParse::Param(Cmd, TEXT("MvsRebindDemo"));
			bReveal = FParse::Param(Cmd, TEXT("MvsForensicReveal"));
			bAnalyse = FParse::Param(Cmd, TEXT("MvsForensicAnalyse"));
			bHudDemo = FParse::Param(Cmd, TEXT("MvsHudDemo"));
			bCombatDemo = FParse::Param(Cmd, TEXT("MvsCombatDemo"));
			bPlainShot = FParse::Param(Cmd, TEXT("MvsShot")) || bHudDemo || bCombatDemo;
		}

		bool AnyScene() const
		{
			return bPause || bWheel || bForensic || bClueLog || bSettings || bControls || bCycleLanguage || bRebindDemo || bPlainShot || StressCount > 0;
		}

		FString ShotName() const
		{
			FString Name = bPlainShot ? TEXT("mvs_hud") : bQuit ? TEXT("mvs_quit") : bRebindDemo ? TEXT("mvs_controls") : bPause ? TEXT("mvs_pause") : bWheel ? TEXT("mvs_wheel") : bForensic ? TEXT("mvs_forensic") : bSettings ? TEXT("mvs_settings") : bControls ? TEXT("mvs_controls") : TEXT("mvs_cluelog");
			// -MvsShotName=<name> overrides the file name, so parallel captures of the same screen never collide.
			FParse::Value(FCommandLine::Get(), TEXT("MvsShotName="), Name);
			return Name;
		}
	};

	float ShotDelay()
	{
		float Delay = 4.f;
		FParse::Value(FCommandLine::Get(), TEXT("MvsShotDelay="), Delay);
		return Delay;
	}

	AMvsCharacter* HeroOf(const FWeakPC& WeakPC)
	{
		return WeakPC.IsValid() ? Cast<AMvsCharacter>(WeakPC->GetPawn()) : nullptr;
	}

	/**
	 * The dev aids' actions, each at a time from start-up. Run turns them into one script (real time, so paused and
	 * slowed screens do not stop it); actions due at the same time run in the order they were added.
	 */
	struct FTimeline
	{
		struct FEntry { float At; FMvsScript::FAction Action; };
		TArray<FEntry> Entries;

		void After(float Seconds, FMvsScript::FAction Action) { Entries.Add({ Seconds, MoveTemp(Action) }); }

		/**
		 * Starts the actions. With UI set, the clock starts once its screen classes are preloaded: a loaded machine can
		 * take longer than a second to preload them, and opening the case file before that would load it on the spot.
		 * Counting from then keeps the actions' spacing (the mid-effect shots depend on it) however long that takes.
		 */
		void Run(const TWeakObjectPtr<UMvsUISubsystem>& UI = nullptr)
		{
			if (Entries.IsEmpty())
			{
				return;
			}
			Entries.StableSort([](const FEntry& A, const FEntry& B) { return A.At < B.At; });
			TSharedRef<FMvsScript> Script = MakeShared<FMvsScript>();
			for (FEntry& Entry : Entries)
			{
				Script->At(Entry.At).Do(MoveTemp(Entry.Action));
			}
			if (!UI.IsValid() || UI->AreScreensLoaded())
			{
				Script->Start();
				return;
			}
			TSharedRef<FMvsScript> Preload = MakeShared<FMvsScript>();
			Preload->WaitUntil([UI]() { return !UI.IsValid() || UI->AreScreensLoaded(); }, 30.f, TEXT("screen classes preloaded"))
				.Do([Script]() { Script->Start(); });
			Preload->Start();
		}
	};

	/** Screenshot runs must be deterministic: the first mouse delta after window capture would otherwise swing the camera. */
	void IgnoreLookForShots(AMvsPlayerController* Controller)
	{
		const TCHAR* Cmd = FCommandLine::Get();
		if (FParse::Param(Cmd, TEXT("MvsShot")) || FParse::Param(Cmd, TEXT("MvsOpenWheel")) || FParse::Param(Cmd, TEXT("MvsForensic"))
			|| FParse::Param(Cmd, TEXT("MvsOpenPause")) || FParse::Param(Cmd, TEXT("MvsOpenQuit")) || FParse::Param(Cmd, TEXT("MvsOpenSettings")) || FParse::Param(Cmd, TEXT("MvsClueLog"))
			|| FParse::Param(Cmd, TEXT("MvsHudDemo")) || FParse::Param(Cmd, TEXT("MvsCombatDemo")))
		{
			Controller->SetIgnoreLookInput(true);
		}
	}

	/** -MvsQuitAfterLoad, -MvsPerf: runs that take over and quit on their own. (The menu-input rules are the automation
	 *  tests Mvs.Functional.*, in the test module, so they are not in the game.) */
	bool RunSelfContained(AMvsPlayerController* Controller, FTimeline& Timeline)
	{
		const TCHAR* Cmd = FCommandLine::Get();
		// Quits as soon as the level and HUD are up: a cheap first launch that gets a freshly built project's one-off
		// start-up work out of the way (the perf gate's warm-up).
		if (FParse::Param(Cmd, TEXT("MvsQuitAfterLoad")))
		{
			Timeline.After(1.f, [] { FPlatformMisc::RequestExit(false); });
			return true;
		}
		// Runs the UI performance harness and quits (see Docs/Performance.md).
		FString PerfLabel;
		if (FParse::Value(Cmd, TEXT("MvsPerf="), PerfLabel) && !PerfLabel.IsEmpty())
		{
			FMvsPerfHarness::Start(Controller, PerfLabel);
			return true;
		}
		return false;
	}

	/** Screenshots must be reproducible: no thug picks a random moment to attack (and flash the vignette) mid-shot. */
	void DisableAttackDirector(AMvsPlayerController* Controller)
	{
		if (UMvsThreatSubsystem* Threats = Controller->GetWorld()->GetSubsystem<UMvsThreatSubsystem>())
		{
			Threats->SetDirectorEnabled(false);
		}
	}

	/** -MvsOpenPause / -Quit / -Wheel / -Settings / -Controls. */
	void OpenScreens(const FFlags& F, UMvsUISubsystem* UI, FTimeline& Timeline)
	{
		// On the first frame, as the screenshots were taken (pause freezes the world behind it at that moment). A designer's
		// Widget Blueprint is not in memory at load the way a C++ class is, so finish the screens' preload first instead
		// of loading one on the spot (S6's gate); a dev-only wait.
		if ((F.bPause || F.bWheel || F.bSettings || F.bControls) && !UI->AreScreensLoaded())
		{
			FlushAsyncLoading();
		}
		if (F.bPause)
		{
			UI->HandleShortcut(TEXT("Pause"));
		}
		if (F.bWheel)
		{
			UI->HandleShortcut(TEXT("GadgetWheel"));
		}
		if (F.bSettings)
		{
			UI->PushScreen(EMvsUILayer::Menu, GetDefault<UMvsUISettings>()->SettingsScreenClass);
		}
		if (F.bControls)
		{
			UI->PushScreen(EMvsUILayer::Menu, GetDefault<UMvsUISettings>()->ControlsScreenClass);
		}
		if (F.bQuit)
		{
			// After the pause menu's intro, the way a player would reach it.
			Timeline.After(1.f, []
			{
				for (TObjectIterator<UPauseMenuScreen> It; It; ++It)
				{
					if (!It->HasAnyFlags(RF_ClassDefaultObject) && It->IsActivated())
					{
						It->RequestQuit();
					}
				}
			});
		}
	}

	/** One second in: the wheel hover and combo, the forensic stand-off and scan, the case file (with fake clues). */
	void StageScene(const FFlags& F, const FWeakPC& WeakPC, const TWeakObjectPtr<UMvsUISubsystem>& WeakUI, FTimeline& Timeline)
	{
		Timeline.After(1.f, [F, WeakPC, WeakUI]
		{
			AMvsCharacter* Hero = HeroOf(WeakPC);
			if (!Hero)
			{
				return;
			}
			if (F.bWheel)
			{
				for (TObjectIterator<UGadgetWheelScreen> It; It; ++It)
				{
					It->SetStickInput(FVector2D(0.9, 0.3)); // hover the right-hand segment
				}
				for (int32 i = 0; i < 7; ++i)
				{
					Hero->GetComboComponent()->RegisterHit();
				}
			}
			if (F.bForensic || F.bClueLog)
			{
				// Stand a few metres from the first clue, facing it and the rest of the roof.
				for (TActorIterator<AClueActor> It(Hero->GetWorld()); It; ++It)
				{
					const FVector Clue = It->GetActorLocation();
					Hero->SetActorLocation(Clue + FVector(-420.f, -260.f, 40.f));
					WeakPC->SetControlRotation(FRotator(-14.f, (Clue - Hero->GetActorLocation()).Rotation().Yaw + 12.f, 0.f));
					break;
				}
				// -MvsForensicReveal / -MvsForensicAnalyse open the mode later, so the shot lands mid-effect.
				if (!F.bReveal && !F.bAnalyse)
				{
					Hero->ToggleForensic();
					Hero->ScanClue();
				}
			}
			if ((F.bClueLog || F.StressCount > 0) && WeakUI.IsValid())
			{
				if (F.StressCount > 0)
				{
					if (auto* ViewModels = WeakPC->GetLocalPlayer()->GetSubsystem<UMvsViewModelSubsystem>())
					{
						ViewModels->AddDebugClues(F.StressCount);
					}
				}
				WeakUI->HandleShortcut(TEXT("ClueLog"));
			}
		});
	}

	/** -MvsForensicReveal / -MvsForensicAnalyse: open the mode (and start analysing) so the shot lands mid-effect. */
	void ForensicMidEffect(const FFlags& F, const FWeakPC& WeakPC, FTimeline& Timeline)
	{
		if (!F.bForensic || !(F.bReveal || F.bAnalyse))
		{
			return;
		}
		const float Delay = ShotDelay();
		// Reveal: open 0.8 s before the shot (the opening pulse is ~half way). Analyse: open earlier, start analysing
		// 0.55 s before the shot (about half of the 1.1 s analysis).
		Timeline.After(F.bReveal ? Delay - 0.8f : Delay - 2.f, [WeakPC]
		{
			if (AMvsCharacter* Hero = HeroOf(WeakPC))
			{
				Hero->ToggleForensic();
			}
		});
		if (F.bAnalyse)
		{
			Timeline.After(Delay - 0.55f, [WeakPC]
			{
				if (AMvsCharacter* Hero = HeroOf(WeakPC))
				{
					Hero->GetForensicComponent()->BeginAnalyse();
				}
			});
		}
	}

	/** -MvsHudDemo: a representative combat moment, a recent hit, a live combo and a gadget recharging. */
	void HudDemo(const FFlags& F, const FWeakPC& WeakPC, FTimeline& Timeline)
	{
		if (!F.bHudDemo)
		{
			return;
		}
		Timeline.After(5.f, [WeakPC]
		{
			if (AMvsCharacter* Hero = HeroOf(WeakPC))
			{
				Hero->GetHealthComponent()->ApplyDamage(38.f);
				Hero->UseGadget(1);
			}
		});
		Timeline.After(7.85f, [WeakPC]
		{
			if (AMvsCharacter* Hero = HeroOf(WeakPC))
			{
				Hero->GetHealthComponent()->ApplyDamage(9.f); // lands just before the screenshot, so ghost and flash show
				for (int32 i = 0; i < 12; ++i)
				{
					Hero->GetComboComponent()->RegisterHit();
				}
			}
		});
	}

	/**
	 * -MvsCombatDemo: forced telegraphs timed so the screenshot lands mid-warning, one thug in view (counter prompt)
	 * and the one most behind the camera (a red edge arrow). The combo crosses 10 just before, so the callout shows too.
	 */
	void CombatDemo(const FFlags& F, const FWeakPC& WeakPC, FTimeline& Timeline)
	{
		if (!F.bCombatDemo)
		{
			return;
		}
		const float Delay = ShotDelay();
		Timeline.After(Delay - 0.45f, [WeakPC]
		{
			UWorld* World = WeakPC.IsValid() ? WeakPC->GetWorld() : nullptr;
			if (UMvsThreatSubsystem* Threats = World ? World->GetSubsystem<UMvsThreatSubsystem>() : nullptr)
			{
				Threats->ForceWarningOnVisible();
				Threats->ForceWarningBehind();
			}
		});
		Timeline.After(Delay - 0.6f, [WeakPC]
		{
			if (AMvsCharacter* Hero = HeroOf(WeakPC))
			{
				for (int32 i = 0; i < 10; ++i)
				{
					Hero->GetComboComponent()->RegisterHit();
				}
			}
		});
	}

	/**
	 * -MvsRebindDemo: rebinds Scan (keyboard slot) from E to R through Enhanced Input user settings, then opens the
	 * controls screen so the result is visible. Exercises the same MapPlayerKey and save path the screen uses.
	 */
	void RebindDemo(const FFlags& F, const FWeakPC& WeakPC, const TWeakObjectPtr<UMvsUISubsystem>& WeakUI, FTimeline& Timeline)
	{
		if (!F.bRebindDemo)
		{
			return;
		}
		Timeline.After(1.5f, [WeakPC, WeakUI]
		{
			if (!WeakPC.IsValid() || !WeakUI.IsValid())
			{
				return;
			}
			auto* Input = WeakPC->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
			if (UEnhancedInputUserSettings* UserSettings = Input ? Input->GetUserSettings() : nullptr)
			{
				FMapPlayerKeyArgs Args;
				Args.MappingName = TEXT("Scan");
				Args.Slot = EPlayerMappableKeySlot::First;
				Args.NewKey = EKeys::R;
				FGameplayTagContainer Failure;
				UserSettings->MapPlayerKey(Args, Failure);
				UserSettings->ApplySettings();
				// Saved like a rebind made on the Controls screen, so a restart shows whether it loads back.
				UserSettings->SaveSettings();
				UE_LOG(LogMvsDevAids, Log, TEXT("Rebind demo: Scan -> R (failure tags: %d)"), Failure.Num());
			}
			WeakUI->PushScreen(EMvsUILayer::Menu, GetDefault<UMvsUISettings>()->ControlsScreenClass);
		});
	}

	/** -MvsCycleLanguage: proves live language switching (with -MvsOpenSettings the screen is already open). */
	void CycleLanguage(const FFlags& F, const FWeakPC& WeakPC, FTimeline& Timeline)
	{
		if (!F.bCycleLanguage)
		{
			return;
		}
		Timeline.After(2.f, [WeakPC]
		{
			if (UMvsSettingsSubsystem* Settings = WeakPC.IsValid() ? UMvsSettingsSubsystem::Get(WeakPC.Get()) : nullptr)
			{
				// A preview, as the settings screen's language row makes (an open screen shows it unapplied).
				FMvsSettingsData Data = Settings->GetSettings();
				Data.Cycle(EMvsSetting::Language, 1);
				Settings->Preview(Data);
			}
		});
	}

	/** The screenshot, later than every action above so shaders (compiled on first run) and transitions have settled. */
	void ScheduleScreenshot(const FFlags& F, const FWeakPC& WeakPC, FTimeline& Timeline)
	{
		const FString ShotName = F.ShotName();
		const int32 StressCount = F.StressCount;
		Timeline.After(ShotDelay(), [ShotName, StressCount, WeakPC]
		{
			if (StressCount > 0)
			{
				// The objective tracks the level's real clues only; the fake ones exist just to stress the list.
				const ULocalPlayer* LocalPlayer = WeakPC.IsValid() ? WeakPC->GetLocalPlayer() : nullptr;
				if (const UMvsViewModelSubsystem* ViewModels = LocalPlayer ? LocalPlayer->GetSubsystem<UMvsViewModelSubsystem>() : nullptr)
				{
					UE_LOG(LogMvsDevAids, Log, TEXT("Objective: %d / %d (clue list holds %d entries)"),
						ViewModels->GetObjectives()->GetFoundCount(), ViewModels->GetObjectives()->GetTotalCount(), ViewModels->GetClues()->GetTotalCount());
				}
				// Proof of virtualisation: rows alive should stay near the viewport size, not the item count.
				int32 Rows = 0;
				for (TObjectIterator<UClueEntryWidget> It; It; ++It)
				{
					Rows += It->HasAnyFlags(RF_ClassDefaultObject) ? 0 : 1;
				}
				UE_LOG(LogMvsDevAids, Log, TEXT("Clue log: %d row widgets alive for %d fake clues"), Rows, StressCount);
			}
			FScreenshotRequest::RequestScreenshot(ShotName, true, false);
		});
	}
}

namespace MvsDevAids
{
	void Run(AMvsPlayerController* Controller, UMvsUISubsystem* UI)
	{
		using namespace MvsDevAidsPrivate;
		if (!Controller || !UI)
		{
			return;
		}
		IgnoreLookForShots(Controller);
		FTimeline Timeline;
		if (RunSelfContained(Controller, Timeline))
		{
			Timeline.Run();
			return;
		}
		const FFlags F;
		if (!F.AnyScene())
		{
			return;
		}
		DisableAttackDirector(Controller);
		OpenScreens(F, UI, Timeline);

		// The order matters where times coincide: actions due in the same frame run in the order they were added.
		const FWeakPC WeakPC(Controller);
		const TWeakObjectPtr<UMvsUISubsystem> WeakUI(UI);
		StageScene(F, WeakPC, WeakUI, Timeline);
		ForensicMidEffect(F, WeakPC, Timeline);
		HudDemo(F, WeakPC, Timeline);
		CombatDemo(F, WeakPC, Timeline);
		RebindDemo(F, WeakPC, WeakUI, Timeline);
		CycleLanguage(F, WeakPC, Timeline);
		ScheduleScreenshot(F, WeakPC, Timeline);
		Timeline.Run(WeakUI);
	}
}

#endif // !UE_BUILD_SHIPPING
