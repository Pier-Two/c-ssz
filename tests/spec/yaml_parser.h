#ifndef TESTS_SPEC_YAML_PARSER_H
#define TESTS_SPEC_YAML_PARSER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum
{
    YAML_NODE_NULL = 0,
    YAML_NODE_BOOL,
    YAML_NODE_INT,
    YAML_NODE_STRING,
    YAML_NODE_SEQUENCE,
    YAML_NODE_MAPPING,
} yaml_node_type_t;

typedef struct yaml_node yaml_node_t;

typedef struct
{
    char *key;
    yaml_node_t *value;
} yaml_pair_t;

struct yaml_node
{
    yaml_node_type_t type;
    union
    {
        bool bool_value;
        char *text;
        struct
        {
            yaml_node_t **items;
            size_t count;
        } sequence;
        struct
        {
            yaml_pair_t *pairs;
            size_t count;
        } mapping;
    } as;
};

yaml_node_t *yaml_parse_file(const char *path);
void yaml_node_free(yaml_node_t *node);

const yaml_node_t *yaml_mapping_get(const yaml_node_t *node, const char *key);

bool yaml_node_parse_u64(const yaml_node_t *node, uint64_t *out_value);
bool yaml_node_parse_decimal_le(const yaml_node_t *node, uint8_t *out, size_t out_len);
bool yaml_node_parse_hex(const yaml_node_t *node, uint8_t **out_bytes, size_t *out_len);

#endif
