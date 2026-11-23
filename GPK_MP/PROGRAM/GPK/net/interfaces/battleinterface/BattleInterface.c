#include "GPK\battle_interface\msg_control.h"

int NetBI_intNRetValue[8];
int NetBI_ChargeState[5];
int NetBI_retComValue;

object NetBInterface;

//chref.skills.repair.active > 0
//chref.skills.repair.totalcooldowntime = int max (>=0)
//chref.skills.repair.currentcooldowntime = int cur (>=0)
//NetClient_ShipStartRepair()

void NetBI_CreateShipPointer()
{
	objShipPointer.Server = false;
	objShipPointer.textures.friend = "battle_interface\Frendly.tga";
	objShipPointer.textures.enemy = "battle_interface\Enemy.tga";
	
	CreateEntity(&objShipPointer, "shippointer");
	
	LayerAddObject("net_execute", &objShipPointer, 222222);
	LayerAddObject("net_realize", &objShipPointer, -1);
}

void NetBI_SelectShipPointer(int wClientID)
{
	int isEnemy = true;
	float fShSpeed = 0.005;
	float fShAmp = 1.0;
	float fShW = 1.0;
	float fShH = 1.0;
	float fTop = 5.0;

	SendMessage(&objShipPointer, "lllfffff", MSG_SP_CHANGESHIP, wClientID, isEnemy, fShSpeed, fShAmp, fShW, fShH, fTop);
}

void Net_GetTextureUVForShip(int nShipType, ref rLeft, ref rTop, ref rRight, ref rBottom)
{
	float fLeft = 0.0;
	float fTop = 0.0;
	float fRight = 0.125;
	float fBottom = 0.25;

	SetShipPictureDataByShipTypeName( NetShips[nShipType].name );
	int nV = BI_intNRetValue[0] / 8;
	int nH = BI_intNRetValue[0] - nV * 8;
	fLeft = stf(nH) * 0.125;
	fTop = stf(nV) * 0.25;
	fRight = fLeft + 0.125;
	fBottom = fTop + 0.25;

	rLeft = fLeft;
	rTop = fTop;
	rRight = fRight;
	rBottom = fBottom;
}

int Net_GetIntactCannonsNum(int wNetClientID)
{
	ref chref = NetClient_GetClient(wNetClientID);
	int nShipIdx = sti(chref.ship.type);

	int q = GetBortIntactCannonsNum(chref, "cannonf", sti(NetShips[nShipIdx].fcannon)) +
		GetBortIntactCannonsNum(chref, "cannonb", sti(NetShips[nShipIdx].bcannon)) +
		GetBortIntactCannonsNum(chref, "cannonl", sti(NetShips[nShipIdx].lcannon)) +
		GetBortIntactCannonsNum(chref, "cannonr", sti(NetShips[nShipIdx].rcannon));
	return q;
}

int Net_GetHullPercent(ref chref)
{
	if(!CheckAttribute(chref,"Ship.HP")) return 100;
	if(!CheckAttribute(chref,"Ship.MaxHP")) return 100;
	int iret = 100 * sti(chref.Ship.HP) / sti(chref.Ship.MaxHP);
	return iret;
}

int Net_GetSailPercent(ref chref)
{
	if(!CheckAttribute(chref,"Ship.SP")) return 100;
	int nShipIdx = sti(chref.ship.type);
	int iSP = sti(NetShips[nShipIdx].SP);
	if(iSP<=0) return 100;
	int iret = 100 * sti(chref.Ship.SP) / iSP;
	return iret;
}

void NetClient_CreateBattleInterface()
{
	NetBI_SetParameterData();
	NetBI_InitializeCommands();
	NetBI_InitBattleInterface();
	InterfaceSpyGlassInit(true);

	// log interface
	Log_RemoveFromSingleLayers();

	LayerAddObject("Net_Execute",&ILogAndActions,-257);
	LayerAddObject("Net_Realize",&ILogAndActions,-1);

	// SP-style quick action panel (icon + text in the top right corner, Space = BIFastCommand)
	CreateILogAndActions(LOG_FOR_SEA);
	Log_SetActiveAction("Nothing");
}

void NetClient_DeleteBattleInterface()
{
	InterfaceSpyGlassRelease();
	NetBI_ReleaseBattleInterface();
}

