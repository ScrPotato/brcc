#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <cctype>
#include <iomanip>

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

void process_file(const std::filesystem::path &file_path, std::ofstream &output_file) {
    std::ifstream input_file(file_path, std::ios::binary);
    if (!input_file.is_open()) {
        std::cerr << "Error opening file: " << file_path << std::endl;
        return;
    }

    std::string sanitized_name = sanitize_filename(file_path.filename().string());
    output_file << "const unsigned char " << sanitized_name << "[] = {";

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
    output_file << "const size_t " << sanitized_name << "_SIZE = " << count << ";\n\n";
}

void process_directory(const std::filesystem::path &dir_path, std::ofstream &output_file) {
    for (const auto &entry : std::filesystem::recursive_directory_iterator(dir_path)) {
        if (entry.is_regular_file()) {
            process_file(entry.path(), output_file);
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

    output_file << "#include <cstddef>\n\n";
    output_file << "namespace BRESOURCECC {\n\n";

    process_directory(folder_path, output_file);

    output_file << "} // namespace BRESOURCECC\n";

    return 0;
}
