void NetServer_OnCommand(int wNetClientID, int iMsg)
{
	ref rClient = NetServer_GetClient(wNetClientID);

	// check client for admin password
	//rClient.

	int iSubCode = NMGetByte(iMsg);
	switch (iSubCode)
	{
		case NSC_COMMAND_STARTGAME:
			NetServer_OnCommandStartGame(wNetClientID);
		break;

		case NSC_COMMAND_RESTARTGAME:
			NetServer_RestartGame();
		break;

		case NSC_COMMAND_ADMIN_LOGIN:
			NetServer_OnAdminLogin(wNetClientID, iMsg);
		break;

		case NSC_COMMAND_ADMIN_CMD:
			NetServer_OnAdminCommand(wNetClientID, iMsg);
		break;

		case NSC_COMMAND_VOTE_KICK:
			int wVotedClientID = NMGetClientID(iMsg);
		break;
	}
}
