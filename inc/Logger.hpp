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
    void info(std::string_view msg);
    void warning(std::string_view msg);
    void error(std::string_view msg);

    void set_debug(bool state){
        isDebug = state;
    }

    virtual ~Logger() {}
    
protected:
    bool isDebug = true;
};

class ConsoleLogger : public Logger{
public:
    void log(Level l, std::string_view msg) override{
        if(isDebug == 1 && l == Level::Info){
            std::cout << "\x1B[38;5;2m[INFO]\x1B[0m ";
            std::cout << msg << std::endl;
        }
        else if(isDebug == 1 && l == Level::Warning){
            std::cout << "\x1B[38;5;3m[WARNING]\x1B[0m ";
            std::cout << msg << std::endl;
        }
        else if(l == Level::Error){
            std::cout << "\x1B[38;5;1m[ERROR]\x1B[0m ";
            std::cout << msg << std::endl;
        }
        
    }
    
    void info(std::string_view msg){
        log(Level::Info, msg);
    }
    
    void warning(std::string_view msg){
        log(Level::Warning, msg);
    }
    
    void error(std::string_view msg){
        log(Level::Error, msg);
    }
};

class FileLogger : public Logger{
public:
    FileLogger(std::string_view file_name){
        file.open(file_name.data());
    }
    void log(Level l, std::string_view msg) override{
        if(isDebug == 1 && l == Level::Info){
            file << "[INFO]";
            file << msg << std::endl;
        }
        else if(isDebug == 1 && l == Level::Warning){
            file << "[WARNING]";
            file << msg << std::endl;
        }
        else if(l == Level::Error){
            file << "[ERROR]";
            file << msg << std::endl;
        }
    }
    
    void info(std::string_view msg){
        log(Level::Info, msg);
    }
    
    void warning(std::string_view msg){
        log(Level::Warning, msg);
    }
    
    void error(std::string_view msg){
        log(Level::Error, msg);
    }
private:
    std::ofstream file;
};

