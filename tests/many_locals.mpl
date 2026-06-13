// Extreme: large stack frame — many locals plus sizable arrays.
// Stresses frame-size accounting, alignment, and rbp-relative addressing
// for offsets well beyond a single byte.

int main(){
    // 40 scalar locals
    int a0=0;  int a1=1;  int a2=2;  int a3=3;  int a4=4;
    int a5=5;  int a6=6;  int a7=7;  int a8=8;  int a9=9;
    int b0=10; int b1=11; int b2=12; int b3=13; int b4=14;
    int b5=15; int b6=16; int b7=17; int b8=18; int b9=19;
    int c0=20; int c1=21; int c2=22; int c3=23; int c4=24;
    int c5=25; int c6=26; int c7=27; int c8=28; int c9=29;
    int d0=30; int d1=31; int d2=32; int d3=33; int d4=34;
    int d5=35; int d6=36; int d7=37; int d8=38; int d9=39;

    // big arrays push other locals to large negative offsets
    int big[1000];
    char buf[777];
    float fs[256];

    int scalars = a0+a1+a2+a3+a4+a5+a6+a7+a8+a9
                + b0+b1+b2+b3+b4+b5+b6+b7+b8+b9
                + c0+c1+c2+c3+c4+c5+c6+c7+c8+c9
                + d0+d1+d2+d3+d4+d5+d6+d7+d8+d9;   // 0..39 = 780

    int i = 0;
    while(i < 1000){ big[i] = i; i += 1; }
    int asum = 0;
    i = 0;
    while(i < 1000){ asum += big[i]; i += 1; }     // 0..999 = 499500

    // touch the far ends of buf and fs
    buf[0] = 65; buf[776] = 90;
    fs[0] = 1.5; fs[255] = 2.5;
    int ends = ((int)buf[0]) + ((int)buf[776]) + ((int)(fs[0] + fs[255]));  // 65+90+4 = 159

    if(scalars == 780) print("many_locals.scalars: PASS (%d)\n", scalars);
    else               print("many_locals.scalars: FAIL (%d, want 780)\n", scalars);
    if(asum == 499500) print("many_locals.array:   PASS (%d)\n", asum);
    else               print("many_locals.array:   FAIL (%d, want 499500)\n", asum);
    if(ends == 159)    print("many_locals.farends: PASS (%d)\n", ends);
    else               print("many_locals.farends: FAIL (%d, want 159)\n", ends);
    return 0;
}