void NetBI_SetParameterData()
{
	NetBInterface.CommandShowParam.maxShowQuantity = 8;
	NetBInterface.CommandShowParam.iconDistance = 4;
	NetBInterface.CommandShowParam.iconWidth = RecalculateHIcon(64);
	NetBInterface.CommandShowParam.iconHeight = RecalculateVIcon(64);
	NetBInterface.CommandShowParam.leftIconsOffset = sti(showWindow.left)+RecalculateHIcon(16);
	NetBInterface.CommandShowParam.downIconsOffset = sti(showWindow.bottom)-RecalculateVIcon(80);
	NetBInterface.CommandShowParam.buttonWidth = RecalculateHIcon(8);
	NetBInterface.CommandShowParam.buttonHeight = RecalculateVIcon(64);
	NetBInterface.CommandShowParam.buttonOffset = RecalculateHIcon(4);
	NetBInterface.CommandShowParam.buttonTexture = "battle_interface\lr_buttons.tga";
	NetBInterface.CommandShowParam.shipStateWidth = RecalculateHIcon(64);
	NetBInterface.CommandShowParam.shipStateHeight = RecalculateVIcon(16);
	NetBInterface.CommandShowParam.shipStateTexture = "battle_interface\indicators.tga";
	NetBInterface.CommandShowParam.shipStateOffset = RecalculateVIcon(0);
	NetBInterface.CommandShowParam.GeraldWidth = RecalculateHIcon(32);
	NetBInterface.CommandShowParam.GeraldHeight = RecalculateVIcon(32);
	NetBInterface.CommandShowParam.commandFont = "bold_numbers";
	NetBInterface.CommandShowParam.printXOffset = RecalculateHIcon(32);
	NetBInterface.CommandShowParam.printYOffset = RecalculateVIcon(-26);
	NetBInterface.CommandShowParam.commandNoteFont = "interface_normal";
	NetBInterface.CommandShowParam.noteXOffset = RecalculateHIcon(0);
	NetBInterface.CommandShowParam.noteYOffset = RecalculateVIcon(-30);
	NetBInterface.CommandShowParam.argbTFactorColor = argb(256,64,64,64);

	NetBInterface.navigation.aspectRatio			= showWindow.aspectRatio;
	NetBInterface.navigation.navigatorWidth			= RecalculateHIcon(256);
	NetBInterface.navigation.navigatorHeight		= RecalculateVIcon(256);
	NetBInterface.navigation.rightPos				= sti(showWindow.right) + RecalculateHIcon(24);
	NetBInterface.navigation.topPos					= sti(showWindow.top) - RecalculateVIcon(24);
	NetBInterface.navigation.speedShowFont			= "bold_numbers";
	NetBInterface.navigation.speedOutYOffset		= RecalculateVIcon(196);
	NetBInterface.navigation.shipSpeedXOffset		= RecalculateHIcon(30);
	NetBInterface.navigation.windSpeedXOffset		= RecalculateHIcon(-30);
	NetBInterface.navigation.fontScale				= 0.6;
	NetBInterface.navigation.windWidth				= 32;
	NetBInterface.navigation.windHeight				= 128;
	NetBInterface.navigation.compasTexture			= "battle_interface\compass.tga";
	NetBInterface.navigation.cannonsTexture			= "battle_interface\indicators_cannons_reload.tga";
	NetBInterface.navigation.emptyTexture			= "battle_interface\indicators_dark_and_center_ship.tga";
	NetBInterface.navigation.windTexture			= "battle_interface\wind_pointer.tga";
	NetBInterface.navigation.leftChargeBegAngle		= 214;
	NetBInterface.navigation.leftChargeEndAngle		= 326;
	NetBInterface.navigation.rightChargeBegAngle	= 146;
	NetBInterface.navigation.rightChargeEndAngle	= 34;
	NetBInterface.navigation.forwardChargeBegAngle	= 343;
	NetBInterface.navigation.forwardChargeEndAngle	= 378;
	NetBInterface.navigation.backwardChargeBegAngle	= 198;
	NetBInterface.navigation.backwardChargeEndAngle	= 162;
	NetBInterface.navigation.mapRadius				= 55;
	NetBInterface.navigation.horizontRadius			= 400;
	NetBInterface.navigation.minScale				= 0.5;
	NetBInterface.navigation.maxScale				= 4;
	NetBInterface.navigation.curScale				= 2;
	NetBInterface.navigation.scaleStep				= 0.1;
	NetBInterface.navigation.argbReadyCannonColor	= argb(255,0,155,0);
	NetBInterface.navigation.argbChargeCannonColor	= argb(255,0,255,0);
	NetBInterface.navigation.argbDamageCannonColor	= argb(255,255,0,0);
	NetBInterface.navigation.argbSeaColor			= argb(255,10,30,80);
	NetBInterface.navigation.argbFireZoneColor		= argb(60,250,250,250);
	NetBInterface.navigation.argbEnemyShipColor		= argb(255,255,0,0);
	NetBInterface.navigation.argbFrendShipColor		= argb(255,0,255,0);
	NetBInterface.navigation.argbNeutralShipColor	= argb(255,128,128,128);
	NetBInterface.navigation.argbDeadShipColor		= argb(255,0,0,255);
	NetBInterface.navigation.argbBackMaxColor		= argb(255,0,0,128);
	NetBInterface.navigation.argbBackMinColor		= argb(0,0,0,128);
	NetBInterface.navigation.shipShowRadius			= 8.0;

	NetBInterface.navigation.chargeTexture			= "battle_interface\list_icon2.tga";
	NetBInterface.navigation.chargeTextureGreed		= "8,8";
	NetBInterface.navigation.chargePos				= "0,"+RecalculateVIcon(270);
	NetBInterface.navigation.chargePictureSize		= RecalculateHIcon(48)+","+RecalculateVIcon(48);
	//
	NetBInterface.navigation.sailstateTexture		= "battle_interface\list_icon2.tga";
	NetBInterface.navigation.sailstateTextureGreed	= "8,8";
	NetBInterface.navigation.sailstatePos			= RecalculateHIcon(30)+","+RecalculateVIcon(170);
	NetBInterface.navigation.sailstatePictureSize	= RecalculateHIcon(48)+","+RecalculateVIcon(48);
	//
	NetBInterface.navigation.windStateTexture		= "battle_interface\list_icon2.tga";
	NetBInterface.navigation.windTextureGreed		= "8,8";
	NetBInterface.navigation.windPos				= RecalculateHIcon(-30)+","+RecalculateVIcon(170);
	NetBInterface.navigation.windPictureSize		= RecalculateHIcon(48)+","+RecalculateVIcon(48);

	NetBInterface.textinfo.Date.font = "interface_normal";
	NetBInterface.textinfo.Date.scale = 1.1;
	NetBInterface.textinfo.Date.pos.x = sti(showWindow.right) - RecalculateHIcon(104);
	NetBInterface.textinfo.Date.pos.y = RecalculateVIcon(200);
	NetBInterface.textinfo.Date.text = XI_convertString("Date:")+GetDataDay()+" "+XI_ConvertString("target_month_" + GetDataMonth())+" "+GetDataYear();
	//
	NetBInterface.textinfo.Location.font = "interface_normal";
	NetBInterface.textinfo.Location.scale = 1.1;
	NetBInterface.textinfo.Location.pos.x = sti(showWindow.right) - RecalculateHIcon(104);
	NetBInterface.textinfo.Location.pos.y = RecalculateVIcon(218);
	NetBInterface.textinfo.Location.text = XI_convertString("Open Sea");
	//
	NetBInterface.textinfo.Speed.font = "interface_normal";
	NetBInterface.textinfo.Speed.scale = 0.9;
	NetBInterface.textinfo.Speed.color = argb(255,255,255,255);
	NetBInterface.textinfo.Speed.shadow = false;
	NetBInterface.textinfo.Speed.pos.x = sti(showWindow.right) - RecalculateHIcon(104);
	NetBInterface.textinfo.Speed.pos.y = RecalculateVIcon(315);
	NetBInterface.textinfo.Speed.text = XI_convertString("Speed");
	//
	NetBInterface.textinfo.Ammo.font = "interface_normal";
	NetBInterface.textinfo.Ammo.scale = 0.9;
	NetBInterface.textinfo.Ammo.color = argb(255,255,255,255);
	NetBInterface.textinfo.Ammo.shadow = false;
	NetBInterface.textinfo.Ammo.pos.x = sti(showWindow.right) - RecalculateHIcon(104);
	NetBInterface.textinfo.Ammo.pos.y = RecalculateVIcon(399);
	NetBInterface.textinfo.Ammo.text = XI_convertString("Ammunition");
	NetBInterface.textinfo.Ammo.refreshable = true;

	NetBInterface.LineInfo.speed.color = argb(255,0,0,0);
	NetBInterface.LineInfo.speed.begin.x = sti(showWindow.right) - RecalculateHIcon(168);
	NetBInterface.LineInfo.speed.begin.y = RecalculateVIcon(334);
	NetBInterface.LineInfo.speed.end.x = RecalculateHIcon(128);
	NetBInterface.LineInfo.speed.end.y = 0;
	//
	NetBInterface.LineInfo.Ammo.color = argb(255,0,0,0);
	NetBInterface.LineInfo.Ammo.begin.x = sti(showWindow.right) - RecalculateHIcon(168);
	NetBInterface.LineInfo.Ammo.begin.y = RecalculateVIcon(418);
	NetBInterface.LineInfo.Ammo.end.x = RecalculateHIcon(128);
	NetBInterface.LineInfo.Ammo.end.y = 0;

	NetBInterface.battleborder.used = false;

	NetBInterface.ShipIcon.sailorfontid			= "interface_normal";
	NetBInterface.ShipIcon.sailorfontcolor		= argb(255,255,255,255);
	NetBInterface.ShipIcon.sailorfontscale		= 1.1;

	NetBInterface.ShipIcon.backtexturename		= "battle_interface\ShipBackIcon.tga";
	NetBInterface.ShipIcon.backcolor			= argb(255,128,128,128);
	NetBInterface.ShipIcon.backuv				= "0.0,0.0,1.0,1.0";
	NetBInterface.ShipIcon.backoffset			= "0.0,0.0";
	NetBInterface.ShipIcon.backiconsize			= "128,128";

	NetBInterface.ShipIcon.shipstatetexturename	= "battle_interface\ShipState.tga";
	NetBInterface.ShipIcon.shipstatecolor		= argb(255,128,128,128);
	NetBInterface.ShipIcon.shiphpuv				= "0.0,0.109,0.5,0.6875";
	NetBInterface.ShipIcon.shiphpoffset			= "-32,-13";
	NetBInterface.ShipIcon.shiphpiconsize		= "64,74";
	NetBInterface.ShipIcon.shipspuv				= "0.5,0.109,1.0,0.6875";
	NetBInterface.ShipIcon.shipspoffset			= "32,-13";
	NetBInterface.ShipIcon.shipspiconsize		= "64,74";

	NetBInterface.ShipIcon.shipclasstexturename	= "battle_interface\ShipClass.tga";
	NetBInterface.ShipIcon.shipclasscolor		= argb(255,128,128,128);
	NetBInterface.ShipIcon.shipclassuv			= "0.0,0.0,1.0,1.0";
	NetBInterface.ShipIcon.shipclassoffset		= "-14,-50";
	NetBInterface.ShipIcon.shipclassiconsize	= "64,16";
	NetBInterface.ShipIcon.gunchargeprogress	= "0.0625, 0.219, 0.359, 0.5, 0.641, 0.781, 0.983";

	NetBInterface.ShipIcon.shiptexturename		= "battle_interface\ship_icons1.tga";
	NetBInterface.ShipIcon.shipcolor			= argb(255,128,128,128);
	NetBInterface.ShipIcon.shipoffset			= "-14,-12";
	NetBInterface.ShipIcon.shipiconsize			= "64,64";

	NetBInterface.ShipIcon.iconoffset1 = "70,70";
	NetBInterface.ShipIcon.iconoffset2 = "70,198";
	NetBInterface.ShipIcon.iconoffset3 = "70,326";
	NetBInterface.ShipIcon.iconoffset4 = "70,454";

	NetBInterface.ShipIcon.commandlistverticaloffset = -40;

	NetBInterface.CommandList.CommandMaxIconQuantity = 8;
	NetBInterface.CommandList.CommandIconSpace = 1;
	NetBInterface.CommandList.CommandIconLeft = 157;
	NetBInterface.CommandList.CommandIconWidth = RecalculateHIcon(48);
	NetBInterface.CommandList.CommandIconHeight = RecalculateVIcon(48);

	NetBInterface.CommandList.CommandNoteFont = "interface_normal";
	NetBInterface.CommandList.CommandNoteColor = argb(255,255,255,255);
	NetBInterface.CommandList.CommandNoteScale = 1.0;
	NetBInterface.CommandList.CommandNoteOffset = RecalculateHIcon(0) + "," + RecalculateVIcon(-42);

	NetBInterface.CommandList.ActiveIcon_Texture = "battle_interface\enter_list.tga";
	NetBInterface.CommandList.ActiveIcon_Offset = RecalculateHIcon(-49) + ",0";
	NetBInterface.CommandList.ActiveIcon_Size = RecalculateHIcon(48) + "," + RecalculateVIcon(48);
	NetBInterface.CommandList.ActiveIcon_UV1 = "0.5,0.0,1.0,1.0";
	NetBInterface.CommandList.ActiveIcon_UV2 = "0.0,0.0,0.5,1.0";
	NetBInterface.CommandList.ActiveIcon_Note = XI_ConvertString("MenuNote");
}

