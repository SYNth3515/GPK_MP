// COOP s27: the locations of the addon "Корсары 3: Тайны дальних морей" (BGTeam) for the land duel: their models are in
// RESOURCE\MODELS\Locations (their own folders, no one of GPK). The array of the locations is full (MAX_LOCATIONS), so a duel in
// one of them fills the slot "OldWorld_Port" (the old world of the GPK scripts has no models: never loaded) - NetTdm_Prepare.
// The key of the list / of the server is "Tdm_" + the id of the location in TDM. Generated (scratchpad tdm_gen.py).
#define NET_TDM_SLOT "OldWorld_Port"

void NetTdm_Loc0(ref rLoc)		// BrainRezak_house
{
	rLoc.filespath.models = "locations\Outside\Rheims House";
	rLoc.image = "loading\outside\jungle_house.tga";
	rLoc.type = "land";
	rLoc.models.always.locators = "Rhouse_l";
	rLoc.models.always.house = "Rhouse";
	rLoc.models.always.grassPatch = "Rhouse_g";
	rLoc.models.always.l1 = "rhouse_b01";
	rLoc.models.always.l1.level = 9;
	rLoc.models.always.l1.tech = "LocationModelBlend";
	rLoc.models.always.l2 = "rhouse_b02";
	rLoc.models.always.l2.level = 8;
	rLoc.models.always.l2.tech = "LocationModelBlend";
	rLoc.models.always.l3 = "rhouse_b03";
	rLoc.models.always.l3.level = 7;
	rLoc.models.always.l3.tech = "LocationModelBlend";
	rLoc.models.always.l4 = "rhouse_b04";
	rLoc.models.always.l4.level = 6;
	rLoc.models.always.l4.tech = "LocationModelBlend";
	rLoc.models.day.charactersPatch = "Rhouse_p";
	rLoc.models.night.charactersPatch = "Rhouse_p";
	rLoc.environment.weather = "true";
	rLoc.environment.sea = "false";
	rLoc.locators_radius.reload.reload1 = 3;
	rLoc.locators_radius.reload.reload1_back = 3;
	rLoc.reload.l1.name = "reload1";
	rLoc.reload.l1.go = NET_TDM_SLOT;
	rLoc.reload.l1.emerge = "reload1";
	rLoc.reload.l1.autoreload = "0";
	rLoc.reload.l1.disable = 1;
	rLoc.reload.l2.name = "reload1_back";
	rLoc.reload.l2.go = NET_TDM_SLOT;
	rLoc.reload.l2.emerge = "reload1_back";
	rLoc.reload.l2.autoreload = "0";
	rLoc.reload.l2.disable = 1;
	rLoc.reload.l3.name = "reload2";
	rLoc.reload.l3.go = NET_TDM_SLOT;
	rLoc.reload.l3.emerge = "reload2";
	rLoc.reload.l3.autoreload = "0";
	rLoc.reload.l3.disable = 1;
}

void NetTdm_Loc1(ref rLoc)		// Bunt_Deck_1
{
	rLoc.filespath.models = "locations\decks\deck01";
	rLoc.image = "loading\salary.tga";
	rLoc.type = "deck";
	rLoc.lockCamAngle = 0.4;
	rLoc.camshuttle = 1;
	rLoc.models.always.locators = "deck01_locators";
	rLoc.models.always.l1 = "deck01";
	rLoc.models.always.l2 = "deck01_fonars";
	rLoc.models.always.window = "deck01_alpha";
	rLoc.models.day.charactersPatch = "deck01_patch";
	rLoc.models.night.charactersPatch = "deck01_patch";
	rLoc.environment.weather = "true";
	rLoc.environment.sea = "true";
	rLoc.reload.l1.name = "reload1";
	rLoc.reload.l1.go = NET_TDM_SLOT;
	rLoc.reload.l1.emerge = "reload1";
	rLoc.reload.l1.autoreload = "0";
	rLoc.reload.l1.disable = 1;
}

void NetTdm_Loc2(ref rLoc)		// Deck_QB
{
	rLoc.filespath.models = "locations\decks\deck02";
	rLoc.type = "deck";
	rLoc.lockCamAngle = 0.4;
	rLoc.camshuttle = 1;
	rLoc.models.always.locators = "deck02_locators";
	rLoc.models.always.l1 = "deck02";
	rLoc.models.always.l2 = "deck02_alpha";
	rLoc.models.day.charactersPatch = "deck02_patch";
	rLoc.models.night.charactersPatch = "deck02_patch";
	rLoc.environment.weather = "true";
	rLoc.environment.sea = "true";
	rLoc.reload.l1.name = "reload1";
	rLoc.reload.l1.go = NET_TDM_SLOT;
	rLoc.reload.l1.emerge = "reload1";
	rLoc.reload.l1.autoreload = "0";
	rLoc.reload.l1.disable = 1;
}

