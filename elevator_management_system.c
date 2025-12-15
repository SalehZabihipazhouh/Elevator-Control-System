#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MIN_F -10
#define MAX_F 30
#define MAX_HIST 10

typedef enum { IDLE, UP, DOWN } Dir;
typedef enum { ACTIVE, EMERGENCY } State;

typedef struct Node {
    int floor;
    struct Node* next;
} Node;

typedef struct Stack {
    int arr[MAX_HIST];
    int count;
} Stack;

typedef struct Elevator {
    char name;
    int cf; // current
    Dir d;
    State state;
    int tf; // target

    Node* up_h;
    Node* down_h;
    Node* vip_up;
    Node* vip_down;

    Stack hist;
} Elevator;

// helper to make node
Node* new_node(int f) {
    Node* p = (Node*)malloc(sizeof(Node));
    if(!p) return NULL;
    p->floor = f;
    p->next = NULL;
    return p;
}

void add_hist(Stack* s, int val) {
    if (s->count < MAX_HIST) {
        s->arr[s->count++] = val;
    } else {
        // shift left
        for(int i=0; i<MAX_HIST-1; i++) s->arr[i] = s->arr[i+1];
        s->arr[MAX_HIST-1] = val;
    }
}

void print_hist(Elevator* e) {
    printf("Hist %c: [ ", e->name);
    for(int i = e->hist.count-1; i>=0; i--) printf("%d ", e->hist.arr[i]);
    printf("]\n");
}

// update the target floor (furthest point)
void update_tf(Elevator* e) {
    int last = e->cf;
    Node* curr;

    if (e->d == UP) {
        // vip priority
        if (e->vip_up) {
            curr = e->vip_up;
            while(curr->next) curr = curr->next;
            last = curr->floor;
        }
        if (e->up_h) {
            curr = e->up_h;
            while(curr->next) curr = curr->next;
            if (curr->floor > last) last = curr->floor;
        }
    }
    else if (e->d == DOWN) {
        if (e->vip_down) {
            curr = e->vip_down;
            while(curr->next) curr = curr->next;
            last = curr->floor;
        }
        if (e->down_h) {
            curr = e->down_h;
            while(curr->next) curr = curr->next;
            if (curr->floor < last) last = curr->floor;
        }
    }
    e->tf = last;
}

Node* insert(Node* head, int f, int asc) {
    // check dup at head
    if (head && head->floor == f) return head;

    Node* tmp = new_node(f);
    if (!head) return tmp;

    int check;
    if (asc) check = (f < head->floor);
    else     check = (f > head->floor);

    if (check) {
        tmp->next = head;
        return tmp;
    }

    Node* curr = head;
    while (curr->next) {
        if (curr->next->floor == f) { free(tmp); return head; } // dup

        if (asc) {
            if (curr->next->floor > f) break;
        } else {
            if (curr->next->floor < f) break;
        }
        curr = curr->next;
    }

    tmp->next = curr->next;
    curr->next = tmp;
    return head;
}

void add_req(Elevator* e, int f, int vip) {
    if (e->state == EMERGENCY) return;

    if (f == e->cf) {
        printf("[%c] Already at %d. Door open.\n", e->name, f);
        add_hist(&e->hist, f);
        return;
    }

    if (f > e->cf) {
        if (vip) e->vip_up = insert(e->vip_up, f, 1);
        else     e->up_h = insert(e->up_h, f, 1);

        if (e->d == IDLE) e->d = UP;
    }
    else {
        if (vip) e->vip_down = insert(e->vip_down, f, 0);
        else     e->down_h = insert(e->down_h, f, 0);

        if (e->d == IDLE) e->d = DOWN;
    }

    update_tf(e);
    if(vip) printf("[%c] VIP added: %d\n", e->name, f);
    else    printf("[%c] Req added: %d\n", e->name, f);
}

int calc(Elevator* e, int rf) {
    if (e->state == EMERGENCY) return 99999;

    int res = abs(e->cf - rf);
    if (e->d == IDLE) return res;

    // cost with penalty
    if (e->d == UP) {
        if (rf >= e->cf) return res;
        else return abs(e->tf - e->cf) + abs(e->tf - rf);
    }
    if (e->d == DOWN) {
        if (rf <= e->cf) return res;
        else return abs(e->tf - e->cf) + abs(e->tf - rf);
    }
    return res;
}