void NetBI_InitializeCommands()
{
	int idLngFile = LanguageOpenFile("commands_name.txt");

	NetBInterface.CommandTextures.list.t0.name = "battle_interface\list_icon2.tga";
	NetBInterface.CommandTextures.list.t0.xsize = 8;
	NetBInterface.CommandTextures.list.t0.ysize = 8;

	// одиночные иконки команд (list_icons.tga 16x8) и иконка бордел€ из быстрого перехода (LandTarget_SLB.tga 16x2)
	NetBInterface.CommandTextures.list.t1.name = "battle_interface\list_icons.tga";
	NetBInterface.CommandTextures.list.t1.xsize = 16;
	NetBInterface.CommandTextures.list.t1.ysize = 8;

	NetBInterface.CommandTextures.list.t2.name = "battle_interface\LandTarget_SLB.tga";
	NetBInterface.CommandTextures.list.t2.xsize = 16;
	NetBInterface.CommandTextures.list.t2.ysize = 2;

	NetBInterface.CommandTextures.ChargeTexNum = 0;
	NetBInterface.CommandTextures.CommandTexNum = 0;

	NetBInterface.Commands.Cancel.enable	= false;
	NetBInterface.Commands.Cancel.picNum	= 50;
	NetBInterface.Commands.Cancel.selPicNum	= 58;
	NetBInterface.Commands.Cancel.texNum	= 0;
	NetBInterface.Commands.Cancel.event		= "Cancel";
	NetBInterface.Commands.Cancel.note		= LanguageConvertString(idLngFile, "sea_Cancel");
	//
	NetBInterface.Commands.Charge.enable	= false;
	NetBInterface.Commands.Charge.picNum	= 21;
	NetBInterface.Commands.Charge.selPicNum	= 29;
	NetBInterface.Commands.Charge.texNum	= 0;
	NetBInterface.Commands.Charge.event		= "BI_Charge";
	NetBInterface.Commands.Charge.note		= LanguageConvertString(idLngFile, "sea_Charge");
	//
	NetBInterface.Commands.Speed.enable		= false;
	NetBInterface.Commands.Speed.picNum		= 17;
	NetBInterface.Commands.Speed.selPicNum	= 25;
	NetBInterface.Commands.Speed.texNum		= 0;
	NetBInterface.Commands.Speed.event		= "BI_Speed";
	NetBInterface.Commands.Speed.note		= LanguageConvertString(idLngFile, "sea_Speed");
	//
	NetBInterface.Commands.Repair.enable	= false;
	NetBInterface.Commands.Repair.picNum	= 55;
	NetBInterface.Commands.Repair.selPicNum	= 63;
	NetBInterface.Commands.Repair.cooldownPicNum = 62;
	NetBInterface.Commands.Repair.texNum	= 0;
	NetBInterface.Commands.Repair.event		= "BI_Repair";
	NetBInterface.Commands.Repair.note		= LanguageConvertString(idLngFile, "sea_Repair");
	//
	NetBInterface.Commands.Board.enable		= false;
	NetBInterface.Commands.Board.picNum		= 30;
	NetBInterface.Commands.Board.selPicNum	= 14;
	NetBInterface.Commands.Board.texNum		= 1;
	NetBInterface.Commands.Board.event		= "BI_Board";
	NetBInterface.Commands.Board.note		= LanguageConvertString(idLngFile, "sea_Board");

	NetBInterface.Commands.Mine.enable		= false;
	NetBInterface.Commands.Mine.picNum		= 25;	// бордель из быстрого перехода (9 + 16)
	NetBInterface.Commands.Mine.selPicNum	= 9;
	NetBInterface.Commands.Mine.texNum		= 2;
	NetBInterface.Commands.Mine.event		= "BI_Mine";
	NetBInterface.Commands.Mine.note		= LanguageConvertString(idLngFile, "sea_Mine");

	NetBInterface.Commands.Brander.enable		= false;
	NetBInterface.Commands.Brander.picNum		= 52;
	NetBInterface.Commands.Brander.selPicNum	= 36;
	NetBInterface.Commands.Brander.texNum		= 1;
	NetBInterface.Commands.Brander.event		= "BI_Brander";
	NetBInterface.Commands.Brander.note			= LanguageConvertString(idLngFile, "sea_Brander");

	NetBInterface.Commands.Dump.enable		= false;
	NetBInterface.Commands.Dump.picNum		= 26;	// как команда Boat в одиночке (обмен между корабл€ми)
	NetBInterface.Commands.Dump.selPicNum	= 10;
	NetBInterface.Commands.Dump.texNum		= 1;
	NetBInterface.Commands.Dump.event		= "BI_Dump";
	NetBInterface.Commands.Dump.note		= LanguageConvertString(idLngFile, "sea_Dump");

	// COOP: active perks of single player (icons of list_icons.tga = texture 1, the same pictures as in single player)
	NetBInterface.Commands.ImmediateReload.enable		= false;
	NetBInterface.Commands.ImmediateReload.picNum		= 53;
	NetBInterface.Commands.ImmediateReload.selPicNum	= 37;
	NetBInterface.Commands.ImmediateReload.texNum		= 1;
	NetBInterface.Commands.ImmediateReload.event		= "BI_ImmediateReload";
	NetBInterface.Commands.ImmediateReload.note			= GetConvertStr("ImmediateReload", "AbilityDescribe.txt");

	NetBInterface.Commands.LightRepair.enable		= false;
	NetBInterface.Commands.LightRepair.picNum		= 54;
	NetBInterface.Commands.LightRepair.selPicNum	= 38;
	NetBInterface.Commands.LightRepair.texNum		= 1;
	NetBInterface.Commands.LightRepair.event		= "BI_LightRepair";
	NetBInterface.Commands.LightRepair.note			= GetConvertStr("LightRepair", "AbilityDescribe.txt");

	NetBInterface.Commands.InstantRepair.enable		= false;
	NetBInterface.Commands.InstantRepair.picNum		= 55;
	NetBInterface.Commands.InstantRepair.selPicNum	= 39;
	NetBInterface.Commands.InstantRepair.texNum		= 1;
	NetBInterface.Commands.InstantRepair.event		= "BI_InstantRepair";
	NetBInterface.Commands.InstantRepair.note		= GetConvertStr("InstantRepair", "AbilityDescribe.txt");

	NetBInterface.Commands.Turn180.enable		= false;
	NetBInterface.Commands.Turn180.picNum		= 56;
	NetBInterface.Commands.Turn180.selPicNum	= 40;
	NetBInterface.Commands.Turn180.texNum		= 1;
	NetBInterface.Commands.Turn180.event		= "BI_Turn180";
	NetBInterface.Commands.Turn180.note			= GetConvertStr("Turn180", "AbilityDescribe.txt");

	NetBInterface.charge.charge1.picNum		= 32; // balls
	NetBInterface.charge.charge1.selPicNum	= 40;
	NetBInterface.charge.charge2.picNum		= 35; // grapes
	NetBInterface.charge.charge2.selPicNum	= 43;
	NetBInterface.charge.charge3.picNum		= 34; // "Knippels"
	NetBInterface.charge.charge3.selPicNum	= 42;
	NetBInterface.charge.charge4.picNum		= 33; // bombs
	NetBInterface.charge.charge4.selPicNum	= 41;

	NetBInterface.UserIcons.ui1.enable	= true;
	NetBInterface.UserIcons.ui1.pic		= 20;
	NetBInterface.UserIcons.ui1.selpic	= 28;
	NetBInterface.UserIcons.ui1.tex		= 0;
	NetBInterface.UserIcons.ui1.name	= "sail_none";
	//----------------------------------------------
	NetBInterface.UserIcons.ui2.enable	= true;
	NetBInterface.UserIcons.ui2.pic		= 19;
	NetBInterface.UserIcons.ui2.selpic	= 27;
	NetBInterface.UserIcons.ui2.tex		= 0;
	NetBInterface.UserIcons.ui2.name	= "sail_midi";
	//----------------------------------------------
	NetBInterface.UserIcons.ui3.enable	= true;
	NetBInterface.UserIcons.ui3.pic		= 18;
	NetBInterface.UserIcons.ui3.selpic	= 26;
	NetBInterface.UserIcons.ui3.tex		= 0;
	NetBInterface.UserIcons.ui3.name	= "sail_fast";

	LanguageCloseFile(idLngFile);
}

// COOP: island for the minimap of the navigator (single player does the same in BI_SetIslandData).
// Called when the battle interface exists AND when the island is created, whichever comes last.
void NetBI_SetIslandData()
{
	if (!isEntity(&NetBInterface)) { return; }
	if (!CheckAttribute(&NCServer, "IslandID")) { return; }

	string sIslandID = NCServer.IslandID;
	if (sIslandID == "") { return; }

	int iIslandIndex = Net_GetIslandIndex(sIslandID);
	if (iIslandIndex == -1) { return; }

	ref rIsland;
	rIsland = GetIslandByIndex(iIslandIndex);
	SendMessage(&NetBInterface, "la", BI_MSG_SET_ISLAND, rIsland);
}

