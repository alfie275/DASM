// Driver program to assemble code

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>

#include "DAsm.h"

int assemble(const char *infile, const char *outfile); // Takes char* for easy passing of arguments from argv
bool getFile(std::string filename, std::stringstream &buffer);
std::string changeOrAddFileExtension(std::string file, std::string new_extension);

int main(int argc, char** argv) {

    const char *infile, *outfile;

    if(argc < 2) {
        std::cerr << "Usage: " << argv[0] << " input.asm [output.bin]" << std::endl;
        return 1;
    } else if(argc < 3) {
        infile = argv[1];
        outfile = changeOrAddFileExtension(argv[1], "bin").c_str();
        std::cout << "Outputing to " << outfile << std::endl;
    } else {
        infile = argv[1];
        outfile = argv[2];
    }

    return assemble(infile, outfile);

}

/**
* Load a file, processing any .include or #include directives.
* Returns whether the load was succesful
*/
bool getFile(std::string filename, std::stringstream &buffer) {

    // Open the file
    std::ifstream file(filename);

    if(!file.good()) {
        // File does not exist
        std::cerr << "Could not open file: " << filename << std::endl;
        return false; // Exit with failure
    }

    std::cerr << "Loading " << filename << std::endl;

    size_t line_number = 0;

    std::string line;
    while(std::getline(file, line)){

        line_number++;

        size_t offset = 0;
        while(offset < line.size() && ::isspace(line[offset])) {
            // Skip any leading spaces
            offset++;
        }
        // This is the start of the first word.
        size_t word_start = offset;

        while(offset < line.size() && !::isspace(line[offset])) {
            // Skip anything that isn't a space
            offset++;
        }
        // This is the end of the first word.
        size_t word_end = offset;

        if(word_start < line.size() && word_end < line.size()) {

            // We found a first word. Clip it out.
            std::string first_str = line.substr(word_start, word_end - word_start);
            std::string rest_str = line.substr(word_end);

            // Capitalize the first part
            std::transform(first_str.begin(), first_str.end(), first_str.begin(), ::toupper);

            if(first_str == "INCLUDE" || first_str == ".INCLUDE" || first_str == "#INCLUDE"){
                // Handle an include

                // Trim whitespace, quotes, and brackets from the filename
                std::string included_filename = rest_str;

                // Match file, "file", and <file>
                size_t start = included_filename.find_first_not_of(" \t'\"<");
                size_t end = included_filename.find_last_not_of(" \t'\">") + 1;

                if(start < included_filename.size() && end <= included_filename.size() && start <= end) {

                    // Trim out just the filename
                    included_filename = included_filename.substr(start, end - start);

                    // TODO: catch include loops

                    // Recurse
                    if(!getFile(included_filename, buffer)) {
                        // Something went wrong including, Exit with failure
                        return false;
                    }

                    // Don't take this line
                    continue;

                } else {
                    std::cerr << "Preprocessor error: " << filename << ": " << line_number << ": Incorrect include syntax" << std::endl;
                    std::cerr << line << std::endl;
                    return false; // Exit with failure
                }

            }
        }

        // If we get here it wasn't an include. Use the line.
        buffer << line << std::endl;
    }

    // Everything was ok, return true
    return true;
}

int assemble(const char *infile, const char *outfile) {
    // Load up the source code.
    std::stringstream buffer;

    if(getFile(infile, buffer)) {
        DAsm::Program lProgram;

        if(lProgram.LoadSource(buffer.str())){
            // It compiled.
            unsigned long pSize;
            //The parameters in this call are optional
            DAsm::word* lMemory = lProgram.ToBlock(DAsm::DCPU_RAM_SIZE, pSize);

            // Open the output file
            std::ofstream out(outfile);

            if(!out.good()) {
                // Cannot write to the output file
                std::cerr << "Cannot open file for writing: " << outfile << std::endl;
                delete lMemory;
                return 1;
            }

            // Dump the program to disk in big endian byte order
            for(int i = 0; i < pSize; i++){
                // Break into bytes
                unsigned char bytes[2];
                bytes[0] = lMemory[i] >> 8;
                bytes[1] = lMemory[i] & 0xFF;

                // Write the bytes
                out.write((char*) bytes, 2);
            }
            std::cout << "Wrote "<< pSize << " words to " << outfile << std::endl;
            delete lMemory;
        }else{
            for(auto&& error : lProgram.mErrors){
                std::cerr << "Error: " << error << std::endl;
            }
            return 1;
        }

    } else {
        // Something went wrong during preprocessing - an error, or non-existing file
        // Error message already given so just exit with failure
        return 1;
    }

    return 0;
}

std::string changeOrAddFileExtension(std::string file, std::string new_extension) {

    std::string dir, filename, rawname;
    std::stringstream output;

    // Seperate the file into the directory and filename
    std::regex re("(^.*(?:\\\\|\\/)|^)(.*)");
    std::smatch match;
    std::regex_search(file, match, re);
    dir = match.str(1);
    filename = match.str(2);

    // Remove file extension from filename if possible and store in rawname
    size_t lastindex = filename.find_last_of(".");
    if(lastindex != std::string::npos) {
        rawname = filename.substr(0, lastindex);
    } else {
        rawname = filename;
    }

    // Add new extension on to rawname
    output << dir << rawname << "." << new_extension;

    return output.str();
}
