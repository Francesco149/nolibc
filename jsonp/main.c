/*
    This code is public domain and comes with no warranty.
    You are free to do whatever you want with it. You can
    contact me at lolisamurai@tfwno.gf but don't expect any
    support.
    I hope you will find the code useful or at least
    interesting to read. Have fun!
    -----------------------------------------------------------
    This file is part of "nolibc", a compilation of reusable
    code snippets I copypaste and tweak for my libc-free
    linux software.

    DISCLAIMER: these snippets might be incomplete, broken or
                just plain wrong, as many of them haven't had
                the chance to be heavily tested yet.
    -----------------------------------------------------------
    jsonp: a tiny standalone json parser written in C89
*/

typedef i32 b32;

#define internal static
#define globvar  static

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

internal
void memeset(void* dst, u8 value, intptr nbytes)
{
    intptr i;

    if (nbytes % sizeof(intptr) == 0)
    {
        intptr* dst_chunks = (intptr*)dst;
        intptr chunk;
        u8* chunk_raw = (u8*)&chunk;

        for (i = 0; i < sizeof(intptr); ++i) {
            chunk_raw[i] = value;
        }

        for (i = 0; i < nbytes / sizeof(intptr); ++i) {
            dst_chunks[i] = chunk;
        }
    }
    else
    {
        u8* dst_bytes = (u8*)dst;

        for (i = 0; i < nbytes; ++i) {
            dst_bytes[i] = value;
        }
    }
}

/* ------------------------------------------------------------- */

void* syscall1(uintptr number, void* arg1);

void* syscall3(
    uintptr number,
    void* arg1,
    void* arg2,
    void* arg3
);

#define stdin  0
#define stdout 1

#define O_RDONLY 00

typedef u32 mode_t;

internal
int open(char const* filename, u32 flags, mode_t mode)
{
    return (int)(intptr)
        syscall3(
            SYS_open,
            (void*)filename,
            (void*)(intptr)flags,
            (void*)(intptr)mode
        );
}

internal
void close(int fd) {
    syscall1(SYS_close, (void*)(intptr)fd);
}

internal
intptr write(int fd, void const* data, uintptr nbytes)
{
    return (uintptr)
        syscall3(
            SYS_write,
            (void*)(intptr)fd,
            (void*)data,
            (void*)nbytes
        );
}

internal
intptr read(int fd, void* data, intptr nbytes)
{
    return (intptr)
        syscall3(
            SYS_read,
            (void*)(intptr)fd,
            data,
            (void*)nbytes
        );
}

/* ------------------------------------------------------------- */

internal
uintptr strlen(char const* str)
{
    char const* p;
    for (p = str; *p; ++p);
    return p - str;
}

internal
uintptr puts(char const* str) {
    return write(stdout, str, strlen(str));
}

#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)

#define log(type, msg) \
    puts(type "|"), \
    puts(__FUNCTION__), \
    puts("|" __FILE__ ":" STRINGIFY(__LINE__) "|" msg)

#define err(msg) log("ERR", msg)
#define info(msg) log("INFO", msg)

#if LOG_DEBUG
#define debug(msg) log("DEBUG", msg)
#else
#define debug(msg)
#endif

internal
b32 strneq(char const* a, char const* b, intptr len)
{
    intptr i;

    for (i = 0; i < len; ++i)
    {
        if (a[i] != b[i]) {
            return 0;
        } else if (!a[i]) {
            return 1;
        }
    }

    return 1;
}

internal
b32 is_whitespace(char c)
{
    return c == ' ' ||
           c == '\r' ||
           c == '\n' ||
           c == '\t' ||
           c == '\f' ||
           c == '\v';
}

/* -------------------------------------------------------------
                         UTILITY FUNCS
   ------------------------------------------------------------- */

typedef struct
{
    char const* start;
    char const* end;
}
string_t;

#if !BENCHMARK
internal
uintptr string_len(string_t const str) {
    return str.end - str.start;
}

internal
uintptr string_put(string_t const str) {
    return write(stdout, str.start, string_len(str));
}
#endif

internal
b32 string_valid(string_t const str) {
    return str.start && str.end && str.end >= str.start;
}

internal
b32 string_neq(string_t const a, char const* b, uintptr n) {
    return strneq(a.start, b, n);
}

internal
void string_put_nows(string_t const s)
{
    char const* p;
    char buf[64];
    char* pbuf = buf;

    for (p = s.start; p < s.end; ++p)
    {
        if (!is_whitespace(*p)) {
            *pbuf++ = *p;
        }

        if (pbuf >= buf + 64) {
            write(stdout, buf, 64);
            pbuf = buf;
        }
    }

    write(stdout, buf, pbuf - buf);
}

