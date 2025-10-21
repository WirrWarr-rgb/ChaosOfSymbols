#include "TileTypeManager.h"
#include "Logger.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <regex>

/// <summary>
/// Самописный парсер JSON
/// </summary>
class SimpleJsonParser {
public:
    /// <summary>
    /// Удаление лишних пробелов и переносов из JSON
    /// </summary>
    static std::string CleanJson(const std::string& json) {
        std::string result;
        bool inString = false;
        char prevChar = 0;

        for (char c : json) {
            if (c == '"' && prevChar != '\\') {
                inString = !inString;
            }

            if (!inString && (c == '\n' || c == '\r' || c == '\t')) {
                continue;
            }

            result += c;
            prevChar = c;
        }
        return result;
    }

    /// <summary>
    /// Парсинг JSON-массива на отдельные объекты
    /// </summary>
    static std::vector<std::string> ParseArray(const std::string& json) {
        std::vector<std::string> result;

        std::string cleanJson = CleanJson(json);
        Logger::Log("Cleaned JSON: " + cleanJson + "\n");

        if (cleanJson.empty() || cleanJson[0] != '[' || cleanJson[cleanJson.size() - 1] != ']') {
            Logger::Log("ERROR: JSON is not a valid array");
            return result;
        }

        std::string content = cleanJson.substr(1, cleanJson.size() - 2);

        int braceCount = 0;
        std::string currentObj;
        bool inString = false;
        char prevChar = 0;

        for (char c : content) {
            if (c == '"' && prevChar != '\\') {
                inString = !inString;
            }

            if (!inString) {
                if (c == '{') {
                    braceCount++;
                }
                else if (c == '}') {
                    braceCount--;
                }
            }

            currentObj += c;
            prevChar = c;

            if (!inString && braceCount == 0 && !currentObj.empty()) {
                if (currentObj[0] == ',') {
                    currentObj = currentObj.substr(1);
                }

                size_t start = currentObj.find_first_not_of(" \t");
                size_t end = currentObj.find_last_not_of(" \t");
                if (start != std::string::npos && end != std::string::npos) {
                    std::string trimmed = currentObj.substr(start, end - start + 1);
                    if (!trimmed.empty()) {
                        result.push_back(trimmed);
                        Logger::Log("Found object: " + trimmed);
                    }
                }

                currentObj.clear();
            }
        }

        Logger::Log("\nParsed " + std::to_string(result.size()) + " objects from JSON array");
        return result;
    }

    /// <summary>
    /// Извелечение значения из JSON-объектов с помощью регулярных выражений
    /// </summary>
    static std::string GetStringValue(const std::string& json, const std::string& key) {
        std::regex pattern("\"" + key + "\"\\s*:\\s*\"([^\"]*)\"");
        std::smatch match;
        if (std::regex_search(json, match, pattern) && match.size() > 1) {
            return match[1];
        }
        return "";
    }

    /// <summary>
    /// Извелечение значения из JSON-объектов с помощью регулярных выражений
    /// </summary>
    static int GetIntValue(const std::string& json, const std::string& key) {
        std::regex pattern("\"" + key + "\"\\s*:\\s*(-?\\d+)");
        std::smatch match;
        if (std::regex_search(json, match, pattern) && match.size() > 1) {
            return std::stoi(match[1]);
        }
        return 0;
    }

    /// <summary>
    /// Извелечение значения из JSON-объектов с помощью регулярных выражений
    /// </summary>
    static bool GetBoolValue(const std::string& json, const std::string& key) {
        std::regex pattern("\"" + key + "\"\\s*:\\s*(true|false)");
        std::smatch match;
        if (std::regex_search(json, match, pattern) && match.size() > 1) {
            return match[1] == "true";
        }
        return false;
    }
};

TileTypeManager::TileTypeManager(const std::string& filePath)
    : m_resourceFilePath(filePath) {}

void TileTypeManager::LoadDefaultTiles() {
    Logger::Log("Loading default tile types...");
    m_tileTypes.clear();

    RegisterTileType(TileType(0, "air", ' ', 0, true, false, 0));
    RegisterTileType(TileType(1, "grass", '.', 10, true, false, 0));
    RegisterTileType(TileType(2, "stone_wall", '#', 8, false, true, 0));
    RegisterTileType(TileType(3, "water", '~', 9, false, false, 0));
    RegisterTileType(TileType(4, "lava", '~', 4, true, false, 5));
    RegisterTileType(TileType(5, "tree", 'T', 2, false, true, 0));
    RegisterTileType(TileType(6, "sand", ',', 14, true, false, 0));
    RegisterTileType(TileType(7, "mountain", '^', 7, false, false, 0));

    Logger::Log("Loaded " + std::to_string(m_tileTypes.size()) + " default tile types");
}


bool TileTypeManager::LoadFromFile() {
    return LoadFromFile(m_resourceFilePath);
}

