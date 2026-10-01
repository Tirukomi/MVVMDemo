// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/GothamDevAids.h"

#if !UE_BUILD_SHIPPING

#include "Accessibility/GothamSettingsSubsystem.h"
#include "Core/GothamCharacter.h"
#include "Core/GothamMenuInputTest.h"
#include "Core/GothamPerfHarness.h"
#include "Core/GothamScript.h"
#include "Core/GothamPlayerController.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "EnhancedInputSubsystems.h"
#include "Gameplay/ClueActor.h"
#include "Gameplay/ComboComponent.h"
#include "Gameplay/DetectiveComponent.h"
#include "Gameplay/HealthComponent.h"
#include "Gameplay/ThreatSubsystem.h"
#include "UI/ClueEntryWidget.h"
#include "UI/GothamUISettings.h"
#include "UI/Layout/GothamUISubsystem.h"
#include "UI/Screens/GadgetWheelScreen.h"
#include "UI/Screens/PauseMenuScreen.h"
#include "UnrealClient.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "ViewModels/ClueViewModels.h"
#include "ViewModels/GothamViewModelSubsystem.h"
#include "ViewModels/ObjectivesViewModel.h"
#include "ViewModels/SettingsViewModel.h"

DEFINE_LOG_CATEGORY_STATIC(LogGothamDevAids, Log, All);

namespace GothamDevAidsPrivate
{
	using FWeakPC = TWeakObjectPtr<AGothamPlayerController>;

	/** Every flag, read once. */
	struct FFlags
	{
		bool bQuit = false;
		bool bPause = false;
		bool bWheel = false;
		bool bDetective = false;
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
			bQuit = FParse::Param(Cmd, TEXT("GothamOpenQuit"));
			bPause = FParse::Param(Cmd, TEXT("GothamOpenPause")) || bQuit;
			bWheel = FParse::Param(Cmd, TEXT("GothamOpenWheel"));
			bDetective = FParse::Param(Cmd, TEXT("GothamDetective"));
			FParse::Value(Cmd, TEXT("GothamClueLog="), StressCount);
			// "-GothamClueLog" and "-GothamClueLog=N" both open the case file.
			bClueLog = FParse::Param(Cmd, TEXT("GothamClueLog")) || StressCount > 0;
			bSettings = FParse::Param(Cmd, TEXT("GothamOpenSettings"));
			bControls = FParse::Param(Cmd, TEXT("GothamOpenControls"));
			bCycleLanguage = FParse::Param(Cmd, TEXT("GothamCycleLanguage"));
			bRebindDemo = FParse::Param(Cmd, TEXT("GothamRebindDemo"));
			bReveal = FParse::Param(Cmd, TEXT("GothamDetectiveReveal"));
			bAnalyse = FParse::Param(Cmd, TEXT("GothamDetectiveAnalyse"));
			bHudDemo = FParse::Param(Cmd, TEXT("GothamHudDemo"));
			bCombatDemo = FParse::Param(Cmd, TEXT("GothamCombatDemo"));
			bPlainShot = FParse::Param(Cmd, TEXT("GothamShot")) || bHudDemo || bCombatDemo;
		}

		bool AnyScene() const
		{
			return bPause || bWheel || bDetective || bClueLog || bSettings || bControls || bCycleLanguage || bRebindDemo || bPlainShot || StressCount > 0;
		}

