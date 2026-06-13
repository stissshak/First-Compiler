// Extreme: int<->float coercion in every position.
// Exercises implicit conversions at init, assignment, compound-assign, binary
// operands, call args, and return — plus explicit (int)/(float) casts.

float half(int n){ return n / 2.0; }         // n coerced to float -> true divide (2.5)
int   trunc_(float f){ return f; }          // implicit float->int on return

int main(){
    float a = 0;            // int literal -> float init
    a = 7;                  // int -> float assign
    a += 3;                 // compound assign, int -> float (a = 10.0)

    float b = a + 5;        // float + int  (15.0)
    float c = 100 - b;      // int - float  (85.0)
    float d = b * 2 / 4;    // mixed chain  (7.5)

    int   e = a + b;        // float sum -> int  (25)
    float f = e + d;        // int + float       (32.5)

    float g = half(5);      // 5/2 with float result -> 2.5
    int   h = trunc_(9.9);  // -> 9

    // deep mixed expression, both classes interleaved
    float big = (1 + 2.0) * (3 - 1.5) + ((float)(10 / 4)) - 0.5;  // 3*1.5+2-0.5 = 6.0

    if(((int)a) == 10)    print("int_float_mix.compound: PASS (%f)\n", a);
    else                  print("int_float_mix.compound: FAIL (%f, want 10)\n", a);
    if(((int)(d*10))==75) print("int_float_mix.chain:    PASS (%f)\n", d);
    else                  print("int_float_mix.chain:    FAIL (%f, want 7.5)\n", d);
    if(e == 25)           print("int_float_mix.toint:    PASS (%d)\n", e);
    else                  print("int_float_mix.toint:    FAIL (%d, want 25)\n", e);
    if(((int)(f*10))==325)print("int_float_mix.f:        PASS (%f)\n", f);
    else                  print("int_float_mix.f:        FAIL (%f, want 32.5)\n", f);
    if(((int)(g*10))==25) print("int_float_mix.argret:   PASS (%f)\n", g);
    else                  print("int_float_mix.argret:   FAIL (%f, want 2.5)\n", g);
    if(h == 9)            print("int_float_mix.trunc:    PASS (%d)\n", h);
    else                  print("int_float_mix.trunc:    FAIL (%d, want 9)\n", h);
    if(((int)big) == 6)   print("int_float_mix.deep:     PASS (%f)\n", big);
    else                  print("int_float_mix.deep:     FAIL (%f, want 6)\n", big);
    return 0;
}
