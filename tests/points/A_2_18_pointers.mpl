// Task A.2.18 — Pointers.
// address-of, dereference read/write, mutation through pointers across a call,
// pointer-to-pointer, pointer arithmetic, and const-pointer rules.

// mutate caller's variables through pointers (call-by-value of the address)
void swap(int *a, int *b){
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

int main(){
    // deref read/write
    int x = 10;
    int *p = &x;
    *p = 25;
    if(x == 25) print("ptr.deref:    PASS\n");
    else        print("ptr.deref:    FAIL (%d)\n", x);

    // mutation through pointers via a function
    int a = 1; int b = 2;
    swap(&a, &b);
    if((a==2) & (b==1)) print("ptr.swap:     PASS\n");
    else                print("ptr.swap:     FAIL (a=%d b=%d)\n", a, b);

    // pointer to pointer
    int v = 7;
    int *q = &v;
    int **qq = &q;
    **qq = 8;
    if(v == 8) print("ptr.ptrptr:   PASS\n");
    else       print("ptr.ptrptr:   FAIL (%d)\n", v);

    // pointer arithmetic scales by element size; p+i == &arr[i]
    int arr[5];
    for(int i = 0; i < 5; ++i){ arr[i] = i * 10; }   // 0,10,20,30,40
    int *base = arr;
    int diff = *(base + 3) - *(base + 1);            // 30 - 10 = 20
    if(diff == 20) print("ptr.arith:    PASS\n");
    else           print("ptr.arith:    FAIL (%d)\n", diff);

    // pointer to const: reads fine, *cp = ... would be a compile error
    const int k = 42;
    const int *cp = &k;
    if(*cp == 42) print("ptr.const:    PASS\n");
    else          print("ptr.const:    FAIL (%d)\n", *cp);
    return 0;
}
