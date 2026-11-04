#include "0cc.h"
#include <limits.h>

typedef struct Initializer Initializer;
struct Initializer {
    Type *tp;
    Node *expr;
    Initializer **children;
    size_t count;
};

typedef struct InitString InitString;
struct InitString {
    InitString *next;
    char *label;
    char *bytes;
    int size;
};
static InitString *strings;

typedef struct {
    char *symbol;
    long long addend;
} InitValue;

static bool at_close(void) {
    return token->kind == TK_RESERVED && token->len == 1 && token->str[0] == '}';
}

static bool starts_string(void) {
    Token *t = token;
    while (t->kind == TK_RESERVED && t->len == 1 && t->str[0] == '(') t = t->next;
    return t->kind == TK_STR;
}

static Initializer *read_initializer(Type *tp, bool elided) {
    Initializer *init = calloc(1, sizeof(Initializer));
    init->tp = tp;
    bool braces = consume("{");
    if (tp->kind != ARRAY) {
        if (tp->kind != CHAR && tp->kind != INT && tp->kind != PTR)
            error_at(token->str, "unsupported global initializer type");
        if (braces) {
            if (consume("}")) init->expr = new_node_num(0);
            else {
                init = read_initializer(tp, false);
                consume(",");
                expect('}');
            }
        } else init->expr = assign();
        return init;
    }

    if (!tp->array_size) {
        init->tp = calloc(1, sizeof(Type));
        *init->tp = *tp;
        tp = init->tp;
    }
    if (tp->ptr_to->kind == CHAR && ((!braces && !elided) || starts_string())) {
        init->expr = assign();
        if (init->expr->kind != ND_STR)
            error_at(token->str, "character array requires a string or initializer list");
        if (!tp->array_size) tp->array_size = init->expr->val;
        if (tp->array_size < init->expr->val - 1)
            error_at(token->str, "string initializer is too long");
        if (braces) { consume(","); expect('}'); }
        return init;
    }
    if (!braces && !elided)
        error_at(token->str, "array initializer requires braces");
    if (!braces && !tp->array_size)
        error_at(token->str, "incomplete array requires explicit initializer braces");
    int element_size = size_of(tp->ptr_to);
    while (!at_close() && (!tp->array_size || init->count < tp->array_size)) {
        if (init->count && !consume(",")) break;
        if (at_close()) break;
        if (init->count >= INT_MAX / element_size)
            error_at(token->str, "initializer is too large");
        init->children = realloc(init->children, (init->count + 1) * sizeof(Initializer *));
        init->children[init->count++] = read_initializer(tp->ptr_to, true);
    }
    if (braces) {
        consume(",");
        if (!at_close()) error_at(token->str, "excess array initializer");
        expect('}');
    }
    if (!tp->array_size) {
        if (!init->count) error_at(token->str, "cannot infer an empty array size");
        tp->array_size = init->count;
    }
    return init;
}

static InitValue constant(Node *node);

static InitValue address(Node *node) {
    if (node->kind == ND_GVAR) return (InitValue){node->gvar_name, 0};
    if (node->kind == ND_MEMBER) {
        InitValue value = address(node->lhs);
        value.addend += node->offset;
        return value;
    }
    if (node->kind == ND_DEREF) return constant(node->lhs);
    if (node->kind == ND_STR) return constant(node);
    error_at(token->str, "global object address required");
    return (InitValue){0};
}

