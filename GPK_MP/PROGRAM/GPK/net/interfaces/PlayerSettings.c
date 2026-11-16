
void InitInterface(string iniName)
{
	int i;

	GameInterface.title = "titleNetPlayerSettings";

	EnumerateIcons("resource\textures\" + sUserFacesPath, "*.tga.tx", "FACESLIST", 0);
	PS_AppendDefaultFaceIcon();
	PS_AppendPortraitIcons();
	EnumerateIcons("resource\textures\" + sUserSailsPath, "*.tga.tx", "SAILSLIST", 0);
	EnumerateIcons("resource\textures\" + sUserFlagsPath, "*.tga.tx", "FLAGSLIST", 0);

	SetEventHandler("GetInterfaceTexture", "ScrollGetTexture", 0);
	SetEventHandler("OnSave", "PS_OnSave", 0);
	SetEventHandler("OnDelete", "PS_OnDelete", 0);
	SetEventHandler("OnSelect", "PS_OnSelect", 0);
	SetEventHandler("OnCancel", "PS_OnCancel", 0);

	SetEventHandler("TableSelectChange", "PS_TableSelectChange", 0);	

	SetEventHandler("frame", "PS_CheckButtons", 0);

	// fill colors list
	for (i=0; i<SAILS_COLOR_QUANTITY; i++)
	{
		string attrName = "pic" + (i + 1);
		GameInterface.COLORSLIST.(attrName).name1 = SailsColors[i].icon;
	}
	GameInterface.COLORSLIST.ListSize = SAILS_COLOR_QUANTITY;

	SendMessage(&GameInterface, "ls", MSG_INTERFACE_INIT, iniName);

	PS_FillProfilesTable();
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
	for (int i=0; i<iCount; i++)
	{
		string sSource = "f" + i;
		string sTarget = "pic" + (iStart + i + 1);
		GameInterface.FACESLIST.(sTarget).name1 = oPortraits.(sSource).FilePath;
		GameInterface.FACESLIST.(sTarget).FileName = oPortraits.(sSource).FileName;
		GameInterface.FACESLIST.(sTarget).FileName.Name = oPortraits.(sSource).Name;
		GameInterface.FACESLIST.(sTarget).FileName.Ext = oPortraits.(sSource).Ext;
	}
	GameInterface.FACESLIST.ListSize = iStart + iCount;
	Trace("PlayerSettings: added single-player portraits: " + iCount);
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
        if (iSelected > 22)
        {
            GameInterface.TABLE_PROFILES.top = iSelected - 22;
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

    aref arProfile;
    arProfile = GetAttributeN(arProfiles, iSelected - 1);

    GameInterface.EDITBOX_PLAYERNAME.str = arProfile.Name;
    GameInterface.EDITBOX_PLAYERPASSWORD.str = arProfile.Password;
    GameInterface.EDITBOX_PLAYERSHIPNAME.str = arProfile.ShipName;

    // Sail color.
    ScrollImage_SetPosition("COLORSLIST", sti(arProfile.ColorSail));

    // Avatar.
    ScrollImage_SetPosition("FACESLIST",
        PS_FindIndex("FACESLIST", arProfile.ImageFace));

    // Flag.
    ScrollImage_SetPosition("FLAGSLIST",
        PS_FindIndex("FLAGSLIST", arProfile.ImageFlag));

    // Sails.
    ScrollImage_SetPosition("SAILSLIST",
        PS_FindIndex("SAILSLIST", arProfile.ImageSail));
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
	// check for Save button
	bool bSave = true;
	if (GameInterface.EDITBOX_PLAYERNAME.str == "") { bSave = false; }
	if (GameInterface.EDITBOX_PLAYERSHIPNAME.str == "") { bSave = false; }
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
	NCProfiles.Profiles.(sNick).ShipName = GameInterface.EDITBOX_PLAYERSHIPNAME.str;

	string sFacePos = "pic" + (sti(GameInterface.FACESLIST.current) + 1);
	NCProfiles.Profiles.(sNick).ImageFace = GameInterface.FACESLIST.(sFacePos).FileName.Name;

	string sSailPos = "pic" + (sti(GameInterface.SAILSLIST.current) + 1);
	NCProfiles.Profiles.(sNick).ImageSail = GameInterface.SAILSLIST.(sSailPos).FileName.Name; 

	string sFlagPos = "pic" + (sti(GameInterface.FLAGSLIST.current) + 1);
	NCProfiles.Profiles.(sNick).ImageFlag = GameInterface.FLAGSLIST.(sFlagPos).FileName.Name;

	NCProfiles.Profiles.(sNick).ColorSail = GameInterface.COLORSLIST.current;

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

void PS_OnSelect()
{
	string sFacePos = "pic" + (sti(GameInterface.FACESLIST.current) + 1);
	string sSailPos = "pic" + (sti(GameInterface.SAILSLIST.current) + 1);
	string sFlagPos = "pic" + (sti(GameInterface.FLAGSLIST.current) + 1);

	NetClient.SailColor = sti(SailsColors[sti(GameInterface.COLORSLIST.current)].Color);
	NetClient.NickName = GameInterface.EDITBOX_PLAYERNAME.str;
	NetClient.ShipName = GameInterface.EDITBOX_PLAYERSHIPNAME.str;
	NetClient.Password = GameInterface.EDITBOX_PLAYERPASSWORD.str;
	NetClient.FaceImage = GameInterface.FACESLIST.(sFacePos).FileName.Name;
	NetClient.FlagImage = GameInterface.FLAGSLIST.(sFlagPos).FileName.Name;
	NetClient.SailImage = GameInterface.SAILSLIST.(sSailPos).FileName.Name;

	NCProfiles.LastProfile.SailColor = NetClient.SailColor;
	NCProfiles.LastProfile.NickName = NetClient.NickName;
	NCProfiles.LastProfile.ShipName = NetClient.ShipName;
	NCProfiles.LastProfile.Password = NetClient.Password;
	NCProfiles.LastProfile.FaceImage = NetClient.FaceImage;
	NCProfiles.LastProfile.FlagImage = NetClient.FlagImage;
	NCProfiles.LastProfile.SailImage = NetClient.SailImage;

	IDoExit(RC_INTERFACE_NET_FINDGAME);
}

void PS_OnCancel()
{
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

	NFDelGroupTextures("FACESLIST");
	NFDelGroupTextures("SAILSLIST");
	NFDelGroupTextures("FLAGSLIST");

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
