#include <iostream>
#include <fstream>
#include <string>

#include <Input.hpp>

using namespace std;

int main(int argc, char *argv[]){
   InputBuffer ib;
   string name = argv[1];
   ib.load_file(name);
   string s = ib.get_buffer();
   cout << s;
}
