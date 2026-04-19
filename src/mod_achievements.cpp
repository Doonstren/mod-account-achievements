/*
<--------------------------------------------------------------------------->
- Developer(s): Lille Carl, Grim/Render
- Complete: %100
- ScriptName: 'AccountAchievements'
- Comment: Tested and Works.
- Orginial Creator: Lille Carl
- Edited: Render/Grim
- Edited: Blkht01(Qeme) Classic Plus - Added Exclusion
<--------------------------------------------------------------------------->
*/

#include "Config.h"
#include "ScriptMgr.h"
#include "Chat.h"
#include "Player.h"
#include <unordered_set>
#include <sstream>

class AccountAchievements : public PlayerScript
{
	static const bool limitrace = true; // This set to true will only achievements from chars on the same team, do what you want. NOT RECOMMANDED TO BE CHANGED!!!
	static const bool limitlevel = false; // This checks the player's level and will only add achievements to players of that level.
	int minlevel = 80; // It's set to players of the level 60. Requires limitlevel to be set to true.
	int setlevel = 1; // Dont Change

	std::unordered_set<uint32> excludedAchievements; // Store excluded achievement IDs

public:
	AccountAchievements() : PlayerScript("AccountAchievements") 
	{
		// Load excluded achievements from config
		std::string excludeList = sConfigMgr->GetOption<std::string>("Account.Achievements.Excluded", "");
		std::stringstream ss(excludeList);
		std::string token;

		while (std::getline(ss, token, ',')) 
		{
			try {
				uint32 id = std::stoul(token);
				excludedAchievements.insert(id);
			} catch (...) {
				LOG_ERROR("module", "Invalid achievement ID in Account.Achievements.Excluded: {}", token);
			}
		}
	}

	void OnPlayerLogin(Player* pPlayer) override
	{
	        if (sConfigMgr->GetOption<bool>("Account.Achievements.Enable", true))
	        {
	                if (sConfigMgr->GetOption<bool>("Account.Achievements.Announce", true))
	                {
	                        ChatHandler(pPlayer->GetSession()).SendSysMessage("This server is running the |cff4CFF00AccountAchievements |rmodule.");
	                }

	                uint32 accountId = pPlayer->GetSession()->GetAccountId();
	                std::string guidsStr = "";
	                QueryResult result1 = CharacterDatabase.Query("SELECT guid, race FROM characters WHERE account = {}", accountId);

	                if (!result1)
	                        return;

	                std::vector<uint32> guids;
	                do
	                {
	                        Field* fields = result1->Fetch();
	                        uint32 race = fields[1].Get<uint8>();

	                        if (!limitrace || (Player::TeamIdForRace(race) == Player::TeamIdForRace(pPlayer->getRace())))
	                        {
	                                guids.push_back(fields[0].Get<uint32>());
	                                if (!guidsStr.empty()) guidsStr += ",";
	                                guidsStr += std::to_string(fields[0].Get<uint32>());
	                        }

	                } while (result1->NextRow());

	                if (guidsStr.empty())
	                        return;

	                std::unordered_set<uint32> achievementsToGrant;
	                QueryResult result2 = CharacterDatabase.Query("SELECT DISTINCT achievement FROM character_achievement WHERE guid IN ({})", guidsStr);

	                if (result2)
	                {
	                        do
	                        {
	                                uint32 achievementID = result2->Fetch()[0].Get<uint32>();
	                                if (excludedAchievements.find(achievementID) == excludedAchievements.end())
	                                {
	                                        if (!pPlayer->HasAchieved(achievementID))
	                                        {
	                                                achievementsToGrant.insert(achievementID);
	                                        }
	                                }
	                        } while (result2->NextRow());
	                }

	                for (uint32 achievementID : achievementsToGrant)
	                {
	                        auto sAchievement = sAchievementStore.LookupEntry(achievementID);
	                        if (sAchievement)
	                        {
	                                AddAchievements(pPlayer, sAchievement->ID);
	                        }
	                }
	        }
	}

	void AddAchievements(Player* player, uint32 AchievementID)
	{
	        if (sConfigMgr->GetOption<bool>("Account.Achievements.Enable", true))
	        {
	                if (limitlevel)
	                        setlevel = minlevel;

	                if (player->GetLevel() >= (uint32)setlevel)
	                {
	                        // Check exclusion before granting achievement
	                        if (excludedAchievements.find(AchievementID) == excludedAchievements.end() && !player->HasAchieved(AchievementID))     
	                        {
	                                player->CompletedAchievement(sAchievementStore.LookupEntry(AchievementID));
	                        }
	                }
	        }
	}

};

void AddAccountAchievementsScripts()
{
	new AccountAchievements;
}