void NetTdm_Loc3(ref rLoc)		// Grenada_house
{
	rLoc.filespath.models = "locations\Inside\SmallHome";
	rLoc.type = "house";
	rLoc.models.always.locators = "SH_l";
	rLoc.models.always.l1 = "SH";
	rLoc.models.always.window = "SH_w";
	rLoc.models.always.window.tech = "LocationWindows";
	rLoc.models.always.window.level = 50;
	rLoc.models.day.charactersPatch = "SH_p";
	rLoc.models.night.charactersPatch = "SH_p";
	rLoc.environment.weather = "true";
	rLoc.environment.sea = "false";
	rLoc.models.back = "back\qcresp_";
	rLoc.reload.l1.name = "Reload1";
	rLoc.reload.l1.go = NET_TDM_SLOT;
	rLoc.reload.l1.emerge = "Reload1";
	rLoc.reload.l1.autoreload = "0";
	rLoc.reload.l1.disable = 1;
}

void NetTdm_Loc4(ref rLoc)		// Grenada_Pirate_house
{
	rLoc.filespath.models = "locations\Outside\Pirate_Outer_Location";
	rLoc.image = "loading\outside\jungle_house.tga";
	rLoc.type = "land";
	rLoc.models.always.locators = "Pirl_l";
	rLoc.models.always.house = "pirl";
	rLoc.models.always.grassPatch = "pirl_g";
	rLoc.models.always.l1 = "plan_1";
	rLoc.models.always.l1.level = 9;
	rLoc.models.always.l1.tech = "LocationModelBlend";
	rLoc.models.always.l2 = "plan_2";
	rLoc.models.always.l2.level = 8;
	rLoc.models.always.l2.tech = "LocationModelBlend";
	rLoc.models.always.l3 = "plan_3";
	rLoc.models.always.l3.level = 7;
	rLoc.models.always.l3.tech = "LocationModelBlend";
	rLoc.models.day.charactersPatch = "Pirl_p";
	rLoc.models.day.fonar = "pirl_fd";
	rLoc.models.night.charactersPatch = "pirl_p";
	rLoc.models.night.fonar = "pirl_fn";
	rLoc.environment.weather = "true";
	rLoc.environment.sea = "false";
	rLoc.locators_radius.reload.reload1_back = 3.0;
	rLoc.reload.l1.name = "reload1";
	rLoc.reload.l1.go = NET_TDM_SLOT;
	rLoc.reload.l1.emerge = "reload1";
	rLoc.reload.l1.autoreload = "0";
	rLoc.reload.l1.disable = 1;
	rLoc.reload.l2.name = "reload1_back";
	rLoc.reload.l2.go = NET_TDM_SLOT;
	rLoc.reload.l2.emerge = "reload1_back";
	rLoc.reload.l2.autoreload = "0";
	rLoc.reload.l2.disable = 1;
	rLoc.reload.l3.name = "reload2";
	rLoc.reload.l3.go = NET_TDM_SLOT;
	rLoc.reload.l3.emerge = "reload2";
	rLoc.reload.l3.autoreload = "0";
	rLoc.reload.l3.disable = 1;
}

