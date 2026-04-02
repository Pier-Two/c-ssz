#include <string.h>

#include "ssz_internal.h"

static ssz_error_t ssz_internal_measure_member(
    ssz_member_codec_t *codec,
    uint64_t member_id,
    size_t *out_len)
{
    ssz_error_t err = SSZ_SUCCESS;

    if ((codec == NULL) || (codec->write == NULL) || (out_len == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = codec->write(codec->ctx, member_id, NULL, 0u, out_len);
    }

    return err;
}

static ssz_error_t ssz_internal_capture_member(
    ssz_member_codec_t *codec,
    uint64_t member_id,
    uint8_t *out_bytes,
    size_t out_cap,
    size_t expected_len)
{
    size_t written = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if ((codec == NULL) || (codec->write == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if ((expected_len != 0u) && (out_bytes == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (out_cap < expected_len)
    {
        err = SSZ_ERR_BUFFER_TOO_SMALL;
    }
    else if (expected_len != 0u)
    {
        err = codec->write(codec->ctx, member_id, out_bytes, out_cap, &written);
        if ((err == SSZ_SUCCESS) && (written != expected_len))
        {
            err = SSZ_ERR_TYPE_MISMATCH;
        }
    }

    return err;
}

static ssz_error_t ssz_internal_default_member(ssz_member_codec_t *codec, uint64_t member_id)
{
    ssz_error_t err = SSZ_SUCCESS;

    if ((codec == NULL) || (codec->read == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = codec->read(codec->ctx, member_id, NULL, 0u);
    }

    return err;
}

static ssz_error_t ssz_internal_restore_member(
    ssz_member_codec_t *codec,
    uint64_t member_id,
    const uint8_t *bytes,
    size_t byte_len)
{
    ssz_error_t err = SSZ_SUCCESS;

    if ((codec == NULL) || (codec->read == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = codec->read(codec->ctx, member_id, bytes, byte_len);
    }

    return err;
}

static ssz_error_t ssz_internal_member_is_default(
    ssz_member_codec_t *codec,
    uint64_t member_id,
    uint8_t *scratch,
    size_t scratch_len,
    bool *out_is_default)
{
    size_t current_len = 0u;
    size_t default_len = 0u;
    size_t total_len = 0u;
    uint8_t *current_bytes = NULL;
    uint8_t *default_bytes = NULL;
    bool is_default = false;
    ssz_error_t err = SSZ_SUCCESS;

    if (out_is_default == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if ((codec == NULL) || (codec->read == NULL) || (codec->write == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = ssz_internal_measure_member(codec, member_id, &current_len);
        if (err == SSZ_SUCCESS)
        {
            current_bytes = scratch;

            if ((scratch == NULL) || (scratch_len < current_len))
            {
                err = SSZ_ERR_BUFFER_TOO_SMALL;
            }
            else
            {
                err = ssz_internal_capture_member(codec, member_id, current_bytes, current_len, current_len);
            }
            if (err == SSZ_SUCCESS)
            {
                err = ssz_internal_default_member(codec, member_id);
                if (err == SSZ_SUCCESS)
                {
                    ssz_error_t restore_err = SSZ_SUCCESS;

                    err = ssz_internal_measure_member(codec, member_id, &default_len);
                    if (err == SSZ_SUCCESS)
                    {
                        if (ssz_internal_add_overflow_size(current_len, default_len, &total_len))
                        {
                            err = SSZ_ERR_OVERFLOW;
                        }
                        else if (scratch_len < total_len)
                        {
                            err = SSZ_ERR_BUFFER_TOO_SMALL;
                        }
                        else
                        {
                            default_bytes = &scratch[current_len];
                            err = ssz_internal_capture_member(
                                codec, member_id, default_bytes, default_len, default_len);
                            if (err == SSZ_SUCCESS)
                            {
                                is_default =
                                    (current_len == default_len) &&
                                    ((current_len == 0u) ||
                                     (memcmp(current_bytes, default_bytes, current_len) == 0));
                            }
                        }
                    }

                    restore_err = ssz_internal_restore_member(codec, member_id, current_bytes, current_len);
                    if (restore_err != SSZ_SUCCESS)
                    {
                        err = restore_err;
                    }
                }
            }
        }
    }

    if (err == SSZ_SUCCESS)
    {
        *out_is_default = is_default;
    }

    return err;
}

static ssz_error_t ssz_internal_default_members(uint64_t member_count, ssz_member_codec_t *codec)
{
    ssz_error_t err = SSZ_SUCCESS;

    if ((codec == NULL) || (codec->read == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        for (uint64_t i = 0u; i < member_count; i++)
        {
            err = ssz_internal_default_member(codec, i);
            if (err != SSZ_SUCCESS)
            {
                break;
            }
        }
    }

    return err;
}

ssz_error_t ssz_default_container(
    const size_t *field_fixed_sizes,
    uint32_t field_count,
    ssz_member_codec_t *codec)
{
    ssz_error_t err = SSZ_SUCCESS;

    if ((field_fixed_sizes == NULL) || (field_count == 0u))
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else
    {
        err = ssz_internal_default_members((uint64_t)field_count, codec);
    }

    return err;
}

ssz_error_t ssz_default_union(
    uint32_t option_count,
    bool has_none,
    ssz_member_codec_t *codec,
    uint8_t *out_selector)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (out_selector == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (option_count == 0u)
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else if (option_count > 256u)
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else if (has_none && (option_count < 2u))
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else
    {
        *out_selector = 0u;
        if (!has_none)
        {
            err = ssz_internal_default_member(codec, 0u);
        }
    }

    return err;
}

ssz_error_t ssz_is_zero_vector_composite(
    uint64_t element_count,
    ssz_member_codec_t *codec,
    uint8_t *scratch,
    size_t scratch_len,
    bool *out_is_zero)
{
    ssz_error_t err = SSZ_SUCCESS;
    bool is_zero = true;

    if (out_is_zero == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (element_count == 0u)
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else if ((codec == NULL) || (codec->read == NULL) || (codec->write == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        for (uint64_t i = 0u; (i < element_count) && (err == SSZ_SUCCESS) && is_zero; i++)
        {
            bool member_is_zero = false;

            err = ssz_internal_member_is_default(codec, i, scratch, scratch_len, &member_is_zero);
            if (err == SSZ_SUCCESS)
            {
                is_zero = member_is_zero;
            }
        }
    }

    if (err == SSZ_SUCCESS)
    {
        *out_is_zero = is_zero;
    }

    return err;
}

ssz_error_t ssz_is_zero_container(
    const size_t *field_fixed_sizes,
    uint32_t field_count,
    ssz_member_codec_t *codec,
    uint8_t *scratch,
    size_t scratch_len,
    bool *out_is_zero)
{
    ssz_error_t err = SSZ_SUCCESS;
    bool is_zero = true;

    if (out_is_zero == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if ((field_fixed_sizes == NULL) || (field_count == 0u))
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else if ((codec == NULL) || (codec->read == NULL) || (codec->write == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        for (uint32_t i = 0u; (i < field_count) && (err == SSZ_SUCCESS) && is_zero; i++)
        {
            bool member_is_zero = false;

            err = ssz_internal_member_is_default(codec, i, scratch, scratch_len, &member_is_zero);
            if (err == SSZ_SUCCESS)
            {
                is_zero = member_is_zero;
            }
        }
    }

    if (err == SSZ_SUCCESS)
    {
        *out_is_zero = is_zero;
    }

    return err;
}

ssz_error_t ssz_is_zero_union(
    uint8_t selector,
    uint32_t option_count,
    bool has_none,
    ssz_member_codec_t *codec,
    uint8_t *scratch,
    size_t scratch_len,
    bool *out_is_zero)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (out_is_zero == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (option_count == 0u)
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else if (option_count > 256u)
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else if (has_none && (option_count < 2u))
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else if ((uint32_t)selector >= option_count)
    {
        err = SSZ_ERR_SELECTOR_INVALID;
    }
    else if (selector != 0u)
    {
        *out_is_zero = false;
    }
    else if (has_none)
    {
        *out_is_zero = true;
    }
    else
    {
        err = ssz_internal_member_is_default(codec, 0u, scratch, scratch_len, out_is_zero);
    }

    return err;
}

const char *ssz_error_string(ssz_error_t error)
{
    const char *error_string = "SSZ_ERR_UNKNOWN";

    switch (error)
    {
        case SSZ_SUCCESS:
            error_string = "SSZ_SUCCESS";
            break;
        case SSZ_ERR_INVALID_ARGUMENT:
            error_string = "SSZ_ERR_INVALID_ARGUMENT";
            break;
        case SSZ_ERR_BUFFER_TOO_SMALL:
            error_string = "SSZ_ERR_BUFFER_TOO_SMALL";
            break;
        case SSZ_ERR_OVERFLOW:
            error_string = "SSZ_ERR_OVERFLOW";
            break;
        case SSZ_ERR_LIMIT_EXCEEDED:
            error_string = "SSZ_ERR_LIMIT_EXCEEDED";
            break;
        case SSZ_ERR_SCHEMA_INVALID:
            error_string = "SSZ_ERR_SCHEMA_INVALID";
            break;
        case SSZ_ERR_ENCODING_INVALID:
            error_string = "SSZ_ERR_ENCODING_INVALID";
            break;
        case SSZ_ERR_OFFSET_INVALID:
            error_string = "SSZ_ERR_OFFSET_INVALID";
            break;
        case SSZ_ERR_TYPE_MISMATCH:
            error_string = "SSZ_ERR_TYPE_MISMATCH";
            break;
        case SSZ_ERR_SELECTOR_INVALID:
            error_string = "SSZ_ERR_SELECTOR_INVALID";
            break;
        case SSZ_ERR_GINDEX_INVALID:
            error_string = "SSZ_ERR_GINDEX_INVALID";
            break;
        case SSZ_ERR_PROOF_INVALID:
            error_string = "SSZ_ERR_PROOF_INVALID";
            break;
        case SSZ_ERR_HASH_FAILURE:
            error_string = "SSZ_ERR_HASH_FAILURE";
            break;
        default:
            break;
    }

    return error_string;
}