/// <summary>
/// Загрузка данных
/// </summary>
bool TileTypeManager::LoadFromFile(const std::string& filePath) {
    Logger::Log("=== LOADING TILE TYPES FROM: " + filePath + " ===");

    std::ifstream file(filePath);
    if (!file.is_open()) {
        Logger::Log("ERROR: Tile config not found: " + filePath);

        Logger::Log("Attempting to create default tile config...");
        if (SaveToFile(filePath)) {
            Logger::Log("Default tile config created successfully");
            file.open(filePath);
            if (!file.is_open()) {
                Logger::Log("ERROR: Still cannot open tile config after creation");
                LoadDefaultTiles();
                return true;
            }
        }
        else {
            Logger::Log("ERROR: Failed to create default tile config");
            LoadDefaultTiles();
            return true;
        }
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string jsonContent = buffer.str();
    file.close();

    Logger::Log("File opened successfully, size: " + std::to_string(jsonContent.size()) + " bytes");

    if (jsonContent.empty()) {
        Logger::Log("ERROR: JSON file is empty");
        LoadDefaultTiles();
        return true;
    }

    size_t start = jsonContent.find_first_not_of(" \t\n\r");
    size_t end = jsonContent.find_last_not_of(" \t\n\r");
    if (start != std::string::npos && end != std::string::npos) {
        jsonContent = jsonContent.substr(start, end - start + 1);
    }

    if (jsonContent[0] != '[' || jsonContent[jsonContent.size() - 1] != ']') {
        Logger::Log("ERROR: JSON content is not an array. Expected [...], but got: " + jsonContent);
        LoadDefaultTiles();
        return true;
    }

    auto tileObjects = SimpleJsonParser::ParseArray(jsonContent);
    Logger::Log("Found " + std::to_string(tileObjects.size()) + " tile objects in JSON\n");

    if (tileObjects.empty()) {
        Logger::Log("WARNING: No tile objects found in JSON, using defaults");
        LoadDefaultTiles();
        return true;
    }

    m_tileTypes.clear();

    for (const auto& tileJson : tileObjects) {
        int id = SimpleJsonParser::GetIntValue(tileJson, "id");
        std::string name = SimpleJsonParser::GetStringValue(tileJson, "name");
        std::string characterStr = SimpleJsonParser::GetStringValue(tileJson, "character");
        char character = characterStr.empty() ? '?' : characterStr[0];
        int color = SimpleJsonParser::GetIntValue(tileJson, "color");
        bool passable = SimpleJsonParser::GetBoolValue(tileJson, "isPassable");
        bool destructible = SimpleJsonParser::GetBoolValue(tileJson, "isDestructible");
        int damage = SimpleJsonParser::GetIntValue(tileJson, "damage");

        Logger::Log("Parsed tile - id: " + std::to_string(id) +
            ", name: " + name +
            ", character: '" + std::string(1, character) + "'" +
            ", color: " + std::to_string(color) +
            ", passable: " + (passable ? "true" : "false"));

        if (id >= 0 && !name.empty()) {
            RegisterTileType(TileType(id, name, character, color, passable, destructible, damage));
            Logger::Log("Successfully registered tile: " + name);
        }
        else {
            Logger::Log("WARNING: Invalid tile - id: " + std::to_string(id) + ", name: " + name);
        }
    }

    Logger::Log("\n=== TILE LOADING COMPLETED: " + std::to_string(m_tileTypes.size()) + " tiles loaded ===\n");

    Logger::Log("=== LOADED TILES SUMMARY ===\n");
    for (const auto& pair : m_tileTypes) {
        const TileType& tile = pair.second;
        Logger::Log("Tile " + std::to_string(tile.GetId()) + ": '" +
            std::string(1, tile.GetCharacter()) + "' - " + tile.GetName() +
            " (color: " + std::to_string(tile.GetColor()) + ")");
    }
    Logger::Log("\n=== END LOADED TILES SUMMARY ===\n");

    return true;
}

/// <summary>
/// Сохранение
/// </summary>
bool TileTypeManager::SaveToFile(const std::string& filePath) {
    std::string outputPath = filePath.empty() ? m_resourceFilePath : filePath;
    std::ofstream file(outputPath);

    if (!file.is_open()) {
        std::cout << "Failed to save tile config: " << outputPath << '\n';
        return false;
    }

    file << "[\n";
    bool first = true;

    for (const auto& pair : m_tileTypes) {
        const TileType& tile = pair.second;

        if (!first) file << ",\n";
        first = false;

        file << "  {\n";
        file << "    \"id\": " << tile.GetId() << ",\n";
        file << "    \"name\": \"" << tile.GetName() << "\",\n";
        file << "    \"character\": \"" << tile.GetCharacter() << "\",\n";
        file << "    \"color\": " << tile.GetColor() << ",\n";
        file << "    \"isPassable\": " << (tile.IsPassable() ? "true" : "false") << ",\n";
        file << "    \"isDestructible\": " << (tile.IsDestructible() ? "true" : "false") << ",\n";
        file << "    \"damage\": " << tile.GetDamage() << "\n";
        file << "  }";
    }

    file << "\n]";
    file.close();

    std::cout << "Saved " << m_tileTypes.size() << " tile types to: " << outputPath << '\n';
    return true;
}

TileType* TileTypeManager::GetTileType(int id) {
    auto it = m_tileTypes.find(id);
    if (it != m_tileTypes.end()) {
        return &it->second;
    }
    return nullptr;
}

void TileTypeManager::RegisterTileType(const TileType& tileType) {
    m_tileTypes[tileType.GetId()] = tileType;
}