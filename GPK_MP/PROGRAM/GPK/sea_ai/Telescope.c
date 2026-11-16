void Telescope_Request()
{
	int nTelescopeIn = GetEventData();
	Trace("COOP: Telescope_Request " + nTelescopeIn + " deck=" + NCDeckCamera.On + " net=" + objISpyGlass.bNet);
	if( objISpyGlass.bNet=="1" ) {
		bNetTelescopeOn = false;
		if (sti(NCDeckCamera.On))
		{
			if (nTelescopeIn != 0) { bNetTelescopeOn = true; }	// COOP v3: the fort is looked for only with the telescope on
			SendMessage(&objISpyGlass, "ll", MSG_TELESCOPE_REQUEST,nTelescopeIn);
		}
	} else {
		if( GetCharacterEquipByGroup(pchar,SPYGLASS_ITEM_TYPE)=="" ) {
			nTelescopeIn = 0;
		}
		if( nTelescopeIn!=0 && SeaCameras.Camera=="SeaDeckCamera" ) {
			SendMessage(&objISpyGlass, "ll", MSG_TELESCOPE_REQUEST,1);
		} else {
			SendMessage(&objISpyGlass, "ll", MSG_TELESCOPE_REQUEST,0);
		}
	}
}

void Telescope_Off()
{
	bNetTelescopeOn = false;
	SendMessage(&objISpyGlass, "ll", MSG_TELESCOPE_REQUEST, 0);
}