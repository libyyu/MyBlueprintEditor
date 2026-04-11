// Runtime/handlers/BuiltinHandlers_Crypto.cpp
// 加密/编码节点实现（纯 C++ 内嵌，无外部依赖，全平台含 WebGL）
//
// 节点列表：
//   Crypto.SHA256      — SHA-256 哈希，输出 hex 字符串
//   Crypto.HMAC        — HMAC-SHA256，输出 hex 字符串
//   Crypto.MD5         — MD5 哈希，输出 hex 字符串
//   Crypto.Base64Enc   — Base64 编码
//   Crypto.Base64Dec   — Base64 解码
//   Crypto.HexEncode   — 字节串 → hex 字符串
//   Crypto.HexDecode   — hex 字符串 → 字节串

#include "BuiltinHandlers_Crypto.h"
#include "../BlueprintRunner.h"
#include <cstdint>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <array>

namespace NodeEditor {
namespace Runtime {

// ============================================================================
// 内嵌 SHA-256 实现（RFC 6234）
// ============================================================================
namespace sha256_impl {

static const uint32_t K[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

static inline uint32_t rotr(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }
static inline uint32_t ch(uint32_t x,uint32_t y,uint32_t z)  { return (x&y)^(~x&z); }
static inline uint32_t maj(uint32_t x,uint32_t y,uint32_t z) { return (x&y)^(x&z)^(y&z); }
static inline uint32_t S0(uint32_t x) { return rotr(x,2)^rotr(x,13)^rotr(x,22); }
static inline uint32_t S1(uint32_t x) { return rotr(x,6)^rotr(x,11)^rotr(x,25); }
static inline uint32_t s0(uint32_t x) { return rotr(x,7)^rotr(x,18)^(x>>3); }
static inline uint32_t s1(uint32_t x) { return rotr(x,17)^rotr(x,19)^(x>>10); }

struct Context {
    uint32_t h[8];
    uint64_t len = 0;
    uint8_t  buf[64];
    size_t   bufLen = 0;

    Context() {
        h[0]=0x6a09e667; h[1]=0xbb67ae85; h[2]=0x3c6ef372; h[3]=0xa54ff53a;
        h[4]=0x510e527f; h[5]=0x9b05688c; h[6]=0x1f83d9ab; h[7]=0x5be0cd19;
    }

    void processBlock(const uint8_t* blk) {
        uint32_t w[64];
        for (int i=0;i<16;i++)
            w[i]=(uint32_t)blk[i*4]<<24|(uint32_t)blk[i*4+1]<<16|(uint32_t)blk[i*4+2]<<8|blk[i*4+3];
        for (int i=16;i<64;i++)
            w[i]=s1(w[i-2])+w[i-7]+s0(w[i-15])+w[i-16];
        uint32_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],hh=h[7];
        for (int i=0;i<64;i++) {
            uint32_t t1=hh+S1(e)+ch(e,f,g)+K[i]+w[i];
            uint32_t t2=S0(a)+maj(a,b,c);
            hh=g; g=f; f=e; e=d+t1;
            d=c; c=b; b=a; a=t1+t2;
        }
        h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d;
        h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=hh;
    }

    void update(const uint8_t* data, size_t sz) {
        len += sz * 8;
        while (sz > 0) {
            size_t copy = 64 - bufLen;
            if (copy > sz) copy = sz;
            memcpy(buf + bufLen, data, copy);
            bufLen += copy; data += copy; sz -= copy;
            if (bufLen == 64) { processBlock(buf); bufLen = 0; }
        }
    }

    void finalize(uint8_t out[32]) {
        buf[bufLen++] = 0x80;
        if (bufLen > 56) {
            while (bufLen < 64) buf[bufLen++] = 0;
            processBlock(buf); bufLen = 0;
        }
        while (bufLen < 56) buf[bufLen++] = 0;
        for (int i=7;i>=0;i--) { buf[bufLen++] = (uint8_t)(len >> (i*8)); }
        processBlock(buf);
        for (int i=0;i<8;i++) {
            out[i*4]=(uint8_t)(h[i]>>24); out[i*4+1]=(uint8_t)(h[i]>>16);
            out[i*4+2]=(uint8_t)(h[i]>>8); out[i*4+3]=(uint8_t)h[i];
        }
    }
};

static std::array<uint8_t,32> hash(const uint8_t* data, size_t sz) {
    Context ctx;
    ctx.update(data, sz);
    std::array<uint8_t,32> out;
    ctx.finalize(out.data());
    return out;
}

static std::array<uint8_t,32> hmac(
    const uint8_t* key, size_t klen,
    const uint8_t* data, size_t dlen)
{
    uint8_t k[64] = {};
    if (klen > 64) {
        auto kh = hash(key, klen);
        memcpy(k, kh.data(), 32);
    } else {
        memcpy(k, key, klen);
    }
    uint8_t ipad[64], opad[64];
    for (int i=0;i<64;i++) { ipad[i]=k[i]^0x36; opad[i]=k[i]^0x5c; }

    Context inner;
    inner.update(ipad, 64);
    inner.update(data, dlen);
    std::array<uint8_t,32> innerHash;
    inner.finalize(innerHash.data());

    Context outer;
    outer.update(opad, 64);
    outer.update(innerHash.data(), 32);
    std::array<uint8_t,32> result;
    outer.finalize(result.data());
    return result;
}

} // namespace sha256_impl

// ============================================================================
// 内嵌 MD5 实现（RFC 1321）
// ============================================================================
namespace md5_impl {

static const uint32_t T[64] = {
    0xd76aa478,0xe8c7b756,0x242070db,0xc1bdceee,0xf57c0faf,0x4787c62a,0xa8304613,0xfd469501,
    0x698098d8,0x8b44f7af,0xffff5bb1,0x895cd7be,0x6b901122,0xfd987193,0xa679438e,0x49b40821,
    0xf61e2562,0xc040b340,0x265e5a51,0xe9b6c7aa,0xd62f105d,0x02441453,0xd8a1e681,0xe7d3fbc8,
    0x21e1cde6,0xc33707d6,0xf4d50d87,0x455a14ed,0xa9e3e905,0xfcefa3f8,0x676f02d9,0x8d2a4c8a,
    0xfffa3942,0x8771f681,0x6d9d6122,0xfde5380c,0xa4beea44,0x4bdecfa9,0xf6bb4b60,0xbebfbc70,
    0x289b7ec6,0xeaa127fa,0xd4ef3085,0x04881d05,0xd9d4d039,0xe6db99e5,0x1fa27cf8,0xc4ac5665,
    0xf4292244,0x432aff97,0xab9423a7,0xfc93a039,0x655b59c3,0x8f0ccc92,0xffeff47d,0x85845dd1,
    0x6fa87e4f,0xfe2ce6e0,0xa3014314,0x4e0811a1,0xf7537e82,0xbd3af235,0x2ad7d2bb,0xeb86d391
};
static const int S[64] = {
    7,12,17,22, 7,12,17,22, 7,12,17,22, 7,12,17,22,
    5, 9,14,20, 5, 9,14,20, 5, 9,14,20, 5, 9,14,20,
    4,11,16,23, 4,11,16,23, 4,11,16,23, 4,11,16,23,
    6,10,15,21, 6,10,15,21, 6,10,15,21, 6,10,15,21
};

static inline uint32_t rotl(uint32_t x, int n) { return (x<<n)|(x>>(32-n)); }

static std::array<uint8_t,16> hash(const uint8_t* data, size_t len) {
    uint32_t a0=0x67452301,b0=0xefcdab89,c0=0x98badcfe,d0=0x10325476;
    // padding
    size_t padLen = len + 1;
    while (padLen % 64 != 56) padLen++;
    padLen += 8;
    std::vector<uint8_t> msg(padLen, 0);
    memcpy(msg.data(), data, len);
    msg[len] = 0x80;
    uint64_t bitLen = (uint64_t)len * 8;
    for (int i=0;i<8;i++) msg[padLen-8+i] = (uint8_t)(bitLen >> (i*8));

    for (size_t off=0; off<padLen; off+=64) {
        uint32_t M[16];
        for (int i=0;i<16;i++)
            M[i]=(uint32_t)msg[off+i*4]|(uint32_t)msg[off+i*4+1]<<8|
                 (uint32_t)msg[off+i*4+2]<<16|(uint32_t)msg[off+i*4+3]<<24;
        uint32_t A=a0,B=b0,C=c0,D=d0;
        for (int i=0;i<64;i++) {
            uint32_t F,g;
            if      (i<16) { F=(B&C)|(~B&D); g=i; }
            else if (i<32) { F=(D&B)|(~D&C); g=(5*i+1)%16; }
            else if (i<48) { F=B^C^D;        g=(3*i+5)%16; }
            else           { F=C^(B|~D);     g=(7*i)%16; }
            F += A + T[i] + M[g];
            A=D; D=C; C=B; B=B+rotl(F,S[i]);
        }
        a0+=A; b0+=B; c0+=C; d0+=D;
    }
    std::array<uint8_t,16> out;
    for (int i=0;i<4;i++) {
        out[i]=(uint8_t)(a0>>(i*8)); out[4+i]=(uint8_t)(b0>>(i*8));
        out[8+i]=(uint8_t)(c0>>(i*8)); out[12+i]=(uint8_t)(d0>>(i*8));
    }
    return out;
}

} // namespace md5_impl

// ============================================================================
// Base64 实现
// ============================================================================
namespace base64_impl {

static const char ENC[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::string encode(const uint8_t* data, size_t len) {
    std::string out;
    out.reserve(((len + 2) / 3) * 4);
    for (size_t i=0; i<len; i+=3) {
        uint32_t v = (uint32_t)data[i] << 16;
        if (i+1<len) v |= (uint32_t)data[i+1] << 8;
        if (i+2<len) v |= (uint32_t)data[i+2];
        out += ENC[(v>>18)&63];
        out += ENC[(v>>12)&63];
        out += (i+1<len) ? ENC[(v>>6)&63] : '=';
        out += (i+2<len) ? ENC[v&63]      : '=';
    }
    return out;
}

static std::string decode(const std::string& s) {
    auto val = [](char c) -> int {
        if (c>='A'&&c<='Z') return c-'A';
        if (c>='a'&&c<='z') return c-'a'+26;
        if (c>='0'&&c<='9') return c-'0'+52;
        if (c=='+') return 62;
        if (c=='/') return 63;
        return -1;
    };
    std::string out;
    for (size_t i=0; i+3<s.size(); i+=4) {
        int a=val(s[i]),b=val(s[i+1]),c=val(s[i+2]),d=val(s[i+3]);
        if (a<0||b<0) break;
        out += (char)((a<<2)|(b>>4));
        if (c>=0) out += (char)(((b&15)<<4)|(c>>2));
        if (d>=0) out += (char)(((c&3)<<6)|d);
    }
    return out;
}

} // namespace base64_impl

// ============================================================================
// 工具：bytes → hex
// ============================================================================
static std::string toHex(const uint8_t* data, size_t len) {
    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    for (size_t i=0; i<len; i++)
        ss << std::setw(2) << (int)data[i];
    return ss.str();
}

// ============================================================================
// RegisterHandlers_Crypto
// ============================================================================
void RegisterHandlers_Crypto(
    std::unordered_map<std::string, NodeHandler>& handlers)
{
    // ========================================================================
    // Crypto.SHA256
    //   in:  Data(String)
    //   out: Hash(String)  — 64字符 hex
    // ========================================================================
    handlers["Crypto.SHA256"] = [](ExecutionContext& ctx) -> bool {
        std::string data = ctx.GetInputValue("Data").asString();
        auto h = sha256_impl::hash(
            reinterpret_cast<const uint8_t*>(data.data()), data.size());
        ctx.SetOutputValue("Hash", Variant(toHex(h.data(), 32)));
        return true;
    };

    // ========================================================================
    // Crypto.HMAC
    //   in:  Key(String), Data(String), Algorithm(String) — 目前支持 "sha256"（默认）
    //   out: Hash(String)  — hex
    //        HashBase64(String) — base64
    // ========================================================================
    handlers["Crypto.HMAC"] = [](ExecutionContext& ctx) -> bool {
        std::string key  = ctx.GetInputValue("Key").asString();
        std::string data = ctx.GetInputValue("Data").asString();
        // Algorithm 暂时只支持 sha256，预留接口
        auto h = sha256_impl::hmac(
            reinterpret_cast<const uint8_t*>(key.data()),  key.size(),
            reinterpret_cast<const uint8_t*>(data.data()), data.size());
        ctx.SetOutputValue("Hash",       Variant(toHex(h.data(), 32)));
        ctx.SetOutputValue("HashBase64", Variant(base64_impl::encode(h.data(), 32)));
        return true;
    };

    // ========================================================================
    // Crypto.MD5
    //   in:  Data(String)
    //   out: Hash(String)  — 32字符 hex
    // ========================================================================
    handlers["Crypto.MD5"] = [](ExecutionContext& ctx) -> bool {
        std::string data = ctx.GetInputValue("Data").asString();
        auto h = md5_impl::hash(
            reinterpret_cast<const uint8_t*>(data.data()), data.size());
        ctx.SetOutputValue("Hash", Variant(toHex(h.data(), 16)));
        return true;
    };

    // ========================================================================
    // Crypto.Base64Enc
    //   in:  Data(String)  — 原始字节串
    //   out: Result(String) — base64 编码
    // ========================================================================
    handlers["Crypto.Base64Enc"] = [](ExecutionContext& ctx) -> bool {
        std::string data = ctx.GetInputValue("Data").asString();
        ctx.SetOutputValue("Result", Variant(base64_impl::encode(
            reinterpret_cast<const uint8_t*>(data.data()), data.size())));
        return true;
    };

    // ========================================================================
    // Crypto.Base64Dec
    //   in:  Data(String)  — base64 字符串
    //   out: Result(String) — 解码后字节串
    // ========================================================================
    handlers["Crypto.Base64Dec"] = [](ExecutionContext& ctx) -> bool {
        std::string data = ctx.GetInputValue("Data").asString();
        ctx.SetOutputValue("Result", Variant(base64_impl::decode(data)));
        return true;
    };

    // ========================================================================
    // Crypto.HexEncode
    //   in:  Data(String)   — 任意字节串
    //   out: Result(String) — hex 字符串
    // ========================================================================
    handlers["Crypto.HexEncode"] = [](ExecutionContext& ctx) -> bool {
        std::string data = ctx.GetInputValue("Data").asString();
        ctx.SetOutputValue("Result", Variant(toHex(
            reinterpret_cast<const uint8_t*>(data.data()), data.size())));
        return true;
    };

    // ========================================================================
    // Crypto.HexDecode
    //   in:  Data(String)   — hex 字符串（偶数位）
    //   out: Result(String) — 解码后字节串
    //        Valid(Boolean)  — 格式是否正确
    // ========================================================================
    handlers["Crypto.HexDecode"] = [](ExecutionContext& ctx) -> bool {
        std::string hex = ctx.GetInputValue("Data").asString();
        if (hex.size() % 2 != 0) {
            ctx.SetOutputValue("Result", Variant(std::string("")));
            ctx.SetOutputValue("Valid",  Variant(false));
            return true;
        }
        std::string out;
        out.reserve(hex.size() / 2);
        bool valid = true;
        for (size_t i=0; i<hex.size(); i+=2) {
            auto nibble = [](char c) -> int {
                if (c>='0'&&c<='9') return c-'0';
                if (c>='a'&&c<='f') return c-'a'+10;
                if (c>='A'&&c<='F') return c-'A'+10;
                return -1;
            };
            int hi = nibble(hex[i]), lo = nibble(hex[i+1]);
            if (hi<0 || lo<0) { valid=false; break; }
            out += (char)((hi<<4)|lo);
        }
        ctx.SetOutputValue("Result", Variant(valid ? out : std::string("")));
        ctx.SetOutputValue("Valid",  Variant(valid));
        return true;
    };
}

} // namespace Runtime
} // namespace NodeEditor
