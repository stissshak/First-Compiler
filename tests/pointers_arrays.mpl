// Extreme: pointer indirection, pointer arithmetic, and struct access.
// Multi-level pointers, element-size scaling on +/-, and ->/. chains.
// NOTE: struct fields can't be arrays in MPL (field = type identifier), so the
// struct here uses a pointer into a standalone array instead.

struct Span{
    int  len;
    int *data;
}

int main(){
    // triple indirection: ***ppp aliases v
    int v = 41;
    int *p = &v;
    int **pp = &p;
    int ***ppp = &pp;
    ***ppp = ***ppp + 1;            // v becomes 42
    if(v == 42) print("pointers_arrays.indirect: PASS (%d)\n", v);
    else        print("pointers_arrays.indirect: FAIL (%d, want 42)\n", v);

    // pointer arithmetic walks an int array; *(arr+i) == arr[i]
    int arr[10];
    for(int i = 0; i < 10; ++i){ arr[i] = i * i; }   // 0,1,4,...,81
    int *q = arr;
    int psum = 0;
    for(int i = 0; i < 10; ++i){ psum += *(q + i); } // 0+1+4+...+81 = 285
    if(psum == 285) print("pointers_arrays.ptrarith: PASS (%d)\n", psum);
    else            print("pointers_arrays.ptrarith: FAIL (%d, want 285)\n", psum);

    // struct holding a length + pointer into the array, accessed via . and ->
    Span s;
    s.len = 10;
    s.data = arr;
    Span *sp = &s;
    int vsum = 0;
    for(int i = 0; i < sp->len; ++i){ vsum += sp->data[i]; }  // same 285
    if(vsum == 285) print("pointers_arrays.struct:   PASS (%d)\n", vsum);
    else            print("pointers_arrays.struct:   FAIL (%d, want 285)\n", vsum);

    // whole-struct copy then mutate the copy; original must be unchanged
    Span t = s;
    t.len = 0;
    if(s.len == 10) print("pointers_arrays.copy:     PASS (%d)\n", s.len);
    else            print("pointers_arrays.copy:     FAIL (%d, want 10)\n", s.len);
    return 0;
}
