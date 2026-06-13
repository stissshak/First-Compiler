// Float special values: inf / -inf / nan literals + IEEE comparison semantics.
// nan is unordered: every ordered comparison is false, only != is true.

float g_inf = inf;          // global literal initializer
float g_nan = nan;

int main(){
    float p = inf;
    float n = -inf;
    float q = nan;

    // literals match what computation yields
    int lit = (p == 1.0 / 0.0)
            & (n == (0.0 - 1.0) / 0.0)
            & (1.0 / p == 0.0);
    if(lit) print("float_special.literals: PASS\n");
    else    print("float_special.literals: FAIL\n");

    // ordering of the infinities
    int ord = (p > 1000000000.0) & (n < -1000000000.0) & (p > n);
    if(ord) print("float_special.order:    PASS\n");
    else    print("float_special.order:    FAIL\n");

    // arithmetic with infinity
    int ar = (p + 1.0 == p) & (p - 1.0 == p);
    if(ar) print("float_special.arith:    PASS\n");
    else   print("float_special.arith:    FAIL\n");

    // nan is unordered: ==,<,>,<=,>= all false; != true
    int nanc = (q != q)
             & (!(q == q)) & (!(q < 0.0)) & (!(q > 0.0))
             & (!(q <= 0.0)) & (!(q >= 0.0));
    if(nanc) print("float_special.nan:      PASS\n");
    else     print("float_special.nan:      FAIL\n");

    // globals carry the special values too
    int glob = (g_inf == p) & (g_nan != g_nan);
    if(glob) print("float_special.global:   PASS\n");
    else     print("float_special.global:   FAIL\n");
    return 0;
}
