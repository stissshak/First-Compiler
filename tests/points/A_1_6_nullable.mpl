// Task A.1.6 — Nullable types.
// `null` is a void* zero that converts to any pointer type; pointers compare
// against it and can be (re)assigned null.

struct Node{ int v; Node *next; }

int main(){
    // null initialization for several pointer types
    int   *ip = null;
    char  *cp = null;
    Node  *np = null;
    void  *vp = null;

    int allnull = (ip == null) & (cp == null) & (np == null) & (vp == null);
    if(allnull) print("nullable.init:    PASS\n");
    else        print("nullable.init:    FAIL\n");

    // assign a real address, then compare both ways
    int x = 99;
    ip = &x;
    if((ip != null) & (*ip == 99)) print("nullable.assign:  PASS\n");
    else                           print("nullable.assign:  FAIL\n");

    // reset back to null
    ip = null;
    if(ip == null) print("nullable.reset:   PASS\n");
    else           print("nullable.reset:   FAIL\n");

    // null as a list terminator: a one-node list whose next is null
    Node n;
    n.v = 7;
    n.next = null;
    int len = 0;
    Node *cur = &n;
    while(cur != null){ len += 1; cur = cur->next; }
    if(len == 1) print("nullable.list:    PASS\n");
    else         print("nullable.list:    FAIL (%d)\n", len);
    return 0;
}
