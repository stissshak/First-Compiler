// Extreme: deeply nested control flow.
// Many nested if/while/for layers and long jump chains — stresses label
// generation and branch emission.

int main(){
    // 12-deep nested if (every condition true) sets a flag at the bottom
    int hit = 0;
    int x = 12;
    if(x > 0){ if(x > 1){ if(x > 2){ if(x > 3){ if(x > 4){ if(x > 5){
    if(x > 6){ if(x > 7){ if(x > 8){ if(x > 9){ if(x > 10){ if(x > 11){
        hit = 1;
    }}}}}}}}}}}}

    if(hit == 1) print("deep_nesting.if:    PASS\n");
    else         print("deep_nesting.if:    FAIL (hit=%d)\n", hit);

    // 4-deep nested loops: 5*5*5*5 = 625 iterations
    int count = 0;
    int i = 0;
    while(i < 5){
        int j = 0;
        while(j < 5){
            for(int k = 0; k < 5; ++k){
                int m = 0;
                while(m < 5){ count += 1; m += 1; }
            }
            j += 1;
        }
        i += 1;
    }
    if(count == 625) print("deep_nesting.loops: PASS (%d)\n", count);
    else             print("deep_nesting.loops: FAIL (%d, want 625)\n", count);

    // break/continue inside nested loops
    int s = 0;
    for(int p = 0; p < 100; ++p){
        if(p == 10) break;
        if((p % 2) == 0) continue;
        s += p;                     // 1+3+5+7+9 = 25
    }
    if(s == 25) print("deep_nesting.flow:  PASS (%d)\n", s);
    else        print("deep_nesting.flow:  FAIL (%d, want 25)\n", s);
    return 0;
}
