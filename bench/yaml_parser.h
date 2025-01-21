#ifndef YAML_PARSER_H
#define YAML_PARSER_H

#include <stdint.h>
#include <stddef.h>

typedef struct
{
    char *key;
    char *value;
} YamlKeyValPair;

typedef struct
{
    YamlKeyValPair *pairs;
    size_t num_pairs;
    size_t capacity;
} YamlObject;

uint8_t *read_yaml_field(const char *file_path, const char *field_name, size_t *out_size);
YamlObject *read_yaml_array_of_objects(const char *file_path, const char *array_name, size_t *out_count);

#endif /* YAML_PARSER_H */