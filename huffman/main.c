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
    huffman: implementation of file compression using huffman
             coding
*/

typedef i32 b32;

#define internal static
#define globvar static

#define min(a, b) ((a) < (b) ? (a) : (b))

void* syscall1(uintptr number, void* arg1);

void* syscall3(
    uintptr number,
    void* arg1,
    void* arg2,
    void* arg3
);

void* syscall6(
    uintptr number,
    void* arg1,
    void* arg2,
    void* arg3,
    void* arg4,
    void* arg5,
    void* arg6
);

/* ------------------------------------------------------------- */

internal
void memecpy(void* dst, void const* src, intptr nbytes)
{
    intptr i;
    u8* dst_bytes;
    u8* src_bytes;

    if (nbytes / sizeof(intptr))
    {
        intptr* dst_chunks = (intptr*)dst;
        intptr* src_chunks = (intptr*)src;

        for (i = 0; i < nbytes / sizeof(intptr); ++i) {
            dst_chunks[i] = src_chunks[i];
        }

        nbytes %= sizeof(intptr);
        dst = &dst_chunks[i];
        src = &src_chunks[i];
    }

    dst_bytes = (u8*)dst;
    src_bytes = (u8*)src;

    for (i = 0; i < nbytes; ++i) {
        dst_bytes[i] = src_bytes[i];
    }
}

internal
void memeset(void* dst, u8 value, intptr nbytes)
{
    intptr i;
    u8* dst_bytes;

    if (nbytes / sizeof(intptr))
    {
        intptr* dst_chunks = (intptr*)dst;
        intptr chunk;
        u8* raw_chunk = (u8*)&chunk;

        for (i = 0; i < sizeof(intptr); ++i) {
            raw_chunk[i] = value;
        }

        for (i = 0; i < nbytes / sizeof(intptr); ++i) {
            dst_chunks[i] = chunk;
        }

        nbytes %= sizeof(intptr);
        dst = &dst_chunks[i];
    }

    dst_bytes = (u8*)dst;

    for (i = 0; i < nbytes; ++i) {
        dst_bytes[i] = value;
    }
}

/* ------------------------------------------------------------- */

#define stdin 0

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

#define PROT_READ  0x1
#define PROT_WRITE 0x2

#define MAP_PRIVATE   0x02
#define MAP_ANONYMOUS 0x20

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

internal
void* mmap(
    void* addr,
    uintptr nbytes,
    int prot,
    int flags,
    int fd,
    u32 offset)
{
#ifdef LEGACY_MMAP
    void* params[7];

    offset = (offset + PAGE_SIZE / 2) / PAGE_SIZE;
    offset *= PAGE_SIZE;
    nbytes = (nbytes + PAGE_SIZE / 2) / PAGE_SIZE;
    nbytes *= PAGE_SIZE;

    params[0] = addr;
    params[1] = (void*)(intptr)nbytes;
    params[2] = (void*)(intptr)prot;
    params[3] = (void*)(intptr)flags;
    params[4] = (void*)(intptr)fd;
    params[5] = (void*)(intptr)offset;
    params[6] = 0;

    return syscall1(SYS_mmap, (void*)params);
#else
    return
        syscall6(
            SYS_mmap,
            addr,
            (void*)nbytes,
            (void*)(intptr)prot,
            (void*)(intptr)flags,
            (void*)(intptr)fd,
            (void*)(intptr)offset
        );
#endif
}

/* ------------------------------------------------------------- */

#define FD_BUFSIZE 8000

typedef struct
{
    int handle;
    b32 flush_every_line;
    uintptr buf_nbytes;
    char buf[FD_BUFSIZE];
}
fd_t;

/*
internal
int fopen(fd_t* fd, char const* filename, u32 flags, mode_t mode)
{
    fd->handle = open(filename, flags, mode);
    fd->buf_nbytes = 0;
    fd->flush_every_line = 0;

    return fd->handle;
}
*/

internal
intptr fflush(fd_t* fd)
{
    intptr res =
        fd->buf_nbytes ? write(fd->handle, fd->buf, fd->buf_nbytes)
                       : 0;

    fd->buf_nbytes = 0;

    return res;
}

/*
internal
void fclose(fd_t* fd)
{
    fflush(fd);
    close(fd->handle);
    fd->handle = 0;
}
*/