		FString ShotName() const
		{
			FString Name = bPlainShot ? TEXT("gotham_hud") : bQuit ? TEXT("gotham_quit") : bRebindDemo ? TEXT("gotham_controls") : bPause ? TEXT("gotham_pause") : bWheel ? TEXT("gotham_wheel") : bDetective ? TEXT("gotham_detective") : bSettings ? TEXT("gotham_settings") : bControls ? TEXT("gotham_controls") : TEXT("gotham_cluelog");
			// -GothamShotName=<name> overrides the file name, so parallel captures of the same screen never collide.
			FParse::Value(FCommandLine::Get(), TEXT("GothamShotName="), Name);
			return Name;
		}
	};

	float ShotDelay()
	{
		float Delay = 4.f;
		FParse::Value(FCommandLine::Get(), TEXT("GothamShotDelay="), Delay);
		return Delay;
	}

	AGothamCharacter* HeroOf(const FWeakPC& WeakPC)
	{
		return WeakPC.IsValid() ? Cast<AGothamCharacter>(WeakPC->GetPawn()) : nullptr;
	}

	/**
	 * The dev aids' actions, each at a time from start-up. Run turns them into one script (real time, so paused and
	 * slowed screens do not stop it); actions due at the same time run in the order they were added.
	 */
	struct FTimeline
	{
		struct FEntry { float At; FGothamScript::FAction Action; };
		TArray<FEntry> Entries;

		void After(float Seconds, FGothamScript::FAction Action) { Entries.Add({ Seconds, MoveTemp(Action) }); }

		void Run()
		{
			if (Entries.IsEmpty())
			{
				return;
			}
			Entries.StableSort([](const FEntry& A, const FEntry& B) { return A.At < B.At; });
			TSharedRef<FGothamScript> Script = MakeShared<FGothamScript>();
			for (FEntry& Entry : Entries)
			{
				Script->At(Entry.At).Do(MoveTemp(Entry.Action));
			}
			Script->Start();
		}
	};

	/** Screenshot runs must be deterministic: the first mouse delta after window capture would otherwise swing the camera. */
	void IgnoreLookForShots(AGothamPlayerController* Controller)
	{
		const TCHAR* Cmd = FCommandLine::Get();
		if (FParse::Param(Cmd, TEXT("GothamShot")) || FParse::Param(Cmd, TEXT("GothamOpenWheel")) || FParse::Param(Cmd, TEXT("GothamDetective"))
			|| FParse::Param(Cmd, TEXT("GothamOpenPause")) || FParse::Param(Cmd, TEXT("GothamOpenQuit")) || FParse::Param(Cmd, TEXT("GothamOpenSettings")) || FParse::Param(Cmd, TEXT("GothamClueLog"))
			|| FParse::Param(Cmd, TEXT("GothamHudDemo")) || FParse::Param(Cmd, TEXT("GothamCombatDemo")))
		{
			Controller->SetIgnoreLookInput(true);
		}
	}

	/** -GothamQuitAfterLoad, -GothamMenuInputTest, -GothamPerf: runs that take over and quit on their own. */
	bool RunSelfContained(AGothamPlayerController* Controller, FTimeline& Timeline)
	{
		const TCHAR* Cmd = FCommandLine::Get();
		// Quits as soon as the level and HUD are up: a cheap first launch that gets a freshly built project's one-off
		// start-up work out of the way (the perf gate's warm-up).
		if (FParse::Param(Cmd, TEXT("GothamQuitAfterLoad")))
		{
			Timeline.After(1.f, [] { FPlatformMisc::RequestExit(false); });
			return true;
		}
		// Drives the menus through Slate input and logs PASS / FAIL per rule, then quits.
		if (FParse::Param(Cmd, TEXT("GothamMenuInputTest")))
		{
			FGothamMenuInputTest::Start(Controller);
			return true;
		}
		// Runs the UI performance harness and quits (see Docs/Performance.md).
		FString PerfLabel;
		if (FParse::Value(Cmd, TEXT("GothamPerf="), PerfLabel) && !PerfLabel.IsEmpty())
		{
			FGothamPerfHarness::Start(Controller, PerfLabel);
			return true;
		}
		return false;
	}

	/** Screenshots must be reproducible: no thug picks a random moment to attack (and flash the vignette) mid-shot. */
	void DisableAttackDirector(AGothamPlayerController* Controller)
	{
		if (UGothamThreatSubsystem* Threats = Controller->GetWorld()->GetSubsystem<UGothamThreatSubsystem>())
		{
			Threats->SetDirectorEnabled(false);
		}
	}

	/** -GothamOpenPause / -Quit / -Wheel / -Settings / -Controls. */
	void OpenScreens(const FFlags& F, UGothamUISubsystem* UI, FTimeline& Timeline)
	{
		if (F.bPause)
		{
			UI->TogglePauseMenu();
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
		if (F.bWheel)
		{
			UI->OpenGadgetWheel();
		}
		if (F.bSettings)
		{
			UI->PushScreen(EGothamUILayer::Menu, GetDefault<UGothamUISettings>()->SettingsScreenClass.LoadSynchronous());
		}
		if (F.bControls)
		{
			UI->PushScreen(EGothamUILayer::Menu, GetDefault<UGothamUISettings>()->ControlsScreenClass.LoadSynchronous());
		}
	}

	/** One second in: the wheel hover and combo, the detective stand-off and scan, the case file (with fake clues). */
	void StageScene(const FFlags& F, const FWeakPC& WeakPC, const TWeakObjectPtr<UGothamUISubsystem>& WeakUI, FTimeline& Timeline)
	{
		Timeline.After(1.f, [F, WeakPC, WeakUI]
		{
			AGothamCharacter* Hero = HeroOf(WeakPC);
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
			if (F.bDetective || F.bClueLog)
			{
				// Stand a few metres from the first clue, facing it and the rest of the roof.
				for (TActorIterator<AClueActor> It(Hero->GetWorld()); It; ++It)
				{
					const FVector Clue = It->GetActorLocation();
					Hero->SetActorLocation(Clue + FVector(-420.f, -260.f, 40.f));
					WeakPC->SetControlRotation(FRotator(-14.f, (Clue - Hero->GetActorLocation()).Rotation().Yaw + 12.f, 0.f));
					break;
				}
				// -GothamDetectiveReveal / -GothamDetectiveAnalyse open the mode later, so the shot lands mid-effect.
				if (!F.bReveal && !F.bAnalyse)
				{
					Hero->ToggleDetective();
					Hero->ScanClue();
				}
			}
			if ((F.bClueLog || F.StressCount > 0) && WeakUI.IsValid())
			{
				if (F.StressCount > 0)
				{
					if (auto* ViewModels = WeakPC->GetLocalPlayer()->GetSubsystem<UGothamViewModelSubsystem>())
					{
						ViewModels->AddDebugClues(F.StressCount);
					}
				}
				WeakUI->ToggleClueLog();
			}
		});
	}

	/** -GothamDetectiveReveal / -GothamDetectiveAnalyse: open the mode (and start analysing) so the shot lands mid-effect. */
	void DetectiveMidEffect(const FFlags& F, const FWeakPC& WeakPC, FTimeline& Timeline)
	{
		if (!F.bDetective || !(F.bReveal || F.bAnalyse))
		{
			return;
		}
		const float Delay = ShotDelay();
		// Reveal: open 0.8 s before the shot (the opening pulse is ~half way). Analyse: open earlier, start analysing
		// 0.55 s before the shot (about half of the 1.1 s analysis).
		Timeline.After(F.bReveal ? Delay - 0.8f : Delay - 2.f, [WeakPC]
		{
			if (AGothamCharacter* Hero = HeroOf(WeakPC))
			{
				Hero->ToggleDetective();
			}
		});
		if (F.bAnalyse)
		{
			Timeline.After(Delay - 0.55f, [WeakPC]
			{
				if (AGothamCharacter* Hero = HeroOf(WeakPC))
				{
					Hero->GetDetectiveComponent()->BeginAnalyse();
				}
			});
		}
	}

	/** -GothamHudDemo: a representative combat moment, a recent hit, a live combo and a gadget recharging. */
	void HudDemo(const FFlags& F, const FWeakPC& WeakPC, FTimeline& Timeline)
	{
		if (!F.bHudDemo)
		{
			return;
		}
		Timeline.After(5.f, [WeakPC]
		{
			if (AGothamCharacter* Hero = HeroOf(WeakPC))
			{
				Hero->GetHealthComponent()->ApplyDamage(38.f);
				Hero->UseGadget(1);
			}
		});
		Timeline.After(7.85f, [WeakPC]
		{
			if (AGothamCharacter* Hero = HeroOf(WeakPC))
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
	 * -GothamCombatDemo: forced telegraphs timed so the screenshot lands mid-warning, one thug in view (counter prompt)
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
			if (UGothamThreatSubsystem* Threats = World ? World->GetSubsystem<UGothamThreatSubsystem>() : nullptr)
			{
				Threats->ForceWarningOnVisible();
				Threats->ForceWarningBehind();
			}
		});
		Timeline.After(Delay - 0.6f, [WeakPC]
		{
			if (AGothamCharacter* Hero = HeroOf(WeakPC))
			{
				for (int32 i = 0; i < 10; ++i)
				{
					Hero->GetComboComponent()->RegisterHit();
				}
			}
		});
	}

	/**
	 * -GothamRebindDemo: rebinds Scan (keyboard slot) from E to R through Enhanced Input user settings, then opens the
	 * controls screen so the result is visible. Exercises the same MapPlayerKey and save path the screen uses.
	 */
	void RebindDemo(const FFlags& F, const FWeakPC& WeakPC, const TWeakObjectPtr<UGothamUISubsystem>& WeakUI, FTimeline& Timeline)
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
				UE_LOG(LogGothamDevAids, Log, TEXT("Rebind demo: Scan -> R (failure tags: %d)"), Failure.Num());
			}
			WeakUI->PushScreen(EGothamUILayer::Menu, GetDefault<UGothamUISettings>()->ControlsScreenClass.LoadSynchronous());
		});
	}

	/** -GothamCycleLanguage: proves live language switching (with -GothamOpenSettings the screen is already open). */
	void CycleLanguage(const FFlags& F, const FWeakPC& WeakPC, FTimeline& Timeline)
	{
		if (!F.bCycleLanguage)
		{
			return;
		}
		Timeline.After(2.f, [WeakPC]
		{
			if (UGothamSettingsSubsystem* Settings = WeakPC.IsValid() ? UGothamSettingsSubsystem::Get(WeakPC.Get()) : nullptr)
			{
				Settings->GetViewModel()->Cycle(EGothamSetting::Language, 1);
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
				if (const UGothamViewModelSubsystem* ViewModels = LocalPlayer ? LocalPlayer->GetSubsystem<UGothamViewModelSubsystem>() : nullptr)
				{
					UE_LOG(LogGothamDevAids, Log, TEXT("Objective: %d / %d (clue list holds %d entries)"),
						ViewModels->GetObjectives()->GetFoundCount(), ViewModels->GetObjectives()->GetTotalCount(), ViewModels->GetClues()->GetTotalCount());
				}
				// Proof of virtualisation: rows alive should stay near the viewport size, not the item count.
				int32 Rows = 0;
				for (TObjectIterator<UClueEntryWidget> It; It; ++It)
				{
					Rows += It->HasAnyFlags(RF_ClassDefaultObject) ? 0 : 1;
				}
				UE_LOG(LogGothamDevAids, Log, TEXT("Clue log: %d row widgets alive for %d fake clues"), Rows, StressCount);
			}
			FScreenshotRequest::RequestScreenshot(ShotName, true, false);
		});
	}
}

namespace GothamDevAids
{
	void Run(AGothamPlayerController* Controller, UGothamUISubsystem* UI)
	{
		using namespace GothamDevAidsPrivate;
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
		const TWeakObjectPtr<UGothamUISubsystem> WeakUI(UI);
		StageScene(F, WeakPC, WeakUI, Timeline);
		DetectiveMidEffect(F, WeakPC, Timeline);
		HudDemo(F, WeakPC, Timeline);
		CombatDemo(F, WeakPC, Timeline);
		RebindDemo(F, WeakPC, WeakUI, Timeline);
		CycleLanguage(F, WeakPC, Timeline);
		ScheduleScreenshot(F, WeakPC, Timeline);
		Timeline.Run();
	}
}

#endif // !UE_BUILD_SHIPPING
