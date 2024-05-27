#include "Filesystem.h"
#include <iostream>
#include <fstream>

namespace Quasar {
void Filesystem::Write(const std::string& filename, const std::string& data) {
    std::ofstream outfile(filename, std::ios::out | std::ios::trunc);
    if (!outfile) {
        std::cerr << "Error: Could not open the file for writing." << std::endl;
        return;
    }
    outfile << data;
    outfile.close();
}

void Filesystem::Append(const std::string& filename, const std::string& data) {
    std::ofstream outfile(filename, std::ios::out | std::ios::app);
    if (!outfile) {
        std::cerr << "Error: Could not open the file for appending." << std::endl;
        return;
    }
    outfile << data;
    outfile.close();
}

std::string Filesystem::Read(const std::string& filename) {
    std::ifstream infile(filename, std::ios::in);
    if (!infile) {
        std::cerr << "Error: Could not open the file for reading." << std::endl;
        return "";
    }
    std::string data((std::istreambuf_iterator<char>(infile)), std::istreambuf_iterator<char>());
    infile.close();
    return data;
}

std::vector<char> Filesystem::ReadBinary(const std::string& filename) {
    std::ifstream infile(filename, std::ios::in | std::ios::binary);
    if (!infile) {
        std::cerr << "Error: Could not open the file for reading in binary mode." << std::endl;
        return {};
    }
    infile.seekg(0, std::ios::end);
    std::streamsize size = infile.tellg();
    infile.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (!infile.read(buffer.data(), size)) {
        std::cerr << "Error: Could not read the file data." << std::endl;
        return {};
    }
    infile.close();
    return buffer;
}

bool Filesystem::Delete(const std::string& filename) {
    if (std::remove(filename.c_str()) != 0) {
        std::cerr << "Error: Could not delete the file." << std::endl;
        return false;
    }
    return true;
}

bool Filesystem::Exists(const std::string& filename) {
    std::ifstream infile(filename);
    return infile.good();
}

}
