#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "yaml_parser.h"

static int get_indentation(const char *line)
{
    int count = 0;
    while (*line == ' ' || *line == '\t')
    {
        count++;
        line++;
    }
    return count;
}

static char *trim_left(char *str)
{
    while (isspace((unsigned char)*str))
    {
        str++;
    }
    return str;
}

static void rstrip(char *str)
{
    char *end = str + strlen(str) - 1;
    while (end >= str && isspace((unsigned char)*end))
    {
        *end = '\0';
        end--;
    }
}

static char **split_field_name(const char *field_name, int *count)
{
    *count = 0;
    if (!field_name || !*field_name)
    {
        return NULL;
    }
    char *temp = strdup(field_name);
    if (!temp)
    {
        return NULL;
    }
    int segments = 1;
    for (char *p = temp; *p; p++)
    {
        if (*p == '.')
        {
            segments++;
        }
    }
    char **parts = (char **)calloc(segments, sizeof(char *));
    if (!parts)
    {
        free(temp);
        return NULL;
    }
    int idx = 0;
    char *token = strtok(temp, ".");
    while (token)
    {
        parts[idx++] = strdup(token);
        token = strtok(NULL, ".");
    }
    *count = segments;
    free(temp);
    return parts;
}

static char *build_path(char **keys, int key_count)
{
    if (key_count <= 0)
    {
        return NULL;
    }
    int total_len = 0;
    for (int i = 0; i < key_count; i++)
    {
        total_len += (int)strlen(keys[i]);
    }
    total_len += (key_count - 1);
    char *path = (char *)malloc(total_len + 1);
    if (!path)
    {
        return NULL;
    }
    path[0] = '\0';
    for (int i = 0; i < key_count; i++)
    {
        strcat(path, keys[i]);
        if (i < key_count - 1)
        {
            strcat(path, ".");
        }
    }
    return path;
}

static void pop_stack(char *keys_stack[], int *stack_size)
{
    if (*stack_size > 0)
    {
        free(keys_stack[*stack_size - 1]);
        (*stack_size)--;
    }
}

static void commit_current_object(YamlObject *current_obj, YamlObject **objects, size_t *objects_capacity, size_t *count)
{
    if (current_obj->num_pairs == 0)
    {
        return;
    }
    if (*count == *objects_capacity)
    {
        size_t new_cap = (*objects_capacity == 0) ? 4 : (*objects_capacity * 2);
        YamlObject *new_arr = (YamlObject *)realloc(*objects, new_cap * sizeof(YamlObject));
        if (!new_arr)
        {
            return;
        }
        *objects = new_arr;
        *objects_capacity = new_cap;
    }
    (*objects)[*count].pairs = current_obj->pairs;
    (*objects)[*count].num_pairs = current_obj->num_pairs;
    (*objects)[*count].capacity = current_obj->capacity;
    (*count)++;
    current_obj->pairs = NULL;
    current_obj->num_pairs = 0;
    current_obj->capacity = 0;
}

static void add_pair_to_current(YamlObject *current_obj, const char *key, const char *value)
{
    if (!key || !value)
    {
        return;
    }
    if (current_obj->num_pairs == current_obj->capacity)
    {
        size_t new_cap = (current_obj->capacity == 0) ? 4 : (current_obj->capacity * 2);
        YamlKeyValPair *new_pairs = (YamlKeyValPair *)realloc(current_obj->pairs, new_cap * sizeof(YamlKeyValPair));
        if (!new_pairs)
        {
            return;
        }
        current_obj->pairs = new_pairs;
        current_obj->capacity = new_cap;
    }
    current_obj->pairs[current_obj->num_pairs].key = strdup(key);
    current_obj->pairs[current_obj->num_pairs].value = strdup(value);
    current_obj->num_pairs++;
}

