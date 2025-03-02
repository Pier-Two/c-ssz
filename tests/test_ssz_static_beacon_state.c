#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include "snappy_decode.h"
#include "ssz_serialize.h"
#include "ssz_deserialize.h"
#include "ssz_constants.h"
#include "ssz_macros.h"
#include <time.h>
#include "ssz_merkle.h"
#include "yaml_parser.h"

#ifndef TESTS_DIR
#define TESTS_DIR "tests/fixtures/mainnet/phase0/ssz_static/BeaconState/ssz_random"
#endif

#define NUM_RUNS 1000
#define INNER_RUNS 1000

/* Fixed-size field constants for BeaconState */
#define SIZE_ROOT 32
#define SIZE_PUBKEY 48
#define SIZE_SLOT 8
#define SIZE_EPOCH 8
#define SIZE_VERSION 4
#define SIZE_VALIDATOR_INDEX 8
#define SIZE_COMMITTEE_INDEX 8
#define SIZE_DEPOSIT_COUNT 8
#define SIZE_BLOCK_HASH 32
#define SIZE_GWEI 8
#define SIZE_SLASHED 1
#define SIZE_FORK (SIZE_VERSION + SIZE_VERSION + SIZE_EPOCH)
#define SIZE_BEACON_BLOCK_HEADER (SIZE_SLOT + SIZE_VALIDATOR_INDEX + SIZE_ROOT + SIZE_ROOT + SIZE_ROOT)
#define SIZE_BLOCK_ROOTS (SLOTS_PER_HISTORICAL_ROOT * SIZE_ROOT)
#define SIZE_STATE_ROOTS (SLOTS_PER_HISTORICAL_ROOT * SIZE_ROOT)
#define SIZE_ETH1_DATA (SIZE_ROOT + SIZE_DEPOSIT_COUNT + SIZE_BLOCK_HASH)
#define SIZE_VALIDATOR (SIZE_PUBKEY + SIZE_ROOT + SIZE_GWEI + SIZE_SLASHED + SIZE_EPOCH + SIZE_EPOCH + SIZE_EPOCH + SIZE_EPOCH)
#define SIZE_CHECKPOINT (SIZE_EPOCH + SIZE_ROOT)
#define SIZE_ATTESTATION_DATA (SIZE_SLOT + SIZE_COMMITTEE_INDEX + SIZE_ROOT + SIZE_CHECKPOINT + SIZE_CHECKPOINT)

/* Beacon State constants */
#define SLOTS_PER_HISTORICAL_ROOT 8192
#define HISTORICAL_ROOTS_LENGTH 16777216
#define EPOCHS_PER_HISTORICAL_VECTOR 65536
#define EPOCHS_PER_SLASHINGS_VECTOR 8192
#define JUSTIFICATION_BITS_LENGTH 4
#define SLOTS_PER_EPOCH 32
#define EPOCHS_PER_ETH1_VOTING_PERIOD 64
#define MAX_VALIDATORS_PER_COMMITTEE 128
#define VALIDATOR_REGISTRY_LIMIT 1099511627776
#define MAX_ATTESTATIONS 128
#define MAX_FAILURES 1024

typedef struct
{
    uint8_t previous_version[4];
    uint8_t current_version[4];
    uint64_t epoch;
} Fork;

typedef struct
{
    uint64_t slot;
    uint64_t proposer_index;
    uint8_t parent_root[SIZE_ROOT];
    uint8_t state_root[SIZE_ROOT];
    uint8_t body_root[SIZE_ROOT];
} BeaconBlockHeader;

typedef struct
{
    uint64_t length;
    uint8_t *data;
} HistoricalRoots;

typedef struct
{
    uint8_t deposit_root[SIZE_ROOT];
    uint64_t deposit_count;
    uint8_t block_hash[SIZE_ROOT];
} Eth1Data;

typedef struct
{
    uint64_t length;
    Eth1Data *data;
} Eth1DataVotes;

typedef struct
{
    uint8_t pubkey[48];
    uint8_t withdrawal_credentials[SIZE_ROOT];
    uint64_t effective_balance;
    bool slashed;
    uint64_t activation_eligibility_epoch;
    uint64_t activation_epoch;
    uint64_t exit_epoch;
    uint64_t withdrawable_epoch;
} Validator;

typedef struct
{
    uint64_t length;
    Validator *data;
} Validators;

typedef struct
{
    uint64_t length;
    uint64_t *data;
} Balances;

typedef struct
{
    uint64_t epoch;
    uint8_t root[SIZE_ROOT];
} Checkpoint;

typedef struct
{
    uint64_t length;
    bool *data;
} AggregationBits;

