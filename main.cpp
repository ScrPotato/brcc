#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <cctype>
#include <iomanip>
#include <vector>

std::string sanitize_filename(const std::string &file_name) {
    std::string sanitized;
    for (char c : file_name) {
        if (std::isalnum(c) || c == '_') {
            sanitized += c;
        } else {
            sanitized += '_';
        }
    }
    return sanitized;
}

void process_file(const std::filesystem::path &file_path, std::ofstream &output_file, std::vector<std::string> &file_registry) {
    std::ifstream input_file(file_path, std::ios::binary);
    if (!input_file.is_open()) {
        std::cerr << "Error opening file: " << file_path << std::endl;
        return;
    }

    std::string original_name = file_path.filename().string();
    file_registry.push_back(original_name);

    output_file << "const unsigned char " << sanitize_filename(original_name) << "[] = {";

    char byte;
    size_t count = 0;
    while (input_file.read(&byte, 1)) {
        if (count > 0) {
            output_file << ", ";
        }
        if (count % 16 == 0) {
            output_file << "\n    ";
        }
        output_file << "0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (static_cast<unsigned char>(byte) & 0xFF);
        ++count;
    }

    output_file << "\n};\n";
    output_file << "const size_t " << sanitize_filename(original_name) << "_SIZE = " << std::dec << count << ";\n\n";
}

void process_directory(const std::filesystem::path &dir_path, std::ofstream &output_file, std::vector<std::string> &file_registry) {
    for (const auto &entry : std::filesystem::recursive_directory_iterator(dir_path)) {
        if (entry.is_regular_file()) {
            process_file(entry.path(), output_file, file_registry);
        }
    }
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

    output_file << "#include <cstddef>\n#include <cstring>\n\n";
    std::string filename = output_file_path.replace_extension("").string();
    std::transform(filename.begin(), filename.end(), filename.begin(), ::toupper);
    output_file << "namespace BRESOURCECC_" << filename << " {" <<  "\n\n";

    std::vector<std::string> file_registry;
    process_directory(folder_path, output_file, file_registry);

    output_file << "const char* FILE_NAMES[] = {\n";
    for (const auto &name : file_registry) {
        output_file << "    \"" << name << "\",\n";
    }
    output_file << "};\n\n";

    output_file << "const unsigned char* FILE_DATA[] = {\n";
    for (const auto &name : file_registry) {
        output_file << "    " << sanitize_filename(name) << ",\n";
    }
    output_file << "};\n\n";

    output_file << "const size_t FILE_SIZES[] = {\n";
    for (const auto &name : file_registry) {
        output_file << "    " << sanitize_filename(name) << "_SIZE,\n";
    }
    output_file << "};\n\n";

    output_file << "const size_t FILE_COUNT = " << std::dec << file_registry.size() << ";\n\n";

    std::string code = R"(
const unsigned char* getFile(const char* filename, size_t* size_out) {
    for (size_t i = 0; i < FILE_COUNT; ++i) {
        const char* file_name = FILE_NAMES[i];
        size_t j = 0;
        while (filename[j] == file_name[j] && filename[j] != '\0' && file_name[j] != '\0') {
            ++j;
        }
        if (filename[j] == '\0' && file_name[j] == '\0') {
            if (size_out) {
                *size_out = FILE_SIZES[i];
            }
            return FILE_DATA[i];
        }
    }
    return NULL;
}

)";
    output_file << code;

    output_file << "} // namespace BRESOURCECC_" << filename << "\n";

    return 0;
}
