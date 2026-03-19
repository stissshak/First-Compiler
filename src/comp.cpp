#include <iostream>
#include <fstream>
#include <string>

#include "Input.hpp"
#include "Lexer.hpp"

using namespace std;

int main(int argc, char *argv[]){
    InputBuffer ib;
    if(argc < 2) return 1;
    string name = argv[1];
    ib.load_file(name);
    string s = ib.get_buffer();
    cout << s;
    auto arr = Lexer(s).tokenize();
    for(auto i : arr){
        cout << i.data << endl;     
    }
}
