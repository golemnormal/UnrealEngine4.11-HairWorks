// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "XmppPrivatePCH.h"
#include "XmppConnection.h"

bool FXmppUserJid::ParseResource(FString& OutAppId, FString& OutPlatform) const
{
	OutAppId.Empty();
	OutPlatform.Empty();

	TArray<FString> ParsedResource;
	if (Resource.ParseIntoArray(ParsedResource, TEXT(":"), false) > 1)
	{
		if (ParsedResource[0].StartsWith(TEXT("V")))
		{
			uint32 Version = FCString::Atoi((*ParsedResource[0]) + 1);
			if (Version == 2)
			{
				if (ParsedResource.Num() >= 3)
				{
					OutAppId = ParsedResource[1];
					OutPlatform = ParsedResource[2];
					return true;
				}
			}
		}
	}
	else
	{
		FString ClientId, NotUsed;
		if (Resource.Split(TEXT("-"), &ClientId, &NotUsed) &&
			!ClientId.IsEmpty())
		{
			OutAppId = ClientId;
			return true;
		}
	}
	return false;
}