void NetTdm_Loc5(ref rLoc)		// Grenada_town
{
	rLoc.filespath.models = "locations\town_QC\town";
	rLoc.image = "loading\towns\Pirates.tga";
	rLoc.type = "town";
	rLoc.models.always.locators = "QCtown_l";
	rLoc.models.always.town = "QCtown";
	rLoc.models.always.grassPatch = "QCtown_g";
	rLoc.models.always.l1 = "plan_1";
	rLoc.models.always.l1.level = 9;
	rLoc.models.always.l1.tech = "LocationModelBlend";
	rLoc.models.always.l2 = "plan_2";
	rLoc.models.always.l2.level = 8;
	rLoc.models.always.l2.tech = "LocationModelBlend";
	rLoc.models.always.l3 = "plan_3";
	rLoc.models.always.l3.level = 7;
	rLoc.models.always.l3.tech = "LocationModelBlend";
	rLoc.models.day.charactersPatch = "QCtown_p";
	rLoc.models.day.fonar = "QCtown_fd";
	rLoc.models.night.charactersPatch = "QCtown_p";
	rLoc.models.night.fonar = "QCtown_fn";
	rLoc.environment.weather = "true";
	rLoc.environment.sea = "false";
	rLoc.reload.l1.name = "reload1";
	rLoc.reload.l1.go = NET_TDM_SLOT;
	rLoc.reload.l1.emerge = "reload1";
	rLoc.reload.l1.autoreload = "0";
	rLoc.reload.l1.disable = 1;
	rLoc.reload.l2.name = "reload2";
	rLoc.reload.l2.go = NET_TDM_SLOT;
	rLoc.reload.l2.emerge = "reload2";
	rLoc.reload.l2.autoreload = "0";
	rLoc.reload.l2.disable = 1;
	rLoc.reload.l3.name = "reload5";
	rLoc.reload.l3.go = NET_TDM_SLOT;
	rLoc.reload.l3.emerge = "reload5";
	rLoc.reload.l3.autoreload = "0";
	rLoc.reload.l3.disable = 1;
	rLoc.reload.l4.name = "reload6";
	rLoc.reload.l4.go = NET_TDM_SLOT;
	rLoc.reload.l4.emerge = "reload6";
	rLoc.reload.l4.autoreload = "0";
	rLoc.reload.l4.disable = 1;
	rLoc.reload.l5.name = "Reload9";
	rLoc.reload.l5.go = NET_TDM_SLOT;
	rLoc.reload.l5.emerge = "Reload9";
	rLoc.reload.l5.autoreload = "0";
	rLoc.reload.l5.disable = 1;
	rLoc.reload.l6.name = "reload10";
	rLoc.reload.l6.go = NET_TDM_SLOT;
	rLoc.reload.l6.emerge = "reload10";
	rLoc.reload.l6.autoreload = "0";
	rLoc.reload.l6.disable = 1;
}

void NetTdm_Loc6(ref rLoc)		// Grenada_town_exit
{
	rLoc.filespath.models = "locations\town_QC\exit";
	rLoc.image = "loading\outside\Pirate_Fort.tga";
	rLoc.type = "jungle";
	rLoc.models.always.locators = "QCexit_l";
	rLoc.models.always.exit = "QCexit";
	rLoc.models.always.grassPatch = "QCexit_g";
	rLoc.models.always.l1 = "plan_1";
	rLoc.models.always.l1.level = 9;
	rLoc.models.always.l1.tech = "LocationModelBlend";
	rLoc.models.always.l2 = "plan_2";
	rLoc.models.always.l2.level = 8;
	rLoc.models.always.l2.tech = "LocationModelBlend";
	rLoc.models.always.l3 = "plan_3";
	rLoc.models.always.l3.level = 7;
	rLoc.models.always.l3.tech = "LocationModelBlend";
	rLoc.models.day.charactersPatch = "QCexit_p";
	rLoc.models.night.charactersPatch = "QCexit_p";
	rLoc.environment.weather = "true";
	rLoc.environment.sea = "false";
	rLoc.locators_radius.reload.reload2 = 3.0;
	rLoc.locators_radius.reload.reload3 = 3.0;
	rLoc.locators_radius.reload.Reload2_back = 2.0;
	rLoc.locators_radius.reload.Reload3_back = 2.0;
	rLoc.reload.l1.name = "Reload1";
	rLoc.reload.l1.go = NET_TDM_SLOT;
	rLoc.reload.l1.emerge = "Reload1";
	rLoc.reload.l1.autoreload = "0";
	rLoc.reload.l1.disable = 1;
	rLoc.reload.l2.name = "Reload2";
	rLoc.reload.l2.go = NET_TDM_SLOT;
	rLoc.reload.l2.emerge = "Reload2";
	rLoc.reload.l2.autoreload = "0";
	rLoc.reload.l2.disable = 1;
	rLoc.reload.l3.name = "Reload3";
	rLoc.reload.l3.go = NET_TDM_SLOT;
	rLoc.reload.l3.emerge = "Reload3";
	rLoc.reload.l3.autoreload = "0";
	rLoc.reload.l3.disable = 1;
	rLoc.reload.l4.name = "Reload2_back";
	rLoc.reload.l4.go = NET_TDM_SLOT;
	rLoc.reload.l4.emerge = "Reload2_back";
	rLoc.reload.l4.autoreload = "0";
	rLoc.reload.l4.disable = 1;
	rLoc.reload.l5.name = "Reload3_back";
	rLoc.reload.l5.go = NET_TDM_SLOT;
	rLoc.reload.l5.emerge = "Reload3_back";
	rLoc.reload.l5.autoreload = "0";
	rLoc.reload.l5.disable = 1;
	rLoc.reload.l6.name = "Reload4";
	rLoc.reload.l6.go = NET_TDM_SLOT;
	rLoc.reload.l6.emerge = "Reload4";
	rLoc.reload.l6.autoreload = "0";
	rLoc.reload.l6.disable = 1;
	rLoc.reload.l7.name = "Reload5";
	rLoc.reload.l7.go = NET_TDM_SLOT;
	rLoc.reload.l7.emerge = "Reload5";
	rLoc.reload.l7.autoreload = "0";
	rLoc.reload.l7.disable = 1;
}

