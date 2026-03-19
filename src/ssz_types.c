#include <stdlib.h>
#include <string.h>

#include "ssz_internal.h"

static ssz_error_t ssz_internal_capture_member(
    ssz_member_codec_t *codec,
    uint64_t member_id,
    uint8_t **out_bytes,
    size_t *out_len)
{
    size_t expected_len = 0u;
    uint8_t *bytes = NULL;

    if ((codec == NULL) || (codec->write == NULL) || (out_bytes == NULL) || (out_len == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    ssz_error_t err = codec->write(codec->ctx, member_id, NULL, 0u, &expected_len);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    if (expected_len != 0u)
    {
        bytes = (uint8_t *)malloc(expected_len);
        if (bytes == NULL)
        {
            return SSZ_ERR_OVERFLOW;
        }

        size_t written = 0u;
        err = codec->write(codec->ctx, member_id, bytes, expected_len, &written);
        if ((err == SSZ_SUCCESS) && (written != expected_len))
        {
            err = SSZ_ERR_TYPE_MISMATCH;
        }
        if (err != SSZ_SUCCESS)
        {
            free(bytes);
            return err;
        }
    }

    *out_bytes = bytes;
    *out_len = expected_len;
    return SSZ_SUCCESS;
}

static ssz_error_t ssz_internal_default_member(ssz_member_codec_t *codec, uint64_t member_id)
{
    if ((codec == NULL) || (codec->read == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    return codec->read(codec->ctx, member_id, NULL, 0u);
}

static ssz_error_t ssz_internal_restore_member(
    ssz_member_codec_t *codec,
    uint64_t member_id,
    const uint8_t *bytes,
    size_t byte_len)
{
    if ((codec == NULL) || (codec->read == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    return codec->read(codec->ctx, member_id, bytes, byte_len);
}

static ssz_error_t ssz_internal_member_is_default(
    ssz_member_codec_t *codec,
    uint64_t member_id,
    bool *out_is_default)
{
    uint8_t *current_bytes = NULL;
    size_t current_len = 0u;
    uint8_t *default_bytes = NULL;
    size_t default_len = 0u;
    bool is_default = false;

    if (out_is_default == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if ((codec == NULL) || (codec->read == NULL) || (codec->write == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    ssz_error_t err = ssz_internal_capture_member(codec, member_id, &current_bytes, &current_len);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    err = ssz_internal_default_member(codec, member_id);
    if (err != SSZ_SUCCESS)
    {
        free(current_bytes);
        return err;
    }

    err = ssz_internal_capture_member(codec, member_id, &default_bytes, &default_len);
    if (err != SSZ_SUCCESS)
    {
        ssz_error_t restore_err =
            ssz_internal_restore_member(codec, member_id, current_bytes, current_len);
        free(current_bytes);
        if (restore_err != SSZ_SUCCESS)
        {
            return restore_err;
        }
        return err;
    }

    is_default =
        (current_len == default_len) &&
        ((current_len == 0u) || (memcmp(current_bytes, default_bytes, current_len) == 0));

    err = ssz_internal_restore_member(codec, member_id, current_bytes, current_len);

    free(default_bytes);
    free(current_bytes);

    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    *out_is_default = is_default;
    return SSZ_SUCCESS;
}

static ssz_error_t ssz_internal_default_members(uint64_t member_count, ssz_member_codec_t *codec)
{
    if ((codec == NULL) || (codec->read == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    for (uint64_t i = 0u; i < member_count; i++)
    {
        ssz_error_t err = ssz_internal_default_member(codec, i);
        if (err != SSZ_SUCCESS)
        {
            return err;
        }
    }

    return SSZ_SUCCESS;
}

ssz_error_t ssz_default_container(
    const size_t *field_fixed_sizes,
    uint32_t field_count,
    ssz_member_codec_t *codec)
{
    if ((field_fixed_sizes == NULL) || (field_count == 0u))
    {
        return SSZ_ERR_SCHEMA_INVALID;
    }

    return ssz_internal_default_members((uint64_t)field_count, codec);
}

ssz_error_t ssz_default_union(
    uint32_t option_count,
    bool has_none,
    ssz_member_codec_t *codec,
    uint8_t *out_selector)
{
    if (out_selector == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (option_count == 0u)
    {
        return SSZ_ERR_SCHEMA_INVALID;
    }
    if (option_count > 256u)
    {
        return SSZ_ERR_SCHEMA_INVALID;
    }
    if (has_none && (option_count < 2u))
    {
        return SSZ_ERR_SCHEMA_INVALID;
    }

    *out_selector = 0u;

    if (has_none)
    {
        return SSZ_SUCCESS;
    }

    return ssz_internal_default_member(codec, 0u);
}

ssz_error_t ssz_is_zero_vector_composite(
    uint64_t element_count,
    ssz_member_codec_t *codec,
    bool *out_is_zero)
{
    if (out_is_zero == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (element_count == 0u)
    {
        return SSZ_ERR_SCHEMA_INVALID;
    }
    if ((codec == NULL) || (codec->read == NULL) || (codec->write == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    for (uint64_t i = 0u; i < element_count; i++)
    {
        bool member_is_zero = false;
        ssz_error_t err = ssz_internal_member_is_default(codec, i, &member_is_zero);
        if (err != SSZ_SUCCESS)
        {
            return err;
        }
        if (!member_is_zero)
        {
            *out_is_zero = false;
            return SSZ_SUCCESS;
        }
    }

    *out_is_zero = true;
    return SSZ_SUCCESS;
}

ssz_error_t ssz_is_zero_container(
    const size_t *field_fixed_sizes,
    uint32_t field_count,
    ssz_member_codec_t *codec,
    bool *out_is_zero)
{
    if (out_is_zero == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if ((field_fixed_sizes == NULL) || (field_count == 0u))
    {
        return SSZ_ERR_SCHEMA_INVALID;
    }
    if ((codec == NULL) || (codec->read == NULL) || (codec->write == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    for (uint32_t i = 0u; i < field_count; i++)
    {
        bool member_is_zero = false;
        ssz_error_t err = ssz_internal_member_is_default(codec, i, &member_is_zero);
        if (err != SSZ_SUCCESS)
        {
            return err;
        }
        if (!member_is_zero)
        {
            *out_is_zero = false;
            return SSZ_SUCCESS;
        }
    }

    *out_is_zero = true;
    return SSZ_SUCCESS;
}

ssz_error_t ssz_is_zero_union(
    uint8_t selector,
    uint32_t option_count,
    bool has_none,
    ssz_member_codec_t *codec,
    bool *out_is_zero)
{
    if (out_is_zero == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (option_count == 0u)
    {
        return SSZ_ERR_SCHEMA_INVALID;
    }
    if (option_count > 256u)
    {
        return SSZ_ERR_SCHEMA_INVALID;
    }
    if (has_none && (option_count < 2u))
    {
        return SSZ_ERR_SCHEMA_INVALID;
    }
    if ((uint32_t)selector >= option_count)
    {
        return SSZ_ERR_SELECTOR_INVALID;
    }

    if (selector != 0u)
    {
        *out_is_zero = false;
        return SSZ_SUCCESS;
    }
    if (has_none)
    {
        *out_is_zero = true;
        return SSZ_SUCCESS;
    }

    return ssz_internal_member_is_default(codec, 0u, out_is_zero);
}

const char *ssz_error_string(ssz_error_t error)
{
    switch (error)
    {
        case SSZ_SUCCESS:
            return "SSZ_SUCCESS";
        case SSZ_ERR_INVALID_ARGUMENT:
            return "SSZ_ERR_INVALID_ARGUMENT";
        case SSZ_ERR_BUFFER_TOO_SMALL:
            return "SSZ_ERR_BUFFER_TOO_SMALL";
        case SSZ_ERR_OVERFLOW:
            return "SSZ_ERR_OVERFLOW";
        case SSZ_ERR_LIMIT_EXCEEDED:
            return "SSZ_ERR_LIMIT_EXCEEDED";
        case SSZ_ERR_SCHEMA_INVALID:
            return "SSZ_ERR_SCHEMA_INVALID";
        case SSZ_ERR_ENCODING_INVALID:
            return "SSZ_ERR_ENCODING_INVALID";
        case SSZ_ERR_OFFSET_INVALID:
            return "SSZ_ERR_OFFSET_INVALID";
        case SSZ_ERR_TYPE_MISMATCH:
            return "SSZ_ERR_TYPE_MISMATCH";
        case SSZ_ERR_SELECTOR_INVALID:
            return "SSZ_ERR_SELECTOR_INVALID";
        case SSZ_ERR_GINDEX_INVALID:
            return "SSZ_ERR_GINDEX_INVALID";
        case SSZ_ERR_PROOF_INVALID:
            return "SSZ_ERR_PROOF_INVALID";
        case SSZ_ERR_HASH_FAILURE:
            return "SSZ_ERR_HASH_FAILURE";
        default:
            break;
    }

    return "SSZ_ERR_UNKNOWN";
}
