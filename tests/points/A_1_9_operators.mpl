// Task A.1.9 — Extended operators.
// Bitwise (& | ^ ~ << >>), every compound assignment, ++/--, and the defined
// shift-count behavior (mod 64; negative count uses its unsigned pattern).

int main(){
    // plain bitwise
    int band = 0xF0 & 0x0F;     // 0
    int bor  = 0xF0 | 0x0F;     // 255
    int bxor = 0xFF ^ 0x0F;     // 240
    int bnot = ~0;              // -1
    int shl  = 1 << 4;          // 16
    int shr  = 256 >> 2;        // 64
    if((band==0) & (bor==255) & (bxor==240) & (bnot==-1) & (shl==16) & (shr==64))
         print("ops.bitwise:   PASS\n");
    else print("ops.bitwise:   FAIL\n");

    // arithmetic compound assignment
    int a = 10;
    a += 5; a -= 3; a *= 2; a /= 5; a %= 3;   // 10,15,12,24,4,1
    if(a==1) print("ops.compound1: PASS\n"); else print("ops.compound1: FAIL (%d)\n", a);

    // bitwise compound assignment
    int b = 0xFF;
    b &= 0x0F; b |= 0x30; b ^= 0x0F; b <<= 2; b >>= 1;   // 255,15,63,48,192,96
    if(b==96) print("ops.compound2: PASS\n"); else print("ops.compound2: FAIL (%d)\n", b);

    // increment / decrement, pre and post, value + side effect
    int p = 5; int post = p++;   // post=5, p=6
    int q = 5; int pre  = ++q;   // pre=6,  q=6
    int r = 5; r++; ++r; r--; --r;   // back to 5
    if((post==5) & (p==6) & (pre==6) & (q==6) & (r==5))
         print("ops.incdec:    PASS\n");
    else print("ops.incdec:    FAIL\n");

    // shift count is taken mod 64 (shifts run on 64-bit registers)
    long one = 1;
    int modded = ((one << 64) == 1) & ((one << 65) == 2) & ((256 >> -1) == 0);
    if(modded) print("ops.shiftmod:  PASS\n"); else print("ops.shiftmod:  FAIL\n");

    // right shift: arithmetic for signed, logical for uint
    int  neg = -16;
    uint u   = 0xFFFFFFFF;
    if(((neg >> 2) == -4) & ((u >> 28) == 15))
         print("ops.shiftsign: PASS\n");
    else print("ops.shiftsign: FAIL\n");
    return 0;
}
