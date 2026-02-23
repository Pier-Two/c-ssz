#include "spec_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

static char *spec_strdup(const char *text)
{
    size_t len;
    char *copy;

    if (text == NULL)
    {
        return NULL;
    }

    len = strlen(text);
    copy = (char *)malloc(len + 1u);
    if (copy == NULL)
    {
        return NULL;
    }

    memcpy(copy, text, len + 1u);
    return copy;
}

char *spec_join_path(const char *base, const char *name)
{
    size_t base_len;
    size_t name_len;
    char *path;

    if ((base == NULL) || (name == NULL))
    {
        return NULL;
    }

    base_len = strlen(base);
    name_len = strlen(name);

    path = (char *)malloc(base_len + 1u + name_len + 1u);
    if (path == NULL)
    {
        return NULL;
    }

    memcpy(path, base, base_len);
    path[base_len] = '/';
    memcpy(path + base_len + 1u, name, name_len + 1u);

    return path;
}

void spec_report_init(spec_report_t *report)
{
    if (report == NULL)
    {
        return;
    }

    memset(report, 0, sizeof(*report));
}

void spec_report_free(spec_report_t *report)
{
    if (report == NULL)
    {
        return;
    }

    for (size_t i = 0u; i < report->failure_count; i++)
    {
        free(report->failure_messages[i]);
    }
    free(report->failure_messages);

    memset(report, 0, sizeof(*report));
}

void spec_report_record_failure(spec_report_t *report, const char *case_path, const char *reason)
{
    char **new_messages;
    size_t new_count;
    size_t msg_len;
    char *message;

    if ((report == NULL) || (case_path == NULL) || (reason == NULL))
    {
        return;
    }

    msg_len = strlen(case_path) + strlen(reason) + 4u;
    message = (char *)malloc(msg_len + 1u);
    if (message == NULL)
    {
        return;
    }

    snprintf(message, msg_len + 1u, "%s: %s", case_path, reason);

    new_count = report->failure_count + 1u;
    new_messages = (char **)realloc(report->failure_messages, new_count * sizeof(char *));
    if (new_messages == NULL)
    {
        free(message);
        return;
    }

    report->failure_messages = new_messages;
    report->failure_messages[report->failure_count] = message;
    report->failure_count = new_count;
}

int spec_report_print(const char *label, const spec_report_t *report)
{
    size_t passed = 0u;
    size_t total = 0u;

    if ((label == NULL) || (report == NULL))
    {
        return 1;
    }

    passed = report->valid_passed + report->invalid_passed;
    total = report->total_valid + report->total_invalid;

    if (report->failure_count == 0u)
    {
        printf("[OK] %s %zu/%zu tests passed\n", label, passed, total);
        return 0;
    }

    printf("[FAIL] %s %zu/%zu tests passed\n", label, passed, total);
    for (size_t i = 0u; i < report->failure_count; i++)
    {
        printf("  - %s\n", report->failure_messages[i]);
    }

    return 1;
}

bool spec_read_binary_file(const char *path, uint8_t **out_bytes, size_t *out_len)
{
    FILE *file;
    long file_size;
    uint8_t *bytes;
    size_t read_len;

    if ((path == NULL) || (out_bytes == NULL) || (out_len == NULL))
    {
        return false;
    }

    *out_bytes = NULL;
    *out_len = 0u;

    file = fopen(path, "rb");
    if (file == NULL)
    {
        return false;
    }

    if (fseek(file, 0L, SEEK_END) != 0)
    {
        fclose(file);
        return false;
    }

    file_size = ftell(file);
    if (file_size < 0L)
    {
        fclose(file);
        return false;
    }

    if (fseek(file, 0L, SEEK_SET) != 0)
    {
        fclose(file);
        return false;
    }

    bytes = (uint8_t *)malloc((size_t)file_size == 0u ? 1u : (size_t)file_size);
    if (bytes == NULL)
    {
        fclose(file);
        return false;
    }

    read_len = fread(bytes, 1u, (size_t)file_size, file);
    fclose(file);

    if (read_len != (size_t)file_size)
    {
        free(bytes);
        return false;
    }

    *out_bytes = bytes;
    *out_len = read_len;
    return true;
}

bool spec_read_snappy_file(const char *path, uint8_t **out_bytes, size_t *out_len)
{
    uint8_t *compressed = NULL;
    size_t compressed_len = 0u;
    size_t decoded_len = 0u;
    uint8_t *decoded;
    snappy_status status;

    if ((out_bytes == NULL) || (out_len == NULL))
    {
        return false;
    }

    *out_bytes = NULL;
    *out_len = 0u;

    if (!spec_read_binary_file(path, &compressed, &compressed_len))
    {
        return false;
    }

    status = snappy_uncompressed_length((const char *)compressed, compressed_len, &decoded_len);
    if (status != SNAPPY_OK)
    {
        free(compressed);
        return false;
    }

    decoded = (uint8_t *)malloc(decoded_len == 0u ? 1u : decoded_len);
    if (decoded == NULL)
    {
        free(compressed);
        return false;
    }

    status = snappy_uncompress(
        (const char *)compressed,
        compressed_len,
        (char *)decoded,
        &decoded_len);

    free(compressed);

    if (status != SNAPPY_OK)
    {
        free(decoded);
        return false;
    }

    *out_bytes = decoded;
    *out_len = decoded_len;
    return true;
}

