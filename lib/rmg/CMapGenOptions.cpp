#include "StdInc.h"
#include "CMapGenOptions.h"

#include "../GameConstants.h"
#include "../CRandomGenerator.h"
#include "../VCMI_Lib.h"
#include "../CTownHandler.h"

CMapGenOptions::CMapGenOptions() : width(72), height(72), hasTwoLevels(true),
	playersCnt(RANDOM_SIZE), teamsCnt(RANDOM_SIZE), compOnlyPlayersCnt(0), compOnlyTeamsCnt(RANDOM_SIZE),
	waterContent(EWaterContent::RANDOM), monsterStrength(EMonsterStrength::RANDOM)
{

}

si32 CMapGenOptions::getWidth() const
{
	return width;
}

void CMapGenOptions::setWidth(si32 value)
{
	if(value > 0)
	{
		width = value;
	}
	else
	{
		throw std::runtime_error("A map width lower than 1 is not allowed.");
	}
}

si32 CMapGenOptions::getHeight() const
{
	return height;
}

void CMapGenOptions::setHeight(si32 value)
{
	if(value > 0)
	{
		height = value;
	}
	else
	{
		throw std::runtime_error("A map height lower than 1 is not allowed.");
	}
}

bool CMapGenOptions::getHasTwoLevels() const
{
	return hasTwoLevels;
}

void CMapGenOptions::setHasTwoLevels(bool value)
{
	hasTwoLevels = value;
}

si8 CMapGenOptions::getPlayersCnt() const
{
	return playersCnt;
}

void CMapGenOptions::setPlayersCnt(si8 value)
{
	if((value >= 1 && value <= PlayerColor::PLAYER_LIMIT_I) || value == RANDOM_SIZE)
	{
		playersCnt = value;
        resetPlayersMap();
	}
	else
	{
		throw std::runtime_error("Players count of RMG options should be between 1 and " +
			boost::lexical_cast<std::string>(PlayerColor::PLAYER_LIMIT) + " or CMapGenOptions::RANDOM_SIZE for random.");
	}
}

si8 CMapGenOptions::getTeamsCnt() const
{
	return teamsCnt;
}

void CMapGenOptions::setTeamsCnt(si8 value)
{
	if(playersCnt == RANDOM_SIZE || (value >= 0 && value < playersCnt) || value == RANDOM_SIZE)
	{
		teamsCnt = value;
	}
	else
	{
		throw std::runtime_error("Teams count of RMG options should be between 0 and <" +
			boost::lexical_cast<std::string>(playersCnt) + "(players count) - 1> or CMapGenOptions::RANDOM_SIZE for random.");
	}
}

si8 CMapGenOptions::getCompOnlyPlayersCnt() const
{
	return compOnlyPlayersCnt;
}

void CMapGenOptions::setCompOnlyPlayersCnt(si8 value)
{
	if(value == RANDOM_SIZE || (value >= 0 && value <= PlayerColor::PLAYER_LIMIT_I - playersCnt))
	{
		compOnlyPlayersCnt = value;
        resetPlayersMap();
	}
	else
	{
		throw std::runtime_error(std::string("Computer only players count of RMG options should be ") +
			"between 0 and <PlayerColor::PLAYER_LIMIT - " + boost::lexical_cast<std::string>(playersCnt) +
			"(playersCount)> or CMapGenOptions::RANDOM_SIZE for random.");
	}
}

si8 CMapGenOptions::getCompOnlyTeamsCnt() const
{
	return compOnlyTeamsCnt;
}

void CMapGenOptions::setCompOnlyTeamsCnt(si8 value)
{
	if(value == RANDOM_SIZE || compOnlyPlayersCnt == RANDOM_SIZE || (value >= 0 && value <= std::max(compOnlyPlayersCnt - 1, 0)))
	{
		compOnlyTeamsCnt = value;
	}
	else
	{
		throw std::runtime_error(std::string("Computer only teams count of RMG options should be ") +
			"between 0 and <" + boost::lexical_cast<std::string>(compOnlyPlayersCnt) +
			"(compOnlyPlayersCnt) - 1> or CMapGenOptions::RANDOM_SIZE for random.");
	}
}

EWaterContent::EWaterContent CMapGenOptions::getWaterContent() const
{
	return waterContent;
}

void CMapGenOptions::setWaterContent(EWaterContent::EWaterContent value)
{
	waterContent = value;
}

EMonsterStrength::EMonsterStrength CMapGenOptions::getMonsterStrength() const
{
	return monsterStrength;
}

void CMapGenOptions::setMonsterStrength(EMonsterStrength::EMonsterStrength value)
{
	monsterStrength = value;
}

void CMapGenOptions::resetPlayersMap()
{
    players.clear();
    int realPlayersCnt = playersCnt == RANDOM_SIZE ? PlayerColor::PLAYER_LIMIT_I : playersCnt;
    int realCompOnlyPlayersCnt = compOnlyPlayersCnt == RANDOM_SIZE ? (PlayerColor::PLAYER_LIMIT_I - realPlayersCnt) : compOnlyPlayersCnt;
    for(int color = 0; color < (realPlayersCnt + realCompOnlyPlayersCnt); ++color)
    {
        CPlayerSettings player;
        player.setColor(PlayerColor(color));
        player.setPlayerType((color >= playersCnt) ? EPlayerType::COMP_ONLY : EPlayerType::AI);
        players[PlayerColor(color)] = player;
    }
}

const std::map<PlayerColor, CMapGenOptions::CPlayerSettings> & CMapGenOptions::getPlayersSettings() const
{
    return players;
}

