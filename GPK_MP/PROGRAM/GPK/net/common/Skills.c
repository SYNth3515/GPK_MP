object NetSkills[16];
int iNetSkillsNum;

ref Net_GetSkillByName(string sSkill) 
{
	for (int i=0; i<iNetSkillsNum; i++) { if (sSkill == NetSkills[i].Name) return &NetSkills[i]; }
	trace("WARNING! NetSkills ID (" + sSkill + ") not found!");
}
ref Net_GetSkillByIndex(int iIndex) { return &NetSkills[iIndex]; }
int Net_GetSkillsNum() { return iNetSkillsNum; }

int Net_GetSkillCost(int iSkillIndex, int iNumSkillPoints)
{
	return sti(NetSkills[iSkillIndex].Cost) * iNumSkillPoints;
}

// COOP s28: the prices of the skills x10 (one point; the first point is free in the buy screen). All six skills at 10 cost
// 9 * 9000 = 81000 (the budget of a full fast manowar, see common\Perks.c)
void Net_InitSkills()
{
	ref rS;
	int n = 0;

	makeref(rS, NetSkills[n]); n++;
	rS.Name = "Accuracy";
	rS.Cost = 1500;

	makeref(rS, NetSkills[n]); n++;
	rS.Name = "Cannons";
	rS.Cost = 1500;

	makeref(rS, NetSkills[n]); n++;
	rS.Name = "Sailing";
	rS.Cost = 1000;
	
	makeref(rS, NetSkills[n]); n++;
	rS.Name = "Defence";
	rS.Cost = 1500;

	makeref(rS, NetSkills[n]); n++;
	rS.Name = "Repair";
	rS.Cost = 2000;

	makeref(rS, NetSkills[n]); n++;
	rS.Name = "Grappling";
	rS.Cost = 1500;

	iNetSkillsNum = n;
}