#ifndef LP_HASHLIB_H
#define LP_HASHLIB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ================================================================
 * CRC32 — Table-based + SSE4.2 hardware acceleration
 * ================================================================ */

/* Detect SSE4.2 for hardware CRC32C */
#if defined(__SSE4_2__) || (defined(_MSC_VER) && defined(__AVX__))
  #include <nmmintrin.h>
  #define LP_HAS_SSE42_CRC 1
#else
  #define LP_HAS_SSE42_CRC 0
#endif

/* Pre-computed CRC32 lookup table (CRC-32/ISO-HDLC polynomial 0xEDB88320) */
static uint32_t _lp_crc32_table[256];
static int _lp_crc32_table_init = 0;

static inline void _lp_crc32_init_table(void) {
    if (_lp_crc32_table_init) return;
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int j = 0; j < 8; j++)
            c = (c >> 1) ^ (0xEDB88320 & (-(c & 1)));
        _lp_crc32_table[i] = c;
    }
    _lp_crc32_table_init = 1;
}

static inline char* lp_hashlib_crc32(const char* data) {
    if (!data) { char* r = (char*)malloc(1); r[0]=0; return r; }
    _lp_crc32_init_table();
    size_t len = strlen(data);
    const uint8_t* buf = (const uint8_t*)data;
    uint32_t crc = 0xFFFFFFFF;

#if LP_HAS_SSE42_CRC
    /* SSE4.2 fast path: process 8 bytes at a time */
    size_t i = 0;
    uint64_t crc64 = (uint64_t)crc;
    for (; i + 8 <= len; i += 8)
        crc64 = _mm_crc32_u64(crc64, *(const uint64_t*)(buf + i));
    crc = (uint32_t)crc64;
    for (; i < len; i++)
        crc = _lp_crc32_table[(crc ^ buf[i]) & 0xFF] ^ (crc >> 8);
#else
    for (size_t i = 0; i < len; i++)
        crc = _lp_crc32_table[(crc ^ buf[i]) & 0xFF] ^ (crc >> 8);
#endif

    crc ^= 0xFFFFFFFF;
    char* hex = (char*)malloc(9);
    if (hex) { sprintf(hex, "%08x", crc); }
    return hex;
}

/* ================================================================
 * MD5 Implementation
 * ================================================================ */
#define _LP_MD5_F(x,y,z) (((x)&(y))|((~(x))&(z)))
#define _LP_MD5_G(x,y,z) (((x)&(z))|((y)&(~(z))))
#define _LP_MD5_H(x,y,z) ((x)^(y)^(z))
#define _LP_MD5_I(x,y,z) ((y)^((x)|(~(z))))
#define _LP_MD5_RL(x,n)  (((x)<<(n))|((x)>>(32-(n))))

static inline void _lp_md5_transform(uint32_t state[4], const uint8_t block[64]) {
    uint32_t a=state[0], b=state[1], c=state[2], d=state[3], x[16];
    for (int i=0;i<16;i++) x[i]=((uint32_t)block[i*4])|((uint32_t)block[i*4+1]<<8)|
                                 ((uint32_t)block[i*4+2]<<16)|((uint32_t)block[i*4+3]<<24);
    static const uint32_t T[64]={
        0xd76aa478,0xe8c7b756,0x242070db,0xc1bdceee,0xf57c0faf,0x4787c62a,0xa8304613,0xfd469501,
        0x698098d8,0x8b44f7af,0xffff5bb1,0x895cd7be,0x6b901122,0xfd987193,0xa679438e,0x49b40821,
        0xf61e2562,0xc040b340,0x265e5a51,0xe9b6c7aa,0xd62f105d,0x02441453,0xd8a1e681,0xe7d3fbc8,
        0x21e1cde6,0xc33707d6,0xf4d50d87,0x455a14ed,0xa9e3e905,0xfcefa3f8,0x676f02d9,0x8d2a4c8a,
        0xfffa3942,0x8771f681,0x6d9d6122,0xfde5380c,0xa4beea44,0x4bdecfa9,0xf6bb4b60,0xbebfbc70,
        0x289b7ec6,0xeaa127fa,0xd4ef3085,0x04881d05,0xd9d4d039,0xe6db99e5,0x1fa27cf8,0xc4ac5665,
        0xf4292244,0x432aff97,0xab9423a7,0xfc93a039,0x655b59c3,0x8f0ccc92,0xffeff47d,0x85845dd1,
        0x6fa87e4f,0xfe2ce6e0,0xa3014314,0x4e0811a1,0xf7537e82,0xbd3af235,0x2ad7d2bb,0xeb86d391
    };
    static const int s[64]={7,12,17,22,7,12,17,22,7,12,17,22,7,12,17,22,
                            5,9,14,20,5,9,14,20,5,9,14,20,5,9,14,20,
                            4,11,16,23,4,11,16,23,4,11,16,23,4,11,16,23,
                            6,10,15,21,6,10,15,21,6,10,15,21,6,10,15,21};
    for (int i=0;i<64;i++){
        uint32_t f,g;
        if(i<16){f=_LP_MD5_F(b,c,d);g=i;}
        else if(i<32){f=_LP_MD5_G(b,c,d);g=(5*i+1)%16;}
        else if(i<48){f=_LP_MD5_H(b,c,d);g=(3*i+5)%16;}
        else{f=_LP_MD5_I(b,c,d);g=(7*i)%16;}
        uint32_t tmp=d; d=c; c=b;
        b=b+_LP_MD5_RL(a+f+T[i]+x[g],s[i]);
        a=tmp;
    }
    state[0]+=a; state[1]+=b; state[2]+=c; state[3]+=d;
}

