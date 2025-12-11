#include "HelpSystem.h"
#include <algorithm>

HelpSystem& HelpSystem::GetInstance() {
    static HelpSystem instance;
    return instance;
}

void HelpSystem::AddHelpEntry(const std::string& itemId, const std::string& helpText) {
    m_helpEntries[itemId] = helpText;
}

std::string HelpSystem::GetHelpText(const std::string& itemId) const {
    auto it = m_helpEntries.find(itemId);
    if (it != m_helpEntries.end()) {
        return it->second;
    }

    std::string helpText = GetHelpForItem(itemId);
    if (helpText != "No help available for this item.") {
        return helpText;
    }

    return "No help available for this item.";
}

bool HelpSystem::HasHelpEntry(const std::string& itemId) const {
    return m_helpEntries.find(itemId) != m_helpEntries.end();
}

void HelpSystem::ClearAllEntries() {
    m_helpEntries.clear();
    m_currentItemId.clear();
    m_itemHelpMap.clear();
    m_buttonHelpMap.clear();
}

void HelpSystem::SetCurrentItem(const std::string& itemId) {
    m_currentItemId = itemId;
}

void HelpSystem::ResetCurrentItem() {
    m_currentItemId.clear();
}

void HelpSystem::RegisterWorldTabHelp() {
    AddHelpEntry("world_name", "World Name: The name of your game world. This will be displayed in the main menu and save files.");
    AddHelpEntry("world_width", "World Width: The horizontal size of your world in tiles. Range: 20-200 tiles.");
    AddHelpEntry("world_height", "World Height: The vertical size of your world in tiles. Range: 20-100 tiles.");
    AddHelpEntry("random_generation", "Random Generation: When enabled, world will be randomly generated. When disabled, you can specify a seed.");
    AddHelpEntry("seed", "Seed: A number that determines the random generation. Same seed = same world.");
    AddHelpEntry("noise_frequency", "Noise Frequency: Controls the 'roughness' of terrain. Higher values = more detailed terrain.");
    AddHelpEntry("neighbor_radius", "Neighbor Radius: The radius of the neighborhood check (e.g. 0 - 4 neighbors, 1 - 8 neighbors, 2 - 24 neighbors, 3 - 48 neighbors, etc).");

    m_itemHelpMap["World Name: "] = "world_name";
    m_itemHelpMap["Width: "] = "world_width";
    m_itemHelpMap["Height: "] = "world_height";
    m_itemHelpMap["Random Generation: "] = "random_generation";
    m_itemHelpMap["Seed: "] = "seed";
    m_itemHelpMap["Noise Frequency: "] = "noise_frequency";
    m_itemHelpMap["Neighbor Radius: "] = "neighbor_radius";
}

void HelpSystem::RegisterPlayerTabHelp() {
    AddHelpEntry("player_start_x", "Start X: The X coordinate where the player spawns in the world.");
    AddHelpEntry("player_start_y", "Start Y: The Y coordinate where the player spawns in the world.");
    AddHelpEntry("player_max_hp", "Max HP: The maximum health points the player can have.");
    AddHelpEntry("player_max_hunger", "Max Hunger: The maximum hunger level the player can have.");
    AddHelpEntry("enable_hp", "Enable HP: Toggle whether the player has a health system.");
    AddHelpEntry("enable_hunger", "Enable Hunger: Toggle whether the player has a hunger system.");

    m_itemHelpMap["Start X: "] = "player_start_x";
    m_itemHelpMap["Start Y: "] = "player_start_y";
    m_itemHelpMap["Max HP: "] = "player_max_hp";
    m_itemHelpMap["Max Hunger: "] = "player_max_hunger";
    m_itemHelpMap["Enable HP: "] = "enable_hp";
    m_itemHelpMap["Enable Hunger: "] = "enable_hunger";
}