void emergency(Elevator* e) {
    printf("\n!!! %c EMERGENCY STOP !!!\n", e->name);
    e->state = EMERGENCY;
    e->d = IDLE;
    e->tf = e->cf;

    // manual free lists
    Node* c; Node* n;

    c = e->up_h; while(c){ n=c->next; free(c); c=n; } e->up_h=NULL;
    c = e->down_h; while(c){ n=c->next; free(c); c=n; } e->down_h=NULL;
    c = e->vip_up; while(c){ n=c->next; free(c); c=n; } e->vip_up=NULL;
    c = e->vip_down; while(c){ n=c->next; free(c); c=n; } e->vip_down=NULL;
}

void reset(Elevator* e) {
    if (e->state != EMERGENCY) return;
    printf("[%c] Reset done. Active.\n", e->name);
    e->state = ACTIVE;
    e->d = IDLE;
    e->tf = e->cf;
}

void step(Elevator* e) {
    if (e->state == EMERGENCY || e->d == IDLE) return;

    int dest = -999;
    int vu = (e->vip_up != NULL);
    int vd = (e->vip_down != NULL);

    // logic for next dest
    if (e->d == UP) {
        if (vu) dest = e->vip_up->floor;
        else if (vd) {
            printf("[%c] VIP Priority -> Switch DOWN\n", e->name);
            e->d = DOWN;
            update_tf(e);
            return;
        }
        else if (e->up_h) dest = e->up_h->floor;
        else if (e->down_h) {
            e->d = DOWN; update_tf(e); return;
        }
        else { e->d = IDLE; return; }
    }
    else if (e->d == DOWN) {
        if (vd) dest = e->vip_down->floor;
        else if (vu) {
            printf("[%c] VIP Priority -> Switch UP\n", e->name);
            e->d = UP;
            update_tf(e);
            return;
        }
        else if (e->down_h) dest = e->down_h->floor;
        else if (e->up_h) {
            e->d = UP; update_tf(e); return;
        }
        else { e->d = IDLE; return; }
    }

    if (dest != -999) {
        // move physics
        if (e->cf < dest) {
            e->cf++;
            printf("[%c] ^ %d\n", e->name, e->cf);
        } else if (e->cf > dest) {
            e->cf--;
            printf("[%c] v %d\n", e->name, e->cf);
        }

        // arrived?
        if (e->cf == dest) {
            printf("[%c] * DING * %d\n", e->name, e->cf);
            add_hist(&e->hist, e->cf);

            // remove node
            Node** p = NULL;
            if (e->d == UP) {
                if (e->vip_up && e->vip_up->floor == e->cf) p = &e->vip_up;
                else if (e->up_h && e->up_h->floor == e->cf) p = &e->up_h;
            } else {
                if (e->vip_down && e->vip_down->floor == e->cf) p = &e->vip_down;
                else if (e->down_h && e->down_h->floor == e->cf) p = &e->down_h;
            }

            if (p && *p) {
                Node* t = *p;
                *p = t->next;
                free(t);
            }
            update_tf(e);
        }
    }
}

void req(Elevator sys[], int f, int vip) {
    if (f < MIN_F || f > MAX_F) {
        printf("Err floor\n");
        return;
    }

    int c1 = calc(&sys[0], f);
    int c2 = calc(&sys[1], f);

    if (c1 <= c2) {
        printf("Req %d -> A (%d)\n", f, c1);
        add_req(&sys[0], f, vip);
    } else {
        printf("Req %d -> B (%d)\n", f, c2);
        add_req(&sys[1], f, vip);
    }
}

int main() {
    Elevator sys[2];

    // init A
    sys[0].name='A'; sys[0].cf=0; sys[0].d=IDLE; sys[0].state=ACTIVE; sys[0].tf=0;
    sys[0].up_h=NULL; sys[0].down_h=NULL; sys[0].vip_up=NULL; sys[0].vip_down=NULL;
    sys[0].hist.count=0;

    // init B
    sys[1].name='B'; sys[1].cf=0; sys[1].d=IDLE; sys[1].state=ACTIVE; sys[1].tf=20;
    sys[1].up_h=NULL; sys[1].down_h=NULL; sys[1].vip_up=NULL; sys[1].vip_down=NULL;
    sys[1].hist.count=0;

    printf("--- Start ---\n");

    // scenarios
    req(sys, 5, 0);
    req(sys, -2, 1); // vip

    // run loop
    for(int i=0; i<5; i++) {
        printf("\nTime %d:\n", i);
        step(&sys[0]);
        step(&sys[1]);
    }

    // emergency test
    emergency(&sys[0]);
    req(sys, 10, 0);

    reset(&sys[0]);
    req(sys, 10, 0);

    print_hist(&sys[0]);
    print_hist(&sys[1]);

    return 0;
}
