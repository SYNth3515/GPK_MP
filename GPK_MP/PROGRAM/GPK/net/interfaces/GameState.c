
void InitInterface(string iniName)
{
	GameInterface.title = "titleNetGameState";

	GameInterface.TABLE_LIST.hr.height = 34;
	GameInterface.TABLE_LIST.hr.td1.str = XI_ConvertString("capGS_Player");
	GameInterface.TABLE_LIST.hr.td1.scale = 0.9;
	GameInterface.TABLE_LIST.hr.td2.str = "";
	GameInterface.TABLE_LIST.hr.td2.scale = 0.9;
	GameInterface.TABLE_LIST.hr.td3.str = XI_ConvertString("capGS_Team");
	GameInterface.TABLE_LIST.hr.td3.scale = 0.9;
	GameInterface.TABLE_LIST.hr.td4.str = XI_ConvertString("capGS_Damage");
	GameInterface.TABLE_LIST.hr.td4.scale = 0.9;
	GameInterface.TABLE_LIST.hr.td5.str = XI_ConvertString("capGS_Accuracy");
	GameInterface.TABLE_LIST.hr.td5.scale = 0.9;
	GameInterface.TABLE_LIST.hr.td6.str = XI_ConvertString("capGS_Score");
	GameInterface.TABLE_LIST.hr.td6.scale = 0.9;
	GameInterface.TABLE_LIST.hr.td7.str = XI_ConvertString("capGS_Win");
	GameInterface.TABLE_LIST.hr.td7.scale = 0.9;

	SendMessage(&GameInterface, "ls", MSG_INTERFACE_INIT, iniName);
	SetEventHandler("OnCancel", "ICancelProcedure", 0);
	SetEventHandler("Control Activation", "INetInterfaceControlProcess", 1);
	FreezeMainNetLayers(true);

	SetEventHandler("eventSetGameState", "EventProcessAddToTable", 0);
	SetEventHandler("evUpdateTable", "UpdateTable", 0);
	SetEventHandler("exitCancel", "ICancelProcedure", 0);
	Event("evUpdateTable");
}

void IDoExit(int exitCode)
{
	DelEventHandler("evUpdateTable", "UpdateTable");
	DelEventHandler("eventSetGameState", "EventProcessAddToTable");
	DelEventHandler("Control Activation", "INetInterfaceControlProcess");
	DelEventHandler("OnCancel", "ICancelProcedure");
	FreezeMainNetLayers(false);

	interfaceResultCommand = exitCode;
	EndCancelInterface(true);
}

void ICancelProcedure()
{
	IDoExit( RC_INTERFACE_NET_GAMESTATE );
}

void INetInterfaceControlProcess()
{
	string sControl = GetEventData();
	if( sControl == "gamestat" ) IDoExit( RC_INTERFACE_NET_GAMESTATE );
}

void UpdateTable()
{
	//PostEvent( "eventSetGameState", 1, "l", 0 );
	if (NetLand_IsActive()) { UpdateTableLand(); return; }
	object tmpData;

	for( int n=0; n<NET_MAXCLIENTS; n++ )
	{
		ref rClient = &NCClients[n];

		DeleteAttribute(&tmpData, "");

		// COOP: players in the lobby of the running game have no ship yet, ships of players who left are shown too
		bool bShown = (rClient.Use == "1");
		if (!COOP_CliSlotLive(n) || sti(rClient.Ship.Type) < 0) { bShown = false; }
		if (!bShown && COOP_CliSlotLive(n) && sti(rClient.Ship.Type) >= 0 && CheckAttribute(rClient, "NickName") && rClient.NickName != "") { bShown = true; }
		if( bShown )
		{
			//-----
			tmpData.facetexture = rClient.UserData.Face.Texture;
			tmpData.playername = rClient.NickName;
			tmpData.shipname = rClient.Ship.Name;
			tmpData.shiptype = rClient.Ship.Type;
			tmpData.Team = rClient.Team;

			//-----
			tmpData.damage = makeint(rClient.Stat.DamageInflicted);//"51437";
			tmpData.accuracy = "" + sti(rClient.Stat.Accuracy) + "%";//"73%";

			// player kills
			aref arKills; makearef(arKills, rClient.Stat.Kills);
			int iNumKills = GetAttributesNum(arKills);
			for (int u=1; u<iNumKills + 1; u++)
			{
				string sKill = "l" + u;
				tmpData.score.(sKill).facetexture = rClient.Stat.Kills.(sKill).FaceTexture;//"NET\Faces\face1.tga";
				tmpData.score.(sKill).charname = rClient.Stat.Kills.(sKill).NickName;
				tmpData.score.(sKill).shipname = rClient.Stat.Kills.(sKill).ShipName;
				tmpData.score.(sKill).shiptype = rClient.Stat.Kills.(sKill).ShipType;
			}

			// COOP: respawn mode: score = ships sunk / own ship lost (server values, right for players who joined later too)
			if (sti(NCServer.Respawn))
			{
				DeleteAttribute(&tmpData, "score");
				tmpData.scoretext = "Потоплено: " + sti(rClient.Stat.Frags) + "\nПотерь: " + sti(rClient.Stat.Deaths);
			}

			tmpData.winstate = "В бою";
			if (CheckAttribute(rClient, "ShipState"))
			{
				switch (sti(rClient.ShipState))
				{
					case 1: tmpData.winstate = "Ремонт"; break;
					case 2: tmpData.winstate = "Потоплен"; break;
				}
			}
			if (rClient.Use != "1" && !NetClient_IsBotID(n)) { tmpData.winstate = "Вышел"; }		// COOP s28: a bot did not leave

			AddToTable( sti(rClient.ID), &tmpData );
		}
	}

	PostEvent( "evUpdateTable", 1000 ); // запуск обновления данных через 3 сек.
}