static inline char* lp_hashlib_md5(const char* data) {
    if (!data) { char* r=(char*)malloc(1); r[0]=0; return r; }
    uint32_t state[4]={0x67452301,0xefcdab89,0x98badcfe,0x10325476};
    size_t len=strlen(data);
    const uint8_t* ptr=(const uint8_t*)data;
    size_t total=len;
    while(len>=64){_lp_md5_transform(state,ptr);ptr+=64;len-=64;}
    uint8_t block[64]; memset(block,0,64); memcpy(block,ptr,len);
    block[len]=0x80;
    if(len>=56){_lp_md5_transform(state,block);memset(block,0,64);}
    uint64_t bits=total*8;
    memcpy(block+56,&bits,8);
    _lp_md5_transform(state,block);
    char* hex=(char*)malloc(33);
    if(hex){
        uint8_t digest[16];
        memcpy(digest,state,16);
        for(int i=0;i<16;i++) sprintf(hex+i*2,"%02x",digest[i]);
        hex[32]='\0';
    }
    return hex;
}

/* ================================================================
 * SHA-1 Implementation
 * ================================================================ */
#define _LP_SHA1_RL(x,n) (((x)<<(n))|((x)>>(32-(n))))

static inline void _lp_sha1_transform(uint32_t state[5], const uint8_t data[64]) {
    uint32_t w[80], a=state[0], b=state[1], c=state[2], d=state[3], e=state[4];
    for (int i=0;i<16;i++) w[i]=((uint32_t)data[i*4]<<24)|((uint32_t)data[i*4+1]<<16)|
                                 ((uint32_t)data[i*4+2]<<8)|((uint32_t)data[i*4+3]);
    for (int i=16;i<80;i++) w[i]=_LP_SHA1_RL(w[i-3]^w[i-8]^w[i-14]^w[i-16],1);
    for (int i=0;i<80;i++){
        uint32_t f,k;
        if(i<20){f=(b&c)|((~b)&d);k=0x5A827999;}
        else if(i<40){f=b^c^d;k=0x6ED9EBA1;}
        else if(i<60){f=(b&c)|(b&d)|(c&d);k=0x8F1BBCDC;}
        else{f=b^c^d;k=0xCA62C1D6;}
        uint32_t tmp=_LP_SHA1_RL(a,5)+f+e+k+w[i];
        e=d; d=c; c=_LP_SHA1_RL(b,30); b=a; a=tmp;
    }
    state[0]+=a; state[1]+=b; state[2]+=c; state[3]+=d; state[4]+=e;
}

static inline char* lp_hashlib_sha1(const char* data) {
    if (!data) { char* r=(char*)malloc(1); r[0]=0; return r; }
    uint32_t state[5]={0x67452301,0xEFCDAB89,0x98BADCFE,0x10325476,0xC3D2E1F0};
    size_t len=strlen(data), total=len;
    const uint8_t* ptr=(const uint8_t*)data;
    while(len>=64){_lp_sha1_transform(state,ptr);ptr+=64;len-=64;}
    uint8_t block[64]; memset(block,0,64); memcpy(block,ptr,len);
    block[len]=0x80;
    if(len>=56){_lp_sha1_transform(state,block);memset(block,0,64);}
    uint64_t bits=total*8;
    for(int i=0;i<8;i++) block[63-i]=(uint8_t)(bits>>(i*8));
    _lp_sha1_transform(state,block);
    char* hex=(char*)malloc(41);
    if(hex){ for(int i=0;i<5;i++) sprintf(hex+i*8,"%08x",state[i]); hex[40]='\0'; }
    return hex;
}

