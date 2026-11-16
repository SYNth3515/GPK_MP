
void NetClient_CreateInterfaceEnvironment()
{
	SetEventHandler("Control Activation", "NetClient_ProcessInterfaceControls", 0);
}

void NetClient_DeleteInterfaceEnvironment()
{
	DelEventHandler("Control Activation", "NetClient_ProcessInterfaceControls");
}

void NetClient_ProcessInterfaceControls()
{
	string ControlName = GetEventData();
	if( ControlName == "gamechat" )	LaunchNetInterface_Chat();
	if( ControlName == "gamestat" )	LaunchNetInterface_GameStat();
	if( ControlName == "toplist" )	LaunchNetInterface_TopList();
}

void LaunchNetInterface_Chat()
{
	if(procInterfacePrepare(INTERFACE_NET_CHAT))
	{
		nPrevInterface = -1;
		CurrentInterface = INTERFACE_NET_CHAT;
		InitInterface(Interfaces[CurrentInterface].IniFile);
		NetLand_KeepSceneBehind();
	}
}

void LaunchNetInterface_GameStat()
{
	if(procInterfacePrepare(INTERFACE_NET_GAMESTATE))
	{
		nPrevInterface = -1;
		CurrentInterface = INTERFACE_NET_GAMESTATE;
		InitInterface(Interfaces[CurrentInterface].IniFile);
		NetLand_KeepSceneBehind();
	}
}

void LaunchNetInterface_TopList()
{
	if(procInterfacePrepare(INTERFACE_NET_TOPLIST))
	{
		nPrevInterface = -1;
		CurrentInterface = INTERFACE_NET_TOPLIST;
		InitInterface(Interfaces[CurrentInterface].IniFile);
	}
}

void LaunchNetInterface_Dump()
{
	// registered here as well as in BaseInterface.c, so the jettison window starts even if that file is an old one
	Interfaces[INTERFACE_NET_DUMP].SectionName = "GPK\net\interfaces\DumpCargo.c";
	Interfaces[INTERFACE_NET_DUMP].IniFile = "RESOURCE\INI\INTERFACES\Net\DumpCargo.ini";

	if(procInterfacePrepare(INTERFACE_NET_DUMP))
	{
		nPrevInterface = -1;
		CurrentInterface = INTERFACE_NET_DUMP;
		InitInterface(Interfaces[CurrentInterface].IniFile);
	}
}

void FreezeMainNetLayers(bool bFreeze)
{
	if( bFreeze ) {
		LayerFreeze("Net_Realize",true);
		//LayerFreeze("Net_Execute",true);
	} else {
		LayerFreeze("Net_Realize",false);
		//LayerFreeze("Net_Execute",false);
	}
}
