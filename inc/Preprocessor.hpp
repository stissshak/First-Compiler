// MPL/inc/Preprocessor.hpp

#pragma once

#include <iostream>
#include <string>
#include <unordered_set>
#include <filesystem>

#include "Input.hpp"

class Preprocessor{
public:
    std::string include_files(const std::string &path){
        auto canon = std::filesystem::weakly_canonical(path).string();
        if (!included.insert(canon).second) return "";

        InputBuffer b;
        b.load_file(path);
        std::string content = b.mutable_buffer();
        std::string res;

        std::size_t last_pos = 0;
        std::size_t search_from = 0;
        
        while(true){
            std::size_t pos = content.find("#include \"", last_pos);
            if(pos == std::string::npos) break;

            if(!at_line_start(content, pos)){
                search_from = pos + 1;
                continue;
            }

            res.append(content, last_pos, pos - last_pos);
            
            std::size_t start_path = pos + sizeof("#include \"") - 1;
            std::size_t end_path = content.find('"', start_path);
            if(end_path == std::string::npos) break;

            std::string include_path = content.substr(start_path, end_path - start_path);
            res.append(include_files(include_path));

            last_pos = end_path + 1;
            search_from = last_pos;
        }

        res.append(content, last_pos, std::string::npos);
        return res;
    }


private:
    static bool at_line_start(const std::string& s, std::size_t pos){
        while (pos > 0){
            --pos;
            char c = s[pos];
            if (c == '\n') return true;
            if (c != ' ' && c != '\t') return false;
        }
        return true;
    }

    std::unordered_set<std::string> included;
};
