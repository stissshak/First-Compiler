// Extreme: argument passing past the 6-register SysV boundary.
// Args 1-6 go in rdi,rsi,rdx,rcx,r8,r9; args 7+ come off the stack at
// positive rbp offsets. Also mixes int + float args (separate GP/SSE arg
// counters) and exercises varargs through print.

int sum10(int a, int b, int c, int d, int e,
          int f, int g, int h, int i, int j){
    return a + b + c + d + e + f + g + h + i + j;   // 4 of these live on the stack
}

// interleaved int/float params: GP and SSE arg registers advance independently
float mix(int a, float b, int c, float d, int e, float f, int g, float h){
    return ((float)(a + c + e + g)) + b + d + f + h;
}

int main(){
    int s = sum10(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
    if(s == 55) print("many_args.sum10: PASS (%d)\n", s);
    else        print("many_args.sum10: FAIL (%d, want 55)\n", s);

    // ints 1+3+5+7=16, floats 2+4+6+8=20 -> 36.0
    float m = mix(1, 2.0, 3, 4.0, 5, 6.0, 7, 8.0);
    if(((int)m) == 36) print("many_args.mix:   PASS (%f)\n", m);
    else               print("many_args.mix:   FAIL (%f, want 36)\n", m);

    // varargs stress: many mixed conversions in one printf
    print("many_args.varargs: %d %f %c %s %d\n", 42, 3.5, 'X', "ok", -7);
    return 0;
}