internal
intptr fwrite(fd_t* fd, void const* data, uintptr nbytes)
{
    u8* p = (u8*)data;
    uintptr total = nbytes;

    while (nbytes)
    {
        intptr res;
        b32 should_flush = 0;

        uintptr to_write =
            min(nbytes, FD_BUFSIZE - fd->buf_nbytes);

        memecpy(fd->buf + fd->buf_nbytes, p, to_write);
        fd->buf_nbytes += to_write;

        if (fd->buf_nbytes >= FD_BUFSIZE) {
            should_flush = 1;
        }

        else if (fd->flush_every_line)
        {
            uintptr i;

            for (i = 0; i < to_write; ++i)
            {
                if (p[i] == '\n' || p[i] == '\r') {
                    should_flush = 1;
                    break;
                }
            }
        }

        if (should_flush)
        {
            uintptr to_flush = fd->buf_nbytes;

            res = fflush(fd);
            if (res != to_flush) {
                return res;
            }
        }

        nbytes -= to_write;
        p += to_write;
    }

    return total;
}

#define fread(fd, data, nbytes) read(fd->handle, data, nbytes)

globvar fd_t stdout_;
globvar fd_t* stdout = &stdout_;

globvar fd_t stderr_;
globvar fd_t* stderr = &stderr_;

internal
void stdinit()
{
    stdout->handle = 1;
    stdout->flush_every_line = 1;
    stderr->handle = 2;
    stderr->flush_every_line = 1;
}

internal
void stdfflush()
{
    fflush(stdout);
    fflush(stderr);
}

/* ------------------------------------------------------------- */

internal
uintptr strlen(char const* str)
{
    char const* p;
    for (p = str; *p; ++p);
    return p - str;
}

#ifdef HUFFMAN_DEBUG
intptr uitoa(
    u8 base,
    uintptr val,
    char* buf,
    intptr width,
    char filler)
{
    char c;
    char* p = buf;
    intptr res;

    if (!base) {
        return 0;
    }

    if (base > 16) {
        return 0;
    }

    do
    {
        u8 digit = val % base;
        val /= base;
        *p++ = "0123456789abcdef"[digit];
    }
    while(val);

    while (p - buf < width) {
        *p++ = filler;
    }

    res = p - buf;
    *p-- = 0;

    while (p > buf)
    {
        /* flip the string */
        c = *p;
        *p-- = *buf;
        *buf++ = c;
    }

    return res;
}
#endif

#define fputs(fd, str) fwrite(fd, str, strlen(str))
#define puts(str) fputs(stdout, str)
#define errs(str) fputs(stderr, str)

/* ------------------------------------------------------------- */

internal
void swap_intptr(intptr* a, intptr* b)
{
    intptr tmp = *a;
    *a = *b;
    *b = tmp;
}

typedef b32 qsort_intptr_cmp(intptr a, intptr b);

internal
uintptr qsort_intptr_partition(
    intptr* arr,
    qsort_intptr_cmp* cmp,
    uintptr lo,
    uintptr hi)
{
    uintptr pivot = arr[hi];
    uintptr i = lo - 1;
    uintptr j;

    for (j = lo; j <= hi - 1; ++j)
    {
        if (cmp(arr[j], pivot))
        {
            ++i;
            swap_intptr(&arr[i], &arr[j]);
        }
    }

    swap_intptr(&arr[++i], &arr[hi]);

    return i;
}

internal
void qsort_intptr(
    intptr* arr,
    qsort_intptr_cmp* cmp,
    uintptr lo, uintptr hi)
{
    uintptr p;

    if (lo >= hi) {
        return;
    }

    p = qsort_intptr_partition(arr, cmp, lo, hi);
    qsort_intptr(arr, cmp, lo, p - 1);
    qsort_intptr(arr, cmp, p + 1, hi);
}

/* ------------------------------------------------------------- */

#define HUFFMAN_NULL_VAL 0x1000

typedef struct huffman_node_
{
    u16 val;
    uintptr weight;
    struct huffman_node_* parent;
    struct huffman_node_* child[2];
}
huffman_node;

typedef struct
{
    u16 value;
    u8 nbits;
    char character;
}
huffman_code;