void NetBI_InitBattleInterface()
{

	NetBInterface.SelCharacter = -1;
	NetBInterface.boardRadius = 50;
	NetBInterface.freeDistance = MIN_ENEMY_DISTANCE_TO_DISABLE_MAP_ENTER;
	// COOP: state bars over the ships (copied from the single player BattleInterface: relation flag + hull / sails / crew)
	NetBInterface.ShipInfoImages.RelationTexture = "battle_interface\ship_arrows1.tga";
	NetBInterface.ShipInfoImages.RelationOffset.x = 0.0;
	NetBInterface.ShipInfoImages.RelationOffset.y = 0.3;
	NetBInterface.ShipInfoImages.RelationOffset.z = 0.0;
	NetBInterface.ShipInfoImages.RelationSize = "0.75,0.5";
	NetBInterface.ShipInfoImages.RelationUV1 = "0.0,0.0,0.25,1.0"; // friend
	NetBInterface.ShipInfoImages.RelationUV2 = "0.25,0.0,0.5,1.0"; // enemy
	NetBInterface.ShipInfoImages.RelationUV3 = "0.5,0.0,0.75,1.0"; // neutral

	NetBInterface.ShipInfoImages.ProgressTexture = "battle_interface\indicators.tga";
	NetBInterface.ShipInfoImages.ProgressSize = "2.0,0.1";
	NetBInterface.ShipInfoImages.ProgressBackOffset.x = 0.0;
	NetBInterface.ShipInfoImages.ProgressBackOffset.y = 0.65;
	NetBInterface.ShipInfoImages.ProgressBackOffset.z = 0.0;
	NetBInterface.ShipInfoImages.ProgressBackSize = "2.0,0.3";
	NetBInterface.ShipInfoImages.ProgressBackUV = "0.0,0.5,1.0,1.0";
	NetBInterface.ShipInfoImages.HullOffset.x = 0.0;
	NetBInterface.ShipInfoImages.HullOffset.y = 0.75;
	NetBInterface.ShipInfoImages.HullOffset.z = 0.0;
	NetBInterface.ShipInfoImages.HullUV = "0.0,0.333,1.0,0.5";
	NetBInterface.ShipInfoImages.SailOffset.x = 0.0;
	NetBInterface.ShipInfoImages.SailOffset.y = 0.65;
	NetBInterface.ShipInfoImages.SailOffset.z = 0.0;
	NetBInterface.ShipInfoImages.SailUV = "0.0,0.167,1.0,0.333";
	NetBInterface.ShipInfoImages.CrewOffset.x = 0.0;
	NetBInterface.ShipInfoImages.CrewOffset.y = 0.55;
	NetBInterface.ShipInfoImages.CrewOffset.z = 0.0;
	NetBInterface.ShipInfoImages.CrewUV = "0.0,0.0,1.0,0.166";

	NetBInterface.ShifInfoVisible = true;

	NetBInterface.blindSpeed = 0.003;
	NetBInterface.MaxWind = 30.0;
	NetBInterface.MaxShipSpeed = 20.0;
	NetBInterface.ShowNavigator = true;
	NetBInterface.ShowCommands = true;
	if(iArcadeSails == 1) {
		NetBInterface.ShipSpeedScaler = 1.0 / 2.5;		// COOP s45: the knots of the single player (the speed is x2.5 of it now, server\OnShip.c); it was 1 / 8
	} else {
		NetBInterface.ShipSpeedScaler = 1.0 / 3.0;
	}

	NetBInterface.MainChrIndex = NetClient_GetThisClientID();
	CreateEntity(&NetBInterface,"battle_interface");
	NetBInterface.Server = false;
	DeleteAttribute(&NetBInterface,"navigation");
	DeleteAttribute(&NetBInterface,"ShipIcon");
	DeleteAttribute(&NetBInterface,"CommandList");
	LayerAddObject("Net_Execute",&NetBInterface,-1);
	LayerAddObject("Net_Realize",&NetBInterface,-1);

	SetEventHandler(BI_EVENT_GET_DATA,"NetBI_GetData",0);

	// COOP: bars over the ships (hull, sails, crew + friend/enemy flag), always visible like with a good spyglass in single
	SetEventHandler(BI_EVENT_CHECK_SHIPSTATE_SHOW,"NetBI_GetSSShow",0);

	SetEventHandler("NCEvent_ShipCreate","NetBI_CreateShip",0);
	SetEventHandler("NCEvent_ShipDelete","NetBI_DeleteShip",0);
	SetEventHandler("NCEvent_ShipDead","NetBI_DeadShip",0);

	SetEventHandler("BI_CallUpdateShip","NetBI_UpdateShip",0);

	SetEventHandler("BI_SetPossibleCommands","NetBI_SetPossibleCommands",0);
	SetEventHandler("BI_CommandEndChecking","NetBI_CommandEndChecking",0);
	SetEventHandler("BI_LaunchCommand","NetBI_LaunchCommand",0);

	SetEventHandler("BI_GetChargeQuantity","NetBI_GetChargeQuantity",0);
	SetEventHandler("GetCurrentCharge","Net_GetCurrentCharge",0);

	SetEventHandler("neGetCooldownFactor","Net_GetCooldownFactor",0);

	SendMessage( &NetBInterface, "l", BI_MSG_REFRESH );

	// COOP: minimap of the island (if the island is already created; otherwise NetClient_CreateIslandReal does it)
	if (isEntity(&NCIsland)) { NetBI_SetIslandData(); }

	SetEventHandler("DoSailHole","Net_ProcessSailDamage",0);
	//SetEventHandler("BI_CallUpdateShip","BI_UpdateShip",0);
	//SetEventHandler("evntPerkAgainUsable","BI_PerkAgainUsable",0);
	//SetEventHandler("evntSetUsingAbility","procSetUsingAbility",0);
}

void NetBI_ReleaseBattleInterface()
{
	DeleteClass( &NetBInterface );// BattleInterface
	DeleteClass( &objShipPointer );

	DelEventHandler(BI_EVENT_GET_DATA,"NetBI_GetData");
	DelEventHandler(BI_EVENT_CHECK_SHIPSTATE_SHOW,"NetBI_GetSSShow");

	DelEventHandler(SHIP_CREATE,"NetBI_CreateShip");
	DelEventHandler(SHIP_DELETE,"NetBI_DeleteShip");
	DelEventHandler(SHIP_DEAD,"NetBI_DeadShip");

	DelEventHandler("BI_CallUpdateShip","NetBI_UpdateShip");

	DelEventHandler("BI_SetPossibleCommands","NetBI_SetPossibleCommands");
	DelEventHandler("BI_CommandEndChecking","NetBI_CommandEndChecking");
	DelEventHandler("BI_LaunchCommand","NetBI_LaunchCommand");

	DelEventHandler("neGetCooldownFactor","Net_GetCooldownFactor");
	DelEventHandler("GetCurrentCharge","Net_GetCurrentCharge");
	DelEventHandler("BI_GetChargeQuantity","NetBI_GetChargeQuantity");
	DelEventHandler("DoSailHole","Net_ProcessSailDamage");

	Log_MoveToSingleLayers();
}

int NetBI_SSShow;

// single: BI_GetSSShow (spyglass dependent). In the net game the state bars are always shown.
ref NetBI_GetSSShow()
{
	int wShipID = GetEventData();
	NetBI_SSShow = true;
	// COOP: no bars (hull, sails, crew) over a sunk or wrecked ship
	if (wShipID >= 0 && wShipID < NET_MAXCLIENTS)
	{
		ref rBIShip = NetClient_GetClient(wShipID);
		if (CheckAttribute(rBIShip, "Dead") && sti(rBIShip.Dead)) { NetBI_SSShow = false; }
		if (CheckAttribute(rBIShip, "Wrecked") && sti(rBIShip.Wrecked)) { NetBI_SSShow = false; }
	}
	return &NetBI_SSShow;
}

ref Net_GetCooldownFactor()
{
	string sComName = GetEventData();
	BI_g_fRetVal = 1.0;
	ref chref = NetClient_GetClient( NetClient_GetThisClientID() );

	if( sComName == "BI_Repair" ) {
		float f1 = stf(chref.skills.repair.totalcooldowntime);
		float f2 = stf(chref.skills.repair.currentcooldowntime);
		if( f1>0.0 && f2<f1 ) {
			BI_g_fRetVal = f2 / f1;
		}
		//trace("Repair Cooldown = "+ f2 + " / " + f1);
	}
	return &BI_g_fRetVal;
}

ref Net_GetCurrentCharge()
{
	int wChrID = NetClient_GetThisClientID();
	ref chref = NetClient_GetClient( wChrID );
	NetBI_intNRetValue[0] = -1;

	if( CheckAttribute(chref,"Ship.Cannons.Charge.Type") )
	{
		switch(sti(chref.Ship.Cannons.Charge.Type))
		{
		case GOOD_BALLS:
			NetBI_intNRetValue[0] = 32;
			NetBInterface.textinfo.Ammo.text = XI_convertString("Balls")+": "+sti(chref.ship.goods.balls);
			break;
		case GOOD_GRAPES:
			NetBI_intNRetValue[0] = 35;
			NetBInterface.textinfo.Ammo.text = XI_convertString("Grapes")+": "+sti(chref.ship.goods.grapes);
			break;
		case GOOD_KNIPPELS:
			NetBI_intNRetValue[0] = 34;
			NetBInterface.textinfo.Ammo.text = XI_convertString("Knippels")+": "+sti(chref.ship.goods.knippels);
			break;
		case GOOD_BOMBS:
			NetBI_intNRetValue[0] = 33;
			NetBInterface.textinfo.Ammo.text = XI_convertString("Bombs")+": "+sti(chref.ship.goods.bombs);
			break;
		}
	}

	float fState = NetClient_ShipGetSailState(wChrID);
	if( fState < 0.33 ) {
		NetBI_intNRetValue[1] = 20;
	} else {
		if( fState < 0.66 ) {
			NetBI_intNRetValue[1] = 19;
		} else {
			NetBI_intNRetValue[1] = 18;
		}
	}

	NetBI_intNRetValue[2] = 61;

	return &NetBI_intNRetValue;
}

void NetBI_CreateShipPrivate(int wNetClientID)
{
	int wNetThisClient = NetClient_GetThisClientID();

	ref rc = NetClient_GetClient(wNetClientID);

	bool myShip = false;
	int shipRelation = BI_RELATION_ENEMY;
	if( wNetClientID==wNetThisClient )
	{
		myShip = true;
		shipRelation = BI_RELATION_FRIEND;
	}
	else
	{
		// team games: ships of our team are friends, everything else is an enemy (no neutral in the net game)
		if (sti(NCServer.GameType) != NETGAME_DEATHMATCH)
		{
			ref rThisClient = NetClient_GetClient(wNetThisClient);
			if (sti(rc.Team) == sti(rThisClient.Team)) { shipRelation = BI_RELATION_FRIEND; }
		}
	}

	ref shipRef = Net_GetShipByIndex( sti(rc.ship.type) );

	int iColor = argb(255,255,0,0); // enemy ship
	if( CheckAttribute(&NetBInterface,"navigation.argbEnemyShipColor") ) {
		iColor = sti(NetBInterface.navigation.argbEnemyShipColor);
	}
	if (sti(NCServer.GameType) == NETGAME_DEATHMATCH)
	{
		if (wNetClientID == wNetThisClient) { // our ship
			iColor = argb(255,0,255,0);
			if( CheckAttribute(&NetBInterface,"navigation.argbFrendShipColor") ) {
				iColor = sti(NetBInterface.navigation.argbFrendShipColor);
			}
		}
	}
	else
	{
		iColor = NetTeamColor[sti(rc.Team)];
	}

	// COOP: a bot of the admin: its own relation and colour (server\Bots.c); COOP s28: a bot of a team is like a player of it
	bool bTeamBotBI = false;
	if (NetClient_IsBotID(wNetClientID)) { if (CheckAttribute(rc, "BotRel")) { bTeamBotBI = (sti(rc.BotRel) >= NET_BOTREL_TEAM0); } }
	if (NetClient_IsBotID(wNetClientID) && !bTeamBotBI)
	{
		shipRelation = BI_RELATION_ENEMY;
		iColor = argb(255,255,0,0);
		if (sti(rc.BotRel) == NET_BOTREL_ALLY) { shipRelation = BI_RELATION_FRIEND; iColor = argb(255,0,255,0); }
		if (sti(rc.BotRel) == NET_BOTREL_NEUTRAL) { shipRelation = BI_RELATION_NEUTRAL; iColor = argb(255,255,255,0); }
	}

	// COOP: a sunk / wrecked ship has no bars: single player never creates a dead ship in the interface (BI_CallUpdateShip), the
	// engine takes it out when the ship entity is deleted; here the entity stays, so it is not created / is deleted by hand
	// (NetBI_DeadShip). Our own ship stays: the commands of the interface belong to it.
	if (!myShip && CheckAttribute(rc, "Dead") && sti(rc.Dead)) { return; }

	SendMessage(&NetBInterface, "llaalll", BI_IN_CREATE_SHIP, wNetClientID, rc, shipRef, myShip, shipRelation, iColor);
}

