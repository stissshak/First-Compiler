extern void *memset(void *str, int c, long n);
extern float sin(float a);
extern float cos(float a);
extern int putchar(int c);
extern int usleep(int t);

float A = 0;
float B = 0;
float z[1760];
char b[1760];
int main() {
    float i = 0;
    float j = 0;
    int k = 0;
    print("\x1b[2J");
    while (1) {
        memset(b, 32, 1760);
        memset(z, 0, 14080);   
        j = 0;
        while ((j < 6.28)) {
            i = 0;
            while ((i < 6.28)) {
                float c_ = sin(i);
                float d_ = cos(j);
                float e_ = sin(A);
                float f_ = sin(j);
                float g_ = cos(A);
                float l_ = cos(i);
                float m_ = cos(B);
                float n_ = sin(B);
                float h = (d_ + 2);
                float D = (1 / ((((c_ * h) * e_) + (f_ * g_)) + 5));
                float t = (((c_ * h) * g_) - (f_ * e_));
                int x = ((int)((40 + ((30 * D) * (((l_ * h) * m_) - (t * n_))))));
                int y = ((int)((12 + ((15 * D) * (((l_ * h) * n_) + (t * m_))))));
                int o = (x + (80 * y));
                int N = ((int)((8 * ((((((f_ * e_) - ((c_ * d_) * g_)) * m_) - ((c_ * d_) * e_)) - (f_ * g_)) - ((l_ * d_) * n_)))));
                if ((22 > y)) {
                    if ((y > 0)) {
                        if ((x > 0)) {
                            if ((80 > x)) {
                                if ((D > z[o])) {
                                    z[o] = D;
                                    int idx = 0;
                                    if(N > 0) idx = N;
                                    char ramp[13] = ".,-~:;=!*#$@";
                                    b[o] = ramp[idx];
                                }
                            }
                        }
                    }
                }
                i += 0.02;
            }
            j += 0.07;
        }
        print("\x1b[H");
        k = 0;
        while ((k < 1761)) {
            if (((k % 80) == 0)) {
                putchar(10);
            } else {
                putchar(((int)(b[k])));
            }
            A += 0.00004;
            B += 0.00002;
            k += 1;
        }
        usleep(5000);
    }
    return 0;

}