// Task A.1.13 — Metafunctions: sizeof and typeid, both folded at compile time.
// sizeof gives byte size as int (arrays don't decay; the expr is NOT
// evaluated). typeid gives the type name as a char*.

struct Pt{ int x; int y; }
int add(int a, int b){ return a + b; }

// no string.h here, so compare strings ourselves
int streq(char *a, char *b){
    int i = 0;
    while((a[i] != 0) & (b[i] != 0)){
        if(a[i] != b[i]) return 0;
        i += 1;
    }
    return a[i] == b[i];
}

int sideEffects = 0;
int bump(){ sideEffects += 1; return 0; }

int main(){
    // sizeof of types and of an array (whole array, no decay)
    int arr[5];
    int szOk = (sizeof(int)==4) & (sizeof(float)==8) & (sizeof(char)==1)
             & (sizeof(Pt)==8) & (sizeof(arr)==20) & (sizeof(int*)==8);
    if(szOk) print("meta.sizeof:    PASS\n");
    else     print("meta.sizeof:    FAIL\n");

    // sizeof does not evaluate its operand
    int s = sizeof(bump());
    if((s==4) & (sideEffects==0)) print("meta.noeval:    PASS\n");
    else print("meta.noeval:    FAIL (s=%d effects=%d)\n", s, sideEffects);

    // typeid of various expressions
    int i=0; float f=0.0; int *p=null; int **pp=null; Pt v; Pt *vp=null;
    int(int,int) fn = add;
    int tOk = streq(typeid(i),  "int")
            & streq(typeid(f),  "float")
            & streq(typeid(p),  "int*")
            & streq(typeid(pp), "int**")
            & streq(typeid(v),  "Pt")
            & streq(typeid(vp), "Pt*")
            & streq(typeid(arr),"int[5]")
            & streq(typeid(fn), "int(int, int)");
    if(tOk) print("meta.typeid:    PASS\n");
    else    print("meta.typeid:    FAIL\n");
    return 0;
}
