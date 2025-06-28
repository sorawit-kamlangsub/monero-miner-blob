#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "randomx.h"


void hex_to_bin(const char* hex, uint8_t* out, size_t len) {
    for (size_t i = 0; i < len; i++) {
        sscanf(hex + 2 * i, "%2hhx", &out[i]);
    }
}

int extract_field(const char* json, const char* key, char* output, size_t max_len) {
    const char* found = strstr(json, key);
    if (!found) return 0;

    const char* quote1 = strchr(found, '"');
    if (!quote1) return 0;
    const char* quote2 = strchr(quote1 + 1, '"');
    if (!quote2) return 0;
    const char* quote3 = strchr(quote2 + 1, '"');
    if (!quote3) return 0;
    const char* quote4 = strchr(quote3 + 1, '"');
    if (!quote4) return 0;

    size_t len = quote4 - quote3 - 1;
    if (len >= max_len) return 0;
    strncpy(output, quote3 + 1, len);
    output[len] = '\0';
    return 1;
}

void write_result_json(const char* filename,
                       const char* blob,
                       const char* target,
                       const char* seed_hash,
                       uint32_t nonce,
                       const uint8_t* hash, size_t hash_len) 
{
    FILE* f = fopen(filename, "w");
    if (!f) {
        perror("Failed to open file for writing");
        return;
    }

    fprintf(f, "{\n");
    fprintf(f, "  \"blob\": \"%s\",\n", blob);
    fprintf(f, "  \"target\": \"%s\",\n", target);
    fprintf(f, "  \"seed_hash\": \"%s\",\n", seed_hash);
    fprintf(f, "  \"nonce\": %u,\n", nonce);

    fprintf(f, "  \"hash\": \"");
    for (size_t i = 0; i < hash_len; i++) {
        fprintf(f, "%02x", hash[i]);
    }
    fprintf(f, "\"\n");

    fprintf(f, "}\n");

    fclose(f);
}

int main() {
    char blob_hex[200] = {0};
    char seed_hex[100] = {0};
    char target_hex[10] = {0};

    FILE* f = fopen("job.json", "r");
    if (!f) {
        perror("job.json");
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    rewind(f);
    char* json = (char*)malloc(fsize + 1);
    if (!json) {
        fclose(f);
        return 1;
    }
    fread(json, 1, fsize, f);
    json[fsize] = 0;
    fclose(f);

    if (!extract_field(json, "blob", blob_hex, sizeof(blob_hex)) ||
        !extract_field(json, "seed_hash", seed_hex, sizeof(seed_hex)) ||
        !extract_field(json, "target", target_hex, sizeof(target_hex))) {
        printf("❌ Failed to extract fields from job.json\n");
        free(json);
        return 1;
    }

    free(json);

    uint8_t blob[76];
    hex_to_bin(blob_hex, blob, 76);

    uint8_t seed[32];
    hex_to_bin(seed_hex, seed, 32);

    uint32_t target_compact = (uint32_t)strtoul(target_hex, NULL, 16);
    int exponent = target_compact >> 24;
    int mantissa = target_compact & 0xFFFFFF;
    uint64_t target;
    if (exponent <= 3) {
        target = mantissa >> (8 * (3 - exponent));
    } else {
        target = (uint64_t)mantissa << (8 * (exponent - 3));
    }

    printf("Mining target: 0x%lx\n", target);

    randomx_flags flags = RANDOMX_FLAG_DEFAULT | RANDOMX_FLAG_JIT | RANDOMX_FLAG_HARD_AES;
    randomx_cache* cache = randomx_alloc_cache(flags);
    randomx_init_cache(cache, seed, 32);
    randomx_vm* vm = randomx_create_vm(flags, cache, NULL);

    for (uint32_t nonce = 0; nonce < 1000000; nonce++) {
        memcpy(&blob[39], &nonce, 4);

        uint8_t hash[32];
        randomx_calculate_hash(vm, blob, 76, hash);

        if (nonce % 100000 == 0) {
            printf("Mining... tried nonce: %u\n", nonce);
        }

        uint64_t* hash64 = (uint64_t*)hash;
        if (hash64[3] < target) {
            printf(" Nonce found: %u\n", nonce);
            printf("Hash: ");
            for (int i = 0; i < 32; i++) printf("%02x", hash[i]);
            printf("\n");

            write_result_json("job_result.json", blob_hex, target_hex, seed_hex, nonce, hash, 32);
            break;
        }
    }

    randomx_destroy_vm(vm);
    randomx_release_cache(cache);

    return 0;
}
