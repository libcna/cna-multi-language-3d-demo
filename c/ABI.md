# CNA C API consumer

The C portion of this repository is intentionally a consumer, not another
implementation of the Starfield game and not a second C ABI façade. It includes
CNA's public `<CNA/C/cna.h>` header and links the `cna_c_api` target provided by
the sibling `../cna` repository.

Configure the project with `STARFIELD_ENABLE_CNA=ON` to build
`cna_c_api_consumer_smoke`. The smoke program checks that the ABI version
reported by the linked CNA runtime matches the version in the public headers.
It contains no C++ source and does not link `starfield_core`.

The Starfield gameplay contract remains implemented by the C++ reference in
`cpp`. A future C demo or renderer adapter must use CNA's existing C API rather
than introducing a parallel game-specific ABI in this directory.