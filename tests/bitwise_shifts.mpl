// Extreme: full bitwise/shift operator coverage and non-decimal literals.
// & | ^ ~ << >> plus their compound-assign forms, with hex/binary literals.

int main(){
    int x = 0xF0;          // 240
    int y = 0b1010;        // 10

    int band = x & 0x0F;   // 0
    int bor  = x | y;      // 250
    int bxor = x ^ 0xFF;   // 15
    int bnot = ~0;         // -1
    int shl  = 1 << 10;    // 1024
    int shr  = 0x100 >> 4; // 16

    int acc = 1;
    acc <<= 8;             // 256
    acc |= 0xF;            // 256 | 0x0F  = 271
    acc &= 0xFE;           // 271 & 0xFE  = 14
    acc ^= 0x6;            // 14  ^ 0x06  = 8
    acc >>= 1;             // 8 >> 1      = 4

    if(band == 0)    print("bitwise.and:   PASS\n");    else print("bitwise.and:   FAIL (%d)\n", band);
    if(bor == 250)   print("bitwise.or:    PASS\n");    else print("bitwise.or:    FAIL (%d)\n", bor);
    if(bxor == 15)   print("bitwise.xor:   PASS\n");    else print("bitwise.xor:   FAIL (%d)\n", bxor);
    if(bnot == -1)   print("bitwise.not:   PASS\n");    else print("bitwise.not:   FAIL (%d)\n", bnot);
    if(shl == 1024)  print("bitwise.shl:   PASS\n");    else print("bitwise.shl:   FAIL (%d)\n", shl);
    if(shr == 16)    print("bitwise.shr:   PASS\n");    else print("bitwise.shr:   FAIL (%d)\n", shr);
    if(acc == 4)     print("bitwise.compound: PASS\n"); else print("bitwise.compound: FAIL (%d, want 4)\n", acc);
    return 0;
}
