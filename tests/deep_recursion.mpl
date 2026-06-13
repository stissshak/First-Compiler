// Extreme: deep call stacks and many nested frames.
// Each call pushes a frame (push rbp / sub rsp / saved args); verifies the
// prologue/epilogue and return-value plumbing hold up under depth.

int fib(int n){
    if(n < 2) return n;
    return fib(n - 1) + fib(n - 2);     // ~21891 calls for fib(21)
}

int fact(int n){
    if(n <= 1) return 1;
    return n * fact(n - 1);
}

// Ackermann: tiny inputs, very deep recursion — a classic frame stressor
int ack(int m, int n){
    if(m == 0) return n + 1;
    if(n == 0) return ack(m - 1, 1);
    return ack(m - 1, ack(m, n - 1));
}

// plain countdown to a large depth (linear stack growth)
int countdown(int n){
    if(n == 0) return 0;
    return 1 + countdown(n - 1);
}

int main(){
    int a = fib(21);          // 10946
    int b = fact(10);         // 3628800
    int c = ack(2, 3);        // 9
    int d = countdown(5000);  // 5000

    if(a == 10946)   print("deep_recursion.fib:       PASS (%d)\n", a);
    else             print("deep_recursion.fib:       FAIL (%d, want 10946)\n", a);
    if(b == 3628800) print("deep_recursion.fact:      PASS (%d)\n", b);
    else             print("deep_recursion.fact:      FAIL (%d, want 3628800)\n", b);
    if(c == 9)       print("deep_recursion.ackermann: PASS (%d)\n", c);
    else             print("deep_recursion.ackermann: FAIL (%d, want 9)\n", c);
    if(d == 5000)    print("deep_recursion.countdown: PASS (%d)\n", d);
    else             print("deep_recursion.countdown: FAIL (%d, want 5000)\n", d);
    return 0;
}
