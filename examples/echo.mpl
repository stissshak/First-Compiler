// Reads n, then n numbers, prints them back.
// Shows: builtin print/input/malloc/free, pointers, sizeof, for loops.
int main(){
    int n;
    input("%d", &n);
    int *arr = malloc(n * sizeof(int));
    for(int i = 0; i < n; i += 1){
        input("%d", &arr[i]);
    }
    for(int i = 0; i < n; i += 1){
        print("%d ", arr[i]);
    }
    print("\n");
    free(arr);
    return 0;
}
