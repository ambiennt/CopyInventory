#pragma once

#include <hook.h>
#include <base/base.h>
#include <base/log.h>
#include <base/playerdb.h>
#include <mods/CommandSupport.h>
#include <Actor/Player.h>
#include <Container/PlayerInventory.h>
#include <Container/Container.h>

class CopyInventoryCommand : public Command {
public:

	enum class DestinationContainerType : int8_t {
		Inventory = 0x0,
		Enderchest = 0x1,
	};

	CommandSelector<Player> source, recipient;
	DestinationContainerType type;

	CopyInventoryCommand() : type(DestinationContainerType::Inventory) {
		this->source.setIncludeDeadPlayers(true);
		this->recipient.setIncludeDeadPlayers(true);

		// this works but automatically chooses the first element in the result vector. we want to be able to dynamically
		// respond to result counts, so we can throw unique errors for each type of user error
		//this->source.setResultCount(1);
		//this->recipient.setResultCount(1);
	}

	virtual void execute(const CommandOrigin &origin, CommandOutput &output) override;
	static void setup(CommandRegistry *registry);
	std::string containerTypeToString(DestinationContainerType k);
	void copyFullInventory(Player &sourcePlayer, Player &recipientPlayer, CommandOutput &output);
};

class CopyMainhandItemCommand : public Command {
public:
	CommandSelector<Player> source, recipient;
	bool dropItemIfNeeded;

	CopyMainhandItemCommand() : dropItemIfNeeded(true) {
		this->source.setIncludeDeadPlayers(true);
		this->recipient.setIncludeDeadPlayers(true);
	}

	virtual void execute(const CommandOrigin &origin, CommandOutput &output) override;
	static void setup(CommandRegistry *registry);
	void copyMainhandItem(Player &sourcePlayer, Player &recipientPlayer, CommandOutput &output);
};

namespace CopyItemUtils {

void initializeCopyItemCommands(CommandRegistry *registry);

} // namespace CopyItemUtils

DEF_LOGGER("CopyInventoryCommand");