uint8_t *read_yaml_field(const char *file_path, const char *field_name, size_t *out_size)
{
    if (!file_path || !field_name || !out_size)
    {
        return NULL;
    }

    *out_size = 0;
    FILE *fp = fopen(file_path, "r");
    if (!fp)
    {
        return NULL;
    }

    int desired_count = 0;
    char **desired_parts = split_field_name(field_name, &desired_count);
    if (!desired_parts)
    {
        fclose(fp);
        return NULL;
    }

    int stack_size = 0;
    char *keys_stack[64];
    int indent_stack[64];
    uint8_t *result = NULL;
    char line[1024];

    while (fgets(line, sizeof(line), fp))
    {
        rstrip(line);
        int indent = get_indentation(line);
        char *content = trim_left(line);

        if (!*content)
        {
            continue;
        }

        if (content[0] == '-')
        {
            if (stack_size > 0)
            {
                char *current_path = build_path(keys_stack, stack_size);
                if (current_path)
                {
                    if (strcmp(current_path, field_name) == 0)
                    {
                        char *array_val = content + 1;
                        while (*array_val && isspace((unsigned char)*array_val))
                        {
                            array_val++;
                        }

                        if (strncmp(array_val, "'0x", 3) == 0 || strncmp(array_val, "\"0x", 3) == 0)
                        {
                            array_val += 3;
                            char *endq = strchr(array_val, '\'');
                            if (!endq)
                            {
                                endq = strchr(array_val, '\"');
                            }
                            if (endq)
                            {
                                *endq = '\0';
                            }
                            size_t hex_len = strlen(array_val);
                            size_t num_bytes = hex_len / 2;
                            uint8_t *new_buf = (uint8_t *)realloc(result, (*out_size + num_bytes));
                            if (!new_buf)
                            {
                                free(result);
                                free(current_path);
                                fclose(fp);
                                for (int i = 0; i < desired_count; i++)
                                {
                                    free(desired_parts[i]);
                                }
                                free(desired_parts);
                                return NULL;
                            }
                            result = new_buf;
                            for (size_t i = 0; i < num_bytes; i++)
                            {
                                unsigned int byte_val;
                                sscanf(&array_val[i * 2], "%2x", &byte_val);
                                result[*out_size + i] = (uint8_t)byte_val;
                            }
                            *out_size += num_bytes;
                        }
                        else if (strncmp(array_val, "0x", 2) == 0)
                        {
                            array_val += 2;
                            size_t hex_len = strlen(array_val);
                            size_t num_bytes = hex_len / 2;
                            uint8_t *new_buf = (uint8_t *)realloc(result, (*out_size + num_bytes));
                            if (!new_buf)
                            {
                                free(result);
                                free(current_path);
                                fclose(fp);
                                for (int i = 0; i < desired_count; i++)
                                {
                                    free(desired_parts[i]);
                                }
                                free(desired_parts);
                                return NULL;
                            }
                            result = new_buf;
                            for (size_t i = 0; i < num_bytes; i++)
                            {
                                unsigned int byte_val;
                                sscanf(&array_val[i * 2], "%2x", &byte_val);
                                result[*out_size + i] = (uint8_t)byte_val;
                            }
                            *out_size += num_bytes;
                        }
                        else
                        {
                            unsigned long long dec_val = strtoull(array_val, NULL, 10);
                            uint8_t temp[8];
                            temp[0] = (uint8_t)(dec_val >> 0);
                            temp[1] = (uint8_t)(dec_val >> 8);
                            temp[2] = (uint8_t)(dec_val >> 16);
                            temp[3] = (uint8_t)(dec_val >> 24);
                            temp[4] = (uint8_t)(dec_val >> 32);
                            temp[5] = (uint8_t)(dec_val >> 40);
                            temp[6] = (uint8_t)(dec_val >> 48);
                            temp[7] = (uint8_t)(dec_val >> 56);

                            uint8_t *new_buf = (uint8_t *)realloc(result, (*out_size + 8));
                            if (!new_buf)
                            {
                                free(result);
                                free(current_path);
                                fclose(fp);
                                for (int i = 0; i < desired_count; i++)
                                {
                                    free(desired_parts[i]);
                                }
                                free(desired_parts);
                                return NULL;
                            }
                            result = new_buf;
                            memcpy(result + *out_size, temp, 8);
                            *out_size += 8;
                        }
                    }
                    free(current_path);
                }
            }
            continue;
        }
        else
        {
            while (stack_size > 0 && indent_stack[stack_size - 1] >= indent)
            {
                pop_stack(keys_stack, &stack_size);
            }
        }

        char *sep = strchr(content, ':');
        if (sep)
        {
            *sep = '\0';
            char *key = content;
            char *val = sep + 1;
            while (*val && isspace((unsigned char)*val))
            {
                val++;
            }

            char *key_copy = strdup(key);
            if (!key_copy)
            {
                fclose(fp);
                for (int i = 0; i < stack_size; i++)
                {
                    free(keys_stack[i]);
                }
                for (int i = 0; i < desired_count; i++)
                {
                    free(desired_parts[i]);
                }
                free(desired_parts);
                return NULL;
            }

            keys_stack[stack_size] = key_copy;
            indent_stack[stack_size] = indent;
            stack_size++;

            char *full_path = build_path(keys_stack, stack_size);
            if (full_path && strcmp(full_path, field_name) == 0)
            {
                if (!*val)
                {
                    free(full_path);
                    continue;
                }

                if (strncmp(val, "'0x", 3) == 0 || strncmp(val, "\"0x", 3) == 0)
                {
                    val += 3;
                    char *endq = strchr(val, '\'');
                    if (!endq)
                    {
                        endq = strchr(val, '\"');
                    }
                    if (endq)
                    {
                        *endq = '\0';
                    }
                    size_t hex_len = strlen(val);
                    size_t num_bytes = hex_len / 2;
                    result = (uint8_t *)malloc(num_bytes);
                    if (!result)
                    {
                        free(full_path);
                        fclose(fp);
                        for (int i = 0; i < stack_size; i++)
                        {
                            free(keys_stack[i]);
                        }
                        for (int i = 0; i < desired_count; i++)
                        {
                            free(desired_parts[i]);
                        }
                        free(desired_parts);
                        return NULL;
                    }
                    for (size_t i = 0; i < num_bytes; i++)
                    {
                        unsigned int byte_val;
                        sscanf(&val[i * 2], "%2x", &byte_val);
                        result[i] = (uint8_t)byte_val;
                    }
                    *out_size = num_bytes;
                }
                else if (strncmp(val, "0x", 2) == 0)
                {
                    val += 2;
                    size_t hex_len = strlen(val);
                    size_t num_bytes = hex_len / 2;
                    result = (uint8_t *)malloc(num_bytes);
                    if (!result)
                    {
                        free(full_path);
                        fclose(fp);
                        for (int i = 0; i < stack_size; i++)
                        {
                            free(keys_stack[i]);
                        }
                        for (int i = 0; i < desired_count; i++)
                        {
                            free(desired_parts[i]);
                        }
                        free(desired_parts);
                        return NULL;
                    }
                    for (size_t i = 0; i < num_bytes; i++)
                    {
                        unsigned int byte_val;
                        sscanf(&val[i * 2], "%2x", &byte_val);
                        result[i] = (uint8_t)byte_val;
                    }
                    *out_size = num_bytes;
                }
                else if (val[0] == '[')
                {
                    // Use a dynamically resized buffer instead of a fixed-size char[].
                    size_t buffer_capacity = 1024 * 1024;
                    size_t buffer_len = 0;
                    char *buffer = (char *)malloc(buffer_capacity);
                    if (!buffer)
                    {
                        free(full_path);
                        fclose(fp);
                        for (int i = 0; i < stack_size; i++)
                        {
                            free(keys_stack[i]);
                        }
                        for (int i = 0; i < desired_count; i++)
                        {
                            free(desired_parts[i]);
                        }
                        free(desired_parts);
                        return NULL;
                    }

                    memset(buffer, 0, buffer_capacity);
                    strncpy(buffer, val, buffer_capacity - 1);
                    buffer_len = strlen(buffer);

                    while (!strchr(buffer, ']'))
                    {
                        if (!fgets(line, sizeof(line), fp))
                        {
                            // We reached EOF without finding ']'
                            break;
                        }
                        rstrip(line);
                        char *additional = trim_left(line);
                        size_t alen = strlen(additional);

                        // Expand if needed
                        while (buffer_len + alen + 1 >= buffer_capacity)
                        {
                            size_t new_capacity = buffer_capacity * 2;
                            char *new_buffer = realloc(buffer, new_capacity);
                            if (!new_buffer)
                            {
                                free(buffer);
                                buffer = NULL;
                                break;
                            }
                            buffer = new_buffer;
                            buffer_capacity = new_capacity;
                        }
                        if (!buffer)
                        {
                            break;
                        }
                        strcat(buffer + buffer_len, additional);
                        buffer_len += alen;
                    }

                    if (buffer)
                    {
                        char *start = strchr(buffer, '[');
                        char *endp = strrchr(buffer, ']');
                        if (start && endp)
                        {
                            *endp = 0;
                            start++;
                            char *token = strtok(start, ",");
                            while (token)
                            {
                                while (isspace((unsigned char)*token))
                                {
                                    token++;
                                }

                                if (strncmp(token, "'0x", 3) == 0 || strncmp(token, "\"0x", 3) == 0)
                                {
                                    token += 3;
                                    char *endq = strchr(token, '\'');
                                    if (!endq)
                                    {
                                        endq = strchr(token, '\"');
                                    }
                                    if (endq)
                                    {
                                        *endq = 0;
                                    }
                                    size_t hex_len = strlen(token);
                                    size_t num_bytes = hex_len / 2;
                                    uint8_t *new_buf = (uint8_t *)realloc(result, (*out_size + num_bytes));
                                    if (!new_buf)
                                    {
                                        free(result);
                                        free(buffer);
                                        free(full_path);
                                        fclose(fp);
                                        for (int i = 0; i < stack_size; i++)
                                        {
                                            free(keys_stack[i]);
                                        }
                                        for (int i = 0; i < desired_count; i++)
                                        {
                                            free(desired_parts[i]);
                                        }
                                        free(desired_parts);
                                        return NULL;
                                    }
                                    result = new_buf;
                                    for (size_t i = 0; i < num_bytes; i++)
                                    {
                                        unsigned int byte_val;
                                        sscanf(&token[i * 2], "%2x", &byte_val);
                                        result[*out_size + i] = (uint8_t)byte_val;
                                    }
                                    *out_size += num_bytes;
                                }
                                else if (strncmp(token, "0x", 2) == 0)
                                {
                                    token += 2;
                                    size_t hex_len = strlen(token);
                                    size_t num_bytes = hex_len / 2;
                                    uint8_t *new_buf = (uint8_t *)realloc(result, (*out_size + num_bytes));
                                    if (!new_buf)
                                    {
                                        free(result);
                                        free(buffer);
                                        free(full_path);
                                        fclose(fp);
                                        for (int i = 0; i < stack_size; i++)
                                        {
                                            free(keys_stack[i]);
                                        }
                                        for (int i = 0; i < desired_count; i++)
                                        {
                                            free(desired_parts[i]);
                                        }
                                        free(desired_parts);
                                        return NULL;
                                    }
                                    result = new_buf;
                                    for (size_t i = 0; i < num_bytes; i++)
                                    {
                                        unsigned int byte_val;
                                        sscanf(&token[i * 2], "%2x", &byte_val);
                                        result[*out_size + i] = (uint8_t)byte_val;
                                    }
                                    *out_size += num_bytes;
                                }
                                else
                                {
                                    unsigned long long dec_val = strtoull(token, NULL, 10);
                                    uint8_t temp[8];
                                    temp[0] = (uint8_t)(dec_val >> 0);
                                    temp[1] = (uint8_t)(dec_val >> 8);
                                    temp[2] = (uint8_t)(dec_val >> 16);
                                    temp[3] = (uint8_t)(dec_val >> 24);
                                    temp[4] = (uint8_t)(dec_val >> 32);
                                    temp[5] = (uint8_t)(dec_val >> 40);
                                    temp[6] = (uint8_t)(dec_val >> 48);
                                    temp[7] = (uint8_t)(dec_val >> 56);

                                    uint8_t *new_buf = (uint8_t *)realloc(result, (*out_size + 8));
                                    if (!new_buf)
                                    {
                                        free(result);
                                        free(buffer);
                                        free(full_path);
                                        fclose(fp);
                                        for (int i = 0; i < stack_size; i++)
                                        {
                                            free(keys_stack[i]);
                                        }
                                        for (int i = 0; i < desired_count; i++)
                                        {
                                            free(desired_parts[i]);
                                        }
                                        free(desired_parts);
                                        return NULL;
                                    }
                                    result = new_buf;
                                    memcpy(result + *out_size, temp, 8);
                                    *out_size += 8;
                                }

                                token = strtok(NULL, ",");
                            }
                        }
                        free(buffer);
                    }
                }
                else
                {
                    unsigned long long dec_val = strtoull(val, NULL, 10);
                    result = (uint8_t *)malloc(8);
                    if (!result)
                    {
                        free(full_path);
                        fclose(fp);
                        for (int i = 0; i < stack_size; i++)
                        {
                            free(keys_stack[i]);
                        }
                        for (int i = 0; i < desired_count; i++)
                        {
                            free(desired_parts[i]);
                        }
                        free(desired_parts);
                        return NULL;
                    }
                    result[0] = (uint8_t)(dec_val >> 0);
                    result[1] = (uint8_t)(dec_val >> 8);
                    result[2] = (uint8_t)(dec_val >> 16);
                    result[3] = (uint8_t)(dec_val >> 24);
                    result[4] = (uint8_t)(dec_val >> 32);
                    result[5] = (uint8_t)(dec_val >> 40);
                    result[6] = (uint8_t)(dec_val >> 48);
                    result[7] = (uint8_t)(dec_val >> 56);
                    *out_size = 8;
                }
                free(full_path);
                break;
            }
            free(full_path);
        }
    }

    fclose(fp);

    for (int i = 0; i < stack_size; i++)
    {
        free(keys_stack[i]);
    }

    for (int i = 0; i < desired_count; i++)
    {
        free(desired_parts[i]);
    }
    free(desired_parts);

    return result;
}