typedef struct
{
    uint64_t slot;
    uint64_t index;
    uint8_t beacon_block_root[SIZE_ROOT];
    Checkpoint source;
    Checkpoint target;
} AttestationData;

typedef struct
{
    AggregationBits aggregation_bits;
    AttestationData data;
    uint64_t inclusion_delay;
    uint64_t proposer_index;
} PendingAttestation;

typedef struct
{
    uint64_t length;
    PendingAttestation *data;
} EpochAttestations;

typedef struct
{
    uint64_t genesis_time;
    uint8_t genesis_validators_root[SIZE_ROOT];
    uint64_t slot;
    Fork fork;
    BeaconBlockHeader latest_block_header;
    uint8_t block_roots[SLOTS_PER_HISTORICAL_ROOT][SIZE_ROOT];
    uint8_t state_roots[SLOTS_PER_HISTORICAL_ROOT][SIZE_ROOT];
    HistoricalRoots historical_roots;
    Eth1Data eth1_data;
    Eth1DataVotes eth1_data_votes;
    uint64_t eth1_deposit_index;
    Validators validators;
    Balances balances;
    uint8_t randao_mixes[EPOCHS_PER_HISTORICAL_VECTOR][SIZE_ROOT];
    uint64_t slashings[EPOCHS_PER_SLASHINGS_VECTOR];
    EpochAttestations previous_epoch_attestations;
    EpochAttestations current_epoch_attestations;
    bool justification_bits[JUSTIFICATION_BITS_LENGTH];
    Checkpoint previous_justified_checkpoint;
    Checkpoint current_justified_checkpoint;
    Checkpoint finalized_checkpoint;
} BeaconState;

#define FORK_FIELD                                                                         \
    DESERIALIZE_VECTOR_FIELD(obj, offset, previous_version, ssz_deserialize_vector_uint8); \
    DESERIALIZE_VECTOR_FIELD(obj, offset, current_version, ssz_deserialize_vector_uint8);  \
    DESERIALIZE_BASIC_FIELD(obj, offset, epoch, ssz_deserialize_uint64);
DEFINE_DESERIALIZE_CONTAINER(Fork, FORK_FIELD);

#define BEACONBLOCKHEADER_FIELDS                                                      \
    DESERIALIZE_BASIC_FIELD(obj, offset, slot, ssz_deserialize_uint64);               \
    DESERIALIZE_BASIC_FIELD(obj, offset, proposer_index, ssz_deserialize_uint64);     \
    DESERIALIZE_VECTOR_FIELD(obj, offset, parent_root, ssz_deserialize_vector_uint8); \
    DESERIALIZE_VECTOR_FIELD(obj, offset, state_root, ssz_deserialize_vector_uint8);  \
    DESERIALIZE_VECTOR_FIELD(obj, offset, body_root, ssz_deserialize_vector_uint8);
DEFINE_DESERIALIZE_CONTAINER(BeaconBlockHeader, BEACONBLOCKHEADER_FIELDS);

#define ETH1DATA_FIELD                                                                 \
    DESERIALIZE_VECTOR_FIELD(obj, offset, deposit_root, ssz_deserialize_vector_uint8); \
    DESERIALIZE_BASIC_FIELD(obj, offset, deposit_count, ssz_deserialize_uint64);       \
    DESERIALIZE_VECTOR_FIELD(obj, offset, block_hash, ssz_deserialize_vector_uint8);
DEFINE_DESERIALIZE_CONTAINER(Eth1Data, ETH1DATA_FIELD);
DEFINE_DESERIALIZE_LIST(Eth1DataVotes, Eth1Data, SIZE_ETH1_DATA, deserialize_Eth1Data);

#define VALIDATOR_FIELD                                                                          \
    DESERIALIZE_VECTOR_FIELD(obj, offset, pubkey, ssz_deserialize_vector_uint8);                 \
    DESERIALIZE_VECTOR_FIELD(obj, offset, withdrawal_credentials, ssz_deserialize_vector_uint8); \
    DESERIALIZE_BASIC_FIELD(obj, offset, effective_balance, ssz_deserialize_uint64);             \
    DESERIALIZE_BASIC_FIELD(obj, offset, slashed, ssz_deserialize_boolean);                      \
    DESERIALIZE_BASIC_FIELD(obj, offset, activation_eligibility_epoch, ssz_deserialize_uint64);  \
    DESERIALIZE_BASIC_FIELD(obj, offset, activation_epoch, ssz_deserialize_uint64);              \
    DESERIALIZE_BASIC_FIELD(obj, offset, exit_epoch, ssz_deserialize_uint64);                    \
    DESERIALIZE_BASIC_FIELD(obj, offset, withdrawable_epoch, ssz_deserialize_uint64);