// COOP: the land duel (net\Land.c): the players, their kills, deaths and the damage they have done
void UpdateTableLand()
{
	object tmpL;
	for (int n=0; n<NET_MAXPLAYERS; n++)
	{
		ref rC = &NCClients[n];
		if (!CheckAttribute(rC, "Use")) { continue; }
		if (rC.Use != "1") { continue; }
		if (!CheckAttribute(rC, "NickName")) { continue; }
		DeleteAttribute(&tmpL, "");
		tmpL.land = 1;
		tmpL.facetexture = "";
		if (CheckAttribute(rC, "UserData.Face.Texture")) { tmpL.facetexture = rC.UserData.Face.Texture; }
		tmpL.playername = rC.NickName;
		tmpL.shipname = "";
		if (CheckAttribute(rC, "LandLo.Level")) { tmpL.shipname = "Уровень " + rC.LandLo.Level; }
		if (n == wClientID) { tmpL.shipname = "Уровень " + NetLand_LoLevel(); }
		tmpL.shiptype = 0;
		tmpL.Team = 0;
		if (CheckAttribute(rC, "Team")) { tmpL.Team = rC.Team; }
		tmpL.damage = NetLand_StatValue(n, "Dmg");
		tmpL.accuracy = "-";
		tmpL.scoretext = "Убийства: " + NetLand_StatValue(n, "Kills") + "\nСмерти: " + NetLand_StatValue(n, "Deaths");
		tmpL.winstate = "Жив";
		if (NetLand_StatValue(n, "Dead") == 1) { tmpL.winstate = "Мёртв"; }
		AddToTable(n, &tmpL);
	}
	PostEvent("evUpdateTable", 1000);
}

void EventProcessAddToTable()
{
	int wClientID = GetEventData();

	object tmpData;

	tmpData.facetexture = "NET\Faces\face0.tga";
	tmpData.playername = "Temp Player Name";
	tmpData.shipname = "Temp Ship Name";
	tmpData.teamtexture = "NET\Flags\f1.tga";
	tmpData.damage = "51437";
	tmpData.accuracy = "73%";
	tmpData.score.l1.facetexture = "NET\Faces\face1.tga";
	tmpData.score.l1.charname = "Temp Looser Name";
	tmpData.score.l1.shipname = "Temp Looser Ship Name";
	tmpData.winstate = "+";

	AddToTable( wClientID, &tmpData );
}