void NetTdm_Loc7(ref rLoc)		// GrottoQBP
{
	rLoc.filespath.models = "locations\Grotto\";
	rLoc.type = "cave";
	rLoc.models.always.town = "Grotto";
	rLoc.models.always.seabed = "Grotto_sb";
	rLoc.models.always.grassPatch = "Grotto_grass";
	rLoc.models.always.locators = "Grotto_locators";
	rLoc.models.always.l2 = "Gold";
	rLoc.models.always.l2.tech = "EnvironmentShader";
	rLoc.models.always.l2.reflection = 1;
	rLoc.models.always.Waterfall1 = "water";
	rLoc.models.always.Waterfall1.uvslide.v0 = 0.0;
	rLoc.models.always.Waterfall1.uvslide.v1 = 0.0;
	rLoc.models.always.Waterfall1.tech = "LocationWaterFall";
	rLoc.models.always.Waterfall1.level = 50;
	rLoc.models.always.Waterfall2 = "cfall";
	rLoc.models.always.Waterfall2.uvslide.v0 = 0.3;
	rLoc.models.always.Waterfall2.uvslide.v1 = 0.0;
	rLoc.models.always.Waterfall2.tech = "LocationWaterFall";
	rLoc.models.always.Waterfall2.level = 50;
	rLoc.models.day.charactersPatch = "Grotto_patch_day";
	rLoc.models.night.charactersPatch = "Grotto_patch_day";
	rLoc.environment.weather = "true";
	rLoc.environment.sea = "true";
	rLoc.reload.l1.name = "reload1";
	rLoc.reload.l1.go = NET_TDM_SLOT;
	rLoc.reload.l1.emerge = "reload1";
	rLoc.reload.l1.autoreload = "0";
	rLoc.reload.l1.disable = 1;
}

void NetTdm_Loc8(ref rLoc)		// MountainPathDMChestQTDM
{
	rLoc.filespath.models = "locations\MountainPathDMChest";
	rLoc.image = "loading\outside\harbor3.tga";
	rLoc.type = "seashore";
	rLoc.models.always.locators = "MountainPathDMChest_locators";
	rLoc.models.always.shore = "MountainPathDMChest";
	rLoc.models.always.shore.foam = "1";
	rLoc.models.always.seabed = "MountainPathDMChest_sb";
	rLoc.models.always.Waterfall1 = "waterfall1";
	rLoc.models.always.Waterfall1.uvslide.v0 = 0.5;
	rLoc.models.always.Waterfall1.uvslide.v1 = 0.0;
	rLoc.models.always.Waterfall1.tech = "LocationWaterFall";
	rLoc.models.always.Waterfall1.level = 50;
	rLoc.models.always.Waterfall2 = "waterfall2";
	rLoc.models.always.Waterfall2.uvslide.v0 = 0.5;
	rLoc.models.always.Waterfall2.tech = "LocationWaterFall";
	rLoc.models.always.Waterfall2.level = 50;
	rLoc.models.day.charactersPatch = "MountainPathDMChest_patch_day";
	rLoc.models.night.charactersPatch = "MountainPathDMChest_patch_day";
	rLoc.environment.weather = "true";
	rLoc.environment.sea = "true";
	rLoc.locators_radius.reload.reload1 = 3;
	rLoc.reload.l1.name = "reload1";
	rLoc.reload.l1.go = NET_TDM_SLOT;
	rLoc.reload.l1.emerge = "reload1";
	rLoc.reload.l1.autoreload = "0";
	rLoc.reload.l1.disable = 1;
}

