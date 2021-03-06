// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UdpMessagingPrivatePCH.h"
#if WITH_EDITOR
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#endif
#include "ModuleInterface.h"
#include "ModuleManager.h"


DEFINE_LOG_CATEGORY(LogUdpMessaging);

#define LOCTEXT_NAMESPACE "FUdpMessagingModule"


/**
 * Implements the UdpMessagingModule module.
 */
class FUdpMessagingModule
	: public FSelfRegisteringExec
	, public IModuleInterface
{
public:

	// FSelfRegisteringExec interface

	virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override
	{
		if (!FParse::Command(&Cmd, TEXT("UDPMESSAGING")))
		{
			return false;
		}

		if (FParse::Command(&Cmd, TEXT("STATUS")))
		{
			UUdpMessagingSettings* Settings = GetMutableDefault<UUdpMessagingSettings>();

			// bridge status
			if (MessageBridge.IsValid())
			{
				if (MessageBridge->IsEnabled())
				{
					Ar.Log(TEXT("Message Bridge: Initialized and enabled"));
				}
				else
				{
					Ar.Log(TEXT("Message Bridge: Initialized, but disabled"));
				}
			}
			else
			{
				Ar.Log(TEXT("Message Bridge: Not initialized."));
			}

			// bridge settings
			Ar.Logf(TEXT("  Unicast Endpoint: %s"), *Settings->UnicastEndpoint);
			Ar.Logf(TEXT("  Multicast Endpoint: %s"), *Settings->MulticastEndpoint);
			Ar.Logf(TEXT("  Multicast TTL: %i"), Settings->MulticastTimeToLive);

			if (Settings->StaticEndpoints.Num() > 0)
			{
				Ar.Log(TEXT("  Static Endpoints:"));

				for (const auto& StaticEndpoint : Settings->StaticEndpoints)
				{
					Ar.Logf(TEXT("  > %s"), *StaticEndpoint);
				}
			}
			else
			{
				Ar.Log(TEXT("  Static Endpoints: None"));
			}

#if PLATFORM_DESKTOP
			// tunnel status
			if (MessageTunnel.IsValid())
			{
				if (MessageTunnel->IsServerRunning())
				{
					Ar.Log(TEXT("Message Tunnel: Initialized and started"));
				}
				else
				{
					Ar.Log(TEXT("Message Tunnel: Initialized, but stopped"));
				}
			}
			else
			{
				Ar.Log(TEXT("Message Tunnel: Not initialized."));
			}

			Ar.Logf(TEXT("  Unicast Endpoint: %s"), *Settings->TunnelUnicastEndpoint);
			Ar.Logf(TEXT("  Multicast Endpoint: %s"), *Settings->TunnelMulticastEndpoint);
			Ar.Log(TEXT("  Remote Endpoints:"));

			for (const auto& RemoteEndpoint : Settings->RemoteTunnelEndpoints)
			{
				Ar.Logf(TEXT("  > %s"), *RemoteEndpoint);
			}

			if (MessageTunnel.IsValid())
			{
				Ar.Logf(TEXT("  Total Bytes In: %i"), MessageTunnel->GetTotalInboundBytes());
				Ar.Logf(TEXT("  Total Bytes Out: %i"), MessageTunnel->GetTotalOutboundBytes());

				TArray<TSharedPtr<IUdpMessageTunnelConnection>> Connections;
			
				if (MessageTunnel->GetConnections(Connections) > 0)
				{
					Ar.Log(TEXT("  Active Connections:"));

					for (const auto& Connection : Connections)
					{
						Ar.Logf(TEXT("  > %s, Open: %s, Uptime: %s, Bytes Received: %i, Bytes Sent: %i"),
							*Connection->GetName().ToString(),
							Connection->IsOpen() ? *GYes.ToString() : *GNo.ToString(),
							*Connection->GetUptime().ToString(),
							Connection->GetTotalBytesReceived(),
							Connection->GetTotalBytesSent()
						);
					}
				}
				else
				{
					Ar.Log(TEXT("  Active Connections: None"));
				}
			}
#endif
		}
		else if (FParse::Command(&Cmd, TEXT("RESTART")))
		{
			RestartServices();
		}
		else if (FParse::Command(&Cmd, TEXT("SHUTDOWN")))
		{
			ShutdownBridge();
#if PLATFORM_DESKTOP
			ShutdownTunnel();
#endif
		}
		else
		{
			// show usage
			Ar.Log(TEXT("Usage: UDPMESSAGING <Command>"));
			Ar.Log(TEXT(""));
			Ar.Log(TEXT("Command"));
			Ar.Log(TEXT("    RESTART = Restarts the message bridge and message tunnel, if enabled"));
			Ar.Log(TEXT("    SHUTDOWN = Shut down the message bridge and message tunnel, if running"));
			Ar.Log(TEXT("    STATUS = Displays the status of the UDP message transport"));
		}

		return true;
	}

public:

	// IModuleInterface interface

	virtual void StartupModule() override
	{
		if (!SupportsNetworkedTransport())
		{
			return;
		}

		// load dependencies
		if (!FModuleManager::Get().LoadModule(TEXT("Networking")).IsValid())
		{
			UE_LOG(LogUdpMessaging, Error, TEXT("The required module 'Networking' failed to load. Plug-in 'UDP Messaging' cannot be used."));

			return;
		}

#if WITH_EDITOR
		// register settings
		ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");

		if (SettingsModule != nullptr)
		{
			ISettingsSectionPtr SettingsSection = SettingsModule->RegisterSettings("Project", "Plugins", "UdpMessaging",
				LOCTEXT("UdpMessagingSettingsName", "UDP Messaging"),
				LOCTEXT("UdpMessagingSettingsDescription", "Configure the UDP Messaging plug-in."),
				GetMutableDefault<UUdpMessagingSettings>()
			);

			if (SettingsSection.IsValid())
			{
				SettingsSection->OnModified().BindRaw(this, &FUdpMessagingModule::HandleSettingsSaved);
			}
		}
#endif // WITH_EDITOR

		// register application events
		FCoreDelegates::ApplicationHasReactivatedDelegate.AddRaw(this, &FUdpMessagingModule::HandleApplicationHasReactivated);
		FCoreDelegates::ApplicationWillDeactivateDelegate.AddRaw(this, &FUdpMessagingModule::HandleApplicationWillDeactivate);

		RestartServices();
	}

	virtual void ShutdownModule() override
	{
		// unregister application events
		FCoreDelegates::ApplicationHasReactivatedDelegate.RemoveAll(this);
		FCoreDelegates::ApplicationWillDeactivateDelegate.RemoveAll(this);

#if WITH_EDITOR
		// unregister settings
		ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");

		if (SettingsModule != nullptr)
		{
			SettingsModule->UnregisterSettings("Project", "Plugins", "UdpMessaging");
		}
#endif

		// shut down services
		ShutdownBridge();
#if PLATFORM_DESKTOP
		ShutdownTunnel();
#endif
	}

	virtual bool SupportsDynamicReloading() override
	{
		return true;
	}

protected:

	/** Initializes the message bridge with the current settings. */
	void InitializeBridge()
	{
		ShutdownBridge();

		UUdpMessagingSettings* Settings = GetMutableDefault<UUdpMessagingSettings>();
		bool ResaveSettings = false;

		FIPv4Endpoint UnicastEndpoint;
		FIPv4Endpoint MulticastEndpoint;

		if (!FIPv4Endpoint::Parse(Settings->UnicastEndpoint, UnicastEndpoint))
		{
			if (!Settings->UnicastEndpoint.IsEmpty())
			{
				UE_LOG(LogUdpMessaging, Warning, TEXT("Invalid setting for UnicastEndpoint '%s' - binding to all local network adapters instead"), *Settings->UnicastEndpoint);
			}

			UnicastEndpoint = FIPv4Endpoint::Any;
			Settings->UnicastEndpoint = UnicastEndpoint.ToText().ToString();
			ResaveSettings = true;
		}

		if (!FIPv4Endpoint::Parse(Settings->MulticastEndpoint, MulticastEndpoint))
		{
			if (!Settings->MulticastEndpoint.IsEmpty())
			{
				UE_LOG(LogUdpMessaging, Warning, TEXT("Invalid setting for MulticastEndpoint '%s' - using default endpoint '%s' instead"), *Settings->MulticastEndpoint, *UDP_MESSAGING_DEFAULT_MULTICAST_ENDPOINT.ToText().ToString());
			}

			MulticastEndpoint = UDP_MESSAGING_DEFAULT_MULTICAST_ENDPOINT;
			Settings->MulticastEndpoint = MulticastEndpoint.ToText().ToString();
			ResaveSettings = true;
		}

		if (Settings->MulticastTimeToLive == 0)
		{
			Settings->MulticastTimeToLive = 1;
			ResaveSettings = true;		
		}

		if (ResaveSettings)
		{
			Settings->SaveConfig();
		}

		UE_LOG(LogUdpMessaging, Log, TEXT("Initializing bridge on interface %s to multicast group %s."), *UnicastEndpoint.ToString(), *MulticastEndpoint.ToText().ToString());

		MessageBridge = FMessageBridgeBuilder()
			.UsingTransport(MakeShareable(new FUdpMessageTransport(UnicastEndpoint, MulticastEndpoint, Settings->MulticastTimeToLive)));
	}

#if PLATFORM_DESKTOP
	/** Initializes the message tunnel with the current settings. */
	void InitializeTunnel()
	{
		ShutdownTunnel();

		UUdpMessagingSettings* Settings = GetMutableDefault<UUdpMessagingSettings>();
		bool ResaveSettings = false;

		FIPv4Endpoint UnicastEndpoint;
		FIPv4Endpoint MulticastEndpoint;

		if (!FIPv4Endpoint::Parse(Settings->TunnelUnicastEndpoint, UnicastEndpoint))
		{
			UE_LOG(LogUdpMessaging, Warning, TEXT("Invalid setting for UnicastEndpoint '%s' - binding to all local network adapters instead"), *Settings->UnicastEndpoint);

			UnicastEndpoint = FIPv4Endpoint::Any;
			Settings->TunnelUnicastEndpoint = UnicastEndpoint.ToString();
			ResaveSettings = true;
		}

		if (!FIPv4Endpoint::Parse(Settings->TunnelMulticastEndpoint, MulticastEndpoint))
		{
			UE_LOG(LogUdpMessaging, Warning, TEXT("Invalid setting for MulticastEndpoint '%s' - using default endpoint '%s' instead"), *Settings->MulticastEndpoint, *UDP_MESSAGING_DEFAULT_MULTICAST_ENDPOINT.ToText().ToString());

			MulticastEndpoint = UDP_MESSAGING_DEFAULT_MULTICAST_ENDPOINT;
			Settings->TunnelMulticastEndpoint = MulticastEndpoint.ToString();
			ResaveSettings = true;
		}

		if (ResaveSettings)
		{
			Settings->SaveConfig();
		}

		UE_LOG(LogUdpMessaging, Log, TEXT("Initializing tunnel on interface %s to multicast group %s."), *UnicastEndpoint.ToString(), *MulticastEndpoint.ToText().ToString());

		MessageTunnel = MakeShareable(new FUdpMessageTunnel(UnicastEndpoint, MulticastEndpoint));

		// initiate connections
		for (int32 EndpointIndex = 0; EndpointIndex < Settings->RemoteTunnelEndpoints.Num(); ++EndpointIndex)
		{
			FIPv4Endpoint RemoteEndpoint;

			if (FIPv4Endpoint::Parse(Settings->RemoteTunnelEndpoints[EndpointIndex], RemoteEndpoint))
			{
				MessageTunnel->Connect(RemoteEndpoint);
			}
			else
			{
				UE_LOG(LogUdpMessaging, Warning, TEXT("Invalid UDP RemoteTunnelEndpoint '%s' - skipping"), *Settings->RemoteTunnelEndpoints[EndpointIndex]);
			}
		}
	}
#endif

	/** Restarts the bridge and tunnel services. */
	void RestartServices()
	{
		const UUdpMessagingSettings& Settings = *GetDefault<UUdpMessagingSettings>();

		if (Settings.EnableTransport)
		{
			InitializeBridge();
		}
		else
		{
			ShutdownBridge();
		}

#if PLATFORM_DESKTOP
		if (Settings.EnableTunnel)
		{
			InitializeTunnel();
		}
		else
		{
			ShutdownTunnel();
		}
#endif
	}

	/**
	 * Checks whether networked message transport is supported.
	 *
	 * @todo udpmessaging: gmp: this should be moved into an Engine module, so it can be shared with other transports
	 * @return true if networked transport is supported, false otherwise.
	 */
	bool SupportsNetworkedTransport() const
	{
		if (FApp::IsGame())
		{
			// only allow in Debug and Development configurations for now
			if ((FApp::GetBuildConfiguration() == EBuildConfigurations::Shipping) || (FApp::GetBuildConfiguration() == EBuildConfigurations::Test))
			{
				return false;
			}

			// disallow unsupported platforms
			if (!FPlatformMisc::SupportsMessaging() || !FPlatformProcess::SupportsMultithreading())
			{
				return false;
			}
		}

		if (FApp::IsGame() || IsRunningCommandlet())
		{
			// only allow UDP transport if explicitly desired
			if (!FParse::Param(FCommandLine::Get(), TEXT("Messaging")))
			{
				return false;
			}
		}

		return true;
	}

	/** Shuts down the message bridge. */
	void ShutdownBridge()
	{
		if (MessageBridge.IsValid())
		{
			MessageBridge->Disable();
			FPlatformProcess::Sleep(0.1f);
			MessageBridge.Reset();
		}
	}

#if PLATFORM_DESKTOP
	/** Shuts down the message tunnel. */
	void ShutdownTunnel()
	{
		if (MessageTunnel.IsValid())
		{
			MessageTunnel->StopServer();
			MessageTunnel.Reset();
		}		
	}
#endif

private:

	/** Callback for when an has been reactivated (i.e. return from sleep on iOS). */
	void HandleApplicationHasReactivated()
	{
		RestartServices();
	}

	/** Callback for when the application will be deactivated (i.e. sleep on iOS).*/
	void HandleApplicationWillDeactivate()
	{
		ShutdownBridge();
#if PLATFORM_DESKTOP
		ShutdownTunnel();
#endif
	}

	/** Callback for when the settings were saved. */
	bool HandleSettingsSaved()
	{
		RestartServices();

		return true;
	}

private:

	/** Holds the message bridge if present. */
	IMessageBridgePtr MessageBridge;

#if PLATFORM_DESKTOP
	/** Holds the message tunnel if present. */
	IUdpMessageTunnelPtr MessageTunnel;
#endif
};


IMPLEMENT_MODULE(FUdpMessagingModule, UdpMessaging);


#undef LOCTEXT_NAMESPACE