internal
void string_print_indicator(
    string_t const str,
    char const* p,
    uintptr n)
{
    string_t s;

    s.start = s.end = p;

    s.start = max(str.start, p - n);
    s.end = min(str.end, p + n);

    string_put_nows(s);
    puts("\n");

    for (; s.start < s.end; ++s.start)
    {
        if (s.start == p) {
            puts("^");
            break;
        }

        if (!is_whitespace(*s.start)) {
            puts(" ");
        }
    }
}

/* -------------------------------------------------------------
                        MEMORY MANAGEMENT
   ------------------------------------------------------------- */

typedef struct
{
    u8* start;
    u8* end;
    u8* p;
}
allocator_t;

internal
void* allocator_push(allocator_t* alloc, uintptr nbytes)
{
    u8* res;

    if (alloc->p + nbytes >= alloc->end)
    {
        err("out of memory! please recompile with more memory "
            "reserved or check your code for misbehaving "
            "allocator usage.");

        return 0;
    }

    res = alloc->p;
    alloc->p += nbytes;

    return res;
}

/*
internal
void allocator_pop(allocator_t* alloc, uintptr nbytes)
{
    if (alloc->p - nbytes < alloc->start)
    {
        err("misbehaving deallocation tried to free past the start"
            " of the memory buffer");

        return;
    }

    alloc->p -= nbytes;
}
*/

/* ------------------------------------------------------------- */

globvar u8 memory[0xFFFF];

globvar allocator_t alloc = {
    memory, memory + sizeof(memory), memory
};

/* -------------------------------------------------------------
                              JSONP
   ------------------------------------------------------------- */

#define JSON_INVALID 0
#define JSON_ROOT    1

#define JSON_ARRAY         2
#define JSON_OBJECT        3
#define JSON_PAIR          4
#define JSON_ARRAY_ELEMENT 5

#define JSON_STRING  6
#define JSON_NUMBER  7
#define JSON_TRUE    8
#define JSON_FALSE   9
#define JSON_NULL    10

typedef struct _json_node
{
    u8 type;
    string_t raw;
    struct _json_node* child;
    struct _json_node* next;
}
json_node;

#if JSON_DEBUG
internal
void print_json_node(json_node const* n)
{
    puts("-------------------------------------------------------"
         "------------\n");
    switch(n->type)
    {
#define t(x) case x: puts(#x "\n"); break;
        t(JSON_INVALID)
        t(JSON_ROOT)
        t(JSON_ARRAY)
        t(JSON_OBJECT)
        t(JSON_PAIR)
        t(JSON_ARRAY_ELEMENT)
        t(JSON_STRING)
        t(JSON_NUMBER)
        t(JSON_TRUE)
        t(JSON_FALSE)
        t(JSON_NULL)
#undef t
    }

    string_put(n->raw);
    puts("\n");
}
#endif

internal
void jsonp_init(json_node* tree, string_t str)
{
    memeset(tree, 0, sizeof(json_node));
    tree->raw = str;
    tree->type = JSON_ROOT;
}

internal
char jsonp_peekch(string_t* p)
{
    for (; is_whitespace(*p->start); ++p->start)
    {
        if (!string_valid(*p)) {
            debug("string not valid (reached end?)");
            return 0;
        }
    }

    return *p->start;
}

internal
char jsonp_nextch(string_t* p)
{
    char c = jsonp_peekch(p);
    ++p->start;
    return c;
}

internal
b32 jsonp_string(string_t* p, string_t* pstr)
{
    b32 escaped = 0;

    if (jsonp_nextch(p) != '"') {
        debug("didn't find opening double quote\n");
        return 0;
    }

    pstr->start = p->start;

    while (string_valid(*p))
    {
        char c = jsonp_nextch(p);

        if (!escaped && c == '"') {
            pstr->end = p->start - 1;
            return 1;
        }

        if (escaped)
        {
            escaped = 0;

            if (c == 'u') {
                p->start += 4;
                /* TODO: validate hex and remaining size */
            }
        } else {
            escaped = c == '\\';
        }
    }

    debug("didn't find closing double quote\n");

    return 0;
}

#define indicator(p) \
    string_print_indicator(parent->raw, p, 33), puts("\n")

#if LOG_DEBUG
#define jdebug(msg) debug(msg), debug_indicator(pc)
#else
#define jdebug(msg)
#endif

#define jerr(msg) err(msg), indicator(it.start - 1)