#ifdef HUFFMAN_DEBUG
internal
void huffman_print_tree(
    huffman_node const* n,
    uintptr level,
    uintptr code,
    u8 codelen)
{
    uintptr i;
    char buf[21];

    for (i = 0; i < level; ++i) {
        errs("  ");
    }

    errs("(");

    if (n->val != HUFFMAN_NULL_VAL)
    {
        errs("'");

        if (n->val >= 0x20 && n->val <= 0x7E) {
            fwrite(stderr, &n->val, 1);
        } else {
            errs("\\x");
            uitoa(16, n->val, buf, 2, '0');
            errs(buf);
        }

        errs("' ");
    }

    errs("weight: ");
    uitoa(10, n->weight, buf, 0, 0);
    errs(buf);
    errs(")");

    if (n->val != HUFFMAN_NULL_VAL)
    {
        errs(" <-- ");
        uitoa(2, code, buf, codelen, '0');
        errs(buf);
    }

    errs("\n");

    if (n->val != HUFFMAN_NULL_VAL) {
        return;
    }

    if (codelen >= 15) {
        errln("FUCK");
        return;
    }

    /* !BRANCHING RECURSION! */

    huffman_print_tree(
        n->child[0],
        level + 1,
        code << 1,
        codelen + 1
    );

    huffman_print_tree(
        n->child[1],
        level + 1,
        (code << 1)|1,
        codelen + 1
    );
}
#else
#define huffman_print_tree(n, level, code, codelen)
#endif

internal
void huffman_walk_tree(
    huffman_node const* n,
    huffman_code* codes,
    uintptr level,
    uintptr code,
    u8 codelen)
{
    if (n->val != HUFFMAN_NULL_VAL)
    {
        huffman_code* c = &codes[n->val];
        c->value = code;
        c->nbits = codelen;
        return;
    }

    /* !BRANCHING RECURSION! */

    huffman_walk_tree(
        n->child[0],
        codes,
        level + 1,
        code << 1,
        codelen + 1
    );

    huffman_walk_tree(
        n->child[1],
        codes,
        level + 1,
        (code << 1)|1,
        codelen + 1
    );
}

internal
b32 huffman(u8 const* data, intptr n, huffman_code* codes)
{
    huffman_node tree[0x200];
    huffman_node* nodes[2];
    huffman_node* newnode = 0;
    huffman_node* root = 0;
    uintptr i, j;

    memeset(tree, 0, sizeof(tree));

    /* frequencies */
    for (i = 0; i < 0x100; ++i) {
        codes[i].character = (char)i;
        tree[i].val = i;
    }

    for (i = 0; i < n; ++i) {
        ++tree[data[i]].weight;
    }

    while (1)
    {
        /*
            find 2 lowest weight nodes
            also store the first uninitalized node
        */
        memeset(nodes, 0, sizeof(nodes));
        newnode = 0;

        for (j = 0; j < 2; ++j)
        {
            for (i = 0; i < 0x200; ++i)
            {
                huffman_node* n = &tree[i];

                if (j && n == nodes[0]) {
                    continue;
                }

                if (!n->weight)
                {
                    /* uninitialized */

                    if (!newnode)
                    {
                        newnode = n;
                        n->val = HUFFMAN_NULL_VAL;
                        /* reuses nodes that had 0 frequency */
                    }

                    continue;
                }

                if (n->parent) {
                    /* already used */
                    continue;
                }

                if (!nodes[j] || n->weight < nodes[j]->weight)
                {
                    /* store lowest weight node and make sure to
                       not select the same node twice */
                    nodes[j] = n;
                }
            }

            if (!nodes[j]) {
                break;
            }
        }

        /* no more unused nodes? we're done */
        if (!nodes[0] || !nodes[1]) {
            break;
        }

        if (!newnode) {
            return 0;
        }

        /* join the two lowest weight nodes with a new node */
        newnode->weight = nodes[0]->weight + nodes[1]->weight;
        memecpy(newnode->child, nodes, sizeof(nodes));
        nodes[0]->parent = newnode;
        nodes[1]->parent = newnode;

        root = newnode;
    }

    if (!root) {
        return 0;
    }

    huffman_print_tree(root, 0, 0, 0);
    huffman_walk_tree(root, codes, 0, 0, 0);

    return 1;
}

/* sort by code length first and by alphabetical order second */
b32 huffman_code_cmp(huffman_code* a, huffman_code* b)
{
    if (a->nbits < b->nbits) {
        return 1;
    }

    return a->nbits == b->nbits && a->character <= b->character;
}

#ifdef HUFFMAN_DEBUG
#define print_codes()                           \
    for (i = 0; i < 0x100; ++i)                  \
    {                                           \
        char buf[17];                           \
        huffman_code* c = index[i];             \
        u8 ch = (u8)(c - codes);                \
                                                \
        if (!c->nbits) {                        \
            continue;                           \
        }                                       \
                                                \
        if (ch >= 0x20 && ch <= 0x7E) {         \
            errs("   ");                        \
        }                                       \
                                                \
        errs("'");                              \
                                                \
        if (ch >= 0x20 && ch <= 0x7E) {         \
            fwrite(stderr, &ch, 1);             \
        } else {                                \
            errs("\\x");                        \
            uitoa(16, (uintptr)ch, buf, 2, '0');\
            errs(buf);                          \
        }                                       \
                                                \
        errs("': ");                            \
                                                \
        uitoa(2, c->value, buf, c->nbits, '0'); \
        errs(buf);                              \
                                                \
        uitoa(10, c->nbits, buf, 0, 0);         \
        errs(" (");                             \
        errs(buf);                              \
        errs(" bits)");                         \
        errs("\n");                             \
    }
