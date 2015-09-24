// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneSequence.h"
#include "NiagaraAnimation.generated.h"


/**
 * Movie scene animation used by Niagara.
 */
UCLASS(BlueprintType, MinimalAPI)
class UNiagaraAnimation
	: public UMovieSceneSequence
{
	GENERATED_UCLASS_BODY()

public:
	// UMovieSceneAnimation overrides

	virtual bool AllowsSpawnableObjects() const override;
	virtual void BindPossessableObject(const FGuid& ObjectId, UObject& PossessedObject) override;
	virtual bool CanPossessObject(UObject& Object) const override;
	virtual void DestroyAllSpawnedObjects() override;
	virtual UObject* FindObject(const FGuid& ObjectId) const override;
	virtual FGuid FindObjectId(UObject& Object) const override;
	virtual UMovieScene* GetMovieScene() const override;
	virtual UObject* GetParentObject(UObject* Object) const override;
	virtual void SpawnOrDestroyObjects(bool DestroyAll) override;
	virtual void UnbindPossessableObjects(const FGuid& ObjectId) override;

#if WITH_EDITOR
	virtual bool TryGetObjectDisplayName(const FGuid& ObjectId, FText& OutDisplayName) const override;
#endif

	/** Pointer to the movie scene that controls this animation. */
	UPROPERTY()
	UMovieScene* MovieScene;
};