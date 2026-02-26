#include <Input.hpp>

#include <fstream>
#include <iostream>

void InputBuffer::load_file(const std::string &path){
    std::ifstream fs{path};
    if(!fs.is_open()){
        std::cout << "failed to open file " << path << std::endl;
        throw 1;
    }

    buffer = std::string(std::istreambuf_iterator<char>(fs), std::istreambuf_iterator<char>());
    return;  
}