void NetBI_UpdateShip()
{
	int wNetClientID = GetEventData();
	NetBI_CreateShipPrivate(wNetClientID);
}

void NetBI_CreateShip()
{
	int wNetClientID = GetEventData();
	NetBI_CreateShipPrivate(wNetClientID);
	// COOP s27: the relation / colour of a ship depends on OUR team: the ships created before our own one (our team was not
	// known yet) could stay with the wrong colour on the minimap. When our ship appears, all the others are set again.
	if (wNetClientID == NetClient_GetThisClientID())
	{
		for (int iBIR=0; iBIR<NET_MAXCLIENTS; iBIR++)
		{
			if (iBIR == wNetClientID) { continue; }
			if (!COOP_CliSlotLive(iBIR)) { continue; }
			if (!CheckAttribute(&NCClients[iBIR], "CoopReady")) { continue; }
			if (!sti(NCClients[iBIR].CoopReady)) { continue; }
			NetBI_CreateShipPrivate(iBIR);
		}
		RefreshBattleInterface();
	}
}

void NetBI_DeleteShip()
{
	RefreshBattleInterface();
}

// COOP s27: the ring of the cannons round the compass stayed bright green (as if the charge were unknown) after a deleted
// ship left the interface: the interface had lost its main ship (the charge is read from it). After a ship leaves the interface
// our own ship is put into it again as the main ship.
void NetBI_MainShipAgain()
{
	int wMe = NetClient_GetThisClientID();
	if (wMe < 0 || wMe >= NET_MAXCLIENTS) { return; }
	if (!COOP_CliSlotLive(wMe)) { return; }
	if (!CheckAttribute(&NCClients[wMe], "CoopReady")) { return; }
	if (!sti(NCClients[wMe].CoopReady)) { return; }
	if (!IsEntity(&NCClients[wMe])) { return; }
	SendMessage(&NetBInterface, "ll", BI_IN_DELETE_SHIP, wMe);
	NetBI_CreateShipPrivate(wMe);
	RefreshBattleInterface();
}

void NetBI_DeadShip()
{
	// COOP: the ships that sank / were wrecked leave the interface (their bars of hull, sails and crew and their pointer):
	// the engine does it itself in single player when it deletes the ship entity, here the entity stays
	int wBIThis = NetClient_GetThisClientID();
	for (int i=0; i<NET_MAXCLIENTS; i++)
	{
		if (i == wBIThis) { continue; }
		ref rBIDead = NetClient_GetClient(i);
		if (!CheckAttribute(rBIDead, "Ship.Type")) { continue; }
		if (CheckAttribute(rBIDead, "Dead") && sti(rBIDead.Dead))
		{
			SendMessage(&NetBInterface, "ll", BI_IN_DELETE_SHIP, i);
		}
	}
	RefreshBattleInterface();
	NetBI_MainShipAgain();		// COOP s27
}

void NetBI_SetPossibleCommands()
{
	int chIdx = GetEventData();

	// дл€ главного персонажа
	// COOP: a wrecked (respawn) / sinking ship has no commands
	bool bAlive = !NetClient_IsThisShipDead();
	bool bPerks = bAlive;		// COOP s28: every active perk by itself (bought in the lobby or the switch of the server)

	NetBInterface.Commands.Charge.enable	= bAlive;//CheckAttribute(mainCh,"Ship.Cannons.Charge.Type");
	NetBInterface.Commands.Speed.enable		= bAlive;
	NetBInterface.Commands.Repair.enable	= bAlive;
	bool bBoard = bAlive;
	if (NetClient_BoardTarget < 0) { bBoard = false; }
	if (sti(NCServer.NoBoarding)) { bBoard = false; }
	NetBInterface.Commands.Board.enable		= bBoard;
	NetBInterface.Commands.Mine.enable		= bAlive;
	NetBInterface.Commands.Brander.enable	= bPerks && NetClient_SeaPerk(NET_SPERK_BRANDER);		// COOP: the brander is an active perk
	NetBInterface.Commands.Dump.enable		= bAlive;

	// COOP: active perks, shown while they can be used (the log tells when they are ready again)
	NetBInterface.Commands.ImmediateReload.enable	= bPerks && NetClient_ActivePerkReady(NET_APERK_IMMEDIATERELOAD);		// (ready = bought and not cooling down)
	NetBInterface.Commands.Turn180.enable			= bPerks && NetClient_ActivePerkReady(NET_APERK_TURN180);
	NetBInterface.Commands.LightRepair.enable		= bPerks && NetClient_ActivePerkReady(NET_APERK_LIGHTREPAIR);
	NetBInterface.Commands.InstantRepair.enable		= bPerks && NetClient_ActivePerkReady(NET_APERK_INSTANTREPAIR);
}

ref NetBI_GetCooldownFactor()
{
	ref chref = NetClient_GetClient( NetClient_GetThisClientID() );
}

ref NetBI_CommandEndChecking()
{
	NetBI_retComValue = 0;
	string comName = GetEventData();

	switch(comName)
	{
	case "BI_Charge":
		NetBI_retComValue = BI_COMMODE_CANNON_CHARGE;
		break;
	case "BI_Speed":
		NetBI_retComValue = BI_COMMODE_USER_ICONS;
		break;
	case "BI_Repair":
		NetBI_retComValue = 0;
		break;
	case "BI_Board":
		NetBI_retComValue = 0;
		break;
	case "BI_Mine":
		NetBI_retComValue = 0;
		break;
	case "BI_Brander":
		NetBI_retComValue = 0;
		break;
	case "BI_Dump":
		NetBI_retComValue = 0;
		break;
	case "BI_ImmediateReload":
		NetBI_retComValue = 0;
		break;
	case "BI_Turn180":
		NetBI_retComValue = 0;
		break;
	case "BI_LightRepair":
		NetBI_retComValue = 0;
		break;
	case "BI_InstantRepair":
		NetBI_retComValue = 0;
		break;
	}

	return &NetBI_retComValue;
}

void NetBI_LaunchCommand()
{
	int charIdx = GetEventData();
	string commandName = GetEventData();
	int targetNum = GetEventData();
	string locName = GetEventData();

	if(targetNum==-1 && locName=="cancel") commandName = "";

	switch(commandName)
	{
	case "BI_Charge":
		string sChargeType="Balls";
		switch(targetNum)
		{
		case 1:
			//Log_SetStringToLog("Cannonballs");
			PlaySound("interface\_balls.wav");
			sChargeType="Balls";
			break;
		case 2:
			//Log_SetStringToLog("Grapes");
			PlaySound("interface\_grapes.wav");
			sChargeType="Grapes";
			break;
		case 3:
			//Log_SetStringToLog("Knippels");
			PlaySound("interface\_chain.wav");
			sChargeType="Knippels";
			break;
		case 4:
			//Log_SetStringToLog("Bombs");
			PlaySound("interface\_bombs.wav");
			sChargeType="Bombs";
			break;
		}
		NetClient_ShipChangeCharge(sChargeType);
		break;
	case "BI_Speed":
		switch(locName)
		{
		case "sail_none":
			NetClient_ShipSetSailState(0.0);
			break;
		case "sail_midi":
			NetClient_ShipSetSailState(0.5);
			break;
		case "sail_fast":
			NetClient_ShipSetSailState(1.0);
			break;
		}
		break;
	case "BI_Repair":
		NetClient_ShipStartRepair();
		break;
	case "BI_Board":
		NetClient_RequestBoarding();
		break;
	case "BI_Mine":
		NetClient_ShipDropMine();
		break;
	case "BI_Brander":
		NetClient_ShipBranderPress();
		break;
	case "BI_Dump":
		LaunchNetInterface_Dump();
		break;
	case "BI_ImmediateReload":
		NetClient_UseActivePerk(NET_APERK_IMMEDIATERELOAD);
		break;
	case "BI_Turn180":
		NetClient_UseActivePerk(NET_APERK_TURN180);
		break;
	case "BI_LightRepair":
		NetClient_UseActivePerk(NET_APERK_LIGHTREPAIR);
		break;
	case "BI_InstantRepair":
		NetClient_UseActivePerk(NET_APERK_INSTANTREPAIR);
		break;
	}
}

ref NetBI_GetChargeQuantity()
{
	int chIdx = GetEventData();
	NetBI_ChargeState[0] = 4;
	NetBI_ChargeState[1] = 100;//GetCargoGoods(refChar,GOOD_BALLS);
	NetBI_ChargeState[2] = 100;//GetCargoGoods(refChar,GOOD_GRAPES);
	NetBI_ChargeState[3] = 100;//GetCargoGoods(refChar,GOOD_KNIPPELS);
	NetBI_ChargeState[4] = 100;//GetCargoGoods(refChar,GOOD_BOMBS);

	return &NetBI_ChargeState;
}

ref NetBI_GetData()
{
	int dataType = GetEventData();
	int wNetClientID;
	ref chRef;

	switch(dataType)
	{
		// ѕолучаем номер картинки корабл€
	case BIDT_SHIPPICTURE:
		wNetClientID = GetEventData();
		chRef = NetClient_GetClient(wNetClientID);
		int st = sti(chRef.ship.type);
		// COOP s27: the picture by the NET ship table (NetShips); SetShipPictureDataByShipType used the table of the single
		// player game (ShipsTypes), whose numbers are other ships -> a wrong icon (a xebec of somebody else)
		BI_intNRetValue[0] = 0;
		BI_intNRetValue[1] = 1;
		BI_intNRetValue[2] = BI_ICONS_TEXTURE_SHIP1;
		if (st >= 0 && st < iNetShipsNum) { SetShipPictureDataByShipTypeName(NetShips[st].name); }
		break;
	}
	return &BI_intNRetValue;
}

