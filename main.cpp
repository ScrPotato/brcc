#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>
#include <unordered_set>

std::string sanitize_filename(const std::string &file_name, std::unordered_set<std::string> &existing_names) {
    std::string sanitized;
    for (char c : file_name) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') {
            sanitized += c;
        } else {
            sanitized += '_';
        }
    }
    std::string unique_name = sanitized;
    while (existing_names.find(unique_name) != existing_names.end()) {
        unique_name = "_" + unique_name;
    }
    existing_names.insert(unique_name);
    return unique_name;
}

struct FileInfo {
    std::string relative_path;
    std::string sanitized_name;
    size_t size;
};

void process_file(const std::filesystem::path &file_path, std::ofstream &output_file, std::vector<FileInfo> &file_registry, const std::filesystem::path &base_path, std::unordered_set<std::string> &existing_names) {
    std::ifstream input_file(file_path, std::ios::binary);
    if (!input_file.is_open()) {
        std::cerr << "Error opening file: " << file_path << std::endl;
        return;
    }
    std::filesystem::path relative_path = std::filesystem::relative(file_path, base_path);
    std::string relative_path_str = relative_path.generic_string();
    FileInfo info;
    info.relative_path = relative_path_str;
    info.sanitized_name = sanitize_filename(relative_path_str, existing_names);
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(input_file)), std::istreambuf_iterator<char>());
    info.size = bytes.size();
    file_registry.push_back(info);
    output_file << "constexpr unsigned char " << info.sanitized_name << "[] = {";
    for (size_t i = 0; i < bytes.size(); ++i) {
        if (i > 0) {
            output_file << ", ";
        }
        if (i % 16 == 0) {
            output_file << "\n    ";
        }
        output_file << "0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[i]);
    }
    output_file << "\n};\n";
    output_file << "constexpr size_t " << info.sanitized_name << "_SIZE = " << std::dec << info.size << ";\n\n";
}

void process_directory(const std::filesystem::path &dir_path, std::ofstream &output_file, std::vector<FileInfo> &file_registry, std::unordered_set<std::string> &existing_names) {
    for (const auto &entry : std::filesystem::recursive_directory_iterator(dir_path)) {
        if (entry.is_regular_file()) {
            process_file(entry.path(), output_file, file_registry, dir_path, existing_names);
        }
    }
    std::sort(file_registry.begin(), file_registry.end(), [](const FileInfo &a, const FileInfo &b) {
        return a.relative_path < b.relative_path;
    });
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <folder_path> <output_file.cpp>\n";
        return 1;
    }
    std::filesystem::path folder_path = argv[1];
    std::filesystem::path output_file_path = argv[2];
    std::ofstream output_file(output_file_path);
    if (!output_file.is_open()) {
        std::cerr << "Error opening output file: " << output_file_path << std::endl;
        return 1;
    }
    output_file << "#pragma once\n#include <cstddef>\n\n";
    std::string filename = output_file_path.stem().string();
    std::transform(filename.begin(), filename.end(), filename.begin(), ::toupper);
    output_file << "namespace BRESOURCECC_" << filename << " {\n\n";
    std::vector<FileInfo> file_registry;
    std::unordered_set<std::string> existing_names;
    process_directory(folder_path, output_file, file_registry, existing_names);
    output_file << "constexpr const char* FILE_NAMES[] = {\n";
    for (const auto &file : file_registry) {
        output_file << "    \"" << file.relative_path << "\",\n";
    }
    output_file << "};\n\n";
    output_file << "constexpr const unsigned char* FILE_DATA[] = {\n";
    for (const auto &file : file_registry) {
        output_file << "    " << file.sanitized_name << ",\n";
    }
    output_file << "};\n\n";
    output_file << "constexpr const size_t FILE_SIZES[] = {\n";
    for (const auto &file : file_registry) {
        output_file << "    " << file.sanitized_name << "_SIZE,\n";
    }
    output_file << "};\n\n";
    output_file << "constexpr size_t FILE_COUNT = " << file_registry.size() << ";\n\n";
    output_file << "const unsigned char* getFile(const char* filename, size_t* size_out) {\n";
    output_file << "    size_t left = 0;\n    size_t right = FILE_COUNT;\n    while (left < right) {\n        size_t mid = left + (right - left) / 2;\n        size_t i = 0;\n        while (filename[i] != '\\0' && FILE_NAMES[mid][i] != '\\0' && filename[i] == FILE_NAMES[mid][i]) {\n            ++i;\n        }\n        if (filename[i] == '\\0' && FILE_NAMES[mid][i] == '\\0') {\n            if (size_out) {\n                *size_out = FILE_SIZES[mid];\n            }\n            return FILE_DATA[mid];\n        }\n        if (filename[i] < FILE_NAMES[mid][i]) {\n            right = mid;\n        } else {\n            left = mid + 1;\n        }\n    }\n    if (size_out) {\n        *size_out = 0;\n    }\n    return nullptr;\n}\n\n";
    output_file << "} // namespace BRESOURCECC_" << filename << "\n";
    return 0;
}
