// Task A.2.8 — Function overloading.
// Same name, different parameter signatures; the call resolves to the overload
// whose parameters match the argument types exactly (no implicit conversions
// used to choose between candidates).

int   f(int x)        { return x + 1; }       // overload by type ...
float f(float x)      { return x + 0.5; }     // ... int vs float
int   f(int a, int b) { return a + b; }       // ... and by arity

int describe(int *p)  { return 1; }           // overload by pointee type
int describe(char *s) { return 2; }

int only(int x) { return x; }                 // single candidate (no overload)

int main(){
    int   fi = f(10);       // -> int f(int)        = 11
    float ff = f(2.0);      // -> float f(float)    = 2.5
    int   f2 = f(3, 4);     // -> int f(int,int)    = 7
    if((fi==11) & (((int)(ff*10))==25) & (f2==7))
         print("overload.bytype:  PASS\n");
    else print("overload.bytype:  FAIL (fi=%d ff=%f f2=%d)\n", fi, ff, f2);

    int  arr[1]; arr[0] = 9;
    int *ip = arr;
    char *cp = "hi";
    if((describe(ip)==1) & (describe(cp)==2))
         print("overload.byptr:   PASS\n");
    else print("overload.byptr:   FAIL\n");

    // a single candidate still converts its arguments implicitly (char -> int)
    if(only('A') == 65) print("overload.single:  PASS\n");
    else                print("overload.single:  FAIL (%d)\n", only('A'));
    return 0;
}
