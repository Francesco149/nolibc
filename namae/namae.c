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
    namae: a tiny libc-free partial implementation of a dns
           client for Linux.
*/

#define NAMAE_VER "namae v1.0"
#define DNS_SERVER 0xB1B179B9 /* in big endian */

#define internal static

typedef u32 b32;

void* syscall(uintptr number);
void* syscall1(uintptr number, void* arg1);
void* syscall2(uintptr number, void* arg1, void* arg2);
void* syscall3(uintptr number, void* arg1, void* arg2, void* arg3);

/* ------------------------------------------------------------- */

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

internal
int getpid() {
    return (int)(intptr)syscall(SYS_getpid);
}

#define stdout 1
#define stderr 2

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

#define AF_INET 2
#define SOCK_DGRAM 2
#define IPPROTO_UDP 17

typedef struct
{
    u16 family;
    u16 port; /* NOTE: this is big endian!!!!!!! use flip16u */
    u32 addr; /* this is also big endian */
    u8  zero[8];
}
sockaddr_in;

#ifdef SYS_socketcall
/* i386 multiplexes socket calls through socketcall */
#define SYS_SOCKET      1
#define SYS_CONNECT     3

internal
int socketcall(u32 call, void* args)
{
    return (int)(intptr)
        syscall2(
            SYS_socketcall,
            (void*)(intptr)call,
            args
        );
}
#endif

internal
int socket(u16 family, i32 type, i32 protocol)
{
#ifndef SYS_socketcall
    return (int)(intptr)
        syscall3(
            SYS_socket,
            (void*)(intptr)family,
            (void*)(intptr)type,
            (void*)(intptr)protocol
        );
#else
    void* args[3];
    args[0] = (void*)(intptr)family;
    args[1] = (void*)(intptr)type;
    args[2] = (void*)(intptr)protocol;

    return socketcall(SYS_SOCKET, args);
#endif
}

internal
int connect(int sockfd, sockaddr_in const* addr)
{
#ifndef SYS_socketcall
    return (int)(intptr)
        syscall3(
            SYS_connect,
            (void*)(intptr)sockfd,
            (void*)addr,
            (void*)sizeof(sockaddr_in)
        );
#else
    void* args[3];
    args[0] = (void*)(intptr)sockfd;
    args[1] = (void*)addr;
    args[2] = (void*)sizeof(sockaddr_in);

    return socketcall(SYS_CONNECT, args);
#endif
}

/* ------------------------------------------------------------- */

internal
intptr strlen(char const* str)
{
    char const* p;
    for(p = str; *p; ++p);
    return p - str;
}

internal
b32 streq(char const* a, char const* b)
{
    for (; *a && *b; ++a, ++b)
    {
        if (*a != *b) {
            return 0;
        }
    }

    return *a == *b;
}

internal
intptr fputs(int fd, char const* str) {
    return write(fd, str, strlen(str));
}

internal
intptr puts(char const* str) {
    return fputs(stdout, str);
}

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


internal
void print_ipv4(u8 const* bytes)
{
    static char buf[16];
    char* p = buf;

    p += uitoa(10, (uintptr)bytes[0], p, 0, '0');
    *p++ = '.';

    p += uitoa(10, (uintptr)bytes[1], p, 0, '0');
    *p++ = '.';

    p += uitoa(10, (uintptr)bytes[2], p, 0, '0');
    *p++ = '.';

    uitoa(10, (uintptr)bytes[3], p, 0, '0');

    puts(buf);
}

/* reverses byte order of a 16-bit integer (0x1234 -> 0x3412) */
internal
u16 flip16u(u16 v) {
    return (v << 8) | (v >> 8);
}

/* ------------------------------------------------------------- */

/* primitive encoding functions used to build the packets.
   this is all big endian */

internal
uintptr encode2(u8* p, u16 v)
{
    u8 const* s = p;
    *p++ = (u8)(v >> 8);
    *p++ = (u8)(v & 0x00FF);
    return p - s;
}

internal
uintptr decode2(u8 const* p, u16* v)
{
    u8 const* s = p;
    *v = *p++ << 8;
    *v |= *p++;
    return p - s;
}

internal
uintptr decode4(u8 const* p, u32* v)
{
    u8 const* s = p;
    p += decode2(p, (u16*)v + 1);
    p += decode2(p, (u16*)v);
    return p - s;
}

#define MAX_STR 63

/* len will be truncated to MAX_STR if higher */
internal
uintptr encode_str(u8* p, char const* str, u8 len)
{
    u8 const* s = p;
    len &= MAX_STR;

    *p++ = len;

    for (; len; --len) {
        *p++ = (u8)*str++;
    }

    return p - s;
}

internal
uintptr decode_str(u8 const* p, char* str)
{
    u8 const* s = p;
    u8 len = *p++;

    for (; len; --len) {
        *str++ = (char)*p++;
    }

    *str++ = 0;

    return p - s;
}