void CMapGenOptions::setStartingTownForPlayer(PlayerColor color, si32 town)
{
    auto it = players.find(color);
    if(it == players.end()) throw std::runtime_error(boost::str(boost::format("Cannot set starting town for the player with the color '%i'.") % color));
    it->second.setStartingTown(town);
}

void CMapGenOptions::setPlayerTypeForStandardPlayer(PlayerColor color, EPlayerType::EPlayerType playerType)
{
    auto it = players.find(color);
    if(it == players.end()) throw std::runtime_error(boost::str(boost::format("Cannot set player type for the player with the color '%i'.") % color));
    if(playerType == EPlayerType::COMP_ONLY) throw std::runtime_error("Cannot set player type computer only to a standard player.");
    it->second.setPlayerType(playerType);
}

void CMapGenOptions::finalize()
{
    CRandomGenerator gen;
    finalize(gen);
}

void CMapGenOptions::finalize(CRandomGenerator & gen)
{
    if(playersCnt == RANDOM_SIZE)
    {
        // 1 human is required at least
        auto humanPlayers = countHumanPlayers();
        if(humanPlayers == 0) humanPlayers = 1;
        playersCnt = gen.getInteger(humanPlayers, PlayerColor::PLAYER_LIMIT_I);

        // Remove AI players only from the end of the players map if necessary
        for(auto itrev = players.end(); itrev != players.begin();)
        {
            auto it = itrev;
            --it;
            if(players.size() == playersCnt) break;
            if(it->second.getPlayerType() == EPlayerType::AI)
            {
                players.erase(it);
            }
            else
            {
                --itrev;
            }
        }
    }
    if(teamsCnt == RANDOM_SIZE)
    {
        teamsCnt = gen.getInteger(0, playersCnt - 1);
    }
    if(compOnlyPlayersCnt == RANDOM_SIZE)
    {
        compOnlyPlayersCnt = gen.getInteger(0, 8 - playersCnt);
        auto totalPlayersCnt = playersCnt + compOnlyPlayersCnt;

        // Remove comp only players only from the end of the players map if necessary
        for(auto itrev = players.end(); itrev != players.begin();)
        {
            auto it = itrev;
            --it;
            if(players.size() <= totalPlayersCnt) break;
            if(it->second.getPlayerType() == EPlayerType::COMP_ONLY)
            {
                players.erase(it);
            }
            else
            {
                --itrev;
            }
        }

        // Add some comp only players if necessary
        auto compOnlyPlayersToAdd = totalPlayersCnt - players.size();
        for(int i = 0; i < compOnlyPlayersToAdd; ++i)
        {
            CPlayerSettings pSettings;
            pSettings.setPlayerType(EPlayerType::COMP_ONLY);
            pSettings.setColor(getNextPlayerColor());
            players[pSettings.getColor()] = pSettings;
        }
    }
    if(compOnlyTeamsCnt == RANDOM_SIZE)
    {
        compOnlyTeamsCnt = gen.getInteger(0, std::max(compOnlyPlayersCnt - 1, 0));
    }

    // There should be at least 2 players (1-player-maps aren't allowed)
    if(playersCnt + compOnlyPlayersCnt < 2)
    {
        CPlayerSettings pSettings;
        pSettings.setPlayerType(EPlayerType::AI);
        pSettings.setColor(getNextPlayerColor());
        players[pSettings.getColor()] = pSettings;
        playersCnt = 2;
    }

    // 1 team isn't allowed
    if(teamsCnt == 1 && compOnlyPlayersCnt == 0)
    {
        teamsCnt = 0;
    }

    if(waterContent == EWaterContent::RANDOM)
    {
        waterContent = static_cast<EWaterContent::EWaterContent>(gen.getInteger(0, 2));
    }
    if(monsterStrength == EMonsterStrength::RANDOM)
    {
        monsterStrength = static_cast<EMonsterStrength::EMonsterStrength>(gen.getInteger(0, 2));
    }
}

int CMapGenOptions::countHumanPlayers() const
{
    return static_cast<int>(boost::count_if(players, [](const std::pair<PlayerColor, CPlayerSettings> & pair)
    {
        return pair.second.getPlayerType() == EPlayerType::HUMAN;
    }));
}

PlayerColor CMapGenOptions::getNextPlayerColor() const
{
    for(PlayerColor i = PlayerColor(0); i < PlayerColor::PLAYER_LIMIT; i.advance(1))
    {
        if(!players.count(i))
        {
            return i;
        }
    }
    throw std::runtime_error("Shouldn't happen. No free player color exists.");
}

CMapGenOptions::CPlayerSettings::CPlayerSettings() : color(0), startingTown(RANDOM_TOWN), playerType(EPlayerType::AI)
{

}

PlayerColor CMapGenOptions::CPlayerSettings::getColor() const
{
    return color;
}

void CMapGenOptions::CPlayerSettings::setColor(PlayerColor value)
{
    if(value >= PlayerColor(0) && value < PlayerColor::PLAYER_LIMIT)
    {
        color = value;
    }
    else
    {
        throw std::runtime_error("The color of the player is not in a valid range.");
    }
}

si32 CMapGenOptions::CPlayerSettings::getStartingTown() const
{
    return startingTown;
}

void CMapGenOptions::CPlayerSettings::setStartingTown(si32 value)
{
    if(value >= -1 && value < static_cast<int>(VLC->townh->towns.size()))
    {
        startingTown = value;
    }
    else
    {
        throw std::runtime_error("The starting town of the player is not in a valid range.");
    }
}

EPlayerType::EPlayerType CMapGenOptions::CPlayerSettings::getPlayerType() const
{
    return playerType;
}

void CMapGenOptions::CPlayerSettings::setPlayerType(EPlayerType::EPlayerType value)
{
    playerType = value;
}