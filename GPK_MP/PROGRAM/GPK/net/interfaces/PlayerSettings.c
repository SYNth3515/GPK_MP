
#define PS_TABLE_ROWS	4		// COOP s30: the rows of the profiles table (PlayerSettings.ini)

void InitInterface(string iniName)
{
	int i;

	GameInterface.title = "titleNetPlayerSettings";

	// COOP s30: only the icons with a model of the game: the empty face, the portraits with a skin, the NPC skins (the icons
	// of NET\Faces had no model: "a pirate by the slot")
	GameInterface.FACESLIST.ListSize = 0;
	PS_AppendDefaultFaceIcon();
	PS_AppendPortraitIcons();
	PS_AppendNpcIcons();		// COOP: the NPC skins without a portrait (net\Land.c: NetLand_NpcModel), after all the portraits

	SetEventHandler("GetInterfaceTexture", "ScrollGetTexture", 0);
	SetEventHandler("OnSave", "PS_OnSave", 0);
	SetEventHandler("OnDelete", "PS_OnDelete", 0);
	SetEventHandler("OnSelect", "PS_OnSelect", 0);
	SetEventHandler("OnCancel", "PS_OnCancel", 0);

	SetEventHandler("TableSelectChange", "PS_TableSelectChange", 0);	

	SetEventHandler("frame", "PS_CheckButtons", 0);
	// COOP: "Персонаж": the loadout of the land duel in the profile (net\Land.c: NetLand_UiLo*)
	SetEventHandler("OnPsLoadout", "PS_LoOpen", 0);
	SetEventHandler("OnPsShip", "PS_ShipOpen", 0);
	SetEventHandler("OnPsPreview", "PS_Preview", 0);		// COOP: the preview of the skin
	SetEventHandler("OnLoApply", "PS_LoApply", 0);
	SetEventHandler("OnLoBack", "PS_LoBack", 0);
	SetEventHandler("CheckButtonChange", "PS_CheckButtonChange", 0);
	// COOP s30: the hero of the 3D scene behind the screen (net\MenuScene.c): the arrows turn him, "+" / "-" move the camera
	SetEventHandler("OnPsRotL", "PS_RotL", 0);
	SetEventHandler("OnPsRotR", "PS_RotR", 0);
	SetEventHandler("OnPsZoomIn", "PS_ZoomIn", 0);
	SetEventHandler("OnPsZoomOut", "PS_ZoomOut", 0);
	SetEventHandler("NetPS_PortraitsDone", "PS_PortraitsDone", 0);		// COOP s30: /portraits (net\MenuScene.c)
	NetLand_UiLoRegister(true);

	SendMessage(&GameInterface, "ls", MSG_INTERFACE_INIT, iniName);

	// COOP s30: the scene of the main menu behind the screen, the camera in front of the hero (from the ship screen it flies back)
	NetPS_SceneEnsure();
	NetPS_SetView(0);

	PS_FillProfilesTable();
	PS_RestoreFromShip();
	XI_WindowShow("WND_LOADOUT", false);
	XI_WindowDisable("WND_LOADOUT", true);
}

void PS_RotL() { NetPS_Turn(-1); }
void PS_RotR() { NetPS_Turn(1); }
void PS_ZoomIn() { NetPS_Zoom(1); }
void PS_ZoomOut() { NetPS_Zoom(-1); }

// COOP s30: back from the ship screen: the name, the password and the face that were typed here (maybe not saved yet)
void PS_RestoreFromShip()
{
	if (!CheckAttribute(&NetPSEdit, "Ret")) { return; }
	GameInterface.EDITBOX_PLAYERNAME.str = NetPSEdit.Ret.Name;
	GameInterface.EDITBOX_PLAYERPASSWORD.str = NetPSEdit.Ret.Password;
	ScrollImage_SetPosition("FACESLIST", sti(NetPSEdit.Ret.Face));
	DeleteAttribute(&NetPSEdit, "Ret");
}