static int compare_strings(const void *left, const void *right)
{
    const char *const *a = (const char *const *)left;
    const char *const *b = (const char *const *)right;
    return strcmp(*a, *b);
}

void spec_free_string_array(char **items, size_t count)
{
    if (items == NULL)
    {
        return;
    }

    for (size_t i = 0u; i < count; i++)
    {
        free(items[i]);
    }
    free(items);
}

#ifdef _WIN32
static bool is_dot_entry(const char *name)
{
    return (strcmp(name, ".") == 0) || (strcmp(name, "..") == 0);
}

bool spec_list_subdirs(const char *dir_path, char ***out_names, size_t *out_count)
{
    char *pattern;
    WIN32_FIND_DATAA data;
    HANDLE handle;
    char **names = NULL;
    size_t count = 0u;

    if ((dir_path == NULL) || (out_names == NULL) || (out_count == NULL))
    {
        return false;
    }

    *out_names = NULL;
    *out_count = 0u;

    pattern = spec_join_path(dir_path, "*");
    if (pattern == NULL)
    {
        return false;
    }

    handle = FindFirstFileA(pattern, &data);
    free(pattern);

    if (handle == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    do
    {
        char **new_names;
        char *copy;

        if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
        {
            continue;
        }
        if (is_dot_entry(data.cFileName))
        {
            continue;
        }

        copy = spec_strdup(data.cFileName);
        if (copy == NULL)
        {
            spec_free_string_array(names, count);
            FindClose(handle);
            return false;
        }

        new_names = (char **)realloc(names, (count + 1u) * sizeof(char *));
        if (new_names == NULL)
        {
            free(copy);
            spec_free_string_array(names, count);
            FindClose(handle);
            return false;
        }

        names = new_names;
        names[count++] = copy;
    } while (FindNextFileA(handle, &data) != 0);

    FindClose(handle);

    if (count > 1u)
    {
        qsort(names, count, sizeof(char *), compare_strings);
    }

    *out_names = names;
    *out_count = count;
    return true;
}
#else
bool spec_list_subdirs(const char *dir_path, char ***out_names, size_t *out_count)
{
    DIR *dir;
    struct dirent *entry;
    char **names = NULL;
    size_t count = 0u;

    if ((dir_path == NULL) || (out_names == NULL) || (out_count == NULL))
    {
        return false;
    }

    *out_names = NULL;
    *out_count = 0u;

    dir = opendir(dir_path);
    if (dir == NULL)
    {
        return false;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        struct stat st;
        char **new_names;
        char *full_path;
        char *copy;

        if ((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0))
        {
            continue;
        }

        full_path = spec_join_path(dir_path, entry->d_name);
        if (full_path == NULL)
        {
            spec_free_string_array(names, count);
            closedir(dir);
            return false;
        }

        if ((stat(full_path, &st) != 0) || !S_ISDIR(st.st_mode))
        {
            free(full_path);
            continue;
        }

        free(full_path);

        copy = spec_strdup(entry->d_name);
        if (copy == NULL)
        {
            spec_free_string_array(names, count);
            closedir(dir);
            return false;
        }

        new_names = (char **)realloc(names, (count + 1u) * sizeof(char *));
        if (new_names == NULL)
        {
            free(copy);
            spec_free_string_array(names, count);
            closedir(dir);
            return false;
        }

        names = new_names;
        names[count++] = copy;
    }

    closedir(dir);

    if (count > 1u)
    {
        qsort(names, count, sizeof(char *), compare_strings);
    }

    *out_names = names;
    *out_count = count;
    return true;
}
#endif

bool spec_parse_root_from_meta(const char *meta_path, uint8_t out_root[32])
{
    yaml_node_t *root_doc;
    const yaml_node_t *root_field;
    uint8_t *bytes = NULL;
    size_t bytes_len = 0u;

    if ((meta_path == NULL) || (out_root == NULL))
    {
        return false;
    }

    root_doc = yaml_parse_file(meta_path);
    if (root_doc == NULL)
    {
        return false;
    }

    root_field = yaml_mapping_get(root_doc, "root");
    if ((root_field == NULL) || !yaml_node_parse_hex(root_field, &bytes, &bytes_len) ||
        (bytes_len != 32u))
    {
        free(bytes);
        yaml_node_free(root_doc);
        return false;
    }

    memcpy(out_root, bytes, 32u);

    free(bytes);
    yaml_node_free(root_doc);
    return true;
}
