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
    qsort: just a basic implementation of qsort
*/

/* TODO: change pivot to middle to improve efficiency for nearly
         sorted arrays */

typedef i32 b32;

#define internal static
#define globvar static

#define arr_count(a) (sizeof(a) / sizeof((a)[0]))

/* ------------------------------------------------------------- */

void* syscall1(uintptr number, void* arg1);

void* syscall3(
    uintptr number,
    void* arg1,
    void* arg2,
    void* arg3
);

/* ------------------------------------------------------------- */

#define stdin 0
#define stdout 1

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

/* ------------------------------------------------------------- */

internal
void swap_u32(u32* a, u32* b)
{
    u32 tmp = *a;
    *a = *b;
    *b = tmp;
}

typedef b32 qsort32_cmp(u32 a, u32 b);

internal
uintptr qsort32_partition(
    u32* arr,
    qsort32_cmp* cmp,
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
            swap_u32(&arr[i], &arr[j]);
        }
    }

    swap_u32(&arr[++i], &arr[hi]);

    return i;
}

internal
void qsort32(u32* arr, qsort32_cmp* cmp, uintptr lo, uintptr hi)
{
    uintptr p;

    if (lo >= hi) {
        return;
    }

    p = qsort32_partition(arr, cmp, lo, hi);
    qsort32(arr, cmp, lo, p - 1);
    qsort32(arr, cmp, p + 1, hi);
}

/* ------------------------------------------------------------- */

internal
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

/* ------------------------------------------------------------- */

void print_u32_array(u32* v, uintptr n)
{
    uintptr i;

    for (i = 0; i < n; ++i)
    {
        char buf[11];
        uitoa(10, v[i], buf, 0, 0);
        puts(buf);
        puts("\n");
    }
}

#define FNAME "/dev/urandom"
#define NVALUES 10

internal
b32 cmp_u32(u32 a, u32 b) {
    return a <= b;
}

globvar u32 numbers[NVALUES];

int main(int argc, char const* argv[])
{
    int fd;
    intptr n;
    u8* p = (u8*)numbers;

    fd = open(FNAME, O_RDONLY, 0);
    if (fd < 0) {
        puts("Failed to open " FNAME "\n");
        return 1;
    }

    while (p <= (u8*)&numbers[NVALUES - 1])
    {
        n = read(fd, &numbers[0], sizeof(numbers));
        if (n < 0) {
            puts("Failed to read from " FNAME "\n");
            return 1;
        }

        p += n;
    }

    close(fd);

    puts("Unsorted:\n");
    print_u32_array(numbers, arr_count(numbers));

    qsort32(numbers, cmp_u32, 0, arr_count(numbers) - 1);

    puts("\nSorted:\n");
    print_u32_array(numbers, arr_count(numbers));

    return 0;
}
