#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
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
    std::vector<std::future<size_tree>> futures;
    for (const auto& entry : std::filesystem::directory_iterator(path))
        futures.push_back(std::async(std::launch::async, [entry] {            
            try {
                if (entry.is_directory()) return iterate(entry.path());
                size_tree child = { entry.path(), entry.file_size(), {} };

                if (entry.is_symlink()) child.is_symlink = true;
                if (entry.is_regular_file() && (std::filesystem::status(child.path).permissions() & std::filesystem::perms::owner_exec) != std::filesystem::perms::none)
                    child.is_executable = true;
                std::string extension = child.path.extension().string();
                const std::unordered_set<std::string> image = {".jpg", ".jpeg", ".png", ".gif", ".bmp", ".tiff"};
                if (image.count(extension) > 0) child.is_graphical = true;
                const std::unordered_set<std::string> archive = {".zip", ".tar", ".tar.gz", ".tar.bz2", ".rar", ".7z"};
                if (archive.count(extension) > 0) child.is_archive = true;

                return child;
            } catch (const std::filesystem::filesystem_error& error) {
                size_tree child = { entry.path(), 0, {} };
                child.has_error = true;
                return child;
            }
        }));

    size_tree result = { path };

    for (auto& future : futures) {
        size_tree child = future.get();
        result.children.emplace_back(std::move(child));
        result.size += child.size;
    }

    result.is_directory = !result.children.empty();
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

inline std::string colorize(const std::string& text, std::vector<terminal_colors> options, bool reset = true) {
    std::string result = "\x1B[";
    for (auto option : options) result += ';' + std::to_string(option);
    result += 'm' + text;
    if (reset) result += "\x1B[0m";
    return result;
}

inline std::string format(long long bytes) {
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

void print(const size_tree& base, int depth, std::vector<std::string> ignore, bool compact, int indent = 0, std::vector<bool> last = {}) {
    if (std::find(ignore.begin(), ignore.end(), base.path.filename()) != ignore.end()) return;

    std::string prefix = indent > 0 ? std::string(2, ' ') : "";
    for (int i = 1; indent > i; i++) prefix += last[i - 1] ? "   " : (compact ? "│ " : "│  ");
    if (indent > 0) prefix += last[last.size() - 1] ? (compact ? "└ " : "└─ ") : (compact ? "├ " : "├─ ");

    std::vector<terminal_colors> path_color;
    if (base.is_directory) path_color = { BLUE_FG, BOLD_ON };
    else if (base.is_symlink) path_color = { CYAN_FG, BOLD_ON };
    else if (base.is_executable) path_color = { GREEN_FG, BOLD_ON };
    else if (base.is_graphical) path_color = { MAGENTA_FG, BOLD_ON };
    else if (base.is_archive) path_color = { RED_FG, BOLD_ON };
    else if (base.has_error) path_color = { RED_FG, BLACK_BG };
    else path_color = { RESET };

    std::cout << colorize(prefix, { RESET }) << colorize(base.path.filename().string() + (base.is_directory ? '/' : '\0'), path_color) <<
        colorize(" » " + format(base.size), { RESET } ) <<
        colorize(" (" + std::to_string(base.size) + " bytes)", { GREY_FG }) << std::endl;

    if (depth == 0 || !base.is_directory) return;

    size_t count = base.children.size();
    for (size_t i = 0; i < count; ++i) {
        auto current_last = last;
        current_last.push_back(i == count - 1);
        print(base.children[i], depth - 1, ignore, compact, indent + 1, current_last);
    }
}

int main(int argc, char** argv) {
    CLI::App app("General use directory size comparison and overview by @kubgus."); 

    std::string path = "./";
    app.add_option("-p,--path", path, "Specify the path to frisk. (defaults to current working directory)");

    int depth = -1;
    app.add_option("-d,--depth", depth, "Limit the frisk directory depth. (defaults to -1, meaning no limit)");
    
    std::string ignore = ".git,node_modules";
    app.add_option("-i,--ignore", ignore, "Specify a comma-separated list of file/directory names to ignore when printing out the result.");

    bool compact = false;
    app.add_flag("-c,--compact", compact, "Print the output in a more horizontally compact way.");

    CLI11_PARSE(app, argc, argv);

    std::istringstream ss(ignore);
    std::string token;
    char delimeter = ',';
    std::vector<std::string> tokens;
    while (std::getline(ss, token, delimeter)) tokens.push_back(token);

    print(iterate(path.c_str()), depth, tokens, compact);
}
