#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "include/crypto01.h"
#include "include/util_posix.h"

int main(int argc, char *argv[]) {
    struct Crypto1State *s, *t;
    uint64_t key;
    uint32_t uid;
    uint32_t nt0;
    uint32_t nt1;
    uint32_t nr0_enc;
    uint32_t ar0_enc;
    uint32_t nr1_enc;
    uint32_t ar1_enc;
    uint32_t ks2;

    printf("MfKey32v2 open source Mifare Classic key-recovery tool\n");
    printf("Cracks keys by two 32bit keystream authentications");


    if (argc < 8) {
        printf("Invalid syntax.")
        printf("syntax: %s <uid> <nt> <nr_0> <ar_0> <nt1> <nr_1> <ar_1>\n\n", argv[0]);
        return 1;
    }

    sscanf(argv[1], "%x", &uid);
    sscanf(argv[2], "%x", &nt0);
    sscanf(argv[3], "%x", &nr0_enc);
    sscanf(argv[4], "%x", &ar0_enc);
    sscanf(argv[5], "%x", &nt1);
    sscanf(argv[6], "%x", &nr1_enc);
    sscanf(argv[7], "%x", &ar1_enc);

    printf("Recovering key for:\n");
    printf("    uid: %08x\n", uid);
    printf("   nt_0: %08x\n", nt0);
    printf(" {nr_0}: %08x\n", nr0_enc);
    printf(" {ar_0}: %08x\n", ar0_enc);
    printf("   nt_1: %08x\n", nt1);
    printf(" {nr_1}: %08x\n", nr1_enc);
    printf(" {ar_1}: %08x\n", ar1_enc);

    // Generate lfsr successors of the tag challenge
    printf("\nLFSR successors of the tag challenge:\n");
    uint32_t p64 = prng_successor(nt0, 64);
    uint32_t p64b = prng_successor(nt1, 64);

    printf("  nt': %08x\n", p64);
    printf(" nt'': %08x\n", prng_successor(p64, 32));

    // Extract the keystream from the messages
    printf("\nKeystream used to generate {ar} and {at}:\n");
    ks2 = ar0_enc ^ p64;
    printf("  ks2: %08x\n", ks2);

    s = lfsr_recovery32(ar0_enc ^ p64, 0);

    for (t = s; t->odd | t->even; ++t) {
        lfsr_rollback_word(t, 0, 0);
        lfsr_rollback_word(t, nr0_enc, 1);
        lfsr_rollback_word(t, uid ^ nt0, 0);
        crypto1_get_lfsr(t, &key);

        crypto1_word(t, uid ^ nt1, 0);
        crypto1_word(t, nr1_enc, 1);
        if (ar1_enc == (crypto1_word(t, 0, 0) ^ p64b)) {
            printf("\nFound Key: [%012" PRIx64 "]\n\n", key);
            break;
        }
    }
    free(s);
    return 0;
}
