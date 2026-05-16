// MPL/inc/Preprocessor.hpp

#pragma once

#include <iostream>
#include <string>
#include <unordered_set>
#include <filesystem>

#include "Input.hpp"
#include "SourceMap.hpp"

class Preprocessor{
public:
    std::string include_files(const std::string &path){
        std::string out;
        process(path, out);
        return out;
    }

    const SourceMap& get_source_map(){ return smap;}

private:

    std::unordered_set<std::string> included;
    SourceMap smap;

    void process(const std::string& path, std::string& out){
        auto canon = std::filesystem::weakly_canonical(path).string();
        if (!included.insert(canon).second) return;

        InputBuffer b;
        b.load_file(path);
        std::string content = b.mutable_buffer();

        smap.add(out.size(), path, 1);

        std::size_t last_pos = 0;
        std::size_t search_from = 0;
        
        while(true){
            std::size_t pos = content.find("#include \"", search_from);
            if(pos == std::string::npos) break;
            if(!at_line_start(content, pos)){
                search_from = pos + 1;
                continue;
            }

            out.append(content, last_pos, pos - last_pos);
            
            std::size_t start_path = pos + sizeof("#include \"") - 1;
            std::size_t end_path = content.find('"', start_path);
            if(end_path == std::string::npos) break;

            std::string include_path = content.substr(start_path, end_path - start_path);
            process(include_path, out);

            std::size_t line_here = count_newlines(content, end_path + 1) + 1;
            smap.add(out.size(), path, line_here);

            last_pos = end_path + 1;
            search_from = last_pos;
        }

        out.append(content, last_pos, std::string::npos);
    }

    std::size_t count_newlines(const std::string& s, std::size_t end){
        std::size_t n = 0;
        for(std::size_t i = 0; i < end && i < s.size(); ++i){
            if(s[i] == 'n') ++n;
        }
        return n;
    }

    static bool at_line_start(const std::string& s, std::size_t pos){
        while (pos > 0){
            --pos;
            char c = s[pos];
            if (c == '\n') return true;
            if (c != ' ' && c != '\t') return false;
        }
        return true;
    }


};