ref Net_ProcessSailDamage()
{
	// от кого удар
	int shootIdx = GetEventData();
	// перс
	int wNetClientID = GetEventData();

	string sMastName = GetEventData();
	// координаты паруса
	string reyName = GetEventData();
	int groupNum = GetEventData();
	// данные о дырках
	int holeCount = GetEventData();
	int holeData = GetEventData();
	int maxHoleCount = GetEventData();
	// мощность паруса
	float sailPower = GetEventData();

	ref chref = NetServer_GetClient(wNetClientID);//NetClient_GetClient(wNetClientID);

	if (LAi_IsImmortal(chref)) 
	{ 
		BI_g_fRetVal = 0;
		return &BI_g_fRetVal;
	}

	string groupName = ""+groupNum;
	aref arSail;
	makearef(arSail,chref.ship.sails.(reyName).(groupName));

	float sailDmg = 0.0;
	float sailDmgMax = 100.0 * sailPower;
	if( !CheckAttribute(arSail,"dmg") )	{ sailDmg = 0.0; }

	if(sMastName=="*")
	{
		sailDmg = sailDmg + Net_GetRigDamage(shootIdx,sti(NSBalls.CurrentBallType),chref);
		if(sailDmg>sailDmgMax)	{ sailDmg = sailDmgMax; }
		int needHole = GetNeedHoleFromDmg(sailDmg,sailDmgMax,maxHoleCount);
		if(holeCount!=needHole)
		{
			if(holeCount<needHole)
			{
				holeData = RandomHole2Sail(wNetClientID,reyName,groupNum, maxHoleCount, holeData, needHole-holeCount);
				holeCount = needHole;
			}
			else
			{
				sailDmg = GetNeedDmgFromHole(holeCount,sailDmgMax,maxHoleCount);
			}
		}
	}
	else
	{
		if(sMastName!="#")	{ arSail.mastFall = sMastName; }
		sailDmg = sailDmgMax;
	}

/*	if (sailDmg < 0.5)
	{
		Log_SetStringtoLog("MUST DIE!!! " + sailDmg);
	}*/

	arSail.hc = holeCount;
	arSail.hd = holeData;
	arSail.mhc = maxHoleCount;
	arSail.sp = sailPower;
	arSail.dmg = sailDmg;

	chref.ship.SP = Net_CalculateShipSP(chref);
	BI_g_fRetVal = sailDmg;
	return &BI_g_fRetVal;
}

float Net_GetRigDamage(int shootIdx, int iBallType, ref damage_chr)
{
	ref rBall = Net_GetGoodByIndex(iBallType);
	float fDmgRig = stf(rBall.DamageRig);
	fDmgRig = fDmgRig * stf(damage_chr.ship.SailsDamageModifier);
	// COOP s45: like GetRigDamage of the single player (battle_interface\BattleInterface.c): the distance of the flight 1.2 .. 0.25
	// (when the engine gives it), CannonProfessional of the shooter x1.3 (else SailsDamageUp x1.15), ShipDefenseProfessional of
	// the target x0.65
	if (CheckAttribute(&NSBalls, "CurrentMaxBallDistance") && CheckAttribute(&NSBalls, "CurrentBallDistance"))
	{
		float fRigMaxD = stf(NSBalls.CurrentMaxBallDistance);
		if (fRigMaxD > 1.0) { fDmgRig = fDmgRig * Bring2Range(1.2, 0.25, 0.0, fRigMaxD, stf(NSBalls.CurrentBallDistance)); }
	}
	if (NetServer_SeaPerk(shootIdx, NET_SPERK_CANNONPRO)) { fDmgRig = fDmgRig * 1.3; }
	else
	{
		if (NetServer_SeaPerk(shootIdx, NET_SPERK_SAILSDAMAGE)) { fDmgRig = fDmgRig * 1.15; }		// COOP: passive perk SailsDamageUp of the shooter
	}
	if (CheckAttribute(damage_chr, "ID")) { if (NetServer_SeaPerk(sti(damage_chr.ID), NET_SPERK_DEFENCE)) { fDmgRig = fDmgRig * 0.65; } }
	return fDmgRig;
}

//================================================================
// ѕосчитать состо€ние парусов
//================================================================
float Net_CalculateShipSP(ref chref)
{
	float fSP = 100;//GetCharacterShipSP(chref);
	aref arRoot,arGroup,arSail;
	int q,n,i,j;

	makearef(arRoot, chref.ship.sails);
	q = GetAttributesNum(arRoot);
	for(i=0; i<q; i++)
	{
		arGroup = GetAttributeN(arRoot,i);
		n = GetAttributesNum(arGroup);
		for(j=0; j<n; j++)
		{
			arSail = GetAttributeN(arGroup,j);
			fSP -= stf(arSail.dmg);
		}
	}

	if(fSP<0.0) fSP = 0.0;
	return fSP;
}

// ================================================================================================
// COOP: spyglass (event SetTelescopeInfo of the engine, parameter = net client ID of the ship, or NET_FORTCLIENT).
// Same data as spyglass5 of the single player: nation (not in the net game), ship type / name, hull, sails, EXACT crew,
// speed, distance, cannons (intact / installed), charge, caliber, captain skills, musket volley, fort.
// The format of MSG_ISG_UPDATE must be exactly the same as in SetSpyGlassData (battle_interface\ispyglass.c):
// "lsslllfflllllllllllssl" (22 values). The old net version sent 20 values with a wrong format -> crash of the game.
// ================================================================================================
string Net_SpyGlassCannonName(ref chref)
{
	if (!CheckAttribute(chref, "Ship.Cannons.Type")) { return ""; }
	int iType = sti(chref.Ship.Cannons.Type);
	if (iType < 0 || iType >= Net_GetCannonsNum()) { return ""; }

	ref rCannon = Net_GetCannonByIndex(iType);
	string sName = " ѕушки ";
	if (rCannon.name == "culverine") { sName = "  улеврины "; }
	return sName + sti(rCannon.caliber) + "ф";
}

// ship icon for the spyglass: ship_icons2.tga has square cells 0.125 x 0.125 (the same as GetTextureUVForShip of the single player).
// The old code used ship_icons1.tga with cells 0.25 high (that sheet is for the tall icons of the command list) in the square frame
// of the spyglass -> the picture was squeezed 2 times in height and the next icon of the sheet was visible.
void Net_GetSpyShipUV(int nShipType, ref rLeft, ref rTop, ref rRight, ref rBottom)
{
	SetShipPictureDataByShipTypeName( NetShips[nShipType].name );
	int nV = BI_intNRetValue[0] / 8;
	int nH = BI_intNRetValue[0] - nV * 8;
	float fLeft = stf(nH) * 0.125;
	float fTop = stf(nV) * 0.125;

	rLeft = fLeft;
	rTop = fTop;
	rRight = fLeft + 0.125;
	rBottom = fTop + 0.125;
}

// distance (2D) from our ship to the other ship / the fort, like Ship_GetDistance2D of the single player
float Net_SpyGlassDistance(ref chref, bool bFort)
{
	ref rThis = NetClient_GetClient(NetClient_GetThisClientID());

	float fX1 = 0.0;
	float fZ1 = 0.0;
	if (CheckAttribute(rThis, "Ship.NewPos.x"))
	{
		fX1 = stf(rThis.Ship.NewPos.x);
		fZ1 = stf(rThis.Ship.NewPos.z);
	}
	else
	{
		if (!CheckAttribute(rThis, "Ship.Pos.x")) { return 0.0; }
		fX1 = stf(rThis.Ship.Pos.x);
		fZ1 = stf(rThis.Ship.Pos.z);
	}

	float fX2 = 0.0;
	float fZ2 = 0.0;
	if (bFort)
	{
		if (!CheckAttribute(chref, "SpyGlass.x")) { return 0.0; }
		fX2 = stf(chref.SpyGlass.x) + ISG_FORT_OX;
		fZ2 = stf(chref.SpyGlass.z) + ISG_FORT_OZ;
	}
	else
	{
		if (CheckAttribute(chref, "Ship.NewPos.x"))
		{
			fX2 = stf(chref.Ship.NewPos.x);
			fZ2 = stf(chref.Ship.NewPos.z);
		}
		else
		{
			if (!CheckAttribute(chref, "Ship.Pos.x")) { return 0.0; }
			fX2 = stf(chref.Ship.Pos.x);
			fZ2 = stf(chref.Ship.Pos.z);
		}
	}
	float fDX = fX2 - fX1;
	float fDZ = fZ2 - fZ1;
	return stf(FloatToString(sqrt(fDX * fDX + fDZ * fDZ), 1));
}