#endif

internal
void huffman_canonical_from_lengths(
    u8* lengths,
    huffman_code* codes)
{
    uintptr i;
    huffman_code* index[0x100];
    u16 curcode = 0;
    u8 curnbits;

    for (i = 0; i < 0x100; ++i)
    {
        codes[i].character = (char)i;
        codes[i].nbits = lengths[i];
        index[i] = &codes[i];
    }

    qsort_intptr(
        (intptr*)index,
        (qsort_intptr_cmp*)huffman_code_cmp,
        0, 0xFE
    );

    for (i = 0; !index[i]->nbits; ++i);

    curnbits = index[i]->nbits;

    for (; i < 0x100; ++i)
    {
        huffman_code* c = index[i];

        if (c->nbits != curnbits) {
            curcode <<= c->nbits - curnbits;
            curnbits = c->nbits;
        }

        c->value = curcode;

        ++curcode;
    }

#ifdef HUFFMAN_DEBUG
    errs("Reconstructed codes:\n");
    print_codes()
#endif
}

internal
void huffman_canonical(huffman_code* codes)
{
    u8 lengths[0x100];
    uintptr i;

    for (i = 0; i < 0x100; ++i) {
        lengths[i] = codes[i].nbits;
    }

    huffman_canonical_from_lengths(lengths, codes);
}

internal
b32 huffman_encode(
    u8 const* data,
    u8* compressed,
    uintptr n,
    huffman_code* codes,
    uintptr* pnbytes,
    u8* premainder_bits)
{
    uintptr i;

    u8* p = compressed;
    u8 bits_left = 8;

    for (i = 0; i < n; ++i)
    {
        huffman_code* c = &codes[data[i]];
        u8 code_len;
        u16 value;

        if (!c->nbits) {
            return 0;
        }

        code_len = c->nbits;

        /* move the value to the leftmost bit */
        value = c->value << (16 - c->nbits);

        /*
            c->nbits = 4;
            c->value = 00000000_00001101
            value    = 11010000_00000000
                       ^--^
                         the actual code
        */

        /* pack the bits disregarding byte alignment */
        while (code_len)
        {
            u16 mask;
            u8 nbits;

            if (!bits_left)
            {
                ++p;
                bits_left = 8;
            }

            nbits = bits_left < code_len ? bits_left : code_len;

            /* set the first nbits of the mask */
            mask = 0xFF00 >> nbits;
            mask &= 0xFF;
            mask <<= 8;

            /*
                example:
                3 bits left
                mask = 11100000_00000000

                we need to encode the first nbits bits of
                the code into the next nbits bits of the
                current byte
            */

            *p |= (value & mask) >> (16 - bits_left);

            value <<= nbits;
            code_len -= nbits;

            bits_left -= nbits;
        }
    }

    *pnbytes = p - compressed;
    *premainder_bits = 8 - bits_left;

    return 1;
}