DEFINE_DESERIALIZE_CONTAINER(Validator, VALIDATOR_FIELD);
DEFINE_DESERIALIZE_LIST(Validators, Validator, SIZE_VALIDATOR, deserialize_Validator);

#define CHECKPOINT_FIELD                                                 \
    DESERIALIZE_BASIC_FIELD(obj, offset, epoch, ssz_deserialize_uint64); \
    DESERIALIZE_VECTOR_FIELD(obj, offset, root, ssz_deserialize_vector_uint8);
DEFINE_DESERIALIZE_CONTAINER(Checkpoint, CHECKPOINT_FIELD);

#define ATTESTATION_DATA_FIELD                                                                  \
    DESERIALIZE_BASIC_FIELD(obj, offset, slot, ssz_deserialize_uint64);                        \
    DESERIALIZE_BASIC_FIELD(obj, offset, index, ssz_deserialize_uint64);                       \
    DESERIALIZE_VECTOR_FIELD(obj, offset, beacon_block_root, ssz_deserialize_vector_uint8);    \
    DESERIALIZE_CONTAINER_FIELD(obj, offset, source, deserialize_Checkpoint, SIZE_CHECKPOINT); \
    DESERIALIZE_CONTAINER_FIELD(obj, offset, target, deserialize_Checkpoint, SIZE_CHECKPOINT);
DEFINE_DESERIALIZE_CONTAINER(AttestationData, ATTESTATION_DATA_FIELD);

#define PENDING_ATTESTATION_FIELD                                                                        \
    uint32_t agg_bits_rel_offset = 0;                                                                   \
    DESERIALIZE_OFFSET_FIELD(agg_bits_rel_offset, offset, "aggregation_bits");                          \
    DESERIALIZE_CONTAINER_FIELD(obj, offset, data, deserialize_AttestationData, SIZE_ATTESTATION_DATA); \
    DESERIALIZE_BASIC_FIELD(obj, offset, inclusion_delay, ssz_deserialize_uint64);                      \
    DESERIALIZE_BASIC_FIELD(obj, offset, proposer_index, ssz_deserialize_uint64);                       \
    size_t agg_bits_size = data_size - agg_bits_rel_offset;                                             \
    DESERIALIZE_BITLIST_FIELD(obj, agg_bits_rel_offset, agg_bits_size, aggregation_bits, MAX_VALIDATORS_PER_COMMITTEE);
DEFINE_DESERIALIZE_CONTAINER(PendingAttestation, PENDING_ATTESTATION_FIELD);

typedef struct
{
    char folder_name[256];
    char folder_path[1024];
    char message[1024];
} FailureDetail;

FailureDetail failures[MAX_FAILURES];
int failure_count = 0;
int total_valid_tests = 0;
int total_invalid_tests = 0;
int valid_passed = 0;
int valid_failed = 0;
int invalid_passed = 0;
int invalid_failed = 0;

void record_failure(const char *folder_name, const char *folder_path, const char *message)
{
    if (failure_count < MAX_FAILURES)
    {
        snprintf(failures[failure_count].folder_name, sizeof(failures[failure_count].folder_name), "%s", folder_name);
        snprintf(failures[failure_count].folder_path, sizeof(failures[failure_count].folder_path), "%s", folder_path);
        snprintf(failures[failure_count].message, sizeof(failures[failure_count].message), "%s", message);
        failure_count++;
    }
}

unsigned char *snappy_decode(const unsigned char *compressed_data, size_t compressed_size, size_t *decoded_size)
{
    size_t uncompressed_length;
    snappy_status status = snappy_uncompressed_length((const char *)compressed_data, compressed_size, &uncompressed_length);
    if (status != SNAPPY_OK)
    {
        fprintf(stderr, "Error: snappy_uncompressed_length failed with status %d\n", status);
        return NULL;
    }
    unsigned char *decoded = malloc(uncompressed_length);
    if (!decoded)
    {
        perror("malloc");
        return NULL;
    }
    status = snappy_uncompress((const char *)compressed_data, compressed_size, (char *)decoded, &uncompressed_length);
    if (status != SNAPPY_OK)
    {
        fprintf(stderr, "Error: snappy_uncompress failed with status %d\n", status);
        free(decoded);
        return NULL;
    }
    *decoded_size = uncompressed_length;
    return decoded;
}