// avatar path for the spyglass: the same rules as TopList / FindGame / client\OnClient.c
// (face_* = portrait of the single player, otherwise net\faces; a missing custom file = empty face). Cached.
string Net_SpyGlassFacePath(ref chref)
{
	string sEmpty = sUserFacesPath + "_emptyface_.tga";
	if (!CheckAttribute(chref, "UserData.Face")) { return sEmpty; }
	string sFace = chref.UserData.Face;
	if (sFace == "") { return sEmpty; }

	if (findSubStr(sFace, "face_", 0) == 0)
	{
		return "interfaces\portraits\256\" + sFace + ".tga";
	}
	string sNpcSpy = NetLand_NpcFaceTex(sFace);		// COOP: an NPC skin of the profile (net\Land.c)
	if (sNpcSpy != "") { return sNpcSpy; }

	if (CheckAttribute(chref, "UserData.SpyFaceKey") && CheckAttribute(chref, "UserData.SpyFacePath"))
	{
		if (chref.UserData.SpyFaceKey == sFace) { return chref.UserData.SpyFacePath; }
	}

	string sPath = sEmpty;
	object oFaceFiles;
	if (NFFindFiles(&oFaceFiles, "resource\textures\" + sUserFacesPath, sFace + ".tga.tx", false) > 0)
	{
		sPath = sUserFacesPath + sFace + ".tga";
	}
	chref.UserData.SpyFaceKey = sFace;
	chref.UserData.SpyFacePath = sPath;
	return sPath;
}

// ---- the fort in the view of the telescope (the engine reports ships only) ----
#define ISG_FORT_RADIUS		35.0	// m, how far from the view ray the fort locator may be (the locator is not the centre of the fort model)
#define ISG_FORT_RADIUS_K	0.01	// + this * distance to the fort (the further the fort, the bigger the tolerance)
#define ISG_FORT_HEIGHT		25.0	// m, height of the fort above the sea
#define ISG_FORT_VTOL		22.0	// m, vertical tolerance of the view ray at the fort
#define ISG_FORT_MAXDIST	5000.0
#define ISG_FORT_OX			105.0	// COOP s27: the middle of the fort model from the fort point (world x), the same as NET_FORT_COLL_OX of the push (server\Fort.c)
#define ISG_FORT_OZ			8.0		// (world z), NET_FORT_COLL_OZ

int iSpyTickNo = 0;		// timer ticks
int iSpyEngineTick = -100;	// tick of the last SetTelescopeInfo of the engine (a ship is in the view)
int iSpyLastId = -2;
int iSpyTraceCnt = 0;

// diagnostics of the fort check (written to compile.log once a second while the telescope is on and no ship is in the view)
int iSpyDiagTick = -100;
string sSpyFortWhy = "";
float fSpyDbgDist = 0.0;
float fSpyDbgFwd = 0.0;
float fSpyDbgLat = 0.0;
float fSpyDbgVert = 0.0;

bool NetClient_SpyFortInView()
{
	sSpyFortWhy = "no fort data (SpyGlass.x)";
	ref rFort = NetClient_GetClient(NET_FORTCLIENT);
	if (!CheckAttribute(rFort, "SpyGlass.x")) { return false; }
	sSpyFortWhy = "no camera";
	if (!CheckAttribute(&NCDeckCamera, "Pos.x") || !CheckAttribute(&NCDeckCamera, "Dir.x")) { return false; }

	float cx = stf(NCDeckCamera.Pos.x);
	float cy = stf(NCDeckCamera.Pos.y);
	float cz = stf(NCDeckCamera.Pos.z);
	float dx = stf(NCDeckCamera.Dir.x);
	float dy = stf(NCDeckCamera.Dir.y);
	float dz = stf(NCDeckCamera.Dir.z);
	sSpyFortWhy = "camera is not finite";
	if (!COOP_CliFloatOk(cx) || !COOP_CliFloatOk(cy) || !COOP_CliFloatOk(cz)) { return false; }
	if (!COOP_CliFloatOk(dx) || !COOP_CliFloatOk(dy) || !COOP_CliFloatOk(dz)) { return false; }

	sSpyFortWhy = "view is vertical";
	float fHor = sqrt(dx * dx + dz * dz);
	if (fHor < 0.001) { return false; }

	float vx = stf(rFort.SpyGlass.x) + ISG_FORT_OX - cx;		// the middle of the model, not the locator (the admin fort has its shift in SpyGlass.x/z already)
	float vz = stf(rFort.SpyGlass.z) + ISG_FORT_OZ - cz;
	float fDist = sqrt(vx * vx + vz * vz);
	fSpyDbgDist = fDist;
	sSpyFortWhy = "too close / too far";
	if (fDist < 30.0 || fDist > ISG_FORT_MAXDIST) { return false; }

	float fForward = (dx * vx + dz * vz) / fHor;		// along the view ray (horizontal)
	fSpyDbgFwd = fForward;
	sSpyFortWhy = "fort is behind";
	if (fForward <= 0.0) { return false; }
	float fLateral = (dx * vz - dz * vx) / fHor;		// across the view ray
	if (fLateral < 0.0) { fLateral = -fLateral; }
	fSpyDbgLat = fLateral;
	sSpyFortWhy = "fort is aside of the view ray";
	if (fLateral > ISG_FORT_RADIUS + ISG_FORT_RADIUS_K * fForward) { return false; }

	float fRayY = cy + (dy / fHor) * fForward;		// height of the view ray at the fort
	float fVert = fRayY - ISG_FORT_HEIGHT;
	if (fVert < 0.0) { fVert = -fVert; }
	fSpyDbgVert = fVert;
	sSpyFortWhy = "view ray is too high / low at the fort";
	if (fVert > ISG_FORT_VTOL) { return false; }
	sSpyFortWhy = "IN VIEW";
	return true;
}

// COOP v3.1: the fort check is called from several places (the timer may be late or dead): the timer, the heartbeat,
// the engine event ReleaseTelescopeInfo and the TelescopeIn control. iSpyTickNo counts the calls.
int iSpyLastWhyTick = -100;
string sSpyLastWhy = "";

void NetClient_SpyFortUpdate()
{
	iSpyTickNo++;

	if (!bNetTelescopeOn)
	{
		if (bSpyFortShown)
		{
			bSpyFortShown = false;
			SendMessage(&objISpyGlass, "ll", MSG_ISG_VISIBLE, false);
			SendMessage(&objISpyGlass, "ll", MSG_ISG_VISIBLE, 3);		// COOP s27: the fort is no target of the panel (battle_interface.dll patch)
			sSpyShownKey = "";
		}
		return;
	}

	// the engine keeps sending SetTelescopeInfo while a ship is in the view: the ship has the priority
	if (iSpyTickNo - iSpyEngineTick < 5) { return; }

	bool bFortView = NetClient_SpyFortInView();
	// log when the reason changes (and not more often than every 60 calls)
	if (sSpyFortWhy != sSpyLastWhy || iSpyTickNo - iSpyDiagTick >= 60)
	{
		sSpyLastWhy = sSpyFortWhy;
		iSpyDiagTick = iSpyTickNo;
		string sDiagFort = "";
		ref rDiagFort = NetClient_GetClient(NET_FORTCLIENT);
		if (CheckAttribute(rDiagFort, "SpyGlass.x")) { sDiagFort = " fort=" + rDiagFort.SpyGlass.x + "," + rDiagFort.SpyGlass.z; }
		Trace("COOP: spyfort " + sSpyFortWhy + sDiagFort + " dist=" + fSpyDbgDist + " fwd=" + fSpyDbgFwd + " lat=" + fSpyDbgLat + " vert=" + fSpyDbgVert);
	}
	if (!bFortView)
	{
		if (bSpyFortShown)
		{
			bSpyFortShown = false;
			SendMessage(&objISpyGlass, "ll", MSG_ISG_VISIBLE, false);
			SendMessage(&objISpyGlass, "ll", MSG_ISG_VISIBLE, 3);		// COOP s27: the fort is no target of the panel (battle_interface.dll patch)
			sSpyShownKey = "";
		}
		return;
	}

	bSpyFortShown = true;
	Net_SpyGlassShow(NET_FORTCLIENT);
}

int iSpyAliveCnt = 0;

void NetClient_SpyFortTick()
{
	int iGen = GetEventData();
	if (iGen != iSpyFortGen) { return; }
	PostEvent("NetClient_SpyFortTick", 150, "l", iSpyFortGen);

	// the timer is alive (once per ~6 s)
	iSpyAliveCnt++;
	if (iSpyAliveCnt >= 40)
	{
		iSpyAliveCnt = 0;
		Trace("COOP: spyfort timer alive, calls=" + iSpyTickNo);
	}
	NetClient_SpyFortUpdate();
}

// event SetTelescopeInfo of the engine
void Net_SetSpyGlassData()
{
	int wNetClientID = GetEventData();
	if (wNetClientID == NET_FORTCLIENT) { return; }		// COOP s27: the engine keeps the fort as its target (MSG_ISG_VISIBLE 2): the script shows it itself
	if (!Net_SpyGlassCanShow(wNetClientID)) { return; }	// COOP s27: a ship we can not show does not block the fort
	iSpyEngineTick = iSpyTickNo;
	bSpyFortShown = false;
	sSpyShownKey = "";
	Net_SpyGlassShow(wNetClientID);
}

// COOP s27: a player or a live bot (the bots have Use = 0, they were not shown before: the old data stayed on the screen)
bool Net_SpyGlassCanShow(int wNetClientID)
{
	if (wNetClientID < 0) { return false; }
	if (wNetClientID == NET_FORTCLIENT) { return true; }
	if (wNetClientID >= NET_MAXCLIENTS) { return false; }
	if (wNetClientID == NetClient_GetThisClientID()) { return false; }		// COOP s27: the deck camera looked at our own ship
	if (CheckAttribute(&NCClients[wNetClientID], "ShipGone"))
	{
		if (sti(NCClients[wNetClientID].ShipGone)) { return false; }		// a deleted wreck
	}
	if (NetClient_IsCLient(wNetClientID)) { return true; }
	if (NetClient_IsBotID(wNetClientID))
	{
		if (!NetClient_BotRemoved(wNetClientID)) { return true; }
	}
	return false;
}

void Net_SpyGlassShow(int wNetClientID)
{
	if (wNetClientID < 0) { return; }

	bool bFort = false;
	if (wNetClientID == NET_FORTCLIENT) { bFort = true; }
	if (!bFort)
	{
		if (!Net_SpyGlassCanShow(wNetClientID)) { return; }
	}

	ref chref = NetClient_GetClient(wNetClientID);

	string sTextureName = "battle_interface\ship_icons2.tga";
	float uvLeft = 0.0;
	float uvTop = 0.0;
	float uvRight = 0.125;
	float uvBottom = 0.125;

	int shipHull = -1;
	int shipSail = -1;
	int shipCrew = -1;
	int shipCannons = -1;
	int shipMaxCannons = -1;
	int shipCharge = -1;
	int shipNation = 0;		// no nations in the net game: the 'nation' icon slot shows the boarding skill (see FillISpyGlassParameters)
	int shipClass = 1;
	int nSailState = -1;
	float shipSpeed = -1.0;
	string shipName = "";
	string shipType = "";
	string sCaptainName = "";
	string sCannonType = "";
	string sNetUserFaceTex = sUserFacesPath + "_emptyface_.tga";

	int nFencingSkill = 0;
	int nCannonSkill = 0;
	int nAccuracySkill = 0;
	int nNavigationSkill = 0;
	int nBoardingSkill = 0;
	int nGrapplingSkill = 1;	// boarding (Grappling), shown left of the carpentry (Repair)

	if (bFort)
	{
		// ---------------- fort (data from NSC_FORT_INFO, see client\Fort.c) ----------------
		shipType = XI_ConvertString("Fort_Name");
		if (CheckAttribute(chref, "SpyGlass.Name")) { shipName = chref.SpyGlass.Name; }

		sTextureName = "battle_interface\list_icon2.tga";
		uvLeft = 0.375;
		uvTop = 0.75;
		uvRight = 0.5;
		uvBottom = 0.875;

		shipHull = 100;
		if (CheckAttribute(chref, "SpyGlass.CannonsTotal"))
		{
			int iFortTotal = sti(chref.SpyGlass.CannonsTotal);
			int iFortDamaged = sti(chref.SpyGlass.CannonsDamaged);
			if (iFortTotal > 0)
			{
				if (iFortDamaged > iFortTotal) { iFortDamaged = iFortTotal; }
				shipMaxCannons = iFortTotal;
				shipCannons = iFortTotal - iFortDamaged;
				shipHull = makeint(100.0 * makefloat(iFortTotal - iFortDamaged) / makefloat(iFortTotal));
			}
		}
		if (CheckAttribute(chref, "SpyGlass.Crew")) { shipCrew = sti(chref.SpyGlass.Crew); }

		shipCharge = 1;			// the fort shoots bombs (icon 1 = bombs)
		nFencingSkill = 5;		// the skills of the fort (server\Fort.c)
		nCannonSkill = 10;
		nAccuracySkill = 10;
		nNavigationSkill = 5;
		nBoardingSkill = 5;
		nGrapplingSkill = 5;
		sCaptainName = "       ";
	}
	else
	{
		// ---------------- ship of a player ----------------
		if (!CheckAttribute(chref, "Ship.Type")) { return; }
		int nShipIdx = sti(chref.Ship.Type);
		if (nShipIdx < 0 || nShipIdx >= iNetShipsNum) { return; }

		ref rShip = Net_GetShipByIndex(nShipIdx);
		shipType = XI_ConvertString(rShip.Name);
		if (CheckAttribute(chref, "Ship.Name")) { shipName = chref.Ship.Name; }
		shipClass = sti(rShip.Class);

		Net_GetSpyShipUV(nShipIdx, &uvLeft, &uvTop, &uvRight, &uvBottom);

		// hull
		shipHull = 100;
		if (CheckAttribute(chref, "Ship.HP") && CheckAttribute(chref, "Ship.MaxHP"))
		{
			float fMaxHP = stf(chref.Ship.MaxHP);
			if (fMaxHP > 0.0) { shipHull = makeint(100.0 * stf(chref.Ship.HP) / fMaxHP); }
		}
		if (shipHull < 0) { shipHull = 0; }
		if (shipHull > 100) { shipHull = 100; }

		// sails
		shipSail = Net_GetSailPercent(chref);
		if (shipSail < 0) { shipSail = 0; }
		if (shipSail > 100) { shipSail = 100; }

		// exact crew
		if (CheckAttribute(chref, "Ship.Crew.Quantity")) { shipCrew = makeint(stf(chref.Ship.Crew.Quantity)); }

		// cannons: installed (server) minus the destroyed ones (counted from the damages of the ship model)
		int iMaxCannons = Net_GetShipCannonsNum(nShipIdx);
		int iDestroyed = iMaxCannons - Net_GetIntactCannonsNum(wNetClientID);
		if (iDestroyed < 0) { iDestroyed = 0; }
		int iInstalled = iMaxCannons;
		if (CheckAttribute(chref, "Ship.CannonsInstalled")) { iInstalled = sti(chref.Ship.CannonsInstalled); }
		if (iInstalled > iMaxCannons) { iInstalled = iMaxCannons; }
		shipMaxCannons = iMaxCannons;		// max of the ship type (16), the thrown away ones show up as missing: 10 / 16
		shipCannons = iInstalled - iDestroyed;
		if (shipCannons < 0) { shipCannons = 0; }

		// speed
		if (CheckAttribute(chref, "Ship.Speed.z") && CheckAttribute(&NetBInterface, "ShipSpeedScaler"))
		{
			shipSpeed = stf(chref.Ship.Speed.z) * stf(NetBInterface.ShipSpeedScaler);
		}

		// charge
		if (CheckAttribute(chref, "Ship.Cannons.Charge.Type"))
		{
			switch (sti(chref.Ship.Cannons.Charge.Type))
			{
				case GOOD_BALLS: shipCharge = 0; break;
				case GOOD_GRAPES: shipCharge = 3; break;
				case GOOD_KNIPPELS: shipCharge = 2; break;
				case GOOD_BOMBS: shipCharge = 1; break;
			}
		}

		// sails state (icon)
		nSailState = 1;
		float fSailSt = NetClient_ShipGetSailState(wNetClientID);
		if (fSailSt < 0.3) { nSailState = 0; }
		else
		{
			if (fSailSt < 0.6) { nSailState = 1; }
			else { nSailState = 2; }
		}

		// captain
		if (CheckAttribute(chref, "NickName")) { sCaptainName = chref.NickName; }
		if (CheckAttribute(chref, "Skill.Defence")) { nFencingSkill = sti(chref.Skill.Defence); }
		if (CheckAttribute(chref, "Skill.Cannons")) { nCannonSkill = sti(chref.Skill.Cannons); }
		if (CheckAttribute(chref, "Skill.Accuracy")) { nAccuracySkill = sti(chref.Skill.Accuracy); }
		if (CheckAttribute(chref, "Skill.Sailing")) { nNavigationSkill = sti(chref.Skill.Sailing); }
		if (CheckAttribute(chref, "Skill.Repair")) { nBoardingSkill = sti(chref.Skill.Repair); }
		if (CheckAttribute(chref, "Skill.Grappling")) { nGrapplingSkill = sti(chref.Skill.Grappling); }

		sCannonType = Net_SpyGlassCannonName(chref);

		sNetUserFaceTex = Net_SpyGlassFacePath(chref);
	}

	// the same line as in the single player: caliber + musket volley (perk MusketsShoot, everybody has it, not the fort) + captain
	if (bNetBoardPerkMusketsShoot && !bFort)
	{
		sCaptainName = sCannonType + "  ћушкетный залп   " + sCaptainName;
	}
	else
	{
		sCaptainName = sCannonType + "       " + sCaptainName;
	}

	// the 'crew' text of the entity now shows the boarding skill (the entity has no 6th skill field), so the crew goes to the caption line
	if (shipCrew >= 0) { sCaptainName = " Ёкипаж " + shipCrew + "     " + sCaptainName; }

	float fDistance = Net_SpyGlassDistance(chref, bFort);

	if (wNetClientID != iSpyLastId || iSpyTraceCnt > 20)
	{
		iSpyTraceCnt = 0;
		iSpyLastId = wNetClientID;
		int iFortFlag = 0;
		if (bFort) { iFortFlag = 1; }
		Trace("COOP: spy v3 id=" + wNetClientID + " fort=" + iFortFlag + " dist=" + fDistance + " hull=" + shipHull + " crew=" + shipCrew + " cannons=" + shipCannons + "/" + shipMaxCannons + " face=" + sNetUserFaceTex);
	}
	iSpyTraceCnt++;

	// COOP s27: the panel of the fort was sent again every frame (the timer and the event ReleaseTelescopeInfo of the engine) and
	// started its show anew each time: only the icon was seen for a moment. It is sent now only when its data change (the distance
	// in steps of 25 m); a ship comes from the engine (SetTelescopeInfo) as before.
	if (bFort)
	{
		// COOP s27: the panel of the engine is seen only while it has a target: the fort becomes its target (value 2, the patch of
		// battle_interface.dll, _backup_s27\tools\patch_bi_s27.py), sent every time to keep it
		SendMessage(&objISpyGlass, "ll", MSG_ISG_VISIBLE, 2);
		string sKey = "f" + shipName + "|" + shipHull + "|" + shipCrew + "|" + shipCannons + "|" + shipMaxCannons + "|" + makeint(fDistance / 25.0);
		if (sKey == sSpyShownKey) { return; }
		sSpyShownKey = sKey;
	}
	else
	{
		sSpyShownKey = "";
	}

	SendMessage(&objISpyGlass, "lsslllfflllllllllllssl", MSG_ISG_UPDATE,
		shipName, shipType,
		shipHull, shipSail, nGrapplingSkill, shipSpeed, fDistance,
		shipCannons, shipMaxCannons,
		shipCharge, shipNation, nSailState, -1,
		nFencingSkill, nCannonSkill, nAccuracySkill, nNavigationSkill, nBoardingSkill,
		sCaptainName, sNetUserFaceTex, shipClass);
	SendMessage(&objISpyGlass, "lsffff", MSG_ISG_SET_SHIPICON, sTextureName, uvLeft, uvTop, uvRight, uvBottom);
	SendMessage(&objISpyGlass, "ll", MSG_ISG_VISIBLE, true);
}