// COOP s30: the ship settings of the edited profile (NetPSEdit, net\MenuScene.c): the ship screen changes them
string PS_EditStr(string sAttr, string sDef)
{
	if (CheckAttribute(&NetPSEdit, sAttr)) { return NetPSEdit.(sAttr); }
	return sDef;
}

string PS_ShipNameOf(string sNick)
{
	string sN = PS_EditStr("ShipName", "");
	if (sN == "") { sN = sNick; }
	return sN;
}

// COOP: the loadout of the profile that is edited (its nick), with the money of the profile: one million
void PS_LoOpen()
{
	sNetLandProfNick = GameInterface.EDITBOX_PLAYERNAME.str;
	iNetLandBudgetOverride = 1000000;
	DeleteAttribute(&oNetLandLo, "");
	XI_WindowShow("WND_LOADOUT", true);
	XI_WindowDisable("WND_LOADOUT", false);
	NetLand_UiLoOpen();
}

// COOP: "Корабль": the lobby screen without a server edits the ship of this profile (Buy.c: Buy_ProfModeInit)
void PS_ShipOpen()
{
	string sNick = GameInterface.EDITBOX_PLAYERNAME.str;
	if (sNick == "") { sNick = NetLand_ProfNick(); }
	sNetBuyProfNick = sNick;
	bNetBuyProfile = true;
	NetPSEdit.Ret.Name = GameInterface.EDITBOX_PLAYERNAME.str;
	NetPSEdit.Ret.Password = GameInterface.EDITBOX_PLAYERPASSWORD.str;
	NetPSEdit.Ret.Face = GameInterface.FACESLIST.current;
	IDoExit(RC_INTERFACE_NET_BUY);		// the 3D scene stays: the camera flies to the ship
}

void PS_LoBack()
{
	bNetLandUiLo = false;
	XI_WindowShow("WND_LOADOUT", false);
	XI_WindowDisable("WND_LOADOUT", true);
	sNetLandProfNick = "";
	iNetLandBudgetOverride = 0;
	DeleteAttribute(&oNetLandLo, "");		// the next server loads it again (with its money)
	SetCurrentNode("BUTTON_PS_LOADOUT");
}

void PS_LoApply()
{
	NetLand_ProfSaveLo();
	Log_SetStringToLog("Снаряжение сохранено в профиле " + NetLand_ProfNick());
	iNetMMHeroGen++;		// net\MenuScene.c: the hero of the scene in the new loadout
	PS_LoBack();
}

void PS_CheckButtonChange()
{
	string sNode = GetEventData();
	int iNum = GetEventData();
	int iState = GetEventData();
	NetLand_UiLoCheck(sNode, iState);
}

