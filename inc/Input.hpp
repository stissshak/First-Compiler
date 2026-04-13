// /inc/Input.hpp

#pragma once

#include <string>

class InputBuffer{
private:
    std::string buffer;
public:
    void load_file(const std::string &path);
    const std::string &get_buffer() const { return buffer; };
    std::string &steal_buffer() { return buffer;};
};
