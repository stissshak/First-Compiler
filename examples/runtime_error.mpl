// Demonstrates runtime checks. Enter 5 to die with
//   runtime error: index out of bounds at line 9
// and exit code 1. Also guarded: division by zero, null deref, assert.
int main(){
    int a[3] = {1, 2, 3};
    int i;
    input("%d", &i);
    assert(i >= 0);
    print("a[%d] = %d\n", i, a[i]);
    return 0;
}