/* ------------------------------------------------------------- */

#define RCODE_OK       0
#define RCODE_EFMT     1
#define RCODE_ESERV    2
#define RCODE_ENAME    3
#define RCODE_EIMPL    4
#define RCODE_EREFUSED 5

char const* rcode(u8 code)
{
    switch (code)
    {
        case RCODE_EFMT:
            return "Format error: the name server was unable to "
                   "interpret the query";
        case RCODE_ESERV:
            return "Server failure: The name server was unable to "
                   "process this query due to a problem with the "
                   "name server";
        case RCODE_ENAME:
            return "Name error: the domain name referenced in the "
                   " query does not exist";
        case RCODE_EIMPL:
            return "Not implemented: the name server does not "
                   "support the requested kind of query";
        case RCODE_EREFUSED:
            return "Refused: the name server refuses to perform "
                   "the specified operation for policy reasons";
    }

    return "Unknown error";
}

/*
    Section 4.1.3 of https://www.ietf.org/rfc/rfc1035.txt

                                    1  1  1  1  1  1
      0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                      ID                       |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   | <- mask
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                    QDCOUNT                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                    ANCOUNT                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                    NSCOUNT                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                    ARCOUNT                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
*/

#define QR_QUERY       0x0000
#define QR_RESP        0x8000 /* 1 0000 0 0 0 0 000 0000 */
#define OPCODE_QUERY   0x0000
#define OPCODE_STATUS  0x1000 /* 0 0010 0 0 0 0 000 0000 */
#define DNS_RD         0x0100 /* 0 0000 0 0 1 0 000 0000 */
#define DNS_AA         0x0400 /* 0 0000 1 0 0 0 000 0000 */
#define RCODE_MASK     0x000F /* 0 0000 0 0 0 0 000 1111 */

typedef struct
{
    u16 id;
    u16 mask;
    u16 qd_count;
    u16 an_count;
    u16 ns_count;
    u16 ar_count;
}
dns_hdr;

internal
uintptr encode_dns_hdr(u8* p, dns_hdr const* hdr)
{
    u8 const* s = p;

    p += encode2(p, hdr->id);
    p += encode2(p, hdr->mask);
    p += encode2(p, hdr->qd_count);
    p += encode2(p, hdr->an_count);
    p += encode2(p, hdr->ns_count);
    p += encode2(p, hdr->ar_count);

    return p - s;
}

internal
uintptr decode_dns_hdr(u8 const* p, dns_hdr* hdr)
{
    u8 const* s = p;

    p += decode2(p, &hdr->id);
    p += decode2(p, &hdr->mask);
    p += decode2(p, &hdr->qd_count);
    p += decode2(p, &hdr->an_count);
    p += decode2(p, &hdr->ns_count);
    p += decode2(p, &hdr->ar_count);

    return p - s;
}

#define TYPE_A 1
#define CLASS_IN 1

internal
uintptr encode_dns_question(
    u8* p,
    char const* qname,
    u16 qtype,
    u16 qclass)
{
    u8 const* s = p;
    char const* label = qname;

    for (; 1; ++label)
    {
        char c = *label;

        if (c != '.' && c) {
            continue;
        }

        p += encode_str(p, qname, label - qname);
        qname = label + 1;

        if (!c) {
            break;
        }
    }

    *p++ = 0;

    p += encode2(p, qtype);
    p += encode2(p, qclass);

    return p - s;
}

/*
    Section 4.1.4 of https://www.ietf.org/rfc/rfc1035.txt

    To avoid duplicate strings, an entire domain name or a list of
    labels at the end of a domain name is replaced with a pointer
    to a prior occurance of the same name.

    The pointer takes the form of a two octet sequence:

        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        | 1  1|                OFFSET                   |
        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

    The first two bits are ones. This allows a pointer to be
    distinguished from a label, since the label must begin with two
    zero bits because labels are restricted to 63 octets or less.
*/

#define MAX_HOST 0xFF
#define STR_POINTER 0xC0

/* qname is expected to be of size MAX_HOST */
internal
uintptr decode_name(u8 const* p, char* qname, u8 const* data_begin)
{
    u8 const* s = p;

    if ((*p & STR_POINTER) == STR_POINTER)
    {
        /* a pointer basically redirects the parsing of the entire
           name to another location in the packet */
        u16 offset;
        p += decode2(p, &offset);
        offset &= ~(STR_POINTER << 8);
        decode_name(data_begin + offset, qname, data_begin);
        return p - s;
    }

    while (p - s + *p < MAX_HOST - 1)
    {
        p += decode_str(p, qname);
        if (!*p) {
            break;
        }

        for (; *qname; ++qname);
        *qname++ = '.';
    }

    for (; *p; ++p); /* skip rest in case of truncation */

    return ++p - s;
}

