#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define debug(m, e) \
    printf("%s:%d: %s:", __FILE__, __LINE__, m); \
    print_obj(e, 1); \
    puts("");

typedef struct List {
    struct List * next;
    void * data;
} List;

List * symbols = 0;

static int lookahead = 0;
static char token[32];

#define is_space(x) (x == ' ' || x == '\n')
#define is_paren(x) (x == '(' || x == ')')

static void get_token() {
    int index = 0;

    while (is_space(lookahead)) {
        lookahead = getchar();
    }
    if (is_paren(lookahead)) {
        token[index++] = lookahead;
        lookahead = getchar();
    }
    else {
        while (lookahead != EOF && !is_space(lookahead) && !is_paren(lookahead)) {
            token[index++] = lookahead;
            lookahead = getchar();
            
        }
    }
    token[index] = '\0';
}

#define is_pair(x) (((long)x & 0x1) == 0x1) // tag lsb of pointer to pair
#define untag(x) ((long)x & ~0x1)
#define tag(x) ((long)x | 0x1)

#define car(x) (((List *)untag(x))->data)
#define cdr(x) (((List *)untag(x))->next)

#define e_true  cons(intern("quote"), cons(intern("t"), 0))
#define e_false 0

List * cons(void * _car, void * _cdr) {
    List * _pair = calloc(1, sizeof(List)); // this alloc need a free()
    _pair->data = _car;
    _pair->next = _cdr;
    return (List *)tag(_pair);
}

void * intern(char * symbol) {
    List * _pair = symbols = NULL;  // keep a symbol table

    for ( ; _pair; _pair = cdr(_pair)) {
        if (strncmp(symbol, (char *)car(_pair), 32) == 0) {
            return car(_pair);
        }
    }
    symbols = cons(strdup(symbol), symbols);
    return car(symbols);
}

List * get_list(void);  // forward declaration

void * get_object(void) {
    if (token[0] == '(') {
        return get_list();
    }
    return intern(token);
}

List * get_list(void) {
    List * temp = NULL;

    get_token();
    if (token[0] == ')') {
        return 0;
    }
    temp = get_object();
    return cons(temp, get_list()); // recurse
}

void print_object(List * obj, int head) {
    if (!is_pair(obj)) {
        printf("%s", obj ? (char *)obj : "null");
    }
    else {
        if (head) {
            printf("(");
        }
        print_object(car(obj), 1);
        if (cdr(obj) != 0) {
            if (is_pair(cdr(obj))) {
                printf(" ");
                print_object(cdr(obj), 0);
            }
        }
        else {
            printf(")");
        }
    }
}

List * fcons(List * a) { return cons(car(a), car(cdr(a))); }
List * fcar(List * a) { return car(car(a)); }
List * fcdr(List * a) { return cdr(car(a)); }
List * feq(List * a) { return car(a) == car(cdr(a)) ? e_true : e_false; }
List * fpair(List * a) { return is_pair(car(a))     ? e_true : e_false; }
List * fsym(List * a) { return !is_pair(car(a))     ? e_true : e_false; }
List * fnull(List * a) { return car(a) == 0         ? e_true : e_false; }
List * freadobj(List * a) {
    (void)a; // unused parameter, but written this way for symmetry
    lookahead = getchar();
    get_token();
    return get_object();
}
List * fwriteobj(List * a) {
    print_object(car(a), 1);
    puts("");
    return e_true;
}

List * eval(List * exp, List * env); /* forward declaration */

List * evlist(List * list, List * env) {
    List * head = NULL, **args = &head;

    for ( ; list; list = cdr(list)) {
        *args = cons(eval(car(list), env), 0);
        args = &((List *)untag(*args))->next;
    }
    return head;
}

List * apply_primitive(void * primfn, List * args) {
    return ((List * (*) (List *)) primfn) (args);
}

List * eval(List * exp, List * env) {
    if (!is_pair(exp)) {
        for ( ; env != NULL; env = cdr(env)) {
            if (exp == car(car(env))) return car(cdr(car(env)));
        }
        return NULL;
    }
    else {
        if (!is_pair(car(exp))) { /* special forms */
            if (car(exp) == intern("quote")) {
                return car(cdr(exp));
            }
            else if (car(exp) == intern("if")) {
                if (eval(car(cdr(exp)), env) != 0) {
                    return eval(car(cdr(cdr(exp))), env);
                }
                else {
                    return eval(car(cdr(cdr(cdr(exp)))), env);
                }
            }
            else if (car(exp) == intern("lambda")) {
                return exp; // TODO: create a closure and capture free vars
            }
            else if (car(exp) == intern("apply")) { // apply fn to list
                List * args = evlist(cdr(cdr(exp)), env);
                args = car(args); // assumes one arg and that it is a list
                return apply_primitive(eval(car(cdr(exp)), env), args);
            }
            else { // it's a function call
                List * primop = eval(car(exp), env);
                if (is_pair(primop)) {
                // user defined lambda, arg list eval happens in binding below
                    return eval(cons(primop, cdr(exp)), env);
                }
            }
        }
        else { // should be a lambda, bind names into env and eval body
            if (car(car(exp)) == intern("lambda")) {
                List * extenv = env, * names = car(cdr(car(exp))), *vars = cdr(exp);
                for ( ; names; names = cdr(names), vars = cdr(vars)) {
                    extenv = cons(cons(car(names), cons(eval(car(vars), env), 0)), extenv);
                }
                return eval(car(cdr(cdr(car(exp)))), extenv);
            }
        }
    }
    puts("cannot evaluate expression");
    return NULL;
}



int main(void) {
    List * env = cons(cons(intern("car"), cons((void *)fcar, 0)),
                 cons(cons(intern("cdr"), cons((void *)fcdr, 0)),
                 cons(cons(intern("cons"), cons((void *)fcons, 0)),
                 cons(cons(intern("eq?"), cons((void *)feq, 0)),
                 cons(cons(intern("pair?"), cons((void *)fpair, 0)),
                 cons(cons(intern("symbol?"), cons((void *)fsym, 0)),
                 cons(cons(intern("null?"), cons((void *)fnull, 0)),
                 cons(cons(intern("read"), cons((void *)freadobj, 0)),
                 cons(cons(intern("write"), cons((void *)fwriteobj, 0)),
                 cons(cons(intern("null"), cons(0,0)), 0))))))))));
    lookahead = getchar();
    get_token();
    print_object(eval(get_object(), env), 1);
    printf("\n");
    return EXIT_SUCCESS;
}

