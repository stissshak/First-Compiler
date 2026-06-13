// Task A.1.1 — Sized numeric types.
// int(4) uint(4) short(2) long(8) char(1) bool(1) byte(1) float(8), two's-
// complement wrap, and uint's unsigned compare / divide / right-shift.

int main(){
    // sizes match types.md
    int ok = (sizeof(int)==4) & (sizeof(uint)==4) & (sizeof(short)==2)
           & (sizeof(long)==8) & (sizeof(char)==1) & (sizeof(bool)==1)
           & (sizeof(byte)==1) & (sizeof(float)==8);
    if(ok) print("sized.sizes:    PASS\n");
    else   print("sized.sizes:    FAIL\n");

    // long holds values past 2^32
    long L = 5000000000;
    if(L / 1000000 == 5000) print("sized.long:     PASS\n");
    else                    print("sized.long:     FAIL (%ld)\n", L);

    // short narrows to 16 bits on store (70000 & 0xFFFF = 4464)
    short sh = 70000;
    if(((int)sh) == 4464) print("sized.short:    PASS\n");
    else                  print("sized.short:    FAIL (%d)\n", (int)sh);

    // signed int overflow wraps around
    int ov = 2147483647;
    ov = ov + 1;
    if(ov == -2147483648) print("sized.overflow: PASS\n");
    else                  print("sized.overflow: FAIL (%d)\n", ov);

    // uint: unsigned semantics. 0xFFFFFFFF is -1 as int, 4294967295 as uint.
    uint u = 0xFFFFFFFF;
    int  ucmp = u > 5;          // unsigned compare -> true
    int  udiv = u / 2;          // unsigned divide  -> 2147483647
    int  ushr = u >> 28;        // logical shift    -> 15
    if(ucmp & (udiv == 2147483647) & (ushr == 15)) print("sized.uint:     PASS\n");
    else print("sized.uint:     FAIL (cmp=%d div=%u shr=%u)\n", ucmp, udiv, ushr);

    // byte: raw data — only assign / == / != / explicit cast to integer family
    // (char/byte are signed, so keep the value < 128 for an unambiguous (int))
    byte b1 = (byte)100;
    byte b2 = (byte)100;
    byte b3 = (byte)17;
    if((b1 == b2) & (b1 != b3) & (((int)b1) == 100)) print("sized.byte:     PASS\n");
    else print("sized.byte:     FAIL\n");
    return 0;
}
