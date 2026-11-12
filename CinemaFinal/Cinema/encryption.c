#include "headers.h"

/* =====================================================================
   encryption.c
   - SHA-256 implementation (pure C, no external libraries)
   - Salted password hashing for user accounts
   - XOR stream encryption for data files (users.dat, movies.dat, ...)
     Files are stored as: [8-byte magic "CINENC2"] [encrypted payload]
     Files without the magic are treated as legacy plaintext, so data
     created by older versions of the program still loads correctly.
   ===================================================================== */

/* ---------------- SHA-256 CORE ---------------- */

#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))

static const unsigned int SHA_K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

typedef struct {
    unsigned int state[8];
    unsigned long long bitcount;
    unsigned char buffer[64];
    unsigned int buflen;
} SHA256_CTX;

static void sha256_transform(SHA256_CTX *ctx, const unsigned char block[64]) {
    unsigned int w[64];
    unsigned int a, b, c, d, e, f, g, h, t1, t2;
    int i;

    for (i = 0; i < 16; i++) {
        w[i] = ((unsigned int)block[i * 4] << 24) |
               ((unsigned int)block[i * 4 + 1] << 16) |
               ((unsigned int)block[i * 4 + 2] << 8) |
               ((unsigned int)block[i * 4 + 3]);
    }
    for (i = 16; i < 64; i++) {
        unsigned int s0 = ROTR(w[i - 15], 7) ^ ROTR(w[i - 15], 18) ^ (w[i - 15] >> 3);
        unsigned int s1 = ROTR(w[i - 2], 17) ^ ROTR(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }

    a = ctx->state[0]; b = ctx->state[1]; c = ctx->state[2]; d = ctx->state[3];
    e = ctx->state[4]; f = ctx->state[5]; g = ctx->state[6]; h = ctx->state[7];

    for (i = 0; i < 64; i++) {
        t1 = h + (ROTR(e, 6) ^ ROTR(e, 11) ^ ROTR(e, 25)) + ((e & f) ^ (~e & g)) +
             SHA_K[i] + w[i];
        t2 = (ROTR(a, 2) ^ ROTR(a, 13) ^ ROTR(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }

    ctx->state[0] += a; ctx->state[1] += b; ctx->state[2] += c; ctx->state[3] += d;
    ctx->state[4] += e; ctx->state[5] += f; ctx->state[6] += g; ctx->state[7] += h;
}

static void sha256_init(SHA256_CTX *ctx) {
    ctx->state[0] = 0x6a09e667; ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372; ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f; ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab; ctx->state[7] = 0x5be0cd19;
    ctx->bitcount = 0;
    ctx->buflen = 0;
}

static void sha256_update(SHA256_CTX *ctx, const unsigned char *data, size_t len) {
    size_t i;
    for (i = 0; i < len; i++) {
        ctx->buffer[ctx->buflen++] = data[i];
        if (ctx->buflen == 64) {
            sha256_transform(ctx, ctx->buffer);
            ctx->bitcount += 512;
            ctx->buflen = 0;
        }
    }
}

static void sha256_final(SHA256_CTX *ctx, unsigned char out[32]) {
    unsigned long long total_bits = ctx->bitcount + (unsigned long long)ctx->buflen * 8;
    unsigned char pad = 0x80;
    unsigned char zero = 0x00;
    unsigned char lenbuf[8];
    int i;

    sha256_update(ctx, &pad, 1);
    while (ctx->buflen != 56) sha256_update(ctx, &zero, 1);

    for (i = 0; i < 8; i++) lenbuf[i] = (unsigned char)(total_bits >> (56 - i * 8));
    sha256_update(ctx, lenbuf, 8);

    for (i = 0; i < 8; i++) {
        out[i * 4]     = (unsigned char)(ctx->state[i] >> 24);
        out[i * 4 + 1] = (unsigned char)(ctx->state[i] >> 16);
        out[i * 4 + 2] = (unsigned char)(ctx->state[i] >> 8);
        out[i * 4 + 3] = (unsigned char)(ctx->state[i]);
    }
}

void sha256_hex(const char *input, char out_hex[65]) {
    SHA256_CTX ctx;
    unsigned char digest[32];
    sha256_init(&ctx);
    sha256_update(&ctx, (const unsigned char *)input, strlen(input));
    sha256_final(&ctx, digest);
    for (int i = 0; i < 32; i++) sprintf(out_hex + i * 2, "%02x", digest[i]);
    out_hex[64] = 0;
}

/* ---------------- PASSWORD HASHING ---------------- */

static const char *APP_SECRET = "C1N3MA-5YS-5ECR37-2026";

/* Produces a 28-hex-char salted SHA-256 digest (fits MAX_PASSWORD).
   The salt is derived from the user's email, so no extra storage is needed. */
void hash_password(const char *email, const char *password, char out[MAX_PASSWORD]) {
    char salted[256];
    char digest[65];
    snprintf(salted, sizeof(salted), "%s::%s::%s", APP_SECRET, email, password);
    sha256_hex(salted, digest);
    strncpy(out, digest, 27);
    out[27] = '\0';
}

int password_is_hashed(const char *pw) {
    int len = (int)strlen(pw);
    if (len != 27) return 0;
    for (int i = 0; i < len; i++) {
        if (!isxdigit((unsigned char)pw[i])) return 0;
    }
    return 1;
}

/* Verifies a password against the stored value. Also accepts legacy
   plain-text passwords so accounts created before encryption still work;
   the caller can re-save the account to upgrade it to a hash. */
int verify_password(const char *email, const char *plain, const char *stored) {
    if (password_is_hashed(stored)) {
        char hashed[MAX_PASSWORD];
        hash_password(email, plain, hashed);
        return strcmp(hashed, stored) == 0;
    }
    return strcmp(plain, stored) == 0;
}

/* ---------------- DATA FILE ENCRYPTION ---------------- */

#define ENC_MAGIC "CINENC2"
#define ENC_MAGIC_LEN 8

static const unsigned char ENC_KEY[] = "C1N3MA-5YS-5ECR37-2026#DATA";

/* Symmetric XOR keystream (encrypt == decrypt). */
static void xor_crypt(const unsigned char *in, unsigned char *out, size_t n) {
    size_t key_len = sizeof(ENC_KEY) - 1;
    for (size_t i = 0; i < n; i++) {
        unsigned char k = (unsigned char)((ENC_KEY[i % key_len] + (i * 31)) ^ 0x5A);
        out[i] = in[i] ^ k;
    }
}

int enc_file_is_encrypted(const char *path) {
    FILE *fp = fopen(path, "rb");
    if (!fp) return 0;
    char magic[ENC_MAGIC_LEN + 1] = {0};
    size_t got = fread(magic, 1, ENC_MAGIC_LEN, fp);
    fclose(fp);
    return (got == ENC_MAGIC_LEN && memcmp(magic, ENC_MAGIC, ENC_MAGIC_LEN) == 0);
}

/* Writes count records of rec_size bytes, encrypted, with magic header. */
int enc_save_records(const char *path, const void *records, size_t rec_size, int count) {
    FILE *fp = fopen(path, "wb");
    if (!fp) return 0;

    fwrite(ENC_MAGIC, 1, ENC_MAGIC_LEN, fp);

    size_t total = rec_size * (size_t)count;
    if (total > 0 && records) {
        unsigned char *plain = (unsigned char *)malloc(total);
        unsigned char *cipher = (unsigned char *)malloc(total);
        if (!plain || !cipher) {
            free(plain);
            free(cipher);
            fclose(fp);
            return 0;
        }
        memcpy(plain, records, total);
        xor_crypt(plain, cipher, total);
        fwrite(cipher, 1, total, fp);
        free(plain);
        free(cipher);
    }
    fclose(fp);
    return 1;
}

/* Reads records back. Works with both encrypted files (new format) and
   plain files written by older versions (legacy format).
   Returns record count, 0 for an empty file, or -1 if the file is missing.
   *out_records receives a malloc'd array the caller must free(). */
int enc_load_records(const char *path, size_t rec_size, void **out_records) {
    *out_records = NULL;
    FILE *fp = fopen(path, "rb");
    if (!fp) return -1;

    int encrypted = enc_file_is_encrypted(path);
    if (encrypted) fseek(fp, ENC_MAGIC_LEN, SEEK_SET);
    else rewind(fp);

    /* Determine capacity by file size. */
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    if (encrypted) fsize -= ENC_MAGIC_LEN;
    fseek(fp, encrypted ? ENC_MAGIC_LEN : 0, SEEK_SET);
    if (fsize <= 0 || rec_size == 0) {
        fclose(fp);
        return 0;
    }

    int capacity = (int)(fsize / (long)rec_size);
    if (capacity <= 0) {
        fclose(fp);
        return 0;
    }

    unsigned char *buf = (unsigned char *)malloc((size_t)capacity * rec_size);
    if (!buf) {
        fclose(fp);
        return -1;
    }

    int count = 0;
    while (count < capacity) {
        if (fread(buf + (size_t)count * rec_size, rec_size, 1, fp) != 1) break;
        count++;
    }
    fclose(fp);

    if (encrypted && count > 0) {
        xor_crypt(buf, buf, (size_t)count * rec_size);
    }

    if (count == 0) {
        free(buf);
        *out_records = NULL;
        return 0;
    }

    *out_records = buf;
    return count;
}
