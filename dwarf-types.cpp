#include <cstdio>
#include <cstdlib>

extern "C" {
#include "drgn.h"
}

bool a;
int b;
long c;
float d;
double e;

int bs[10];
double es[4];

struct A {
    struct { int x; } _a;
};

struct A sa;

class B {
    class { public: int x; } _b;
};

B sb;

template <typename T>
class C {
    decltype(T::_a) _c;
};

C<A> sc;

size_t fail_count = 0;
void check_name(struct drgn_program *prog, const char *variable_name, const char *expected_name) {
    struct drgn_object obj;
    drgn_object_init(&obj, prog);
    if (auto *err = drgn_program_find_object(prog, variable_name, "dwarf-types.cpp", DRGN_FIND_OBJECT_ANY, &obj)) {
        fprintf(stderr, "Error finding '%s': %s\n", variable_name, err->message);
        exit(-1);
    }

    struct drgn_qualified_type qtype = drgn_object_qualified_type(&obj);
    struct drgn_type *type = qtype.type;

    const char *returned_name = nullptr;
    if (auto *err = drgn_type_reconstruct_name(type, &returned_name)) {
        fprintf(stderr, "Error reconstructing name for '%s': %s\n", variable_name, err->message);
        exit(-1);
    }


    fprintf(stdout, "%s: '%s' == '%s'", variable_name, expected_name, returned_name);
    if (strcmp(expected_name, returned_name) == 0) {
        fprintf(stdout, " OK\n");
    } else {
        fprintf(stdout, " FAILED\n");
        fail_count++;
    }

    drgn_object_deinit(&obj);
}

int main(int argc, const char *argv[]) {
    struct drgn_program *prog = nullptr;
    if (auto *err = drgn_program_create(nullptr, &prog)) {
        fprintf(stderr, "Error creating drgn_program: %s\n", err->message);
        return -1;
    }

    const char *target = argv[0]; //"build/dwarf-types-target";
    if (auto *err = drgn_program_load_debug_info(prog, &target, 1, false, false)) {
        fprintf(stderr, "Error loading debug info: %s\n", err->message);
        return -1;
    }

    puts("=== BASIC TYPES ===");
    check_name(prog, "a", "bool");
    check_name(prog, "b", "int");
    check_name(prog, "c", "long int");
    check_name(prog, "d", "float");
    check_name(prog, "e", "double");

    puts("=== ARRAY TYPES ===");
    check_name(prog, "bs", "int[10]");
    check_name(prog, "es", "double[4]");

    puts("=== STRUCT TYPES ===");
    check_name(prog, "sa", "A");
    check_name(prog, "sb", "B");
    check_name(prog, "sc", "C<A>");

    drgn_program_destroy(prog);

    return fail_count;
}
