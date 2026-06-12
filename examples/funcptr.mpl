// Function pointers: variables, parameters, indirect calls.

int add(int a, int b){ return a + b; }
int sub(int a, int b){ return a - b; }

int apply(int(int, int) op, int x, int y){
    return op(x, y);
}

int main(){
    int(int, int) f = add;
    print("add: %d\n", f(10, 5));
    f = sub;
    print("sub: %d\n", f(10, 5));
    print("apply: %d\n", apply(add, 2, 3));
    return 0;
}
