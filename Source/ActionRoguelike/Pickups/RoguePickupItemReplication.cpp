


#include "RoguePickupItemReplication.h"
#include "RoguePickupSubsystem.h"
#include "Components/InstancedStaticMeshComponent.h"


void FPickupLocationsArray::PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize)
{
	if (AddedIndices.Num() == 0)
	{
		return;
	}
	
	TArray<FTransform> NewTransforms;
	NewTransforms.Reserve(AddedIndices.Num());
	
	for (int Index = 0; Index < AddedIndices.Num(); ++Index)
	{
		const FPickupLocationItem Item = Items[AddedIndices[Index]];
		const FVector Scale = VisualType == ERoguePickupVisualType::Experience ? FVector(0.35f) : FVector::OneVector;
		NewTransforms.Add(FTransform(FRotator::ZeroRotator, Item.CoinLocation, Scale));
	}

	TArray<FPrimitiveInstanceId> NewIDs = VisualType == ERoguePickupVisualType::Experience
		? OwningSubsystem->AddExperienceMeshInstances(NewTransforms)
		: OwningSubsystem->AddCoinMeshInstances(NewTransforms);

	// Map all new IDs back into the matching items, to delete them later
	for (int i = 0; i < AddedIndices.Num(); ++i)
	{
		FPickupLocationItem& Item = Items[AddedIndices[i]];
		Item.ID = NewIDs[i];
	}
}

void FPickupLocationsArray::PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize)
{
	if (RemovedIndices.Num() == 0)
	{
		return;
	}
	
	TArray<FPrimitiveInstanceId> IDsToRemove;
	IDsToRemove.Reserve(RemovedIndices.Num());
	
	for (int32& RemovedIndex : RemovedIndices)
	{
		IDsToRemove.Add(Items[RemovedIndex].ID);
	}

	if (VisualType == ERoguePickupVisualType::Experience)
	{
		OwningSubsystem->RemoveExperienceMeshInstances(IDsToRemove);
	}
	else
	{
		OwningSubsystem->RemoveCoinMeshInstances(IDsToRemove);
	}
}
