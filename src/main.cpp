#include "main.h"
#include <dllentry.h>

void dllenter() {}
void dllexit() {}
void PreInit() {
	Mod::CommandSupport::GetInstance().AddListener(SIG("loaded"), &CopyItemUtils::initializeCopyItemCommands);
}
void PostInit() {}

void CopyItemUtils::initializeCopyItemCommands(CommandRegistry *registry) {
	CopyInventoryCommand::setup(registry);
	CopyMainhandItemCommand::setup(registry);
}

std::string CopyInventoryCommand::containerTypeToString(DestinationContainerType k) {
	switch (k) {
		case DestinationContainerType::Inventory: return std::string("inventory");
		case DestinationContainerType::Enderchest: return std::string("ender chest");
		default: return std::string("unknown");
	}
}

void CopyInventoryCommand::copyFullInventory(Player &sourcePlayer, Player &recipientPlayer, CommandOutput &output) {

	switch (this->type) {
		case DestinationContainerType::Inventory: {

			const auto& sourceInventory = sourcePlayer.getRawInventory();
			auto& recipientInventory = recipientPlayer.getRawInventory();

			const int32_t PLAYER_ARMOR_SLOT_COUNT = 4;
			for (int32_t i = 0; i < PLAYER_ARMOR_SLOT_COUNT; i++) {
				ItemStack armorCopy(sourcePlayer.getArmor((ArmorSlot)i));
				recipientPlayer.setArmor((ArmorSlot)i, armorCopy);
			}

			ItemStack offhandCopy(sourcePlayer.getOffhandSlot());
			recipientPlayer.setOffhandSlot(offhandCopy);

			for (int32_t i = 0; i < recipientInventory.getContainerSize(); i++) {
				ItemStack inventoryCopy(sourceInventory.getItem(i));
				recipientInventory.setItem(i, inventoryCopy);
			}

			ItemStack UIItemCopy(sourcePlayer.getPlayerUIItem());
			recipientPlayer.setPlayerUIItem(PlayerUISlot::CursorSelected, UIItemCopy);

			recipientPlayer.sendInventory(false);
			break;
		}

		case DestinationContainerType::Enderchest: {

			auto sourceEnderChest = sourcePlayer.getEnderChestContainer();
			if (!sourceEnderChest) break;
			auto recipientEnderChest = recipientPlayer.getEnderChestContainer();
			if (!recipientEnderChest) break;

			for (int32_t i = 0; i < recipientEnderChest->getContainerSize(); i++) {
				ItemStack enderChestCopy(sourceEnderChest->getItem(i));
				recipientEnderChest->setItem(i, enderChestCopy);
			}
			break;
		}
		default: break;
	}
}

void CopyInventoryCommand::execute(const CommandOrigin &origin, CommandOutput &output) {

	auto selectedSources = this->source.results(origin);
	auto selectedRecipients = this->recipient.results(origin);

	if (selectedSources.empty()) {
		output.error("No targets matched source selector");
		return;
	}
	if (selectedRecipients.empty()) {
		output.error("No targets matched recipient selector");
		return;
	}
	if (selectedSources.count() > 1) {
		output.error("Too many targets matched source selector");
		return;
	}

	auto sourcePlayer = *selectedSources.begin();

	for (auto tempRecipient : selectedRecipients) {
		if (sourcePlayer != tempRecipient) {
			this->copyFullInventory(*sourcePlayer, *tempRecipient, output);
		}
	}

	int32_t resultCount = selectedRecipients.count();
	std::string successStr = "Successfully copied the " + this->containerTypeToString(this->type) + " contents from " +
		sourcePlayer->mPlayerName + " to " + std::to_string(resultCount) + std::string((resultCount == 1) ? " player" : " players");
	output.success(successStr);
}

void CopyInventoryCommand::setup(CommandRegistry *registry) {
	using namespace commands;

	registry->registerCommand("copyinventory", "Copies a player's full inventory or ender chest contents.",
		CommandPermissionLevel::GameMasters, CommandFlagUsage, CommandFlagNone);

	addEnum<DestinationContainerType>(registry, "containerType", {
		{ "inventory", DestinationContainerType::Inventory },
		{ "enderchest", DestinationContainerType::Enderchest }
	});

	registry->registerOverload<CopyInventoryCommand>("copyinventory",
		mandatory(&CopyInventoryCommand::source, "source"),
		mandatory(&CopyInventoryCommand::recipient, "recipient"),
		optional<CommandParameterDataType::ENUM>(&CopyInventoryCommand::type, "destination", "containerType")
	);
};

void CopyMainhandItemCommand::copyMainhandItem(Player &sourcePlayer, Player &recipientPlayer, CommandOutput &output) {

	ItemStack sourceItemCopy(sourcePlayer.getSelectedItem());

	if (recipientPlayer.add(sourceItemCopy)) {
		recipientPlayer.sendInventory(false);
	}
	else if (this->dropItemIfNeeded) {
		recipientPlayer.drop(sourceItemCopy, false);
	}
}

void CopyMainhandItemCommand::execute(const CommandOrigin &origin, CommandOutput &output) {

	auto selectedSources = this->source.results(origin);
	auto selectedRecipients = this->recipient.results(origin);

	if (selectedSources.empty()) {
		output.error("No targets matched source selector");
		return;
	}
	if (selectedRecipients.empty()) {
		output.error("No targets matched recipient selector");
		return;
	}
	if (selectedSources.count() > 1) {
		output.error("Too many targets matched source selector");
		return;
	}

	auto sourcePlayer = *selectedSources.begin();

	if (!sourcePlayer->getSelectedItem()) {
		output.error("Invalid item in source selector mainhand");
		return;
	}

	// intentionally allow players to copy their own item and give to themselves, why not?
	for (auto tempRecipient : selectedRecipients) {
		this->copyMainhandItem(*sourcePlayer, *tempRecipient, output);
	}

	int32_t resultCount = selectedRecipients.count();
	std::string successStr = "Successfully copied the mainhand contents from " + sourcePlayer->mPlayerName +
		" to " + std::to_string(resultCount) + std::string((resultCount == 1) ? " player" : " players");
	output.success(successStr);
}

void CopyMainhandItemCommand::setup(CommandRegistry *registry) {
	using namespace commands;

	registry->registerCommand("copymainhanditem", "Copies a player's mainhand item.",
		CommandPermissionLevel::GameMasters, CommandFlagUsage, CommandFlagNone);

	registry->registerOverload<CopyMainhandItemCommand>("copymainhanditem",
		mandatory(&CopyMainhandItemCommand::source, "source"),
		mandatory(&CopyMainhandItemCommand::recipient, "recipient"),
		optional(&CopyMainhandItemCommand::dropItemIfNeeded, "dropItemIfNeeded")
	);
};