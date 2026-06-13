// Extreme: GP register pressure.
// 11 simultaneously-live integer temporaries — exactly fills the codegen's
// 11-register GP pool (rbx, r12-r15, rsi, rdi, r8-r11). One more level of
// right-nesting (depth 12) exhausts the pool: the allocator has no spilling,
// so alloc() pops an empty vector and the COMPILER aborts. This is the ceiling.
int main(){
    int v1 = 1;
    int v2 = 2;
    int v3 = 3;
    int v4 = 4;
    int v5 = 5;
    int v6 = 6;
    int v7 = 7;
    int v8 = 8;
    int v9 = 9;
    int v10 = 10;
    int v11 = 11;
    // right-associated so every leaf stays live to the deepest node
    int r = (v1 + (v2 + (v3 + (v4 + (v5 + (v6 + (v7 + (v8 + (v9 + (v10 + v11))))))))));
    if(r == 66) print("register_pressure_gp: PASS (r=%d)\n", r);
    else        print("register_pressure_gp: FAIL (r=%d, want 66)\n", r);
    return 0;
}
