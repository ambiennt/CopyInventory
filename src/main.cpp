#include "main.h"
#include <dllentry.h>

class CopyInventoryCommand : public Command {
public:
    CopyInventoryCommand() {
        source.setIncludeDeadPlayers(true);
        recipient.setIncludeDeadPlayers(true);
        source.setResultCount(1);
        recipient.setResultCount(1);
    }

    CommandSelector<Player> source;
    CommandSelector<Player> recipient;
    enum class ContainerType { inventory, enderchest } type = ContainerType::inventory;

    constexpr const char* containerTypeToString(ContainerType k) {
        switch (k) {
            case ContainerType::inventory:  return "inventory";
            case ContainerType::enderchest: return "ender chest";
            default:                        return "unknown";
        }
    }

    void copy(Player *sourcePlayer, Player *recipientPlayer, CommandOutput &output) {

        switch (type) {
            case ContainerType::inventory:
                {
                    auto sourcePlayerInventory = sourcePlayer->mInventory->inventory.get();
                    auto recipientPlayerInventory = recipientPlayer->mInventory->inventory.get();

                    for (int i = 0; i < 4; i++) {
                        ItemStack armorCopy(sourcePlayer->getArmor((ArmorSlot) i));
                        recipientPlayer->setArmor((ArmorSlot) i, armorCopy);
                    }

                    ItemStack offhandCopy(*sourcePlayer->getOffhandSlot());
                    recipientPlayer->setOffhandSlot(offhandCopy);

                    for (int i = 0; i < recipientPlayerInventory->getContainerSize(); i++) {
                        ItemStack inventoryCopy(sourcePlayerInventory->getItem(i));
                        recipientPlayerInventory->setItem(i, inventoryCopy);
                    }

                    ItemStack UIItemCopy(sourcePlayer->getPlayerUIItem());
                    recipientPlayer->setPlayerUIItem(PlayerUISlot::CursorSelected, UIItemCopy);

                    recipientPlayer->sendInventory(false);
                }
                break;

            case ContainerType::enderchest:
                {
                    auto sourceEnderChest = sourcePlayer->getEnderChestContainer();
                    auto recipientEnderChest = recipientPlayer->getEnderChestContainer();

                    for (int i = 0; i < recipientEnderChest->getContainerSize(); i++) {
                        ItemStack enderChestCopy(sourceEnderChest->getItem(i));
                        recipientEnderChest->setItem(i, enderChestCopy);
                    }
                }
                break;

            default: break;
        }
    }

    void execute(CommandOrigin const &origin, CommandOutput &output) {

        auto selectedSources = source.results(origin);
        auto selectedRecipients = recipient.results(origin);

        if (selectedSources.empty() || selectedRecipients.empty()) {
            return output.error("No targets matched selector");
        }
        if (selectedSources.count() > 1) {
            return output.error("Too many targets matched source selector");
        }

        auto& sourcePlayer = *selectedSources.begin();

        for (auto tempRecipient : selectedRecipients) {
            if (sourcePlayer != tempRecipient) {
                copy(sourcePlayer, tempRecipient, output);
            }
        }

        int resultCount = selectedRecipients.count();
        std::string successStr = "Successfully copied the " + std::string(containerTypeToString(type)) + " contents from " + sourcePlayer->mPlayerName + " to " + std::to_string(resultCount) + std::string(resultCount == 1 ? " player" : " players");
        output.success(successStr);
    }

    static void setup(CommandRegistry *registry) {
        using namespace commands;
        registry->registerCommand(
            "copyinventory", "Copies a player's inventory or ender chest contents.", CommandPermissionLevel::GameMasters, CommandFlagNone, CommandFlagNone);

        commands::addEnum<ContainerType>(registry, "ContainerType", {
            { "inventory", ContainerType::inventory },
            { "enderchest", ContainerType::enderchest }
        });

        registry->registerOverload<CopyInventoryCommand>("copyinventory",
            mandatory(&CopyInventoryCommand::source, "source"),
            mandatory(&CopyInventoryCommand::recipient, "recipient"),
            optional<CommandParameterDataType::ENUM>(&CopyInventoryCommand::type, "type", "ContainerType")
        );
    }
};

void dllenter() {}
void dllexit() {}
void PreInit() {
    Mod::CommandSupport::GetInstance().AddListener(SIG("loaded"), &CopyInventoryCommand::setup);
}
void PostInit() {}