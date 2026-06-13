// Task A.3.9 — Raw / generic pointers (extends A.2.18).
// void* as a generic pointer: malloc returns it, it converts to/from any typed
// pointer, and one void* can address differently-typed buffers in turn.

extern void *memset(void *str, int c, long n);

int main(){
    // malloc gives void*, used as a typed buffer with no explicit cast
    int n = 8;
    int *xs = malloc(n * sizeof(int));
    for(int i = 0; i < n; ++i){ xs[i] = i + 1; }
    int sum = 0;
    for(int i = 0; i < n; ++i){ sum += xs[i]; }      // 1..8 = 36
    free(xs);
    if(sum == 36) print("raw.malloc:   PASS\n");
    else          print("raw.malloc:   FAIL (%d)\n", sum);

    // a single void* reused for an int view then a char view of one block
    void *block = malloc(64);
    memset(block, 0, 64);
    int  *iv = block;            // void* -> int*
    iv[0] = 0x44434241;          // little-endian bytes 'A','B','C','D' (all < 128)
    char *cv = block;            // void* -> char*  (same memory)
    int ok = (cv[0]=='A') & (cv[1]=='B') & (cv[2]=='C') & (cv[3]=='D');
    free(block);
    if(ok) print("raw.generic:  PASS\n");
    else   print("raw.generic:  FAIL\n");

    // round-trip a typed pointer through void* and back
    int v = 1234;
    void *raw = &v;
    int *p = raw;
    if(*p == 1234) print("raw.roundtrip: PASS\n");
    else           print("raw.roundtrip: FAIL (%d)\n", *p);
    return 0;
}