void NetTdm_Loc9(ref rLoc)		// Nevis_mine
{
	rLoc.filespath.models = "Locations\Mine";
	rLoc.image = "Loading\outside\mine.tga";
	rLoc.type = "cave";
	rLoc.models.always.locators = "mine_locators_addon";
	rLoc.models.always.cave = "mine";
	rLoc.models.always.grassPatch = "mine_grass";
	rLoc.models.day.charactersPatch = "mine_patch";
	rLoc.models.day.fonar = "mine_fd";
	rLoc.models.night.charactersPatch = "mine_patch";
	rLoc.models.night.fonar = "mine_fn";
	rLoc.environment.weather = "true";
	rLoc.environment.sea = "false";
	rLoc.locators_radius.reload.reload2 = 2.0;
	rLoc.locators_radius.reload.reload2_back = 2.0;
	rLoc.reload.l1.name = "reload2";
	rLoc.reload.l1.go = NET_TDM_SLOT;
	rLoc.reload.l1.emerge = "reload2";
	rLoc.reload.l1.autoreload = "0";
	rLoc.reload.l1.disable = 1;
	rLoc.reload.l2.name = "reload2_back";
	rLoc.reload.l2.go = NET_TDM_SLOT;
	rLoc.reload.l2.emerge = "reload2_back";
	rLoc.reload.l2.autoreload = "0";
	rLoc.reload.l2.disable = 1;
}

void NetTdm_Loc10(ref rLoc)		// SanSeb_port
{
	rLoc.filespath.models = "locations\Town_Redmond\Port\";
	rLoc.image = "loading\towns\SanSeb.tga";
	rLoc.type = "jungle";
	rLoc.models.always.l1 = "RedPort";
	rLoc.models.always.l1.foam = "1";
	rLoc.models.always.locators = "RedPort_l";
	rLoc.models.always.grassPatch = "Redport_g";
	rLoc.models.always.seabed = "RedPort_sb";
	rLoc.models.always.seabed.foam = "1";
	rLoc.models.day.charactersPatch = "RedPort_pd";
	rLoc.models.day.rinok = "RedPort_e01";
	rLoc.models.day.fonar = "RedPort_fd";
	rLoc.models.day.jumppatch = "RedPort_j";
	rLoc.models.night.charactersPatch = "Redport_pn";
	rLoc.models.night.fonar = "Redport_fn";
	rLoc.models.night.jumppatch = "RedPort_j";
	rLoc.environment.weather = "true";
	rLoc.environment.sea = "true";
	rLoc.reload.l1.name = "reload_2_city";
	rLoc.reload.l1.go = NET_TDM_SLOT;
	rLoc.reload.l1.emerge = "reload_2_city";
	rLoc.reload.l1.autoreload = "0";
	rLoc.reload.l1.disable = 1;
	rLoc.reload.l2.name = "door_1";
	rLoc.reload.l2.go = NET_TDM_SLOT;
	rLoc.reload.l2.emerge = "door_1";
	rLoc.reload.l2.autoreload = "0";
	rLoc.reload.l2.disable = 1;
	rLoc.reload.l3.name = "door_6";
	rLoc.reload.l3.go = NET_TDM_SLOT;
	rLoc.reload.l3.emerge = "door_6";
	rLoc.reload.l3.autoreload = "0";
	rLoc.reload.l3.disable = 1;
	rLoc.reload.l4.name = "door_5";
	rLoc.reload.l4.go = NET_TDM_SLOT;
	rLoc.reload.l4.emerge = "door_5";
	rLoc.reload.l4.autoreload = "0";
	rLoc.reload.l4.disable = 1;
	rLoc.reload.l5.name = "door_3";
	rLoc.reload.l5.go = NET_TDM_SLOT;
	rLoc.reload.l5.emerge = "door_3";
	rLoc.reload.l5.autoreload = "0";
	rLoc.reload.l5.disable = 1;
	rLoc.reload.l6.name = "door_4";
	rLoc.reload.l6.go = NET_TDM_SLOT;
	rLoc.reload.l6.emerge = "door_4";
	rLoc.reload.l6.autoreload = "0";
	rLoc.reload.l6.disable = 1;
	rLoc.reload.l7.name = "door_2";
	rLoc.reload.l7.go = NET_TDM_SLOT;
	rLoc.reload.l7.emerge = "door_2";
	rLoc.reload.l7.autoreload = "0";
	rLoc.reload.l7.disable = 1;
}

