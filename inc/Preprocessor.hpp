// MPL/inc/Preprocessor.hpp

#pragma once

#include <iostream>
#include <string>

#include "Input.hpp"

class Preprocessor{
public:
    std::string include_files(const std::string &path){
        InputBuffer b;
        b.load_file(path);
        std::string content = b.steal_buffer();
        std::string res;

        std::size_t last_pos = 0;
        std::size_t pos = 0;
        
        while((pos = content.find("#include \"", last_pos)) != std::string::npos){
            res.append(content, last_pos, pos - last_pos);
            
            std::size_t start_path = pos + 10;
            std::size_t end_path = content.find("\"", start_path);
            
            if(end_path == std::string::npos) break;

            std::string include_path = content.substr(start_path, end_path - start_path);

            res.append(include_files(include_path));

            last_pos = end_path + 1;
        }

        res.append(content, last_pos, std::string::npos);
        return res;
    }


private:

};
