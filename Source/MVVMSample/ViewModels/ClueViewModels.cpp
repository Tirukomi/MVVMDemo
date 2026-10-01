// Copyright IG. All Rights Reserved.

#include "ViewModels/ClueViewModels.h"

#include "Engine/AssetManager.h"
#include "Engine/Texture2D.h"

#define LOCTEXT_NAMESPACE "Mvs.Clues"

void UClueEntryViewModel::Initialize(FName InClueId, const FText& InTitle, const FText& InDescription, const TSoftObjectPtr<UTexture2D>& InThumbnail)
{
	ClueId = InClueId;
	Title = InTitle;
	Description = InDescription;
	ThumbnailPath = InThumbnail;
	RefreshDisplayText();
}

void UClueEntryViewModel::SetDiscovered(bool bInDiscovered)
{
	UE_MVVM_SET_PROPERTY_VALUE_INLINE(bIsDiscovered, bInDiscovered);
	RefreshDisplayText();
}

void UClueEntryViewModel::RefreshDisplayText()
{
	UE_MVVM_SET_PROPERTY_VALUE(DisplayTitle, bIsDiscovered ? Title : LOCTEXT("Unknown", "???"));
	UE_MVVM_SET_PROPERTY_VALUE(DisplayDescription, bIsDiscovered ? Description : LOCTEXT("UnknownBody", "Not yet investigated."));
}

void UClueEntryViewModel::RequestThumbnail()
{
	if (Thumbnail || ThumbnailHandle.IsValid() || ThumbnailPath.IsNull())
	{
		return;
	}

	if (UTexture2D* Ready = ThumbnailPath.Get())
	{
		UE_MVVM_SET_PROPERTY_VALUE(Thumbnail, Ready);
		return;
	}

	UE_MVVM_SET_PROPERTY_VALUE_INLINE(bIsThumbnailLoading, true);
	ThumbnailHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		ThumbnailPath.ToSoftObjectPath(),
		FStreamableDelegate::CreateUObject(this, &UClueEntryViewModel::OnThumbnailLoaded));
}

void UClueEntryViewModel::OnThumbnailLoaded()
{
	ThumbnailHandle.Reset();
	UE_MVVM_SET_PROPERTY_VALUE(Thumbnail, ThumbnailPath.Get());
	UE_MVVM_SET_PROPERTY_VALUE_INLINE(bIsThumbnailLoading, false);
}

void UClueEntryViewModel::CancelThumbnail()
{
	if (ThumbnailHandle.IsValid())
	{
		ThumbnailHandle->CancelHandle();
		ThumbnailHandle.Reset();
		UE_MVVM_SET_PROPERTY_VALUE_INLINE(bIsThumbnailLoading, false);
	}
}

void UClueListViewModel::SetEntries(TArray<TObjectPtr<UClueEntryViewModel>> InEntries)
{
	Entries = MoveTemp(InEntries);
	// Entries may arrive already discovered (rebinding after progress), so recount rather than assume zero.
	DiscoveredCount = 0;
	for (const UClueEntryViewModel* Entry : Entries)
	{
		DiscoveredCount += Entry->GetIsDiscovered() ? 1 : 0;
	}
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Entries);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(DiscoveredCount);
}

bool UClueListViewModel::MarkDiscovered(FName ClueId)
{
	for (UClueEntryViewModel* Entry : Entries)
	{
		if (Entry->GetClueId() == ClueId)
		{
			if (Entry->GetIsDiscovered())
			{
				return false;
			}
			Entry->SetDiscovered(true);
			UE_MVVM_SET_PROPERTY_VALUE(DiscoveredCount, DiscoveredCount + 1);
			return true;
		}
	}
	return false;
}

#undef LOCTEXT_NAMESPACE
