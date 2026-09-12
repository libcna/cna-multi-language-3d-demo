#include <CNA/C/cna.h>

#include <stdint.h>
#include <stdio.h>

int main(void) {
    const uint32_t runtime_version = cna_get_abi_version();
    if (runtime_version != CNA_ABI_VERSION) {
        fprintf(stderr, "CNA ABI mismatch: runtime=%u headers=%u\n", runtime_version,
                (uint32_t)CNA_ABI_VERSION);
        return 1;
    }
    printf("CNA C ABI %u.%u.%u\n", CNA_ABI_VERSION_MAJOR, CNA_ABI_VERSION_MINOR,
           CNA_ABI_VERSION_PATCH);
    return 0;
}