YamlObject *read_yaml_array_of_objects(const char *file_path, const char *array_name, size_t *out_count)
{
    *out_count = 0;
    if (!file_path || !array_name)
    {
        return NULL;
    }
    FILE *fp = fopen(file_path, "r");
    if (!fp)
    {
        return NULL;
    }
    int stack_size = 0;
    char *keys_stack[64];
    int indent_stack[64];
    int in_target_array = 0;
    int array_depth = -1;
    YamlObject *objects = NULL;
    size_t objects_capacity = 0;
    YamlObject current_obj;
    current_obj.pairs = NULL;
    current_obj.num_pairs = 0;
    current_obj.capacity = 0;
    char line[1024];
    while (fgets(line, sizeof(line), fp))
    {
        rstrip(line);
        int indent = get_indentation(line);
        char *content = trim_left(line);
        if (!*content)
        {
            continue;
        }
        if (content[0] == '-')
        {
            if (in_target_array)
            {
                commit_current_object(&current_obj, &objects, &objects_capacity, out_count);
            }
            content++;
            while (isspace((unsigned char)*content))
            {
                content++;
            }
            if (!*content)
            {
                continue;
            }
        }
        else
        {
            while (stack_size > 0 && indent_stack[stack_size - 1] >= indent)
            {
                pop_stack(keys_stack, &stack_size);
                if (in_target_array && stack_size < array_depth)
                {
                    commit_current_object(&current_obj, &objects, &objects_capacity, out_count);
                    in_target_array = 0;
                }
            }
        }
        char *sep = strchr(content, ':');
        if (!sep)
        {
            continue;
        }
        *sep = '\0';
        char *key = content;
        char *val = sep + 1;
        while (*val && isspace((unsigned char)*val))
        {
            val++;
        }
        char *key_copy = strdup(key);
        if (!key_copy)
        {
            break;
        }
        keys_stack[stack_size] = key_copy;
        indent_stack[stack_size] = indent;
        stack_size++;
        char *full_path = build_path(keys_stack, stack_size);
        if (full_path)
        {
            if (strcmp(full_path, array_name) == 0)
            {
                in_target_array = 1;
                array_depth = stack_size;
            }
            if (in_target_array && strcmp(full_path, array_name) != 0)
            {
                char *last_dot = strrchr(full_path, '.');
                const char *store_key = last_dot ? last_dot + 1 : full_path;
                add_pair_to_current(&current_obj, store_key, val);
            }
        }
        free(full_path);
    }
    fclose(fp);
    if (in_target_array)
    {
        commit_current_object(&current_obj, &objects, &objects_capacity, out_count);
    }
    for (int i = 0; i < stack_size; i++)
    {
        free(keys_stack[i]);
    }
    return objects;
}