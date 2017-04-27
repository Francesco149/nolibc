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
    memealloc:
    A resizable and relocatable memory pool/allocator that uses
    relative pointers.
*/

#define internalfn static
#define globvar static

#define min(a, b) ((a) < (b) ? (a) : (b))

typedef i32 b32;
typedef float f32;

/* ------------------------------------------------------------- */

void* syscall1(uintptr number, void* arg1);
void* syscall2(uintptr number, void* arg1, void* arg2);
void* syscall3(uintptr number, void* arg1, void* arg2, void* arg3);

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

internalfn
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

/* ------------------------------------------------------------- */

#define stdin 0

internalfn
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

#define PROT_READ  0x1
#define PROT_WRITE 0x2

#define MAP_PRIVATE   0x02
#define MAP_ANONYMOUS 0x20

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

internalfn
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

internalfn
int munmap(void* addr, uintptr nbytes)
{
    return (int)(intptr)
        syscall2(
            SYS_munmap,
            addr,
            (void*)nbytes
        );
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
internalfn
int fopen(fd_t* fd, char const* filename, u32 flags, mode_t mode)
{
    fd->handle = open(filename, flags, mode);
    fd->buf_nbytes = 0;
    fd->flush_every_line = 0;

    return fd->handle;
}
*/

internalfn
intptr fflush(fd_t* fd)
{
    intptr res =
        fd->buf_nbytes ? write(fd->handle, fd->buf, fd->buf_nbytes)
                       : 0;

    fd->buf_nbytes = 0;

    return res;
}

/*
internalfn
void fclose(fd_t* fd)
{
    fflush(fd);
    close(fd->handle);
    fd->handle = 0;
}
*/

internalfn
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

internalfn
void stdinit()
{
    stdout->handle = 1;
    stdout->flush_every_line = 1;
}

internalfn
void stdfflush()
{
    fflush(stdout);
}

/* ------------------------------------------------------------- */

#define puts(s) fwrite(stdout, s, strlen(s))

#ifdef _DEBUG
#define dbgputs puts
#else
#define dbgputs(str)
#endif

/* ------------------------------------------------------------- */

#define strcpy(dst, src) memecpy(dst, src, strlen(src) + 1)

internalfn
uintptr strlen(char const* str)
{
    char const* p;
    for (p = str; *p; ++p);
    return p - str;
}

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

/* -------------------------------------------------------------
                           MEMEALLOC
   ------------------------------------------------------------- */

u8* memory = 0;
uintptr memory_size = 0;

u8* pmemory = 0;

/* TODO: use macro trickery to add caller to each allocation */
typedef struct
{
    uintptr size;
    uintptr offset_to_next;
}
allocation_t;

internalfn
void memeinit(uintptr size)
{
    u8* new_memory = (u8*)
        mmap(
            0,
            size,
            PROT_READ | PROT_WRITE,
            MAP_ANONYMOUS | MAP_PRIVATE,
            0, 0
        );

    if (memory)
    {
        uintptr offset = (uintptr)(pmemory - memory);
#ifdef _DEBUG
        char buf[21];

        uitoa(10, size, buf, 0, 0);
#endif

        dbgputs("**** reallocating memory pool with size=");
        dbgputs(buf);
        dbgputs("\n");

        memecpy(new_memory, memory, offset);
        pmemory = new_memory + offset;

        munmap(memory, memory_size);
    }

    else if (!pmemory) {
        pmemory = new_memory;
    }

    memory = new_memory;
    memory_size = size;
}

internalfn
uintptr memealloc(uintptr size)
{
    allocation_t* new_alloc = 0;
    u8* p = memory;
    intptr total_size = size + sizeof(allocation_t);
#ifdef _DEBUG
    char buf[21];

    uitoa(10, size, buf, 0, 0);
#endif

    while (p < pmemory - 1)
    {
        allocation_t* alloc = (allocation_t*)p;
        intptr space_left;

        /* NOTE: gcc silently casts signed to unsigned in
                 unsigned - signed comparisons, and we don't
                 want that, that's why I made space_left signed. */

        if (!alloc->offset_to_next) {
            break;
        }

        space_left =
            (intptr)alloc->offset_to_next -
            alloc->size -
            sizeof(allocation_t) *
            (alloc->size ? 2 : 1);

        /* if alloc is not freed, we will have to create a new
           allocation in the empty space after it, so there will be
           two allocation_t structs */

        if (space_left >= total_size)
        {
            uintptr next = alloc->offset_to_next;

            if (alloc->size)
            {
                alloc->offset_to_next =
                    sizeof(allocation_t) + alloc->size;

                next -= alloc->offset_to_next;

                p += alloc->offset_to_next;
            }

            new_alloc = (allocation_t*)p;
            new_alloc->offset_to_next = next;

            dbgputs("**** reusing free'd memory to allocate ");
            dbgputs(buf);
            dbgputs(" bytes\n");

            break;
        }

        p = (u8*)alloc + alloc->offset_to_next;
    }

    if (!new_alloc)
    {
        allocation_t* prev = (allocation_t*)p;

        if (pmemory + total_size >= memory + memory_size)
        {
            return 0; /* out of memory */
        }

        new_alloc = (allocation_t*)pmemory;
        prev->offset_to_next = prev->size + sizeof(allocation_t);
        new_alloc->offset_to_next = 0;

        pmemory += total_size;

        dbgputs("**** allocating ");
        dbgputs(buf);
        dbgputs(" bytes at the end of the memory pool\n");
    }

    new_alloc->size = size;

    return (uintptr)
        ((u8*)new_alloc + sizeof(allocation_t) - memory);
}

internalfn
void memefree(uintptr off)
{
    u8* addr = memory + off;

    allocation_t* alloc =
        (allocation_t*)(addr - sizeof(allocation_t));

#ifdef _DEBUG
    char buf[21];

    dbgputs("**** freeing ");

    uitoa(10, alloc->size, buf, 0, 0);
    dbgputs(buf);

    dbgputs(" bytes at 0x");

    uitoa(16, (uintptr)addr, buf, sizeof(uintptr) * 2, '0');
    dbgputs(buf);

    dbgputs("\n");
#endif

    alloc->size = 0;
}

/* ------------------------------------------------------------- */

/* dummy struct purely to get compiler warnings when I try to use
   a pointer to the wrong type */

#define ptr_type(type) \
typedef struct { uintptr off; } p##type; \
\
type* p##type##_get(p##type p) { return (type*)&memory[p.off]; }

ptr_type(char)
ptr_type(u32)

globvar char buf[21];

internalfn
void print_ptr(void const* addr)
{
    puts("<");
    uitoa(16, (uintptr)addr, buf, sizeof(uintptr) * 2, '0');
    puts(buf);
    puts(">");
}

internalfn
void print_var_name(char const* name, void const* pvar)
{
    puts(name);
    puts(" ");
    print_ptr(pvar);
    puts(" = ");
}

internalfn
void print_str_(char const* name, pchar const pstr)
{
    char const* str = pchar_get(pstr);
    print_var_name(name, str);
    puts(str);
    puts("\n");
}

internalfn
void print_u32_array_(char const* name, pu32 const parr, uintptr n)
{
    u32 const* arr = pu32_get(parr);

    print_var_name(name, arr);

    for (; n; --n, ++arr)
    {
        uitoa(10, (uintptr)*arr, buf, 0, 0);
        puts(buf);
        puts(" ");
    }

    puts("\n");
}

#define print_str(str)          print_str_(#str, str)
#define print_u32_array(arr, n) print_u32_array_(#arr, arr, n)

int main()
{
    pchar a;
    pchar b;
    pu32 c;
    pu32 d;
    pu32 e;
    int i;

    stdinit();
    memeinit(1000000); /* 1mb memory pool */

    a.off = memealloc(128);
    b.off = memealloc(400);
    c.off = memealloc(sizeof(u32) * 10);

    for (i = 0; i < 10; ++i) {
        pu32_get(c)[i] = i;
    }

    strcpy(pchar_get(a), "this is a small string");
    strcpy(
        pchar_get(b),
        "this is a long string this is a long string "
        "this is a long string this is a long string"
    );

    /* print stuff ---------------- */
    print_str(a);
    print_str(b);
    print_u32_array(c, 10);
    /* ---------------------------- */

    memefree(a.off);

    /* resize pool test */
    memeinit(2000000);

    d.off = memealloc(sizeof(u32) * 10);
    a.off = memealloc(600);
    e.off = memealloc(sizeof(u32) * 2);

    pu32_get(e)[0] = 1337;
    pu32_get(e)[1] = 111111;

    strcpy(
        pchar_get(a),
        "this is an even longer string "
        "this is an even longer string "
        "this is an even longer string "
        "this is an even longer string "
        "this is an even longer string "
        "this is an even longer string"
    );

    for (i = 0; i < 10; ++i) {
        pu32_get(d)[i] = pu32_get(c)[i] * 2;
    }

    /* print stuff ---------------- */
    print_str(a);
    print_str(b);
    print_u32_array(c, 10);
    print_u32_array(d, 10);
    print_u32_array(e, 2);
    /* ---------------------------- */

    /* no need to free the rest since the program is terminating */
    stdfflush();

    return 0;
}