void NetTdm_Loc11(ref rLoc)		// SanSebastian_town
{
	rLoc.filespath.models = "locations\Town_Redmond\Town_01";
	rLoc.image = "loading\towns\SanSeb.tga";
	rLoc.type = "town";
	rLoc.models.always.L1 = "Red01";
	rLoc.models.always.locators = "Red01_l";
	rLoc.models.always.grassPatch = "Red01_g";
	rLoc.models.always.dno = "Red01_rb01";
	rLoc.models.always.dno.level = 95;
	rLoc.models.always.dno.uvslide.u1 = -0.12;
	rLoc.models.always.dno.uvslide.v1 = -0.03;
	rLoc.models.always.dno.tech = "LocationRiverFloor";
	rLoc.models.always.mirrow = "Red01_re01";
	rLoc.models.always.mirrow.level = 96;
	rLoc.models.always.dnoBlend = "Red01_rb01";
	rLoc.models.always.dnoBlend.level = 97;
	rLoc.models.always.dnoBlend.uvslide.u1 = -0.1;
	rLoc.models.always.dnoBlend.uvslide.v1 = 0.04;
	rLoc.models.always.dnoBlend.tech = "LocationRiverFloorBlend";
	rLoc.models.always.river = "Red01_rt01";
	rLoc.models.always.river.level = 98;
	rLoc.models.always.river.uvslide.v0 = -0.1;
	rLoc.models.always.river.uvslide.u1 = 0.15;
	rLoc.models.always.river.tech = "LocationRiver";
	rLoc.models.day.fonar = "Red01_fd";
	rLoc.models.day.charactersPatch = "Red01_p";
	rLoc.models.day.rinok = "Red01_e01";
	rLoc.models.night.fonar = "Red01_fn";
	rLoc.models.night.charactersPatch = "Red01_p";
	rLoc.environment.weather = "true";
	rLoc.environment.sea = "false";
	rLoc.reload.l1.name = "reload_2_3";
	rLoc.reload.l1.go = NET_TDM_SLOT;
	rLoc.reload.l1.emerge = "reload_2_3";
	rLoc.reload.l1.autoreload = "0";
	rLoc.reload.l1.disable = 1;
	rLoc.reload.l2.name = "reload_2_4_back";
	rLoc.reload.l2.go = NET_TDM_SLOT;
	rLoc.reload.l2.emerge = "reload_2_4_back";
	rLoc.reload.l2.autoreload = "0";
	rLoc.reload.l2.disable = 1;
	rLoc.reload.l3.name = "reload_1_3_back";
	rLoc.reload.l3.go = NET_TDM_SLOT;
	rLoc.reload.l3.emerge = "reload_1_3_back";
	rLoc.reload.l3.autoreload = "0";
	rLoc.reload.l3.disable = 1;
	rLoc.reload.l4.name = "reload_1_4_back";
	rLoc.reload.l4.go = NET_TDM_SLOT;
	rLoc.reload.l4.emerge = "reload_1_4_back";
	rLoc.reload.l4.autoreload = "0";
	rLoc.reload.l4.disable = 1;
	rLoc.reload.l5.name = "reload_from_port";
	rLoc.reload.l5.go = NET_TDM_SLOT;
	rLoc.reload.l5.emerge = "reload_from_port";
	rLoc.reload.l5.autoreload = "0";
	rLoc.reload.l5.disable = 1;
	rLoc.reload.l6.name = "door_5";
	rLoc.reload.l6.go = NET_TDM_SLOT;
	rLoc.reload.l6.emerge = "door_5";
	rLoc.reload.l6.autoreload = "0";
	rLoc.reload.l6.disable = 1;
	rLoc.reload.l7.name = "door_1";
	rLoc.reload.l7.go = NET_TDM_SLOT;
	rLoc.reload.l7.emerge = "door_1";
	rLoc.reload.l7.autoreload = "0";
	rLoc.reload.l7.disable = 1;
	rLoc.reload.l8.name = "door_11";
	rLoc.reload.l8.go = NET_TDM_SLOT;
	rLoc.reload.l8.emerge = "door_11";
	rLoc.reload.l8.autoreload = "0";
	rLoc.reload.l8.disable = 1;
	rLoc.reload.l9.name = "door_56";
	rLoc.reload.l9.go = NET_TDM_SLOT;
	rLoc.reload.l9.emerge = "door_56";
	rLoc.reload.l9.autoreload = "0";
	rLoc.reload.l9.disable = 1;
	rLoc.reload.l10.name = "door_3";
	rLoc.reload.l10.go = NET_TDM_SLOT;
	rLoc.reload.l10.emerge = "door_3";
	rLoc.reload.l10.autoreload = "0";
	rLoc.reload.l10.disable = 1;
	rLoc.reload.l11.name = "door_9";
	rLoc.reload.l11.go = NET_TDM_SLOT;
	rLoc.reload.l11.emerge = "door_9";
	rLoc.reload.l11.autoreload = "0";
	rLoc.reload.l11.disable = 1;
	rLoc.reload.l12.name = "door_4";
	rLoc.reload.l12.go = NET_TDM_SLOT;
	rLoc.reload.l12.emerge = "door_4";
	rLoc.reload.l12.autoreload = "0";
	rLoc.reload.l12.disable = 1;
	rLoc.reload.l13.name = "door_6";
	rLoc.reload.l13.go = NET_TDM_SLOT;
	rLoc.reload.l13.emerge = "door_6";
	rLoc.reload.l13.autoreload = "0";
	rLoc.reload.l13.disable = 1;
	rLoc.reload.l14.name = "door58";
	rLoc.reload.l14.go = NET_TDM_SLOT;
	rLoc.reload.l14.emerge = "door58";
	rLoc.reload.l14.autoreload = "0";
	rLoc.reload.l14.disable = 1;
	rLoc.reload.l15.name = "door_8";
	rLoc.reload.l15.go = NET_TDM_SLOT;
	rLoc.reload.l15.emerge = "door_8";
	rLoc.reload.l15.autoreload = "0";
	rLoc.reload.l15.disable = 1;
	rLoc.reload.l16.name = "door_57";
	rLoc.reload.l16.go = NET_TDM_SLOT;
	rLoc.reload.l16.emerge = "door_57";
	rLoc.reload.l16.autoreload = "0";
	rLoc.reload.l16.disable = 1;
	rLoc.reload.l17.name = "door_2";
	rLoc.reload.l17.go = NET_TDM_SLOT;
	rLoc.reload.l17.emerge = "door_2";
	rLoc.reload.l17.autoreload = "0";
	rLoc.reload.l17.disable = 1;
	rLoc.reload.l18.name = "door57";
	rLoc.reload.l18.go = NET_TDM_SLOT;
	rLoc.reload.l18.emerge = "door57";
	rLoc.reload.l18.autoreload = "0";
	rLoc.reload.l18.disable = 1;
	rLoc.reload.l19.name = "door_55";
	rLoc.reload.l19.go = NET_TDM_SLOT;
	rLoc.reload.l19.emerge = "door_55";
	rLoc.reload.l19.autoreload = "0";
	rLoc.reload.l19.disable = 1;
	rLoc.reload.l20.name = "shipyard";
	rLoc.reload.l20.go = NET_TDM_SLOT;
	rLoc.reload.l20.emerge = "shipyard";
	rLoc.reload.l20.autoreload = "0";
	rLoc.reload.l20.disable = 1;
}