void HelpSystem::RegisterTilesTabHelp() {
    AddHelpEntry("tile_symbol", "Symbol: The ASCII character that represents this tile in the game.");
    AddHelpEntry("tile_color", "Color: The color of the tile character (0-15).");
    AddHelpEntry("tile_name", "Name: A descriptive name for this tile type.");
    AddHelpEntry("tile_lowland_prob", "Lowland Probability: Chance for this tile to appear in lowland areas (0-100%).");
    AddHelpEntry("tile_plains_prob", "Plains Probability: Chance for this tile to appear in plain areas (0-100%).");
    AddHelpEntry("tile_mountain_prob", "Mountain Probability: Chance for this tile to appear in mountain areas (0-100%).");

    m_itemHelpMap["Symbol: "] = "tile_symbol";
    m_itemHelpMap["Color: "] = "tile_color";
    m_itemHelpMap["Name: "] = "tile_name";
    m_itemHelpMap["Lowland Probability: "] = "tile_lowland_prob";
    m_itemHelpMap["Plains Probability: "] = "tile_plains_prob";
    m_itemHelpMap["Mountain Probability: "] = "tile_mountain_prob";

    AddHelpEntry("tile_list", "Tile List: View and manage all available tile types.");
    AddHelpEntry("add_new_tile", "Add New Tile: Create a new custom tile type.");
    AddHelpEntry("edit_tile", "Edit: Modify the selected tile's properties.");
    AddHelpEntry("delete_tile", "Delete: Remove the selected tile from the list.");

    m_itemHelpMap["Tile List"] = "tile_list";
    m_itemHelpMap["+ Add New Tile"] = "add_new_tile";
    m_itemHelpMap["Edit Tile"] = "edit_tile";
    m_itemHelpMap["Delete Tile"] = "delete_tile";
}

void HelpSystem::RegisterCommonElementsHelp() {
    AddHelpEntry("button_create", "Create: Save current configuration and create the world.");
    AddHelpEntry("button_save_template", "Save Template: Save current configuration as a template for future use.");
    AddHelpEntry("button_back", "Back: Return to previous menu without saving.");
    AddHelpEntry("button_edit", "Edit: Modify the selected field or tile.");
    AddHelpEntry("button_delete", "Delete: Remove the selected tile.");
    AddHelpEntry("button_cancel", "Cancel: Cancel current action and return.");

    AddHelpEntry("tab_world", "World: Configure world size and generation parameters.");
    AddHelpEntry("tab_player", "Player: Configure player starting position and stats.");
    AddHelpEntry("tab_tiles", "Tiles: Create and edit tile types for world generation.");
    AddHelpEntry("tab_cellular", "Cellular Automaton: Configure cellular automaton rules.");
    AddHelpEntry("tab_food", "Food: Configure food spawn settings.");

    m_itemHelpMap["CREATE"] = "button_create";
    m_itemHelpMap["SAVE TEMPLATE"] = "button_save_template";
    m_itemHelpMap["BACK"] = "button_back";
    m_itemHelpMap["- Edit"] = "button_edit";
    m_itemHelpMap["- Delete"] = "button_delete";
    m_itemHelpMap["Cancel"] = "button_cancel";
    m_itemHelpMap["Save"] = "button_create";
    m_itemHelpMap["World"] = "tab_world";
    m_itemHelpMap["Player"] = "tab_player";
    m_itemHelpMap["Tiles"] = "tab_tiles";
    m_itemHelpMap["Cellular Automaton"] = "tab_cellular";
    m_itemHelpMap["Food"] = "tab_food";
}

void HelpSystem::RegisterButtonHelp(const std::string& buttonName, const std::string& helpId) {
    m_buttonHelpMap[buttonName] = helpId;
}

std::string HelpSystem::GetHelpForItem(const std::string& itemName) const {
    auto buttonIt = m_buttonHelpMap.find(itemName);
    if (buttonIt != m_buttonHelpMap.end()) {
        return GetHelpText(buttonIt->second);
    }

    auto itemIt = m_itemHelpMap.find(itemName);
    if (itemIt != m_itemHelpMap.end()) {
        return GetHelpText(itemIt->second);
    }

    for (const auto& pair : m_itemHelpMap) {
        if (!pair.first.empty() && itemName.find(pair.first) == 0) {
            return GetHelpText(pair.second);
        }
    }

    return "No help available for this item.";
}

void HelpSystem::RegisterEditorTabHelp() {
    AddHelpEntry("tab_cellular", "Cellular Automaton: Configure cellular automaton rules for terrain generation.");
    AddHelpEntry("tab_food", "Food: Configure food spawning settings and nutrition values.");

    AddHelpEntry("survival_rules", "Survival Rules: Determine which cells survive (e.g. (count['.'] >= 2 && count['.'] <= 5) || (count['.'] == 1 && count['+'] >= 6)).");
    AddHelpEntry("birth_rules", "Birth Rules: Determines where new cells are born (e.g. (count['.'] >= 2 && count['#'] <= 1)).");
    AddHelpEntry("death_rules", "Death Rules: Determines when cells die (e.g. (count['#'] >= 5) || (count['+'] <= 2)).");

    m_itemHelpMap["Survival Rules: "] = "survival_rules";
    m_itemHelpMap["Birth Rules: "] = "birth_rules";
    m_itemHelpMap["Death Rules: "] = "death_rules";
}

