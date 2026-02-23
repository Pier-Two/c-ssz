#include "ssz_types.h"

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
