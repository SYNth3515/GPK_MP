void InitInterface(string iniName)
{
	GameInterface.title = "";

	SendMessage(&GameInterface, "ls", MSG_INTERFACE_INIT, iniName);
	SetEventHandler("OnEndGameOk", "ICancelProcedure", 0);
	SetEventHandler("exitCancel", "ICancelProcedure", 0);

	FreezeGroupControls("BattleInterfaceControls", true);

	SetFormatedText("TEXT_ENDGAME", oGameOver.Str);
}

void IDoExit(int exitCode)
{
	DelEventHandler("OnEndGameOk", "ICancelProcedure");

	interfaceResultCommand = exitCode;
	EndCancelInterface(true);
}

void ICancelProcedure()
{
	// COOP s30: the host: everybody back to the lobby, the server stays (server\OnStart.c)
	// (not a preview, net\Land.c: Esc closes its server, the lobby would start it again at once)
	if (bNetHostExitToLobby && NetServer_IsStarted() && iNetPreview <= 0)
	{
		IDoExit(RC_INTERFACE_DO_NOTHING);
		NetServer_ReturnToLobby();
		return;
	}
	// COOP: after the game is over everybody goes to the main menu - the same path as the "Exit" button in the
	// find-game screen. Sound, music and all net objects are released, so the next game starts from a clean state.
	Net_Stop();

	IDoExit( RC_INTERFACE_NET_EXIT );

	ReturnToMainMenu();
}