void NetTdm_Loc12(ref rLoc)		// SK_Smugglers_Tavern
{
	rLoc.filespath.models = "locations\Inside\MediumTavern";
	rLoc.image = "loading\inside\Tavern.tga";
	rLoc.type = "tavern";
	rLoc.models.always.locators = "MT_l";
	rLoc.models.always.l1 = "MT";
	rLoc.models.always.window = "MT_w";
	rLoc.models.always.window.tech = "LocationWindows";
	rLoc.models.always.window.level = 50;
	rLoc.models.day.charactersPatch = "MT_p";
	rLoc.models.night.charactersPatch = "MT_p";
	rLoc.environment.weather = "true";
	rLoc.environment.sea = "false";
	rLoc.models.back = "back\smumt_";
	rLoc.reload.l1.name = "reload1";
	rLoc.reload.l1.go = NET_TDM_SLOT;
	rLoc.reload.l1.emerge = "reload1";
	rLoc.reload.l1.autoreload = "0";
	rLoc.reload.l1.disable = 1;
	rLoc.reload.l2.name = "reload2";
	rLoc.reload.l2.go = NET_TDM_SLOT;
	rLoc.reload.l2.emerge = "reload2";
	rLoc.reload.l2.autoreload = "0";
	rLoc.reload.l2.disable = 1;
	rLoc.reload.l3.name = "back_to_tavern";
	rLoc.reload.l3.go = NET_TDM_SLOT;
	rLoc.reload.l3.emerge = "back_to_tavern";
	rLoc.reload.l3.autoreload = "0";
	rLoc.reload.l3.disable = 1;
}

int NetTdm_Count() { return 13; }

string NetTdm_KeyN(int i)
{
	switch (i)
	{
		case 0: return "Tdm_BrainRezak_house"; break;
		case 1: return "Tdm_Bunt_Deck_1"; break;
		case 2: return "Tdm_Deck_QB"; break;
		case 3: return "Tdm_Grenada_house"; break;
		case 4: return "Tdm_Grenada_Pirate_hou"; break;
		case 5: return "Tdm_Grenada_town"; break;
		case 6: return "Tdm_Grenada_town_exit"; break;
		case 7: return "Tdm_GrottoQBP"; break;
		case 8: return "Tdm_MountainPathDMChes"; break;
		case 9: return "Tdm_Nevis_mine"; break;
		case 10: return "Tdm_SanSeb_port"; break;
		case 11: return "Tdm_SanSebastian_town"; break;
		case 12: return "Tdm_SK_Smugglers_Taver"; break;
	}
	return "";
}

