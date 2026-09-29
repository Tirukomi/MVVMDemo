// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "UI/Slate/GothamWheelTypes.h"
#include "GadgetWheel.generated.h"

class SGadgetWheel;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGadgetWheelIndexEvent, int32, ItemIndex);

/** UMG wrapper over SGadgetWheel so designers can place, style and bind the wheel without touching Slate. */
UCLASS()
class MVVMSAMPLE_API UGadgetWheel : public UWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheel", meta = (ShowOnlyInnerProperties))
	FGothamGadgetWheelStyle WheelStyle;

	/** Default items, mostly useful in the designer; at runtime a view model supplies them. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheel")
	TArray<FGothamWheelItem> Items;

	UPROPERTY(BlueprintAssignable, Category = "Wheel|Events")
	FOnGadgetWheelIndexEvent OnItemSelected;

	UPROPERTY(BlueprintAssignable, Category = "Wheel|Events")
	FOnGadgetWheelIndexEvent OnItemHovered;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheel")
	bool bReduceMotion = false;

	UFUNCTION(BlueprintCallable, Category = "Wheel")
	void SetItems(const TArray<FGothamWheelItem>& InItems);

	UFUNCTION(BlueprintCallable, Category = "Wheel")
	void SetStickInput(FVector2D Stick);

	UFUNCTION(BlueprintPure, Category = "Wheel")
	int32 GetHoveredIndex() const;

	/** Selects whatever is hovered (used by hold-to-open flows on release). Returns false if nothing is. */
	UFUNCTION(BlueprintCallable, Category = "Wheel")
	bool CommitHovered();

	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	TSharedPtr<SGadgetWheel> SlateWheel;
};
