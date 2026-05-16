#include <Input.hpp>

#include <fstream>
#include <iostream>

void InputBuffer::load_file(const std::string &path){
    file_name = path;
    std::ifstream fs{path};
    if(!fs.is_open()){
        std::cerr << "failed to open file " << path << std::endl;
        throw std::runtime_error("failed to open:" + path);
    }

    buffer = std::string(std::istreambuf_iterator<char>(fs), std::istreambuf_iterator<char>());
}