// Ensure the built-in faceless avatar is available even if the file enumerator omits it.
void PS_AppendDefaultFaceIcon()
{
	object oDefaultFace;
	string sDefaultName = "_emptyface_";
	int i;
	int iCount = NFFindFiles(&oDefaultFace, "resource\textures\" + sUserFacesPath, "_emptyface_.tga.tx", false);
	if (iCount <= 0) return;
	for (i=0; i<sti(GameInterface.FACESLIST.ListSize); i++)
	{
		string sRow = "pic" + (i + 1);
		if (GameInterface.FACESLIST.(sRow).FileName.Name == sDefaultName) return;
	}
	string sSource = "f0";
	string sTarget = "pic" + (sti(GameInterface.FACESLIST.ListSize) + 1);
	GameInterface.FACESLIST.(sTarget).name1 = oDefaultFace.(sSource).FilePath;
	GameInterface.FACESLIST.(sTarget).FileName = oDefaultFace.(sSource).FileName;
	GameInterface.FACESLIST.(sTarget).FileName.Name = oDefaultFace.(sSource).Name;
	GameInterface.FACESLIST.(sTarget).FileName.Ext = oDefaultFace.(sSource).Ext;
	GameInterface.FACESLIST.ListSize = sti(GameInterface.FACESLIST.ListSize) + 1;
}

// Add every single-player character portrait to the multiplayer avatar list.
// Keep the existing custom/user avatars and append the standard face_*.tga.tx set.
void PS_AppendPortraitIcons()
{
	object oPortraits;
	string sDirectory = "resource\textures\interfaces\portraits\256\";
	int iCount = NFFindFiles(&oPortraits, sDirectory, "face_*.tga.tx", false);
	int iStart = sti(GameInterface.FACESLIST.ListSize);
	int iAdded = 0;
	for (int i=0; i<iCount; i++)
	{
		string sSource = "f" + i;
		string sPName = oPortraits.(sSource).Name;
		if (NetLand_FaceModel(sPName) == "") { continue; }		// COOP s30: a portrait without a skin
		string sTarget = "pic" + (iStart + iAdded + 1);
		GameInterface.FACESLIST.(sTarget).name1 = oPortraits.(sSource).FilePath;
		GameInterface.FACESLIST.(sTarget).FileName = oPortraits.(sSource).FileName;
		GameInterface.FACESLIST.(sTarget).FileName.Name = sPName;
		GameInterface.FACESLIST.(sTarget).FileName.Ext = oPortraits.(sSource).Ext;
		iAdded++;
	}
	iCount = iAdded;
	GameInterface.FACESLIST.ListSize = iStart + iCount;
	Trace("PlayerSettings: added single-player portraits: " + iCount);
}

// COOP: the NPC skins (net\Land.c: NetLand_NpcModel) after all the icons, with the icon NET\Faces\face0; the profile keeps "npc_N"
void PS_AppendNpcIcons()
{
	object oNpcFace;
	// COOP s30: the NPC skins have no portrait: the empty face of NET\Faces
	int iFound = NFFindFiles(&oNpcFace, "resource\textures\" + sUserFacesPath, "_emptyface_.tga.tx", false);
	if (iFound <= 0) { Trace("PlayerSettings: no _emptyface_ icon, the NPC skins are not shown"); return; }
	// COOP s30: the portraits made by /portraits (interfaces\portraits\256\npc_N.tga.tx), the empty face for the rest
	object oNpcPort, oNpcMap;
	int iPortN = NFFindFiles(&oNpcPort, "resource\textures\interfaces\portraits\256\", "npc_*.tga.tx", false);
	int j;
	for (j=0; j<iPortN; j++)
	{
		string sPf = "f" + j;
		string sPn = oNpcPort.(sPf).Name;
		oNpcMap.(sPn) = sPf;
	}
	string sSrc = "f0";
	int iStart = sti(GameInterface.FACESLIST.ListSize);
	int iNpcN = NetLand_NpcCount();
	int iNpcAdded = 0;
	for (int i=0; i<iNpcN; i++)
	{
		if (NetLand_NpcHidden(i + 1)) { continue; }		// COOP s35
		iNpcAdded++;
		string sTarget = "pic" + (iStart + iNpcAdded);
		string sNpcName = "npc_" + (i + 1);
		if (CheckAttribute(&oNpcMap, sNpcName))
		{
			string sPSrc = oNpcMap.(sNpcName);
			GameInterface.FACESLIST.(sTarget).name1 = oNpcPort.(sPSrc).FilePath;
			GameInterface.FACESLIST.(sTarget).FileName = oNpcPort.(sPSrc).FileName;
			GameInterface.FACESLIST.(sTarget).FileName.Ext = oNpcPort.(sPSrc).Ext;
		}
		else
		{
			GameInterface.FACESLIST.(sTarget).name1 = oNpcFace.(sSrc).FilePath;
			GameInterface.FACESLIST.(sTarget).FileName = oNpcFace.(sSrc).FileName;
			GameInterface.FACESLIST.(sTarget).FileName.Ext = oNpcFace.(sSrc).Ext;
		}
		GameInterface.FACESLIST.(sTarget).FileName.Name = sNpcName;
	}
	GameInterface.FACESLIST.ListSize = iStart + iNpcAdded;
	Trace("PlayerSettings: added NPC skins: " + iNpcAdded);
}

// COOP: the model of the chosen face under the faces (the NPC skins have one icon for all)
int iPsSkinShown = -2;
void PS_ShowSkin()
{
	int iCur = sti(GameInterface.FACESLIST.current);
	if (iCur == iPsSkinShown) { return; }
	iPsSkinShown = iCur;
	string sPos = "pic" + (iCur + 1);
	string sFaceS = "";
	if (CheckAttribute(&GameInterface, "FACESLIST." + sPos + ".FileName.Name")) { sFaceS = GameInterface.FACESLIST.(sPos).FileName.Name; }
	string sMdl = NetLand_FaceModel(sFaceS);
	if (sMdl == "") { sMdl = "пират (по слоту)"; }
	SetFormatedText("TEXT_PS_SKIN", "Скин: " + sMdl);
}

// COOP: "Предпросмотр" under the faces: the chosen skin with the loadout of this profile in the big cabin, alone, with an immortal
// doll of the same skin in front of the hero (net\Land.c: NetPreview_*). The choice is not saved in the profile
// COOP s30: the name "/portraits" + "Предпросмотр": the screenshots of all the NPC skins for their portraits (net\MenuScene.c:
// NetPS_Portraits*), the screen hides itself while they are taken
void PS_PortNodes(bool bOn)
{
	SetNodeUsing("FRAME_NICK", bOn);
	SetNodeUsing("FRAME_PROFILES", bOn);
	SetNodeUsing("EDITBOX_PLAYERNAME", bOn);
	SetNodeUsing("EDITBOX_PLAYERPASSWORD", bOn);
	SetNodeUsing("INFO_BORDER", bOn);
	SetNodeUsing("FACESLIST", bOn);
	SetNodeUsing("FACESLIST_LEFT", bOn);
	SetNodeUsing("FACESLIST_RIGHT", bOn);
	SetNodeUsing("BUTTON_SAVE", bOn);
	SetNodeUsing("BUTTON_DELETE", bOn);
	SetNodeUsing("BUTTON_SELECT", bOn);
	SetNodeUsing("BUTTON_CANCEL", bOn);
	SetNodeUsing("STRINGS_CAPTION", bOn);
	SetNodeUsing("TABLE_PROFILES", bOn);
	SetNodeUsing("SCROLL_PROFILES", bOn);
	SetNodeUsing("BUTTON_PS_ROTL", bOn);
	SetNodeUsing("BUTTON_PS_ROTR", bOn);
	SetNodeUsing("BUTTON_PS_ZOOMIN", bOn);
	SetNodeUsing("BUTTON_PS_ZOOMOUT", bOn);
	SetNodeUsing("BUTTON_PS_LOADOUT", bOn);
	SetNodeUsing("BUTTON_PS_SHIP", bOn);
	SetNodeUsing("BUTTON_PS_PREVIEW", bOn);
	SetNodeUsing("TEXT_PS_SKIN", bOn);
}

void PS_PortraitsDone()
{
	PS_PortNodes(true);
	GameInterface.EDITBOX_PLAYERNAME.str = "";
	SetCurrentNode("BUTTON_CANCEL");
}

void PS_Preview()
{
	if (GameInterface.EDITBOX_PLAYERNAME.str == "/portraits")
	{
		PS_PortNodes(false);
		NetPS_PortraitsStart();
		return;
	}
	if (NetPreview_Busy())		// COOP s32 (net\Land.c)
	{
		SetFormatedText("TEXT_PS_SKIN", "Предпросмотр недоступен: запущен сервер. Закройте его в лобби");
		return;
	}
	string sNick = GameInterface.EDITBOX_PLAYERNAME.str;
	if (sNick == "") { sNick = NetLand_ProfNick(); }
	string sShipN = PS_ShipNameOf("Preview");
	string sFacePos = "pic" + (sti(GameInterface.FACESLIST.current) + 1);

	NetClient.SailColor = NetPS_EditSailColor();
	NetClient.NickName = sNick;
	NetClient.ShipName = sShipN;
	NetClient.Password = GameInterface.EDITBOX_PLAYERPASSWORD.str;
	NetClient.FaceImage = GameInterface.FACESLIST.(sFacePos).FileName.Name;
	NetClient.FlagImage = PS_EditStr("ImageFlag", "_emptyflag_");
	NetClient.SailImage = PS_EditStr("ImageSail", "_emptysail_");
	DeleteAttribute(&oNetLandLo, "");		// the loadout of this profile is read again at the start
	Trace("COOP: preview of the skin <" + NetClient.FaceImage + "> of " + sNick);

	NetPS_SceneDelete();		// net\MenuScene.c
	NetPreview_Begin(1);
	IDoExit(RC_INTERFACE_NET_FINDGAME);
}

void PS_FillProfilesTable()
{
    Table_Clear("TABLE_PROFILES", false, true, false);

    string sCurProfileName = "";

    // First priority: the last profile selected by the player.
    if (CheckAttribute(&NCProfiles, "LastProfile.NickName"))
    {
        sCurProfileName = NCProfiles.LastProfile.NickName;
    }

    // Fallback: current profile name.
    if (sCurProfileName == "" && CheckAttribute(&NCProfiles, "CurProfile"))
    {
        sCurProfileName = NCProfiles.CurProfile;
    }

    // Fallback: current network nickname.
    if (sCurProfileName == "" && CheckAttribute(&NetClient, "NickName"))
    {
        sCurProfileName = NetClient.NickName;
    }

    int iCurProfileIndex = -1;

    if (CheckAttribute(&NCProfiles, "Profiles"))
    {
        aref arProfiles;
        makearef(arProfiles, NCProfiles.Profiles);

        int iNumProfiles = GetAttributesNum(arProfiles);

        for (int i = 0; i < iNumProfiles; i++)
        {
            aref arProfile;
            arProfile = GetAttributeN(arProfiles, i);

            string sAttr = "tr" + (i + 1);
            GameInterface.TABLE_PROFILES.(sAttr).td1.str = arProfile.Name;

            if (arProfile.Name == sCurProfileName)
            {
                iCurProfileIndex = i;
            }
        }
    }

    Table_UpdateWindow("TABLE_PROFILES");

        // Select current profile before updating the table.
    if (iCurProfileIndex >= 0)
    {
        int iSelected = iCurProfileIndex + 1;

        GameInterface.TABLE_PROFILES.select = iSelected;

        // Keep the selected row visible.
        if (iSelected > PS_TABLE_ROWS)
        {
            GameInterface.TABLE_PROFILES.top = iSelected - PS_TABLE_ROWS;
        }
        else
        {
            GameInterface.TABLE_PROFILES.top = 0;
        }

        Trace("PlayerSettings: table selection = " + iSelected);
        Trace("PlayerSettings: current profile = " + sCurProfileName);
    }
    else
    {
        GameInterface.TABLE_PROFILES.select = 0;
        GameInterface.TABLE_PROFILES.top = 0;

        Trace("PlayerSettings: current profile not found: " + sCurProfileName);
    }

    Table_UpdateWindow("TABLE_PROFILES");

    // Load profile settings into interface controls.
    if (iCurProfileIndex >= 0)
    {
        PS_LoadProfile(iCurProfileIndex + 1);
    }
}

void PS_LoadProfile(int iSelected)
{
    if (iSelected < 1) return;

    aref arProfiles;
    makearef(arProfiles, NCProfiles.Profiles);

    if (iSelected > GetAttributesNum(arProfiles)) return;		// COOP: an empty row of the table

    aref arProfile;
    arProfile = GetAttributeN(arProfiles, iSelected - 1);

    GameInterface.EDITBOX_PLAYERNAME.str = arProfile.Name;
    GameInterface.EDITBOX_PLAYERPASSWORD.str = arProfile.Password;

    // COOP s30: the ship name, the sails and the flag are edited in the ship screen (the window "Герб/Паруса/Флаг")
    DeleteAttribute(&NetPSEdit, "ShipName");
    DeleteAttribute(&NetPSEdit, "ColorSail");
    DeleteAttribute(&NetPSEdit, "ImageFlag");
    DeleteAttribute(&NetPSEdit, "ImageSail");
    if (CheckAttribute(arProfile, "ShipName")) { NetPSEdit.ShipName = arProfile.ShipName; }
    if (CheckAttribute(arProfile, "ColorSail")) { NetPSEdit.ColorSail = arProfile.ColorSail; }
    if (CheckAttribute(arProfile, "ImageFlag")) { NetPSEdit.ImageFlag = arProfile.ImageFlag; }
    if (CheckAttribute(arProfile, "ImageSail")) { NetPSEdit.ImageSail = arProfile.ImageSail; }

    // Avatar.
    ScrollImage_SetPosition("FACESLIST",
        PS_FindIndex("FACESLIST", arProfile.ImageFace));
}

void PS_TableSelectChange()
{
    string sControl = GetEventData();
    int iSelected = GetEventData();

    PS_LoadProfile(iSelected);
}

int PS_FindIndex(string sControl, string sName)
{
	int iNum = sti(GameInterface.(sControl).ListSize);
	for (int i=0; i<iNum; i++)
	{
		string sRow = "pic" + (i + 1);

		if (GameInterface.(sControl).(sRow).FileName.Name == sName) return i;
	}
	return 0;
}

void PS_CheckButtons()
{
	PS_ShowSkin();		// COOP
	// COOP s30: the hero of the chosen face with the loadout of the typed nick, the ship of that profile (net\MenuScene.c)
	string sPsNick = GameInterface.EDITBOX_PLAYERNAME.str;
	if (sPsNick == "") { sPsNick = NetLand_ProfNick(); }
	string sPsFacePos = "pic" + (sti(GameInterface.FACESLIST.current) + 1);
	string sPsFace = "";
	if (CheckAttribute(&GameInterface, "FACESLIST." + sPsFacePos + ".FileName.Name")) { sPsFace = GameInterface.FACESLIST.(sPsFacePos).FileName.Name; }
	NetPS_WantHero(sPsFace, sPsNick);
	NetPS_WantProfShip(sPsNick);
	// check for Save button
	bool bSave = true;
	if (GameInterface.EDITBOX_PLAYERNAME.str == "") { bSave = false; }
	Button_SetEnable("BUTTON_SAVE", bSave);

	// check for Select button
	Button_SetEnable("BUTTON_SELECT", bSave);

	// check for Delete button
	int iSelected = GameInterface.TABLE_PROFILES.select;
	Button_SetEnable("BUTTON_DELETE", iSelected >= 1);
}

void PS_OnSave()
{
	string sNick = GameInterface.EDITBOX_PLAYERNAME.str;

	NCProfiles.Profiles.(sNick).Name = sNick;

	NCProfiles.Profiles.(sNick).Password = GameInterface.EDITBOX_PLAYERPASSWORD.str;
	NCProfiles.Profiles.(sNick).ShipName = PS_ShipNameOf(sNick);

	string sFacePos = "pic" + (sti(GameInterface.FACESLIST.current) + 1);
	NCProfiles.Profiles.(sNick).ImageFace = GameInterface.FACESLIST.(sFacePos).FileName.Name;

	// COOP s30: the sails and the flag of the ship screen (NetPSEdit); a new profile without them: the white sails, no pictures
	NCProfiles.Profiles.(sNick).ImageSail = PS_EditStr("ImageSail", "_emptysail_");
	NCProfiles.Profiles.(sNick).ImageFlag = PS_EditStr("ImageFlag", "_emptyflag_");
	NCProfiles.Profiles.(sNick).ColorSail = PS_EditStr("ColorSail", "0");
	PS_MakeCurrent();		// COOP s34: the saved profile is the current one

	PS_FillProfilesTable();
}

void PS_OnDelete()
{
	int iSelected = GameInterface.TABLE_PROFILES.select;
	if (iSelected >= 1)
	{
		string sRow = "tr" + iSelected;
		DeleteAttribute(&NCProfiles, "Profiles." + GameInterface.TABLE_PROFILES.(sRow).td1.str);
		PS_FillProfilesTable();
	}
}

// COOP s34: the edited profile becomes the current one (NetClient and NCProfiles.LastProfile). "Сохранить" did not do it:
// after Save + Preview the game came back to the old profile (Net_Stop deletes NetClient, it is read again from LastProfile)
void PS_MakeCurrent()
{
	string sFacePos = "pic" + (sti(GameInterface.FACESLIST.current) + 1);

	NetClient.SailColor = NetPS_EditSailColor();
	NetClient.NickName = GameInterface.EDITBOX_PLAYERNAME.str;
	NetClient.ShipName = PS_ShipNameOf(GameInterface.EDITBOX_PLAYERNAME.str);
	NetClient.Password = GameInterface.EDITBOX_PLAYERPASSWORD.str;
	NetClient.FaceImage = GameInterface.FACESLIST.(sFacePos).FileName.Name;
	NetClient.FlagImage = PS_EditStr("ImageFlag", "_emptyflag_");
	NetClient.SailImage = PS_EditStr("ImageSail", "_emptysail_");

	NCProfiles.LastProfile.SailColor = NetClient.SailColor;
	NCProfiles.LastProfile.NickName = NetClient.NickName;
	NCProfiles.LastProfile.ShipName = NetClient.ShipName;
	NCProfiles.LastProfile.Password = NetClient.Password;
	NCProfiles.LastProfile.FaceImage = NetClient.FaceImage;
	NCProfiles.LastProfile.FlagImage = NetClient.FlagImage;
	NCProfiles.LastProfile.SailImage = NetClient.SailImage;
	DeleteAttribute(&NCClients[wClientID], "LandModel");		// net\Land.c: the skin is taken from the face again
}

void PS_OnSelect()
{
	// COOP s32: "Выбрать" = "Сохранить" + the exit to FindGame
	if (GameInterface.EDITBOX_PLAYERNAME.str != "") { PS_OnSave(); }
	else { PS_MakeCurrent(); }

	NetPS_SceneDelete();		// COOP s30: net\MenuScene.c
	IDoExit(RC_INTERFACE_NET_FINDGAME);
}

void PS_OnCancel()
{
	NetPS_SceneDelete();		// COOP s30: net\MenuScene.c
	IDoExit(RC_INTERFACE_NET_FINDGAME);
}

void IDoExit(int exitCode)
{
	DelEventHandler("GetInterfaceTexture", "ScrollGetTexture");
	DelEventHandler("OnSave", "PS_OnSave");
	DelEventHandler("OnDelete", "PS_OnDelete");
	DelEventHandler("OnSelect", "PS_OnSelect");
	DelEventHandler("OnCancel", "PS_OnCancel");

	DelEventHandler("TableSelectChange", "PS_TableSelectChange");	

	DelEventHandler("frame", "PS_CheckButtons");
	DelEventHandler("OnPsLoadout", "PS_LoOpen");
	DelEventHandler("OnPsShip", "PS_ShipOpen");
	DelEventHandler("OnPsPreview", "PS_Preview");
	DelEventHandler("OnLoApply", "PS_LoApply");
	DelEventHandler("OnLoBack", "PS_LoBack");
	DelEventHandler("CheckButtonChange", "PS_CheckButtonChange");
	DelEventHandler("OnPsRotL", "PS_RotL");
	DelEventHandler("OnPsRotR", "PS_RotR");
	DelEventHandler("OnPsZoomIn", "PS_ZoomIn");
	DelEventHandler("OnPsZoomOut", "PS_ZoomOut");
	DelEventHandler("NetPS_PortraitsDone", "PS_PortraitsDone");
	NetLand_UiLoRegister(false);
	bNetLandUiLo = false;
	sNetLandProfNick = "";
	iNetLandBudgetOverride = 0;

	NFDelGroupTextures("FACESLIST");

	interfaceResultCommand = exitCode;
	
	EndCancelInterface(true);
	
	Net_SaveFile(false, &NCProfiles, "Profiles.nsv");
	//DeleteAttribute(&NCProfiles, "");
}

int ScrollGetTexture()
{
	string sName = GetEventData();
	int iPicIndex = GetEventData();
	string sScrollID = GetEventData();
	int iTexture = NFGetTexture(sScrollID, sName);
	Trace("ScrollGetTexture: " + iTexture + ", " + sName);
	
	return iTexture;
}
