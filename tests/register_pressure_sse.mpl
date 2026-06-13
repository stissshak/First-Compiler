// Extreme: SSE register pressure.
// 16 simultaneously-live float temporaries — exactly fills the 16-register XMM
// pool (xmm0..xmm15). Depth 17 exhausts it and aborts the compiler (no spill).
// Sum 1.0+...+16.0 = 136.0, exact in f64 so == is safe.
int main(){
    float v1 = 1.0;
    float v2 = 2.0;
    float v3 = 3.0;
    float v4 = 4.0;
    float v5 = 5.0;
    float v6 = 6.0;
    float v7 = 7.0;
    float v8 = 8.0;
    float v9 = 9.0;
    float v10 = 10.0;
    float v11 = 11.0;
    float v12 = 12.0;
    float v13 = 13.0;
    float v14 = 14.0;
    float v15 = 15.0;
    float v16 = 16.0;
    float r = (v1 + (v2 + (v3 + (v4 + (v5 + (v6 + (v7 + (v8 + (v9 + (v10 + (v11 + (v12 + (v13 + (v14 + (v15 + v16)))))))))))))));
    if(((int)r) == 136) print("register_pressure_sse: PASS (r=%f)\n", r);
    else                print("register_pressure_sse: FAIL (r=%f, want 136)\n", r);
    return 0;
}