static InitValue constant(Node *node) {
    if (node->kind == ND_NUM) return (InitValue){NULL, node->val};
    if (node->kind == ND_SIZE) return (InitValue){NULL, size_of(node->lhs->tp)};
    if (node->kind == ND_ADDR) return address(node->lhs);
    if ((node->kind == ND_GVAR || node->kind == ND_MEMBER) && node->tp->kind == ARRAY)
        return address(node);
    if (node->kind == ND_DEREF && node->tp->kind == ARRAY) return constant(node->lhs);
    if (node->kind == ND_STR) {
        InitString *str = calloc(1, sizeof(InitString));
        char label[48];
        snprintf(label, sizeof(label), ".Linitstr%d", lavel_id++);
        str->label = strdup(label);
        str->bytes = node->str;
        str->size = node->val;
        str->next = strings;
        strings = str;
        return (InitValue){str->label, 0};
    }
    if (node->kind == ND_NOT) {
        InitValue value = constant(node->lhs);
        return (InitValue){NULL, !value.symbol && !value.addend};
    }
    if (node->kind == ND_LOGAND || node->kind == ND_LOGOR) {
        InitValue lhs = constant(node->lhs);
        bool truth = lhs.symbol || lhs.addend;
        if (node->kind == ND_LOGAND && !truth) return (InitValue){NULL, 0};
        if (node->kind == ND_LOGOR && truth) return (InitValue){NULL, 1};
        InitValue rhs = constant(node->rhs);
        return (InitValue){NULL, rhs.symbol || rhs.addend};
    }
    switch (node->kind) {
    case ND_ADD: case ND_SUB: case ND_MUL: case ND_DIV:
    case ND_EQL: case ND_NEQL: case ND_LST: case ND_LSE:
        break;
    default:
        error_at(token->str, "global initializer must be constant");
    }
    InitValue lhs = constant(node->lhs), rhs = constant(node->rhs);
    if (node->kind == ND_ADD && (!lhs.symbol || !rhs.symbol)) {
        if (rhs.symbol) { InitValue tmp = lhs; lhs = rhs; rhs = tmp; }
        if (lhs.symbol) return (InitValue){lhs.symbol, lhs.addend + rhs.addend};
    }
    if (node->kind == ND_SUB && lhs.symbol && !rhs.symbol)
        return (InitValue){lhs.symbol, lhs.addend - rhs.addend};
    if (lhs.symbol || rhs.symbol)
        error_at(token->str, "unsupported symbolic constant expression");
    long long value = 0;
    switch (node->kind) {
    case ND_ADD: value = lhs.addend + rhs.addend; break;
    case ND_SUB: value = lhs.addend - rhs.addend; break;
    case ND_MUL: value = lhs.addend * rhs.addend; break;
    case ND_DIV:
        if (!rhs.addend) error_at(token->str, "division by zero in initializer");
        value = lhs.addend / rhs.addend; break;
    case ND_EQL: value = lhs.addend == rhs.addend; break;
    case ND_NEQL: value = lhs.addend != rhs.addend; break;
    case ND_LST: value = lhs.addend < rhs.addend; break;
    case ND_LSE: value = lhs.addend <= rhs.addend; break;
    default: break;
    }
    if (value < INT_MIN || value > INT_MAX)
        error_at(token->str, "integer initializer overflow");
    return (InitValue){NULL, value};
}

static void write_initializer(GVar *var, Initializer *init, int offset) {
    if (init->tp->kind == ARRAY) {
        if (init->expr) {
            int size = MIN(init->expr->val, size_of(init->tp));
            memcpy(var->init_data + offset, init->expr->str, size);
        } else {
            int stride = size_of(init->tp->ptr_to);
            for (size_t i = 0; i < init->count; i++)
                write_initializer(var, init->children[i], offset + i * stride);
        }
        return;
    }
    InitValue value = constant(init->expr);
    if (init->tp->kind == PTR) {
        Type *actual = decay_type(init->expr->tp);
        if (value.symbol && (!actual || actual->kind != PTR ||
            (init->tp->ptr_to->kind != VOID && actual->ptr_to->kind != VOID &&
             !compatible_type(init->tp, actual))))
            error_at(token->str, "incompatible pointer initializer");
        if (!value.symbol && value.addend)
            error_at(token->str, "pointer initializer must be an address or zero");
    } else if (value.symbol) {
        error_at(token->str, "integer initializer cannot be an address");
    }
    if (value.symbol) {
        Relocation *rel = calloc(1, sizeof(Relocation));
        rel->offset = offset;
        rel->symbol = value.symbol;
        rel->addend = value.addend;
        rel->next = var->relocations;
        var->relocations = rel;
    } else {
        unsigned long long bytes = value.addend;
        for (int i = 0; i < size_of(init->tp); i++) {
            var->init_data[offset + i] = bytes & 255;
            bytes >>= 8;
        }
    }
}

void global_initializer(GVar *var) {
    Initializer *init = read_initializer(var->tp, false);
    var->tp = init->tp;
    var->has_initializer = true;
    var->init_data = calloc(size_of(var->tp), 1);
    write_initializer(var, init, 0);
    Relocation *ordered = NULL;
    while (var->relocations) {
        Relocation *rel = var->relocations;
        var->relocations = rel->next;
        rel->next = ordered;
        ordered = rel;
    }
    var->relocations = ordered;
}

void gen_global_variable(GVar *var) {
    println("\t.balign %d", align_of(var->tp));
    println(".globl %s", var->name);
    println("%s:", var->name);
    int size = size_of(var->tp);
    if (!var->has_initializer) { println("\t.zero %d", size); return; }
    Relocation *rel = var->relocations;
    for (int i = 0; i < size;) {
        if (rel && rel->offset == i) {
            println("\t.quad %s%+lld", rel->symbol, rel->addend);
            rel = rel->next;
            i += 8;
        } else if (!var->init_data[i]) {
            int start = i;
            while (i < size && (!rel || rel->offset != i) && !var->init_data[i]) i++;
            println("\t.zero %d", i - start);
        } else println("\t.byte %u", var->init_data[i++]);
    }
}

void gen_initializer_strings(void) {
    if (!strings) return;
    println(".pushsection .rodata");
    for (InitString *str = strings; str; str = str->next) {
        println("%s:", str->label);
        for (int i = 0; i < str->size; i++) println("\t.byte %u", (unsigned char)str->bytes[i]);
    }
    println(".popsection");
}
