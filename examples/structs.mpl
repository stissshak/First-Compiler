// Structs: layout with padding, nesting, copy, pointers, ->.

struct Point{
    char tag;
    int x;
    int y;
}

struct Line{
    Point a;
    Point b;
}

int main(){
    Point p = {'P', 10, 32};

    Point q = p;        // whole-struct copy
    q.x = 100;

    Line ln;
    ln.a = p;
    ln.b.x = 7;
    ln.b.y = q.y;

    Point *pp = &p;
    pp->x = 11;

    print("%c p=(%d,%d) q=(%d,%d)\n", p.tag, p.x, p.y, q.x, q.y);
    print("ln.b=(%d,%d) sizeof(Point)=%d typeid=%s\n",
           ln.b.x, ln.b.y, sizeof(Point), typeid(p));
    return 0;
}