string NetTdm_NameN(int i)
{
	switch (i)
	{
		case 0: return "Кюрасао: дом в джунглях"; break;
		case 1: return "Палуба корабля 1"; break;
		case 2: return "Палуба корабля 2"; break;
		case 3: return "Гренада: маленький дом"; break;
		case 4: return "Гренада: пиратский дом"; break;
		case 5: return "Гренада: город"; break;
		case 6: return "Гренада: выход из города"; break;
		case 7: return "Грот"; break;
		case 8: return "Горная тропа"; break;
		case 9: return "Невис: рудник"; break;
		case 10: return "Сан-Себастьян (Старый Свет): порт"; break;
		case 11: return "Сан-Себастьян (Старый Свет): город"; break;
		case 12: return "Сент-Китс: таверна контрабандистов"; break;
	}
	return "";
}

int NetTdm_Index(string sKey)
{
	for (int i=0; i<NetTdm_Count(); i++) { if (NetTdm_KeyN(i) == sKey) { return i; } }
	return -1;
}

bool NetTdm_IsKey(string sLoc)
{
	if (findSubStr(sLoc, "Tdm_", 0) == 0) { return true; }
	return false;
}

string NetTdm_Name(string sKey)
{
	int i = NetTdm_Index(sKey);
	if (i < 0) { return sKey; }
	return NetTdm_NameN(i);
}

string NetTdm_Image(string sKey)
{
	switch (sKey)
	{
		case "Tdm_BrainRezak_house": return "loading\outside\jungle_house.tga"; break;
		case "Tdm_Bunt_Deck_1": return "loading\salary.tga"; break;
		case "Tdm_Deck_QB": return "loading\Town.tga"; break;
		case "Tdm_Grenada_house": return "loading\Town.tga"; break;
		case "Tdm_Grenada_Pirate_hou": return "loading\outside\jungle_house.tga"; break;
		case "Tdm_Grenada_town": return "loading\towns\Pirates.tga"; break;
		case "Tdm_Grenada_town_exit": return "loading\outside\Pirate_Fort.tga"; break;
		case "Tdm_GrottoQBP": return "loading\Town.tga"; break;
		case "Tdm_MountainPathDMChes": return "loading\outside\harbor3.tga"; break;
		case "Tdm_Nevis_mine": return "Loading\outside\mine.tga"; break;
		case "Tdm_SanSeb_port": return "loading\towns\SanSeb.tga"; break;
		case "Tdm_SanSebastian_town": return "loading\towns\SanSeb.tga"; break;
		case "Tdm_SK_Smugglers_Taver": return "loading\inside\Tavern.tga"; break;
	}
	return "loading\Town.tga";
}

// the slot gets the location of the key; the id of the slot is returned ("" = an unknown key: an older client)
string NetTdm_Prepare(string sKey)
{
	int iTdm = NetTdm_Index(sKey);
	if (iTdm < 0) { return ""; }
	int iSlot = FindLocation(NET_TDM_SLOT);
	if (iSlot < 0) { return ""; }
	ref rLoc = &Locations[iSlot];
	DeleteAttribute(rLoc, "");
	rLoc.id = NET_TDM_SLOT;
	rLoc.index = iSlot;
	switch (iTdm)
	{
		case 0: NetTdm_Loc0(rLoc); break;
		case 1: NetTdm_Loc1(rLoc); break;
		case 2: NetTdm_Loc2(rLoc); break;
		case 3: NetTdm_Loc3(rLoc); break;
		case 4: NetTdm_Loc4(rLoc); break;
		case 5: NetTdm_Loc5(rLoc); break;
		case 6: NetTdm_Loc6(rLoc); break;
		case 7: NetTdm_Loc7(rLoc); break;
		case 8: NetTdm_Loc8(rLoc); break;
		case 9: NetTdm_Loc9(rLoc); break;
		case 10: NetTdm_Loc10(rLoc); break;
		case 11: NetTdm_Loc11(rLoc); break;
		case 12: NetTdm_Loc12(rLoc); break;
	}
	LAi_LocationFantomsGen(rLoc, false);
	Trace("COOP: land duel: the TDM location " + sKey + " is in the slot " + NET_TDM_SLOT);
	return NET_TDM_SLOT;
}