unsigned char *read_file(const char *filepath, size_t *size_out)
{
    FILE *fp = fopen(filepath, "rb");
    if (!fp)
        return NULL;
    if (fseek(fp, 0, SEEK_END) != 0)
    {
        fclose(fp);
        return NULL;
    }
    long filesize = ftell(fp);
    if (filesize < 0)
    {
        fclose(fp);
        return NULL;
    }
    rewind(fp);
    unsigned char *buffer = malloc(filesize);
    if (!buffer)
    {
        fclose(fp);
        return NULL;
    }
    size_t read_bytes = fread(buffer, 1, filesize, fp);
    if (read_bytes != (size_t)filesize)
    {
        free(buffer);
        fclose(fp);
        return NULL;
    }
    fclose(fp);
    *size_out = (size_t)filesize;
    return buffer;
}

void print_hex(const unsigned char *data, size_t size)
{
    for (size_t i = 0; i < size; i++)
        printf("%02x", data[i]);
    printf("\n");
}

ssz_error_t deserialize_BeaconState_object(const unsigned char *data, size_t data_size, BeaconState *state)
{
    size_t offset = 0;

    // Deserialize genesis_time
    DESERIALIZE_BASIC_FIELD(state, offset, genesis_time, ssz_deserialize_uint64);

    // Deserialize genesis_validators_root
    DESERIALIZE_VECTOR_FIELD(state, offset, genesis_validators_root, ssz_deserialize_vector_uint8);

    // Deserialize slot
    DESERIALIZE_BASIC_FIELD(state, offset, slot, ssz_deserialize_uint64);

    // Deserialize fork
    DESERIALIZE_CONTAINER_FIELD(state, offset, fork, deserialize_Fork, SIZE_FORK);

    // Deserialize latest_block_header
    DESERIALIZE_CONTAINER_FIELD(state, offset, latest_block_header, deserialize_BeaconBlockHeader, SIZE_BEACON_BLOCK_HEADER);

    // Deserialize block_roots
    DESERIALIZE_VECTOR_ARRAY_FIELD(state, offset, block_roots, SIZE_ROOT, SLOTS_PER_HISTORICAL_ROOT, ssz_deserialize_vector_uint8);

    // Deserialize state_roots
    DESERIALIZE_VECTOR_ARRAY_FIELD(state, offset, state_roots, SIZE_ROOT, SLOTS_PER_HISTORICAL_ROOT, ssz_deserialize_vector_uint8);

    // Deserialize historical_roots offset
    uint32_t historical_roots_offset;
    DESERIALIZE_OFFSET_FIELD(historical_roots_offset, offset, "historical_roots");

    // Deserialize eth1_data
    DESERIALIZE_CONTAINER_FIELD(state, offset, eth1_data, deserialize_Eth1Data, SIZE_ETH1_DATA);

    // Deserialize eth1_data_votes
    uint32_t eth1_data_votes_offset;
    DESERIALIZE_OFFSET_FIELD(eth1_data_votes_offset, offset, "eth1_data_votes");

    // Deserialize eth1_deposit_index
    DESERIALIZE_BASIC_FIELD(state, offset, eth1_deposit_index, ssz_deserialize_uint64);

    // Deserialize validators offset
    uint32_t validators_offset;
    DESERIALIZE_OFFSET_FIELD(validators_offset, offset, "validators");

    // Deserialize balances offset
    uint32_t balances_offset;
    DESERIALIZE_OFFSET_FIELD(balances_offset, offset, "balances");

    // Deserialize randao_mixes
    DESERIALIZE_VECTOR_ARRAY_FIELD(state, offset, randao_mixes, SIZE_ROOT, EPOCHS_PER_HISTORICAL_VECTOR, ssz_deserialize_vector_uint8);

    // Deserialize slashings
    DESERIALIZE_VECTOR_FIELD(state, offset, slashings, ssz_deserialize_vector_uint64);

    // Deserialize previous_epoch_attestations offset
    uint32_t previous_epoch_attestations_offset;
    DESERIALIZE_OFFSET_FIELD(previous_epoch_attestations_offset, offset, "previous_epoch_attestations");

    // Deserialize current_epoch_attestations offset
    uint32_t current_epoch_attestations_offset;
    DESERIALIZE_OFFSET_FIELD(current_epoch_attestations_offset, offset, "current_epoch_attestations");

    // Deserialize justification_bits
    DESERIALIZE_BITVECTOR_FIELD(state, offset, justification_bits, JUSTIFICATION_BITS_LENGTH);

    // Deserialize previous_justified_checkpoint
    DESERIALIZE_CONTAINER_FIELD(state, offset, previous_justified_checkpoint, deserialize_Checkpoint, SIZE_CHECKPOINT);

    // Deserialize current_justified_checkpoint
    DESERIALIZE_CONTAINER_FIELD(state, offset, current_justified_checkpoint, deserialize_Checkpoint, SIZE_CHECKPOINT);

    // Deserialize historical_roots
    size_t hist_roots_size = eth1_data_votes_offset - historical_roots_offset;
    DESERIALIZE_LIST_FIELD(state, historical_roots_offset, hist_roots_size, historical_roots, HISTORICAL_ROOTS_LENGTH, ssz_deserialize_list_uint256);

    // Deserialize eth1_data_votes
    size_t eth1_data_votes_size = validators_offset - eth1_data_votes_offset;
    DESERIALIZE_LIST_CONTAINER_FIELD(state, eth1_data_votes_offset, eth1_data_votes_size, eth1_data_votes, deserialize_Eth1DataVotes);

    // Deserialize validators
    size_t validators_size = balances_offset - validators_offset;
    DESERIALIZE_LIST_CONTAINER_FIELD(state, validators_offset, validators_size, validators, deserialize_Validators);

    // Deserialize balances
    size_t balances_size = previous_epoch_attestations_offset - balances_offset;
    DESERIALIZE_LIST_FIELD(state, balances_offset, balances_size, balances, VALIDATOR_REGISTRY_LIMIT, ssz_deserialize_list_uint64);

    // Deserialize previous_epoch_attestations
    size_t previous_epoch_attestations_size = current_epoch_attestations_offset - previous_epoch_attestations_offset;

    if (previous_epoch_attestations_size == 0) {
        state->previous_epoch_attestations.length = 0;
        state->previous_epoch_attestations.data = NULL;
    } else {
        DESERIALIZE_LIST_VARIABLE_CONTAINER_FIELD(state, previous_epoch_attestations_offset, previous_epoch_attestations_size, previous_epoch_attestations, MAX_ATTESTATIONS * SLOTS_PER_EPOCH, deserialize_PendingAttestation);
    }

    // Deserialize current_epoch_attestations
    size_t current_epoch_attestations_size = data_size - current_epoch_attestations_offset;

    if (current_epoch_attestations_size == 0)
    {
        state->current_epoch_attestations.length = 0;
        state->current_epoch_attestations.data = NULL;
    }
    else
    {
        DESERIALIZE_LIST_VARIABLE_CONTAINER_FIELD(state, current_epoch_attestations_offset, current_epoch_attestations_size, current_epoch_attestations, MAX_ATTESTATIONS * SLOTS_PER_EPOCH, deserialize_PendingAttestation);
    }

    return SSZ_SUCCESS;
}