/* ================================================================
 * SHA-256 Implementation
 * ================================================================ */
static const uint32_t _lp_sha256_k[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

#define _LP_SHA256_RR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))

static inline void _lp_sha256_transform(uint32_t state[8], const uint8_t data[64]) {
    uint32_t w[64], a,b,c,d,e,f,g,h;
    for (int i=0;i<16;i++) w[i]=((uint32_t)data[i*4]<<24)|((uint32_t)data[i*4+1]<<16)|
                                 ((uint32_t)data[i*4+2]<<8)|((uint32_t)data[i*4+3]);
    for (int i=16;i<64;i++){
        uint32_t s0=_LP_SHA256_RR(w[i-15],7)^_LP_SHA256_RR(w[i-15],18)^(w[i-15]>>3);
        uint32_t s1=_LP_SHA256_RR(w[i-2],17)^_LP_SHA256_RR(w[i-2],19)^(w[i-2]>>10);
        w[i]=w[i-16]+s0+w[i-7]+s1;
    }
    a=state[0];b=state[1];c=state[2];d=state[3];
    e=state[4];f=state[5];g=state[6];h=state[7];
    for (int i=0;i<64;i++){
        uint32_t S1=_LP_SHA256_RR(e,6)^_LP_SHA256_RR(e,11)^_LP_SHA256_RR(e,25);
        uint32_t ch=(e&f)^((~e)&g);
        uint32_t t1=h+S1+ch+_lp_sha256_k[i]+w[i];
        uint32_t S0=_LP_SHA256_RR(a,2)^_LP_SHA256_RR(a,13)^_LP_SHA256_RR(a,22);
        uint32_t maj=(a&b)^(a&c)^(b&c);
        uint32_t t2=S0+maj;
        h=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;
    }
    state[0]+=a;state[1]+=b;state[2]+=c;state[3]+=d;
    state[4]+=e;state[5]+=f;state[6]+=g;state[7]+=h;
}

static inline char* lp_hashlib_sha256(const char* data) {
    if (!data) { char* r=(char*)malloc(1); r[0]=0; return r; }
    uint32_t state[8]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,
                       0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    size_t len=strlen(data), total=len;
    const uint8_t* ptr=(const uint8_t*)data;
    while(len>=64){_lp_sha256_transform(state,ptr);ptr+=64;len-=64;}
    uint8_t block[64]; memset(block,0,64); memcpy(block,ptr,len);
    block[len]=0x80;
    if(len>=56){_lp_sha256_transform(state,block);memset(block,0,64);}
    uint64_t bits=total*8;
    for(int i=0;i<8;i++) block[63-i]=(uint8_t)(bits>>(i*8));
    _lp_sha256_transform(state,block);
    char* hex=(char*)malloc(65);
    if(hex){ for(int i=0;i<8;i++) sprintf(hex+i*8,"%08x",state[i]); hex[64]='\0'; }
    return hex;
}

/* File hashing with 64KB buffer */
#define LP_HASH_FILE_BUFSIZE (64*1024)

static inline char* lp_hashlib_sha256_file(const char* filepath) {
    FILE* f = fopen(filepath, "rb");
    if (!f) { char* r=(char*)malloc(1); r[0]=0; return r; }
    uint32_t state[8]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,
                       0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    uint8_t buf[LP_HASH_FILE_BUFSIZE];
    uint64_t total_len = 0;
    size_t n;
    while ((n = fread(buf, 1, LP_HASH_FILE_BUFSIZE, f)) == LP_HASH_FILE_BUFSIZE) {
        for (size_t i = 0; i < n; i += 64) _lp_sha256_transform(state, buf + i);
        total_len += n;
    }
    total_len += n;
    /* Final block with padding */
    size_t rem = n;
    const uint8_t* ptr = buf;
    while(rem>=64){_lp_sha256_transform(state,ptr);ptr+=64;rem-=64;}
    uint8_t block[64]; memset(block,0,64); memcpy(block,ptr,rem);
    block[rem]=0x80;
    if(rem>=56){_lp_sha256_transform(state,block);memset(block,0,64);}
    uint64_t bits=total_len*8;
    for(int i=0;i<8;i++) block[63-i]=(uint8_t)(bits>>(i*8));
    _lp_sha256_transform(state,block);
    fclose(f);
    char* hex=(char*)malloc(65);
    if(hex){ for(int i=0;i<8;i++) sprintf(hex+i*8,"%08x",state[i]); hex[64]='\0'; }
    return hex;
}

#endif /* LP_HASHLIB_H */
