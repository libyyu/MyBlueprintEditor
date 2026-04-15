/*
 * zlib_stub.c
 * 空实现，用于在没有 zlib 的环境下满足 OpenSSL 1.1.1 libcrypto 的链接需求。
 * TLS 压缩（DEFLATE，RFC 3749）已在现代 TLS 中废弃（CRIME 攻击），不影响正常 HTTPS 功能。
 */
#ifdef _MSC_VER
#pragma warning(disable: 4100)  /* unreferenced parameter */
#endif

#include <stddef.h>

typedef unsigned long  uLong;
typedef unsigned int   uInt;
typedef unsigned char  Bytef;
typedef void*          voidpf;

typedef struct z_stream_s {
    Bytef*    next_in;
    uInt      avail_in;
    uLong     total_in;
    Bytef*    next_out;
    uInt      avail_out;
    uLong     total_out;
    char*     msg;
    void*     state;
    voidpf    (*zalloc)(voidpf, uInt, uInt);
    void      (*zfree)(voidpf, voidpf);
    voidpf    opaque;
    int       data_type;
    uLong     adler;
    uLong     reserved;
} z_stream;

typedef z_stream* z_streamp;

#define Z_OK            0
#define Z_STREAM_END    1
#define Z_ERRNO        (-1)
#define Z_BUF_ERROR    (-5)

/* Minimal stubs — return error to disable TLS compression path */
int deflate(z_streamp strm, int flush)              { (void)strm; (void)flush; return Z_BUF_ERROR; }
int deflateEnd(z_streamp strm)                      { (void)strm; return Z_OK; }
int inflate(z_streamp strm, int flush)              { (void)strm; (void)flush; return Z_BUF_ERROR; }
int inflateEnd(z_streamp strm)                      { (void)strm; return Z_OK; }
int deflateInit_(z_streamp strm, int level,
                 const char* version, int stream_size) {
    (void)strm; (void)level; (void)version; (void)stream_size; return Z_BUF_ERROR;
}
int inflateInit_(z_streamp strm,
                 const char* version, int stream_size) {
    (void)strm; (void)version; (void)stream_size; return Z_BUF_ERROR;
}
const char* zError(int err)                         { (void)err; return "zlib stub (disabled)"; }
