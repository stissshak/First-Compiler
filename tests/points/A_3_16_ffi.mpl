// Task A.3.16 — FFI / C interop (extends A.2.18 / A.3.9).
// Declare libc functions with `extern` and call them via the SysV ABI.
// Scalars, char* strings, and double (MPL `float` is 64-bit) cross the boundary.

// NOTE: a libc name that is also a NASM reserved word (e.g. `abs`) cannot be
// linked — NASM rejects `extern abs`. Use a non-reserved name like `strcmp`.
extern int    strcmp(char *a, char *b);
extern int    atoi(char *s);
extern int    toupper(int c);
extern long   strlen(char *s);
extern float  sqrt(float x);                 // C's double sqrt
extern void  *memcpy(void *dst, void *src, long n);

int main(){
    // integer libc call: strcmp returns 0 for equal strings
    if((strcmp("abc", "abc") == 0) & (strcmp("abc", "abd") != 0)) print("ffi.strcmp:  PASS\n");
    else                                                          print("ffi.strcmp:  FAIL\n");

    // char* in, int out
    if(atoi("12345") == 12345) print("ffi.atoi:    PASS\n");
    else                       print("ffi.atoi:    FAIL\n");

    // int in/out, char promotion
    if(toupper('a') == 'A') print("ffi.toupper: PASS\n");
    else                    print("ffi.toupper: FAIL\n");

    // returns long (8-byte); MPL string is already NUL-terminated
    if(strlen("hello, world") == 12) print("ffi.strlen:  PASS\n");
    else                             print("ffi.strlen:  FAIL (%ld)\n", strlen("hello, world"));

    // double in/out across the boundary (xmm registers)
    float r = sqrt(144.0);
    if(((int)r) == 12) print("ffi.sqrt:    PASS\n");
    else               print("ffi.sqrt:    FAIL (%f)\n", r);

    // pointer-taking libc call: copy bytes between buffers
    char src[6] = "abcde";
    char dst[6];
    memcpy(dst, src, 6);
    if((dst[0]=='a') & (dst[4]=='e')) print("ffi.memcpy:  PASS\n");
    else                              print("ffi.memcpy:  FAIL\n");
    return 0;
}