internal
b32 jsonp_walk(json_node* parent)
{
    string_t it = parent->raw;
    json_node* node;

    /* TODO: pull the allocator calls into a generic func */
    node = (json_node*)allocator_push(&alloc, sizeof(json_node));
    memeset(node, 0, sizeof(node));
    node->raw = parent->raw;
    parent->child = node;

    switch (parent->type)
    {
        case JSON_ROOT:
            switch (jsonp_nextch(&it))
            {
                case '{': node->type = JSON_OBJECT; break;
                case '[': node->type = JSON_ARRAY;  break;

                default:
                    jerr("expected object or array\n");
                    return 0;
            }

            if (!jsonp_walk(node)) { /* RECURSION */
                return 0;
            }
            it.start = node->raw.end;
            break;

        case JSON_OBJECT:
        case JSON_ARRAY:
        {
            json_node* prev = parent->child;
            char delim;
            u8 type;

            if (parent->type == JSON_OBJECT)
            {
                delim = '}';
                type = JSON_PAIR;
            }
            else
            {
                delim = ']';
                type = JSON_ARRAY_ELEMENT;
            }

            jsonp_nextch(&it); /* skip opening bracket */

            while (1)
            {
                char c;

                node->type = type;

                node->raw = it;
                if (!jsonp_walk(node)) { /* BRANCHING RECURSION */
                    return 0;
                }

                it.start = node->raw.end;
                c = jsonp_nextch(&it);

                if (c == delim) {
                    break;
                }

                if (c != ',') {
                    jerr("expected ','\n");
                    return 0;
                }

                node = (json_node*)
                    allocator_push(
                        &alloc,
                        sizeof(json_node)
                    );

                memeset(node, 0, sizeof(node));
                prev->next = node;
                prev = node;

                node->raw.start = it.start;
                node->raw.end = parent->raw.end;
            }

            parent->raw.end = it.start;
            break;
        }

        /* TODO: I'd like to be able to fit this switch case in
                 one screenful. shorten this code, maybe pull
                 stuff out? */
        case JSON_PAIR:
        case JSON_ARRAY_ELEMENT:
        {
            char const* pc;

            /* TODO: pull number checking into a separate func */
            /* TODO: use a bitmask? */
            b32 has_dot = 0;
            b32 has_e = 0;
            b32 has_number = 0;
            b32 no_more_numbers = 0;

            if (parent->type == JSON_PAIR)
            {
                node->type = JSON_STRING;

                if (!jsonp_string(&it, &node->raw)) {
                    jerr("expected string\n");
                    return 0;
                }

                node = (json_node*)
                    allocator_push(&alloc, sizeof(json_node));

                memeset(node, 0, sizeof(node));
                node->raw = parent->raw;
                parent->child->next = node;

                if (jsonp_nextch(&it) != ':') {
                    jerr("expected ':'\n");
                    return 0;
                }
            }

            /* skip whitespace so we can strcmp */
            jsonp_peekch(&it);

            /* parse value */
            node->raw.start = it.start;

            if (jsonp_peekch(&it) == '{')
            {
                node->type = JSON_OBJECT;

                if (!jsonp_walk(node)) { /* RECURSION */
                    return 0;
                }

                parent->raw.end = node->raw.end;
                break;
            }

            if (jsonp_peekch(&it) == '[')
            {
                node->type = JSON_ARRAY;

                if (!jsonp_walk(node)) { /* RECURSION */
                    return 0;
                }

                parent->raw.end = node->raw.end;
                break;
            }

            /* TODO: reduce redundancy? */
            if (string_neq(it, "true", 4))
            {
                node->type = JSON_TRUE;
                parent->raw.end = node->raw.end = it.start + 4;
                break;
            }

            if (string_neq(it, "false", 5)) {
                node->type = JSON_FALSE;
                parent->raw.end = node->raw.end = it.start + 5;
                break;
            }

            if (string_neq(it, "null", 4)) {
                node->type = JSON_NULL;
                parent->raw.end = node->raw.end = it.start + 4;
                break;
            }

            /* this is too much indentation. I need to pull
               this out into a function */
            for (pc = it.start; pc != it.end; ++pc)
            {
                if (*pc == '.')
                {
                    /* TODO: don't accept 123. */

                    if (has_dot) {
                        /* 123.123. */
                        jdebug("multiple dots\n");
                        break;
                    }

                    if (has_e) {
                        /* 123e1.23 */
                        jdebug("dot after e\n");
                        break;
                    }

                    if (!has_number) {
                        /* .123 */
                        jdebug("dot before number\n");
                        break;
                    }

                    has_dot = 1;
                    continue;
                }

                if (*pc == 'e' || *pc == 'E')
                {
                    if (has_e) {
                        /* 123e123e */
                        jdebug("multiple e's\n");
                        break;
                    }

                    if (!has_number) {
                        /* e123 */
                        jdebug("e before number\n");
                        break;
                    }

                    if (pc + 1 != it.end)
                    {
                        if (pc[1] == '+' || pc[1] == '-') {
                            ++pc;
                        }
                    }

                    has_e = 1;
                    continue;
                }

                if (*pc < '0' || *pc > '9')
                {
                    b32 is_terminator =
                        *pc == ',' || *pc == '}' || *pc == ']';

                    if (is_whitespace(*pc))
                    {
                        no_more_numbers = has_number;
                        continue;
                    }

                    if (!is_terminator)
                    {
                        jdebug("invalid char within number\n");
                        break;
                    }

                    if (!has_number) {
                        jdebug("terminator with no number\n");
                        break;
                    }

                    node->type = JSON_NUMBER;
                    parent->raw.end = node->raw.end = pc;
                    goto done;
                }

                if (no_more_numbers) {
                    jdebug("number broken by whitespace\n");
                    break;
                }

                has_number = 1;
            }

            if (jsonp_string(&it, &node->raw))
            {
                node->type = JSON_STRING;
                parent->raw.end = node->raw.end + 1;
                break;
            }

            jerr("invalid pair value\n");
            return 0;
        }

        default:
            jerr("this type of node can't have children, wtf?\n");
            return 0;
    }

done:
#if JSON_DEBUG
    print_json_node(node);
    print_json_node(parent);
#endif

    return 1;
}

