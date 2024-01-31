#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <filesystem>
#include <unordered_set>
#include <vector>

#include "vendor/CLI11.hpp"

struct size_tree {
    std::filesystem::path path;
    std::ifstream::pos_type size;
    std::vector<size_tree> children;
    
    bool is_directory = false;
    bool is_symlink = false;
    bool is_executable = false;
    bool is_graphical = false;
    bool is_archive = false;
    bool has_error = false;
};

size_tree iterate(std::filesystem::path path) {
    size_tree result = { path };
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        try {
            if (entry.is_directory()) {
                size_tree child = iterate(entry.path());
                result.children.push_back(child);
                result.size += child.size;
                continue;
            }
            size_tree child = { entry.path(), entry.file_size(), {} };

            std::filesystem::file_status status = std::filesystem::status(child.path);
            if (entry.is_symlink()) child.is_symlink = true;
            if (entry.is_regular_file() && (status.permissions() & std::filesystem::perms::owner_exec) != std::filesystem::perms::none)
                child.is_executable = true;
            const std::unordered_set<std::string> image = {".jpg", ".jpeg", ".png", ".gif", ".bmp", ".tiff"};
            if (image.count(child.path.extension().string()) > 0) child.is_graphical = true;
            const std::unordered_set<std::string> archive = {".zip", ".tar", ".tar.gz", ".tar.bz2", ".rar", ".7z"};
            if (archive.count(child.path.extension().string()) > 0) child.is_archive = true;

            result.children.push_back(child);
            result.size += entry.file_size();
        } catch (const std::filesystem::filesystem_error& error) {
            size_tree child = { entry.path(), 0, {} };
            child.has_error = true;
            result.children.push_back(child);
        }
    }
    result.is_directory = result.children.size() > 0;
    return result;
}

enum terminal_colors {
    BLACK_FG = 30, RED_FG, GREEN_FG, YELLOW_FG, BLUE_FG, MAGENTA_FG, CYAN_FG, WHITE_FG,
    BLACK_BG = 40, RED_BG, GREEN_BG, YELLOW_BG, BLUE_BG, MAGENTA_BG, CYAN_BG, WHITE_BG,
    BOLD_ON = 1, UNDERLINE_ON = 4, INVERSE_ON = 7,
    BOLD_OFF = 21, UNDERLINE_OFF = 24, INVERSE_OFF = 27,
    RESET = 0,
    GREY_FG = 90,
};

std::string colorize(const std::string& text, std::vector<terminal_colors> options, bool reset = true) {
    std::string result = "\x1B[";
    for (auto option : options) result += ';' + std::to_string(option);
    result += 'm' + text;
    if (reset) result += "\x1B[0m";
    return result;
}

std::string format(long long bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
    const int count = sizeof(units) / sizeof(units[0]);
    int index = 0;
    while (bytes >= 1024 && index < count - 1) {
        bytes /= 1024; index++;
    }
    std::ostringstream oss;
    oss << bytes << " " << units[index];
    return oss.str();
}

void print(const size_tree& base, std::vector<std::string> ignore, int indent = 0) {
    if (std::find(ignore.begin(), ignore.end(), base.path.filename()) != ignore.end()) return;
    std::vector<terminal_colors> path_color;
    if (base.is_directory) path_color = { BLUE_FG, BOLD_ON };
    else if (base.is_symlink) path_color = { CYAN_FG, BOLD_ON };
    else if (base.is_executable) path_color = { GREEN_FG, BOLD_ON };
    else if (base.is_graphical) path_color = { MAGENTA_FG, BOLD_ON };
    else if (base.is_archive) path_color = { RED_FG, BOLD_ON };
    else if (base.has_error) path_color = { RED_FG, BLACK_BG };
    else path_color = { RESET };
    std::cout << std::string(indent * 2, ' ') <<
        colorize(base.path.filename().string() + (base.is_directory ? '/' : '\0'), path_color) <<
        colorize(" - " + format(base.size), { RESET } ) <<
        colorize(" (" + std::to_string(base.size) + " bytes)", { GREY_FG }) <<
        std::endl;
    for (const size_tree& child : base.children)
        print(child, ignore, indent + 1);
}

int main(int argc, char** argv) {
    CLI::App app("General use directory size comparison and overview by @kubgus."); 

    std::string path = "./";
    app.add_option("-p,--path", path, "Path to desired directory.");

    std::string ignore = ".git,node_modules";
    app.add_option("-i,--ignore", ignore, "Paths to keep from displaying. (see docs)");

    CLI11_PARSE(app, argc, argv);

    std::istringstream ss(ignore);
    std::string token;
    char delimeter = ',';
    std::vector<std::string> tokens;
    while (std::getline(ss, token, delimeter)) tokens.push_back(token);

    print(iterate(path.c_str()), tokens);
}
