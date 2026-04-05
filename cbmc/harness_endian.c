#include <assert.h>

#include "../src/ssz_internal.h"

extern uint8_t nondet_uint8_t(void);
extern uint16_t nondet_uint16_t(void);
extern uint32_t nondet_uint32_t(void);
extern uint64_t nondet_uint64_t(void);

static void check_u16_from_value(void)
{
    uint16_t value = nondet_uint16_t();
    uint8_t out[2];

    ssz_internal_write_u16_le(out, value);

    assert(out[0] == (uint8_t)(value & 0xFFu));
    assert(out[1] == (uint8_t)((value >> 8u) & 0xFFu));
    assert(ssz_internal_read_u16_le(out) == value);
}

static void check_u16_from_bytes(void)
{
    uint8_t in[2];
    uint8_t out[2];
    uint16_t value;

    in[0] = nondet_uint8_t();
    in[1] = nondet_uint8_t();

    value = ssz_internal_read_u16_le(in);

    assert(value == ((uint16_t)in[0] | (uint16_t)((uint16_t)in[1] << 8u)));

    ssz_internal_write_u16_le(out, value);

    assert(out[0] == in[0]);
    assert(out[1] == in[1]);
}

static void check_u32_from_value(void)
{
    uint32_t value = nondet_uint32_t();
    uint8_t out[4];

    ssz_internal_write_u32_le(out, value);

    assert(out[0] == (uint8_t)(value & 0xFFu));
    assert(out[1] == (uint8_t)((value >> 8u) & 0xFFu));
    assert(out[2] == (uint8_t)((value >> 16u) & 0xFFu));
    assert(out[3] == (uint8_t)((value >> 24u) & 0xFFu));
    assert(ssz_internal_read_u32_le(out) == value);
}

static void check_u32_from_bytes(void)
{
    uint8_t in[4];
    uint8_t out[4];
    uint32_t value;

    in[0] = nondet_uint8_t();
    in[1] = nondet_uint8_t();
    in[2] = nondet_uint8_t();
    in[3] = nondet_uint8_t();

    value = ssz_internal_read_u32_le(in);

    assert(value == ((uint32_t)in[0] | ((uint32_t)in[1] << 8u) | ((uint32_t)in[2] << 16u) |
                     ((uint32_t)in[3] << 24u)));

    ssz_internal_write_u32_le(out, value);

    assert(out[0] == in[0]);
    assert(out[1] == in[1]);
    assert(out[2] == in[2]);
    assert(out[3] == in[3]);
}

static void check_u64_from_value(void)
{
    uint64_t value = nondet_uint64_t();
    uint8_t out[8];

    ssz_internal_write_u64_le(out, value);

    assert(out[0] == (uint8_t)(value & 0xFFu));
    assert(out[1] == (uint8_t)((value >> 8u) & 0xFFu));
    assert(out[2] == (uint8_t)((value >> 16u) & 0xFFu));
    assert(out[3] == (uint8_t)((value >> 24u) & 0xFFu));
    assert(out[4] == (uint8_t)((value >> 32u) & 0xFFu));
    assert(out[5] == (uint8_t)((value >> 40u) & 0xFFu));
    assert(out[6] == (uint8_t)((value >> 48u) & 0xFFu));
    assert(out[7] == (uint8_t)((value >> 56u) & 0xFFu));
    assert(ssz_internal_read_u64_le(out) == value);
}

static void check_u64_from_bytes(void)
{
    uint8_t in[8];
    uint8_t out[8];
    uint64_t value;

    in[0] = nondet_uint8_t();
    in[1] = nondet_uint8_t();
    in[2] = nondet_uint8_t();
    in[3] = nondet_uint8_t();
    in[4] = nondet_uint8_t();
    in[5] = nondet_uint8_t();
    in[6] = nondet_uint8_t();
    in[7] = nondet_uint8_t();

    value = ssz_internal_read_u64_le(in);

    assert(value == ((uint64_t)in[0] | ((uint64_t)in[1] << 8u) | ((uint64_t)in[2] << 16u) |
                     ((uint64_t)in[3] << 24u) | ((uint64_t)in[4] << 32u) |
                     ((uint64_t)in[5] << 40u) | ((uint64_t)in[6] << 48u) |
                     ((uint64_t)in[7] << 56u)));

    ssz_internal_write_u64_le(out, value);

    assert(out[0] == in[0]);
    assert(out[1] == in[1]);
    assert(out[2] == in[2]);
    assert(out[3] == in[3]);
    assert(out[4] == in[4]);
    assert(out[5] == in[5]);
    assert(out[6] == in[6]);
    assert(out[7] == in[7]);
}

void harness_endian(void)
{
    check_u16_from_value();
    check_u16_from_bytes();
    check_u32_from_value();
    check_u32_from_bytes();
    check_u64_from_value();
    check_u64_from_bytes();
}