/* qname is expected to be of size MAX_HOST */
internal
uintptr decode_dns_question(
    u8 const* p,
    char* qname,
    u16* qtype,
    u16* qclass,
    u8 const* data_begin)
{
    u8 const* s = p;

    p += decode_name(p, qname, data_begin);
    p += decode2(p, qtype);
    p += decode2(p, qclass);

    return p - s;
}

typedef struct
{
    char* name;
    u16 type;
    u16 class;
    u32 ttl;
    u16 rd_length;
    void const* data;
}
dns_resource;

/* res->name is expected to be set to a buffer of size MAX_HOST.
   res->data will point to data in p, and will only be valid as
   long as p is valid */
internal
uintptr decode_dns_resource(
    u8 const* p,
    dns_resource* res,
    u8 const* data_begin)
{
    u8 const* s = p;

    p += decode_dns_question(
        p,
        res->name,
        &res->type,
        &res->class,
        data_begin
    );

    p += decode4(p, &res->ttl);
    p += decode2(p, &res->rd_length);

    res->data = p;
    p += res->rd_length;

    return p - s;
}

/* note: ip is in big endian */
internal
int dns_dial(u32 ip)
{
    int fd, err;
    sockaddr_in a;

    memeset(&a, 0, sizeof(sockaddr_in));
    a.family = AF_INET;
    a.port = flip16u(53);
    a.addr = ip;

    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0) {
        return fd;
    }

    err = connect(fd, &a);
    if (err < 0) {
        close(fd);
        return err;
    }

    return fd;
}

/* ------------------------------------------------------------- */

#define BUFSIZE 0xFFFF

#define error(msg) fputs(stderr, msg)
#define die(msg) \
    error(msg); \
    return 1;

#define die_cleanup(msg) \
    error(msg); \
    code = 1; \
    goto cleanup;

internal
int namae_run(int argc, char const* argv[])
{
    int code = 0;
    int fd;

    /* query and response */
    u8 buf[BUFSIZE];
    u8* p;
    dns_hdr hdr;

    /* response */
    char namebuf[MAX_HOST];
    u16 qtype, qclass;
    u16 i;

    /* ----------------------------------------------------- */

    fputs(stderr, "(" NAMAE_VER ")\n\n");

    if (argc < 2)
    {
        fputs(stderr, "Usage: ");
        fputs(stderr, argv[0]);
        fputs(stderr, " domain\n");

        return 1;
    }

    /* ----------------------------------------------------- */

    fd = dns_dial(DNS_SERVER);
    if (fd < 0) {
        die("Failed to connect to DNS server")
    }

    /* ----------------------------------------------------- */

    memeset(&hdr, 0, sizeof(dns_hdr));
    hdr.id = (u16)getpid();
    hdr.mask = QR_QUERY | OPCODE_QUERY | DNS_RD;
    hdr.qd_count = 1;

    p = buf;
    p += encode_dns_hdr(p, &hdr);
    p += encode_dns_question(p, argv[1], TYPE_A, CLASS_IN);

    if (write(fd, buf, p - buf) != p - buf) {
        die_cleanup("write failed\n")
    }

    /* ----------------------------------------------------- */

    if (read(fd, buf, BUFSIZE) < 0) {
        die_cleanup("read failed\n")
    }

cleanup:
    close(fd);

    if (code) {
        return code;
    }

    p = buf;
    p += decode_dns_hdr(p, &hdr);

    if (hdr.id != getpid()) {
        die("ID mismatch\n")
    }

    if (!(hdr.mask & QR_RESP)) {
        die("Expected response, got query\n")
    }

    if ((hdr.mask & RCODE_MASK) != RCODE_OK) {
        die(rcode(hdr.mask & RCODE_MASK))
    }

    if (hdr.qd_count != 1) {
        die("Question count mismatch\n")
    }

    p += decode_dns_question(p, namebuf, &qtype, &qclass, buf);

    if (!streq(namebuf, argv[1])) {
        die("Query hostname mismatch\n")
    }

    if (qtype != TYPE_A) {
        die("Query type mismatch\n")
    }

    if (qclass != CLASS_IN) {
        die("Query class mimatch\n")
    }

    for (i = 0;
         i < hdr.an_count + hdr.ns_count + hdr.ar_count;
         ++i)
    {
        dns_resource r;

        r.name = namebuf;
        p += decode_dns_resource(p, &r, buf);

        puts(r.name);
        puts(" -> ");

        switch (r.type)
        {
            case TYPE_A:
                if (r.rd_length != 4) {
                    die("Invalid length for ipv4 A record\n")
                }

                print_ipv4(r.data);
                break;

            default:
            {
                u8 const* pdata = r.data;

                puts("(unknown resource type ");
                uitoa(10, (uintptr)r.type, namebuf, 0, '0');
                puts(namebuf);
                puts(") ");

                for (;  r.rd_length; --r.rd_length)
                {
                    uitoa(16, (uintptr)*pdata++, namebuf, 2, '0');
                    puts(namebuf);
                    puts(" ");
                }
            }
        }

        puts("\n");
    }

    return 0;
}