void HelpSystem::RegisterEditorButtonsHelp() {
    AddHelpEntry("button_save", "Save: Save current configuration.");
    AddHelpEntry("button_test", "Test: Test the current configuration.");
    AddHelpEntry("button_reset", "Reset: Reset to default values.");
    AddHelpEntry("button_export", "Export: Export configuration to file.");

    m_itemHelpMap["SAVE"] = "button_save";
    m_itemHelpMap["TEST"] = "button_test";
    m_itemHelpMap["RESET"] = "button_reset";
    m_itemHelpMap["EXPORT"] = "button_export";
}

void HelpSystem::RegisterMainMenuHelp() {
    AddHelpEntry("mainmenu_Play", "Play: Start a new game or load an existing save");
    AddHelpEntry("mainmenu_About the Game", "About the Game: View information about the game");
    AddHelpEntry("mainmenu_Exit", "Exit: Close the game");
    AddHelpEntry("mainmenu_Select save", "Select save: Choose a save slot to load");
    AddHelpEntry("mainmenu_Create a world template", "Create a world template: Create a reusable world template");
    AddHelpEntry("mainmenu_Back", "Back: Return to previous menu");

    m_itemHelpMap["Play"] = "mainmenu_Play";
    m_itemHelpMap["About the Game"] = "mainmenu_About the Game";
    m_itemHelpMap["Exit"] = "mainmenu_Exit";
    m_itemHelpMap["Select save"] = "mainmenu_Select save";
    m_itemHelpMap["Create a world template"] = "mainmenu_Create a world template";
    m_itemHelpMap["Back"] = "mainmenu_Back";

    RegisterButtonHelp("Play", "mainmenu_Play");
    RegisterButtonHelp("About the Game", "mainmenu_About the Game");
    RegisterButtonHelp("Exit", "mainmenu_Exit");
    RegisterButtonHelp("Select save", "mainmenu_Select save");
    RegisterButtonHelp("Create a world template", "mainmenu_Create a world template");
    RegisterButtonHelp("Back", "mainmenu_Back");
}

void HelpSystem::RegisterSaveMenuHelp() {
    for (int i = 1; i <= 5; i++) {
        std::string slotId = "savemenu_save_slot_" + std::to_string(i);
        AddHelpEntry(slotId, "Save slot " + std::to_string(i));
        m_itemHelpMap["save_slot_" + std::to_string(i)] = slotId;
    }

    AddHelpEntry("savemenu_Create", "Create: Create a new save in this slot");
    AddHelpEntry("savemenu_Play", "Play: Load and play this save");
    AddHelpEntry("savemenu_Delete", "Delete: Delete this save");
    AddHelpEntry("savemenu_Back", "Back: Return to main menu");

    m_itemHelpMap["Create"] = "savemenu_Create";
    m_itemHelpMap["Play"] = "savemenu_Play";
    m_itemHelpMap["Delete"] = "savemenu_Delete";
    m_itemHelpMap["Back"] = "savemenu_Back";

    m_itemHelpMap["savemenu_Create"] = "savemenu_Create";
    m_itemHelpMap["savemenu_Play"] = "savemenu_Play";
    m_itemHelpMap["savemenu_Delete"] = "savemenu_Delete";
    m_itemHelpMap["savemenu_Back"] = "savemenu_Back";
}

void HelpSystem::RegisterTemplateMenuHelp() {
    for (int i = 1; i <= 30; i++) {
        std::string slotId = "templatemenu_template_slot_" + std::to_string(i);
        AddHelpEntry(slotId, "Template slot " + std::to_string(i) + ": Store or load world templates");
        m_itemHelpMap["template_slot_" + std::to_string(i)] = slotId;
    }

    AddHelpEntry("templatemenu_Create", "Create a new world template in this slot");
    AddHelpEntry("templatemenu_Edit", "Edit the selected template");
    AddHelpEntry("templatemenu_Delete", "Delete the selected template");
    AddHelpEntry("templatemenu_Load", "Load and preview the selected template");

    m_itemHelpMap["Create"] = "templatemenu_Create";
    m_itemHelpMap["Edit"] = "templatemenu_Edit";
    m_itemHelpMap["Load"] = "templatemenu_Load";
    m_itemHelpMap["Back"] = "templatemenu_Back";


    m_itemHelpMap["templatemenu_Create"] = "templatemenu_Create";
    m_itemHelpMap["templatemenu_Edit"] = "templatemenu_Edit";
    m_itemHelpMap["templatemenu_Delete"] = "templatemenu_Delete";
    m_itemHelpMap["templatemenu_Load"] = "templatemenu_Load";
    m_itemHelpMap["templatemenu_Back"] = "templatemenu_Back";
}