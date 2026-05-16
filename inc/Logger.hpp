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

class ConsoleLogger : public Logger{
public:
    void log(Level l, std::string_view msg) override{
        if(l < minLevel){
            std::cerr << "\x1B[38;5;2m[INFO]\x1B[0m ";
            std::cerr << msg << std::endl;
        }
        else if(l < minLevel){
            std::cerr << "\x1B[38;5;3m[WARNING]\x1B[0m ";
            std::cerr << msg << std::endl;
        }
        else if(l == Level::Error){
            std::cerr << "\x1B[38;5;1m[ERROR]\x1B[0m ";
            std::cerr << msg << std::endl;
        }
        
    }
    
    
};

class FileLogger : public Logger{
public:
    FileLogger(const std::string& file_name){
        if(!file.is_open()) throw std::runtime_error("cannot open log file: " + file_name);
    }
    void log(Level l, std::string_view msg) override{
        if(l < minLevel){
            file << "[INFO]";
            file << msg << std::endl;
        }
        else if(l < minLevel){
            file << "[WARNING]";
            file << msg << std::endl;
        }
        else if(l == Level::Error){
            file << "[ERROR]";
            file << msg << std::endl;
        }
    }
    
private:
    std::ofstream file;
};