void process_serialized_file(const char *folder_name, const char *folder_path, const char *serialized_file_path, bool valid)
{
    (void)valid;
    size_t file_size;
    unsigned char *compressed_data = read_file(serialized_file_path, &file_size);
    if (!compressed_data)
    {
        fprintf(stderr, "Failed to read file: %s\n", serialized_file_path);
        record_failure(folder_name, folder_path, "Failed to read file");
        return;
    }
    size_t data_size;
    unsigned char *data = snappy_decode(compressed_data, file_size, &data_size);
    free(compressed_data);
    if (!data)
    {
        fprintf(stderr, "Snappy decode failed for file: %s\n", serialized_file_path);
        record_failure(folder_name, folder_path, "Snappy decode failed");
        return;
    }
    BeaconState state;
    ssz_error_t err = deserialize_BeaconState_object(data, data_size, &state);
    if (err != SSZ_SUCCESS)
    {
        fprintf(stderr, "Failed to deserialize BeaconState from file: %s\n", serialized_file_path);
    }
    else
    {
        printf("Successfully deserialized BeaconState for folder %s\n", folder_path);
    }
    free(data);
}

int main(void)
{
    DIR *dir = opendir(TESTS_DIR);
    if (!dir)
    {
        perror("opendir");
        return EXIT_FAILURE;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;
        char folder_path[1024];
        snprintf(folder_path, sizeof(folder_path), "%s/%s", TESTS_DIR, entry->d_name);
        struct stat st;
        if (stat(folder_path, &st) == -1 || !S_ISDIR(st.st_mode))
            continue;
        char serialized_file_path[1024];
        snprintf(serialized_file_path, sizeof(serialized_file_path), "%s/serialized.ssz_snappy", folder_path);
        if (stat(serialized_file_path, &st) == -1)
        {
            fprintf(stderr, "Skipping folder %s: serialized.ssz_snappy not found\n", folder_path);
            continue;
        }
        process_serialized_file(entry->d_name, folder_path, serialized_file_path, true);
    }
    closedir(dir);
    return EXIT_SUCCESS;
}