internal
b32 huffman_decode(
    u8 const* data,
    u8* uncompressed,
    uintptr n,
    huffman_code* codes)
{
    uintptr i = 0, j;

    u8* p = uncompressed;
    u8 bits_offset = 0;

    while (p - uncompressed < n)
    {
        huffman_code* match = 0;
        u32* p32 = (u32*)&data[i];
        u32 chunk;

        /* TODO: fix this endian-ness mess lol */
        chunk = *p32 >> 24 | *p32 << 24 |
              ((*p32 >> 8) & 0x0000FF00) |
              ((*p32 << 8) & 0x00FF0000);

        for (j = 0; j < 0x100; ++j)
        {
#ifdef HUFFMAN_DEBUG
            uintptr k;
            char buf[33];
#endif
            huffman_code* c = &codes[j];
            u16 value = c->value << (16 - c->nbits);
            u32 mask = 0xFFFF0000 >> c->nbits;

            if (!c->nbits) {
                continue;
            }

#ifdef HUFFMAN_DEBUG
            uitoa(2, chunk, buf, 32, '0');
            errs(buf);
            errs("\n");

            for (k = 0; k < bits_offset; ++k) {
                errs(" ");
            }

            errs("^");

            for (k = 0; k < c->nbits - 2; ++k) {
                errs(" ");
            }

            errs("^\n");

            uitoa(2, (u16)mask, buf, 16, '0');
            errs(buf);
            errs("\n");

            uitoa(2, value, buf, c->nbits, '0');
            errs(buf);
            errs("\n");

            errs("i = ");
            uitoa(10, i, buf, 0, '0');
            errs(buf);
            errs("\n");

            errs("written = ");
            uitoa(10, p - uncompressed, buf, 0, '0');
            errs(buf);
            errs("\n\n");
#endif

            /* TODO: make this readable */
            if (
                ((u16)((chunk << bits_offset) >> 16) & mask) ==
                (value & (u16)mask)
            ){
                match = c;
                break;
            }
        }

        if (!match) {
            /* invalid code */
            return 0;
        }

        *p++ = (u8)j;

        i += (match->nbits + bits_offset) / 8;
        bits_offset = (match->nbits + bits_offset) % 8;
    }

    return 1;
}

typedef struct
{
    u8 buf        [8000000];
    u8 transformed[8000000];
    huffman_code codes[0x100];

    /* codes array is 1 bigger than it should because lengths are
       not aligned to byte boundary and the loop would end up
       accessing oob stuff for the last length */
}
memory;

int main(int argc, char const* argv[])
{
    int fd;
    char const* filename;

    memory* m =
        mmap(
            0,
            sizeof(memory),
            PROT_READ | PROT_WRITE,
            MAP_ANONYMOUS | MAP_PRIVATE,
            0, 0
        );

    uintptr transformed_nbytes;
    u8 transformed_remainder_bits = 0;

    intptr n;
    b32 result;

    if ((intptr)m < 0 && (intptr)m > -100000) {
        errs("mmap failed\n");
        return 1;
    }

    stdinit();

    memeset(m->codes, 0, sizeof(m->codes));

    if (argc != 3)
    {
usage:
        errs("Usage: ");
        errs(argv[0]);
        errs(" <mode (x/c)> <filename (- for stdin)>\n");

        return 1;
    }

    filename = argv[2];

    if (*filename == '-' && !filename[1]) {
        fd = stdin;
    } else {
        fd = open(filename, O_RDONLY, 0);
        if (fd < 0) {
            errs("open\n");
            return 1;
        }
    }

    n = read(fd, m->buf, sizeof(m->buf));
    if (n <= 0) {
        errs("read\n");
        return 1;
    }
    if (n >= sizeof(m->buf)) {
        errs("too big\n");
    }

    if (fd != stdin) {
        close(fd);
    }

    switch (*argv[1])
    {
        case 'c':
        {
            uintptr i;
            u8 lengths[128];

            if (!huffman(m->buf, n, m->codes)) {
                errs("????????????????\n");
                return 1;
            }

            huffman_canonical(m->codes);

            for (i = 0; i < 128; ++i)
            {
                lengths[i]  = m->codes[i * 2    ].nbits << 4;
                lengths[i] |= m->codes[i * 2 + 1].nbits & 0xF;
            }

            /* TODO: support files bigger than 4GB? */
            fwrite(stdout, &n, 4);
            fwrite(stdout, lengths, 128);

            result =
                huffman_encode(
                    m->buf,
                    m->transformed,
                    n,
                    m->codes,
                    &transformed_nbytes,
                    &transformed_remainder_bits
                );

            if (!result) {
                errs("????? invalid code??????\n");
                return 1;
            }
            break;
        }

        case 'x':
        {
            uintptr i;
            u8 unp_lengths[0x100];
            u32 original_size = 0;
            u8* p = m->buf;

            transformed_nbytes = original_size = *(u32*)p;
            p += 4;

            for (i = 0; i < 128; ++i)
            {
                unp_lengths[i * 2    ] = *p    >> 4;
                unp_lengths[i * 2 + 1] = *p++ & 0xF;
            }

            huffman_canonical_from_lengths(unp_lengths, m->codes);

            result =
                huffman_decode(
                    p,
                    m->transformed,
                    original_size,
                    m->codes
                );

            if (!result) {
                errs("????? invalid code??????\n");
                return 1;
            }
            break;
        }

        default:
            goto usage;
    }

    fwrite(
        stdout,
        m->transformed,
        transformed_nbytes + (transformed_remainder_bits ? 1 : 0)
    );

    stdfflush();

    return 0;
}
