// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StreamableManager.h"
#include "MVVMViewModelBase.h"
#include "ClueViewModels.generated.h"

class UTexture2D;

/** One row in the clue log. Undiscovered clues show placeholders; the thumbnail streams in on demand. */
UCLASS(BlueprintType)
class MVVMSAMPLE_API UClueEntryViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	void Initialize(FName InClueId, const FText& InTitle, const FText& InDescription, const TSoftObjectPtr<UTexture2D>& InThumbnail);
	void SetDiscovered(bool bInDiscovered);
	/** Where the clue sits in the world, for markers anchored to it. */
	void SetWorldLocation(const FVector& InLocation) { WorldLocation = InLocation; bHasWorldLocation = true; }
	bool GetWorldLocation(FVector& OutLocation) const { OutLocation = WorldLocation; return bHasWorldLocation; }
	/** A fake clue added by a dev aid to stress the list: never counts toward the objective. */
	void SetIsDebug(bool bInDebug) { bIsDebug = bInDebug; }
	bool IsDebug() const { return bIsDebug; }
	/** Where the entry sits in its list (the case number), set by UClueListViewModel::SetEntries. */
	int32 GetListIndex() const { return ListIndex; }
	void SetListIndex(int32 InIndex) { ListIndex = InIndex; }

	/** Starts streaming the thumbnail if it is not loaded yet. Safe to call repeatedly. */
	void RequestThumbnail();
	/** Cancels an in-flight load, e.g. when the list recycles the row that showed this entry. */
	void CancelThumbnail();

	FName GetClueId() const { return ClueId; }
	bool GetIsDiscovered() const { return bIsDiscovered; }
	const FText& GetDisplayTitle() const { return DisplayTitle; }
	const FText& GetDisplayDescription() const { return DisplayDescription; }
	UTexture2D* GetThumbnail() const { return Thumbnail; }
	bool GetIsThumbnailLoading() const { return bIsThumbnailLoading; }

protected:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter=GetIsDiscovered, meta = (AllowPrivateAccess = "true"))
	bool bIsDiscovered = false;

	/** The real title once discovered, otherwise "???". */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	FText DisplayTitle;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	FText DisplayDescription;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTexture2D> Thumbnail;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter=GetIsThumbnailLoading, meta = (AllowPrivateAccess = "true"))
	bool bIsThumbnailLoading = false;

private:
	void RefreshDisplayText();
	void OnThumbnailLoaded();

	FName ClueId;
	FVector WorldLocation = FVector::ZeroVector;
	bool bHasWorldLocation = false;
	bool bIsDebug = false;
	int32 ListIndex = INDEX_NONE;
	FText Title;
	FText Description;
	TSoftObjectPtr<UTexture2D> ThumbnailPath;
	TSharedPtr<FStreamableHandle> ThumbnailHandle;
};

/** All clues in the level plus discovery progress. The list view virtualises over Entries. */
UCLASS(BlueprintType)
class MVVMSAMPLE_API UClueListViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	/** Replaces the entries (discovered flags reset). */
	void SetEntries(TArray<TObjectPtr<UClueEntryViewModel>> InEntries);

	/** Marks a clue discovered. Returns false if unknown or already discovered. */
	bool MarkDiscovered(FName ClueId);

	const TArray<TObjectPtr<UClueEntryViewModel>>& GetEntries() const { return Entries; }
	int32 GetDiscoveredCount() const { return DiscoveredCount; }
	int32 GetTotalCount() const { return Entries.Num(); }
	/** "Found / total" (e.g. "2 / 5"), ready to bind to a text without a conversion function. */
	const FText& GetProgressText() const { return ProgressText; }

protected:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UClueEntryViewModel>> Entries;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	int32 DiscoveredCount = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	FText ProgressText;

private:
	void UpdateProgressText();
};