void AddToTable(int wClientID, aref arData)
{
	int n;
	string row, sShipGroup;
	ref rShip;

	for(n=1; n<NET_TOTALCLIENTS; n++)
	{
		row = "tr"+n;
		if( !CheckAttribute(&GameInterface,"TABLE_LIST."+row+".clientID") ) // нет такого клиента - создаем новую запись
		{
			// constant data
			GameInterface.TABLE_LIST.(row).td1.icon1.offset = "0,1";
			GameInterface.TABLE_LIST.(row).td1.icon1.width = 26;
			GameInterface.TABLE_LIST.(row).td1.icon1.height = 26;
			GameInterface.TABLE_LIST.(row).td1.icon1.valign = "center";
			GameInterface.TABLE_LIST.(row).td1.icon2.offset = "29,1";
			GameInterface.TABLE_LIST.(row).td1.icon2.width = 26;
			GameInterface.TABLE_LIST.(row).td1.icon2.height = 26;
			GameInterface.TABLE_LIST.(row).td1.icon2.valign = "center";
			GameInterface.TABLE_LIST.(row).td1.textoffset = "60,0";
			GameInterface.TABLE_LIST.(row).td1.valign = "center";
			GameInterface.TABLE_LIST.(row).td1.align = "left";
			GameInterface.TABLE_LIST.(row).td1.scale = 0.9;

			GameInterface.TABLE_LIST.(row).td3.icon.offset = "9,0";
			GameInterface.TABLE_LIST.(row).td3.icon.width = 38;
			GameInterface.TABLE_LIST.(row).td3.icon.height = 32;
			GameInterface.TABLE_LIST.(row).td3.icon.valign = "center";

			GameInterface.TABLE_LIST.(row).td6.textoffset = "60,3";
			GameInterface.TABLE_LIST.(row).td6.valign = "top";
			GameInterface.TABLE_LIST.(row).td6.align = "left";
			GameInterface.TABLE_LIST.(row).td6.scale = 0.9;

			break;
		}
		if( sti(GameInterface.TABLE_LIST.(row).clientID) == wClientID )
			{break;}
	}

	rShip = Net_GetShipByIndex(sti(arData.ShipType));
	sShipGroup = "SHIPS_" + rShip.Name;

	GameInterface.TABLE_LIST.(row).clientID = wClientID;
	GameInterface.TABLE_LIST.(row).td1.icon1.texturepointer = arData.facetexture;
	GameInterface.TABLE_LIST.(row).td1.icon2.group = sShipGroup;
	GameInterface.TABLE_LIST.(row).td1.icon2.image = "ship";
	GameInterface.TABLE_LIST.(row).td1.str = arData.playername + "\n" + arData.shipname;
	if (CheckAttribute(arData, "land")) { DeleteAttribute(&GameInterface, "TABLE_LIST." + row + ".td1.icon2"); }
	//GameInterface.TABLE_LIST.(row).td3.icon.texture = arData.teamtexture;
	if (sti(NCServer.GameType) == NETGAME_DEATHMATCH && !NetLand_TeamGame())		// COOP: a team land duel shows the teams
	{
		GameInterface.TABLE_LIST.(row).td3.icon.group = "NET_TEAMICONS";
		GameInterface.TABLE_LIST.(row).td3.icon.image = "no_team";
	}
	else
	{
		GameInterface.TABLE_LIST.(row).td3.icon.group = "NET_TEAMICONS";
		GameInterface.TABLE_LIST.(row).td3.icon.image = "team" + arData.Team;
	}
	GameInterface.TABLE_LIST.(row).td4.str = arData.damage;
	GameInterface.TABLE_LIST.(row).td5.str = arData.accuracy;


	// score sinked ships
	string aname,siname,siname2;
	string sScore = "";
	aref aroot, acur;
	makearef( aroot, arData.score );
	int q = GetAttributesNum(aroot);
	for(n=0; n<q; n++)
	{
		aname = GetAttributeName( GetAttributeN(aroot,n) )
		if( sScore!="" ) sScore += "\n";
		sScore += aroot.(aname).charname + "\n" + aroot.(aname).shipname;
		
		// killed face 
		siname = "icon" + (n*2 + 1);
		GameInterface.TABLE_LIST.(row).td6.(siname).offset = "0," + (4+n*30);
		GameInterface.TABLE_LIST.(row).td6.(siname).width = 26;
		GameInterface.TABLE_LIST.(row).td6.(siname).height = 26;
		GameInterface.TABLE_LIST.(row).td6.(siname).texturepointer = aroot.(aname).facetexture;

		// killed ship 
		siname2 = "icon" + (n*2 + 2);
		rShip = Net_GetShipByIndex(sti(arData.ShipType));
		sShipGroup = "SHIPS_" + rShip.Name;
		GameInterface.TABLE_LIST.(row).td6.(siname2).offset = "29," + (4+n*30);
		GameInterface.TABLE_LIST.(row).td6.(siname2).width = 26;
		GameInterface.TABLE_LIST.(row).td6.(siname2).height = 26;
		GameInterface.TABLE_LIST.(row).td6.(siname2).group = sShipGroup;
		GameInterface.TABLE_LIST.(row).td6.(siname2).image = "ship";

		siname = "s" + (n*2+1);
		GameInterface.TABLE_LIST.(row).td6.textoffsets.(siname) = 30*n - 1;
	}
	if( q<2 ) {
		GameInterface.TABLE_LIST.(row).height = 32;
	} else {
		GameInterface.TABLE_LIST.(row).height = 2 + 30 * q;
	}
	// COOP: respawn mode: the score is two numbers (a list of every sunk ship would grow without end)
	if (CheckAttribute(arData, "scoretext"))
	{
		sScore = arData.scoretext;
		GameInterface.TABLE_LIST.(row).td6.textoffset = "4,3";
		GameInterface.TABLE_LIST.(row).height = 32;
	}
	GameInterface.TABLE_LIST.(row).td6.str = sScore;

	GameInterface.TABLE_LIST.(row).td7.str = arData.winstate;

	NextFrameRefreshTable();
}

void NextFrameRefreshTable()
{
	SetEventHandler("frame", "RefreshTableByFrameEvent", 0);
}

void RefreshTableByFrameEvent()
{
	DelEventHandler("frame", "RefreshTableByFrameEvent");
	SendMessage(&GameInterface,"lsl",MSG_INTERFACE_MSG_TO_NODE,"TABLE_LIST", 0 );
}
