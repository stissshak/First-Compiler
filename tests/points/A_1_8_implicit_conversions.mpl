// Task A.1.8 — Implicit conversions (the cast matrix, silent cases).
// int<->float, int<->char, int<->bool, char<->bool, the integer family
// (short/long/uint via the int row), and void* <-> T*.

int main(){
    // int -> float (exact) and float -> int (truncate toward zero)
    float f = 3;          // int literal into float
    int   t = 3.9;        // float into int -> 3
    int   tn = -3.9;      // toward zero -> -3
    if((((int)f)==3) & (t==3) & (tn==-3)) print("conv.intfloat: PASS\n");
    else print("conv.intfloat: FAIL (f=%f t=%d tn=%d)\n", f, t, tn);

    // int <-> char
    char c = 65;          // -> 'A'
    int  ci = c;          // 'A' -> 65
    if((c=='A') & (ci==65)) print("conv.intchar:  PASS\n");
    else print("conv.intchar:  FAIL\n");

    // int <-> bool and char <-> bool
    bool b1 = 5;          // nonzero -> true
    bool b0 = 0;          // -> false
    int  bi = b1;         // true -> 1
    bool bc = 'A';        // char -> bool (nonzero)
    if((b1) & (!b0) & (bi==1) & (bc)) print("conv.bool:     PASS\n");
    else print("conv.bool:     FAIL\n");

    // integer family widening/narrowing, all implicit
    long  L = 1000;       // int -> long
    short s = L;          // long -> short (fits)
    int   back = s;       // short -> int
    uint  u = back;       // int -> uint
    if((back==1000) & (((int)u)==1000)) print("conv.intfamily: PASS\n");
    else print("conv.intfamily: FAIL (back=%d)\n", back);

    // void* <-> T*: malloc returns void*, assigned to int* with no cast
    int *p = malloc(sizeof(int));
    *p = 1234;
    void *raw = p;        // T* -> void*
    int *q = raw;         // void* -> T*
    int got = *q;
    free(p);
    if(got==1234) print("conv.voidptr:  PASS\n");
    else          print("conv.voidptr:  FAIL (%d)\n", got);
    return 0;
}
