# Finding 0001: full uint16 little-endian SSZ theory

Status: proved

Targets: `c-ssz/src/ssz_endian.c`, functions `ssz_internal_write_u16_le`
and `ssz_internal_read_u16_le`

Reference: `docs/spec/simple-serialize.md`, `uintN` serialization rule:
`value.to_bytes(N // BITS_PER_BYTE, "little")`, instantiated at `N = 16`,
plus the deserialization section's injectivity requirement.

What was proved:

- `body_ssz_internal_write_u16_le`: for any `uint16_t` value in range and
  writable two-byte `uint8_t` output buffer, the C writer stores byte 0 as
  `value mod 256` and byte 1 as `(value / 256) mod 256`.
- `body_ssz_internal_read_u16_le`: for any readable two-byte input buffer, the
  C reader returns the Coq decoder `ssz_u16_le_decode`.
- `ssz_u16_le_decode_encode_roundtrip`: decoding the model encoding of any
  value in `0..65535` returns the original value.
- `ssz_u16_le_bytes_injective`: distinct values in `0..65535` have distinct
  model encodings.

The model-side encoder is `ssz_u16_le_bytes`; the decoder is
`ssz_u16_le_decode`, both in `formal/model/ssz_endian_model.v`.

Triage: no C defect found in either uint16 endian helper.