#undef jdebug
#undef jerr

#if !BENCHMARK
internal
b32 jsonp_print(json_node const* parent, u32 level)
{
    u32 i;
    json_node const* child;

    for (i = 0; i < level; ++i) {
        puts("  ");
    }

    switch (parent->type)
    {
        case JSON_INVALID:
            err("found node with invalid type");
            return 0;

        case JSON_ROOT:          puts("(root node)");     break;
        case JSON_ARRAY:         puts("array");           break;
        case JSON_OBJECT:        puts("object");          break;
        case JSON_PAIR:          puts("pair");            break;
        case JSON_ARRAY_ELEMENT: puts("(array element)"); break;

        case JSON_STRING:
            puts("string: ");
            goto truefalsenull;

        case JSON_NUMBER:
            puts("number: ");

        case JSON_TRUE:
        case JSON_FALSE:
        case JSON_NULL:
truefalsenull:
            string_put(parent->raw);
            break;
    }

    puts("\n");

    for (child = parent->child; child; child = child->next)
    {
        if (!jsonp_print(child, level + 1)) {
            return 0;
        }
    }

    return 1;
}
#endif

/* ------------------------------------------------------------- */

#define JSON_MAX_SIZE 0xFFFF

internal
b32 jsonp_test(char const* file_name)
{
    int fd;
    intptr nread;
    char json_buf[JSON_MAX_SIZE];
    string_t tok;
    json_node root;
#if BENCHMARK
    int i;
#endif

    if (*file_name == '-' && !file_name[1]) {
        fd = stdin;
    } else {
        fd = open(file_name, O_RDONLY, 0);
        if (fd < 0) {
            err("open");
            return 0;
        }
    }

    nread = read(fd, json_buf, JSON_MAX_SIZE - 1);

    if (nread == JSON_MAX_SIZE - 1) {
        err("file too big");
        return 0;
    }
    else if (nread < 0) {
        err("read");
        return 0;
    }

    if (fd) {
        close(fd);
    }

    fd = 1;

    tok.start = json_buf;
    tok.end = json_buf + nread;

    jsonp_init(&root, tok);

#if BENCHMARK
    for (i = 0; i < 1000000; ++i)
    {
        alloc.p = alloc.start;
#endif

        if (!jsonp_walk(&root)) {
            puts("failed to parse\n");
            return 0;
        }

#if !BENCHMARK
        jsonp_print(&root, 0);
#else
    }
#endif

    return 1;
}

int main(int argc, char const* argv[])
{
    int i;

    for (i = 0; i < argc; ++i) {
        puts(argv[i]);
        puts(" ");
    }

    puts("\n\n");

    if (argc != 2)
    {
        puts("Usage: ");
        puts(argv[0]);
        puts(" <file_name (- for stdin)>");

        return 1;
    }

    if (!jsonp_test(argv[1])) {
        return 1;
    }

    return 0;
}
