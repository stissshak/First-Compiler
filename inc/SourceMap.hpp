// inc/SourceMap.hpp

#pragma once

#include <string>
#include <vector>
#include <algorithm>

struct SourceLoc{
    std::string file_name;
    std::size_t line, col;
};

struct SourceFragment{
    std::size_t buffer_offset;
    std::string file_name;
    std::size_t file_line;
};

struct SourceMap{
public:
    void add(std::size_t offset, std::string file_name, std::size_t line){ fragments.push_back(SourceFragment{offset, std::move(file_name), line});}

    SourceLoc resolve(std::size_t offset, const std::string& buffer){
        auto it = std::upper_bound(fragments.begin(), fragments.end(), offset, [](std::size_t o, const SourceFragment& sf) { return o < sf.buffer_offset; });
        if(it == fragments.begin()) return SourceLoc{"unknown", 1, 1};
        --it;
        std::size_t line = it->file_line;
        std::size_t col = 1;
        for(std::size_t i = it->buffer_offset; i < offset; ++i){
            if(buffer[i] == '\n') {
                ++line;
                col = 1;
            }
            else{
                ++col;
            }
        }
    }
private:
    std::vector<SourceFragment> fragments;
};





