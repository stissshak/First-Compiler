#include <iostream>
#include <string_view>
#include <fstream>

class Logger{
public:
    enum class Level{
      Info,
      Warning,
      Error
    };
    
    virtual void log(Level l, std::string_view msg) = 0;
    
    void info(std::string_view msg){
        log(Level::Info, msg);
    }
    
    void warning(std::string_view msg){
        log(Level::Warning, msg);
    }
    
    void error(std::string_view msg){
        log(Level::Error, msg);
    }

    void set_min_level(Level l){
        minLevel = l;
    }

    virtual ~Logger() {}
    
protected:
    Level minLevel = Level::Info;
};

// msg already carries "file:line:col: error:", logger only filters
class ConsoleLogger : public Logger{
public:
    void log(Level l, std::string_view msg) override{
        if(l < minLevel) return;
        std::cerr << msg << std::endl;
    }
};

class FileLogger : public Logger{
public:
    FileLogger(const std::string& file_name) : file(file_name){
        if(!file.is_open()) throw std::runtime_error("cannot open log file: " + file_name);
    }
    void log(Level l, std::string_view msg) override{
        if(l < minLevel) return;
        file << msg << std::endl;
    }
    
private:
    std::ofstream file;
};

