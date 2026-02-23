#include "yaml_parser.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    char **lines;
    size_t line_count;
    size_t line_index;
} yaml_line_parser_t;

typedef struct
{
    const char *input;
    size_t length;
    size_t pos;
} yaml_flow_parser_t;

static char *yaml_strdup(const char *src)
{
    size_t len;
    char *out;

    if (src == NULL)
    {
        return NULL;
    }

    len = strlen(src);
    out = (char *)malloc(len + 1u);
    if (out == NULL)
    {
        return NULL;
    }

    memcpy(out, src, len + 1u);
    return out;
}

static char *yaml_strndup(const char *src, size_t len)
{
    char *out;

    if (src == NULL)
    {
        return NULL;
    }

    out = (char *)malloc(len + 1u);
    if (out == NULL)
    {
        return NULL;
    }

    memcpy(out, src, len);
    out[len] = '\0';
    return out;
}

static char *trim_left(char *text)
{
    while ((*text != '\0') && isspace((unsigned char)*text))
    {
        text++;
    }
    return text;
}

static void trim_right(char *text)
{
    size_t len;

    if (text == NULL)
    {
        return;
    }

    len = strlen(text);
    while ((len > 0u) && isspace((unsigned char)text[len - 1u]))
    {
        text[len - 1u] = '\0';
        len--;
    }
}

static int line_indent(const char *line)
{
    int indent = 0;

    while ((line[indent] == ' ') || (line[indent] == '\t'))
    {
        indent++;
    }

    return indent;
}

static bool is_separator_line(const char *content)
{
    return (strcmp(content, "---") == 0) || (strcmp(content, "...") == 0);
}

static yaml_node_t *yaml_node_new(yaml_node_type_t type)
{
    yaml_node_t *node = (yaml_node_t *)calloc(1u, sizeof(yaml_node_t));
    if (node != NULL)
    {
        node->type = type;
    }
    return node;
}

static void yaml_node_sequence_free(yaml_node_t *node)
{
    if ((node == NULL) || (node->type != YAML_NODE_SEQUENCE))
    {
        return;
    }

    for (size_t i = 0u; i < node->as.sequence.count; i++)
    {
        yaml_node_free(node->as.sequence.items[i]);
    }
    free(node->as.sequence.items);
}

static void yaml_node_mapping_free(yaml_node_t *node)
{
    if ((node == NULL) || (node->type != YAML_NODE_MAPPING))
    {
        return;
    }

    for (size_t i = 0u; i < node->as.mapping.count; i++)
    {
        free(node->as.mapping.pairs[i].key);
        yaml_node_free(node->as.mapping.pairs[i].value);
    }
    free(node->as.mapping.pairs);
}

void yaml_node_free(yaml_node_t *node)
{
    if (node == NULL)
    {
        return;
    }

    if ((node->type == YAML_NODE_STRING) || (node->type == YAML_NODE_INT))
    {
        free(node->as.text);
    }
    else if (node->type == YAML_NODE_SEQUENCE)
    {
        yaml_node_sequence_free(node);
    }
    else if (node->type == YAML_NODE_MAPPING)
    {
        yaml_node_mapping_free(node);
    }

    free(node);
}

static bool yaml_sequence_add(yaml_node_t *sequence, yaml_node_t *item)
{
    yaml_node_t **new_items;
    size_t new_count;

    if ((sequence == NULL) || (sequence->type != YAML_NODE_SEQUENCE) || (item == NULL))
    {
        return false;
    }

    new_count = sequence->as.sequence.count + 1u;
    new_items =
        (yaml_node_t **)realloc(sequence->as.sequence.items, new_count * sizeof(yaml_node_t *));
    if (new_items == NULL)
    {
        return false;
    }

    sequence->as.sequence.items = new_items;
    sequence->as.sequence.items[sequence->as.sequence.count] = item;
    sequence->as.sequence.count = new_count;
    return true;
}

static bool yaml_mapping_add(yaml_node_t *mapping, const char *key, yaml_node_t *value)
{
    yaml_pair_t *new_pairs;
    size_t new_count;
    char *key_copy;

    if ((mapping == NULL) || (mapping->type != YAML_NODE_MAPPING) || (key == NULL) ||
        (value == NULL))
    {
        return false;
    }

    key_copy = yaml_strdup(key);
    if (key_copy == NULL)
    {
        return false;
    }

    new_count = mapping->as.mapping.count + 1u;
    new_pairs = (yaml_pair_t *)realloc(mapping->as.mapping.pairs, new_count * sizeof(yaml_pair_t));
    if (new_pairs == NULL)
    {
        free(key_copy);
        return false;
    }

    mapping->as.mapping.pairs = new_pairs;
    mapping->as.mapping.pairs[mapping->as.mapping.count].key = key_copy;
    mapping->as.mapping.pairs[mapping->as.mapping.count].value = value;
    mapping->as.mapping.count = new_count;
    return true;
}

static bool is_plain_int_token(const char *token)
{
    size_t i = 0u;

    if ((token == NULL) || (token[0] == '\0'))
    {
        return false;
    }

    if ((token[0] == '+') || (token[0] == '-'))
    {
        i++;
        if (token[i] == '\0')
        {
            return false;
        }
    }

    for (; token[i] != '\0'; i++)
    {
        if (!isdigit((unsigned char)token[i]))
        {
            return false;
        }
    }

    return true;
}

static yaml_node_t *yaml_parse_scalar_token(const char *token)
{
    yaml_node_t *node;
    char *copy;

    if (token == NULL)
    {
        return NULL;
    }

    if (strcmp(token, "null") == 0)
    {
        return yaml_node_new(YAML_NODE_NULL);
    }

    if ((strcmp(token, "true") == 0) || (strcmp(token, "false") == 0))
    {
        node = yaml_node_new(YAML_NODE_BOOL);
        if (node == NULL)
        {
            return NULL;
        }
        node->as.bool_value = (token[0] == 't');
        return node;
    }

    if (is_plain_int_token(token))
    {
        node = yaml_node_new(YAML_NODE_INT);
        if (node == NULL)
        {
            return NULL;
        }
        copy = yaml_strdup(token);
        if (copy == NULL)
        {
            yaml_node_free(node);
            return NULL;
        }
        node->as.text = copy;
        return node;
    }

    node = yaml_node_new(YAML_NODE_STRING);
    if (node == NULL)
    {
        return NULL;
    }

    copy = yaml_strdup(token);
    if (copy == NULL)
    {
        yaml_node_free(node);
        return NULL;
    }

    node->as.text = copy;
    return node;
}

static char *parse_single_quoted(const char *input, size_t length, size_t *pos)
{
    size_t start;
    size_t cursor;
    char *out;
    size_t out_len = 0u;

    if ((input == NULL) || (pos == NULL) || (*pos >= length) || (input[*pos] != '\''))
    {
        return NULL;
    }

    (*pos)++;
    start = *pos;
    cursor = start;

    while (cursor < length)
    {
        if (input[cursor] != '\'')
        {
            cursor++;
            continue;
        }

        if ((cursor + 1u < length) && (input[cursor + 1u] == '\''))
        {
            cursor += 2u;
            out_len++;
            continue;
        }

        out_len += cursor - start;
        out = (char *)malloc(out_len + 1u);
        if (out == NULL)
        {
            return NULL;
        }

        out_len = 0u;
        cursor = start;
        while (cursor < length)
        {
            if (input[cursor] != '\'')
            {
                out[out_len++] = input[cursor++];
                continue;
            }
            if ((cursor + 1u < length) && (input[cursor + 1u] == '\''))
            {
                out[out_len++] = '\'';
                cursor += 2u;
                continue;
            }
            break;
        }

        out[out_len] = '\0';
        *pos = cursor + 1u;
        return out;
    }

    return NULL;
}

static void flow_skip_ws(yaml_flow_parser_t *parser)
{
    while ((parser->pos < parser->length) && isspace((unsigned char)parser->input[parser->pos]))
    {
        parser->pos++;
    }
}

static yaml_node_t *parse_flow_value(yaml_flow_parser_t *parser);

static char *parse_flow_key(yaml_flow_parser_t *parser)
{
    size_t start;
    size_t end;

    flow_skip_ws(parser);
    if (parser->pos >= parser->length)
    {
        return NULL;
    }

    if (parser->input[parser->pos] == '\'')
    {
        return parse_single_quoted(parser->input, parser->length, &parser->pos);
    }

    start = parser->pos;
    while (parser->pos < parser->length)
    {
        char ch = parser->input[parser->pos];
        if (ch == ':')
        {
            break;
        }
        parser->pos++;
    }

    if ((parser->pos >= parser->length) || (parser->input[parser->pos] != ':'))
    {
        return NULL;
    }

    end = parser->pos;
    while ((end > start) && isspace((unsigned char)parser->input[end - 1u]))
    {
        end--;
    }

    return yaml_strndup(parser->input + start, end - start);
}

static yaml_node_t *parse_flow_sequence(yaml_flow_parser_t *parser)
{
    yaml_node_t *node = yaml_node_new(YAML_NODE_SEQUENCE);

    if (node == NULL)
    {
        return NULL;
    }

    parser->pos++;
    flow_skip_ws(parser);

    if ((parser->pos < parser->length) && (parser->input[parser->pos] == ']'))
    {
        parser->pos++;
        return node;
    }

    while (parser->pos < parser->length)
    {
        yaml_node_t *item = parse_flow_value(parser);
        if (item == NULL)
        {
            yaml_node_free(node);
            return NULL;
        }
        if (!yaml_sequence_add(node, item))
        {
            yaml_node_free(item);
            yaml_node_free(node);
            return NULL;
        }

        flow_skip_ws(parser);
        if (parser->pos >= parser->length)
        {
            yaml_node_free(node);
            return NULL;
        }

        if (parser->input[parser->pos] == ',')
        {
            parser->pos++;
            flow_skip_ws(parser);
            continue;
        }

        if (parser->input[parser->pos] == ']')
        {
            parser->pos++;
            return node;
        }

        yaml_node_free(node);
        return NULL;
    }

    yaml_node_free(node);
    return NULL;
}

static yaml_node_t *parse_flow_mapping(yaml_flow_parser_t *parser)
{
    yaml_node_t *node = yaml_node_new(YAML_NODE_MAPPING);

    if (node == NULL)
    {
        return NULL;
    }

    parser->pos++;
    flow_skip_ws(parser);

    if ((parser->pos < parser->length) && (parser->input[parser->pos] == '}'))
    {
        parser->pos++;
        return node;
    }

    while (parser->pos < parser->length)
    {
        char *key;
        yaml_node_t *value;

        key = parse_flow_key(parser);
        if (key == NULL)
        {
            yaml_node_free(node);
            return NULL;
        }

        if ((parser->pos >= parser->length) || (parser->input[parser->pos] != ':'))
        {
            free(key);
            yaml_node_free(node);
            return NULL;
        }

        parser->pos++;
        value = parse_flow_value(parser);
        if (value == NULL)
        {
            free(key);
            yaml_node_free(node);
            return NULL;
        }

        if (!yaml_mapping_add(node, key, value))
        {
            free(key);
            yaml_node_free(value);
            yaml_node_free(node);
            return NULL;
        }
        free(key);

        flow_skip_ws(parser);
        if (parser->pos >= parser->length)
        {
            yaml_node_free(node);
            return NULL;
        }

        if (parser->input[parser->pos] == ',')
        {
            parser->pos++;
            flow_skip_ws(parser);
            continue;
        }

        if (parser->input[parser->pos] == '}')
        {
            parser->pos++;
            return node;
        }

        yaml_node_free(node);
        return NULL;
    }

    yaml_node_free(node);
    return NULL;
}

static yaml_node_t *parse_flow_plain_scalar(yaml_flow_parser_t *parser)
{
    size_t start;
    size_t end;
    char *token;
    yaml_node_t *node;

    flow_skip_ws(parser);
    if (parser->pos >= parser->length)
    {
        return NULL;
    }

    if (parser->input[parser->pos] == '\'')
    {
        char *unquoted = parse_single_quoted(parser->input, parser->length, &parser->pos);
        if (unquoted == NULL)
        {
            return NULL;
        }
        node = yaml_node_new(YAML_NODE_STRING);
        if (node == NULL)
        {
            free(unquoted);
            return NULL;
        }
        node->as.text = unquoted;
        return node;
    }

    start = parser->pos;
    while (parser->pos < parser->length)
    {
        char ch = parser->input[parser->pos];
        if ((ch == ',') || (ch == ']') || (ch == '}'))
        {
            break;
        }
        parser->pos++;
    }

    end = parser->pos;
    while ((end > start) && isspace((unsigned char)parser->input[end - 1u]))
    {
        end--;
    }

    token = yaml_strndup(parser->input + start, end - start);
    if (token == NULL)
    {
        return NULL;
    }

    node = yaml_parse_scalar_token(token);
    free(token);
    return node;
}

static yaml_node_t *parse_flow_value(yaml_flow_parser_t *parser)
{
    flow_skip_ws(parser);
    if (parser->pos >= parser->length)
    {
        return NULL;
    }

    if (parser->input[parser->pos] == '[')
    {
        return parse_flow_sequence(parser);
    }

    if (parser->input[parser->pos] == '{')
    {
        return parse_flow_mapping(parser);
    }

    return parse_flow_plain_scalar(parser);
}

static yaml_node_t *parse_flow_text(const char *text)
{
    yaml_flow_parser_t parser;
    yaml_node_t *node;

    if (text == NULL)
    {
        return NULL;
    }

    parser.input = text;
    parser.length = strlen(text);
    parser.pos = 0u;

    node = parse_flow_value(&parser);
    if (node == NULL)
    {
        return NULL;
    }

    flow_skip_ws(&parser);
    if (parser.pos != parser.length)
    {
        yaml_node_free(node);
        return NULL;
    }

    return node;
}

static int find_top_level_colon(const char *text)
{
    int depth_square = 0;
    int depth_curly = 0;
    bool in_single = false;

    for (int i = 0; text[i] != '\0'; i++)
    {
        char ch = text[i];

        if (in_single)
        {
            if (ch == '\'')
            {
                if (text[i + 1] == '\'')
                {
                    i++;
                }
                else
                {
                    in_single = false;
                }
            }
            continue;
        }

        if (ch == '\'')
        {
            in_single = true;
            continue;
        }

        if (ch == '[')
        {
            depth_square++;
            continue;
        }
        if (ch == ']')
        {
            if (depth_square > 0)
            {
                depth_square--;
            }
            continue;
        }
        if (ch == '{')
        {
            depth_curly++;
            continue;
        }
        if (ch == '}')
        {
            if (depth_curly > 0)
            {
                depth_curly--;
            }
            continue;
        }

        if ((ch == ':') && (depth_square == 0) && (depth_curly == 0))
        {
            return i;
        }
    }

    return -1;
}

static bool flow_is_balanced(const char *text)
{
    int depth_square = 0;
    int depth_curly = 0;
    bool in_single = false;

    for (size_t i = 0u; text[i] != '\0'; i++)
    {
        char ch = text[i];

        if (in_single)
        {
            if (ch == '\'')
            {
                if (text[i + 1u] == '\'')
                {
                    i++;
                }
                else
                {
                    in_single = false;
                }
            }
            continue;
        }

        if (ch == '\'')
        {
            in_single = true;
            continue;
        }

        if (ch == '[')
        {
            depth_square++;
        }
        else if (ch == ']')
        {
            depth_square--;
        }
        else if (ch == '{')
        {
            depth_curly++;
        }
        else if (ch == '}')
        {
            depth_curly--;
        }
    }

    return (!in_single && (depth_square == 0) && (depth_curly == 0));
}

static void yaml_skip_empty(yaml_line_parser_t *parser)
{
    while (parser->line_index < parser->line_count)
    {
        char *line = parser->lines[parser->line_index];
        char *content = trim_left(line);

        if ((*content == '\0') || (*content == '#') || is_separator_line(content))
        {
            parser->line_index++;
            continue;
        }

        break;
    }
}

static yaml_node_t *yaml_parse_block_sequence(yaml_line_parser_t *parser, int indent);
static yaml_node_t *yaml_parse_block_node(yaml_line_parser_t *parser, int min_indent);

static yaml_node_t *yaml_parse_inline_value(
    yaml_line_parser_t *parser,
    const char *text,
    int parent_indent)
{
    char *token;
    yaml_node_t *node;
    (void)parent_indent;

    if (text == NULL)
    {
        return NULL;
    }

    while ((*text != '\0') && isspace((unsigned char)*text))
    {
        text++;
    }

    if ((text[0] == '[') || (text[0] == '{'))
    {
        char *joined = yaml_strdup(text);

        if (joined == NULL)
        {
            return NULL;
        }

        while (!flow_is_balanced(joined) && (parser->line_index < parser->line_count))
        {
            char *next_line = parser->lines[parser->line_index];
            char *next_content = trim_left(next_line);
            size_t joined_len = strlen(joined);
            size_t next_len = strlen(next_content);
            char *new_joined;

            if ((*next_content == '\0') || (*next_content == '#'))
            {
                parser->line_index++;
                continue;
            }

            if (is_separator_line(next_content))
            {
                free(joined);
                return NULL;
            }

            new_joined = (char *)realloc(joined, joined_len + 1u + next_len + 1u);
            if (new_joined == NULL)
            {
                free(joined);
                return NULL;
            }
            joined = new_joined;
            joined[joined_len] = ' ';
            memcpy(joined + joined_len + 1u, next_content, next_len + 1u);

            parser->line_index++;
        }

        node = parse_flow_text(joined);
        free(joined);
        return node;
    }

    if (text[0] == '\'')
    {
        size_t pos = 0u;
        char *str = parse_single_quoted(text, strlen(text), &pos);
        yaml_node_t *str_node;

        if (str == NULL)
        {
            return NULL;
        }

        while ((text[pos] != '\0') && isspace((unsigned char)text[pos]))
        {
            pos++;
        }
        if (text[pos] != '\0')
        {
            free(str);
            return NULL;
        }

        str_node = yaml_node_new(YAML_NODE_STRING);
        if (str_node == NULL)
        {
            free(str);
            return NULL;
        }

        str_node->as.text = str;
        return str_node;
    }

    token = yaml_strdup(text);
    if (token == NULL)
    {
        return NULL;
    }
    trim_right(token);

    node = yaml_parse_scalar_token(token);
    free(token);
    return node;
}

static yaml_node_t *yaml_parse_block_mapping(yaml_line_parser_t *parser, int indent)
{
    yaml_node_t *mapping = yaml_node_new(YAML_NODE_MAPPING);

    if (mapping == NULL)
    {
        return NULL;
    }

    while (parser->line_index < parser->line_count)
    {
        char *line;
        char *content;
        int current_indent;
        int colon_index;
        char *key;
        char *value_part;
        yaml_node_t *value_node;

        yaml_skip_empty(parser);
        if (parser->line_index >= parser->line_count)
        {
            break;
        }

        line = parser->lines[parser->line_index];
        current_indent = line_indent(line);
        if (current_indent < indent)
        {
            break;
        }
        if (current_indent != indent)
        {
            yaml_node_free(mapping);
            return NULL;
        }

        content = trim_left(line);
        if ((content[0] == '-') && ((content[1] == '\0') || isspace((unsigned char)content[1])))
        {
            break;
        }

        colon_index = find_top_level_colon(content);
        if (colon_index < 0)
        {
            yaml_node_free(mapping);
            return NULL;
        }

        key = yaml_strndup(content, (size_t)colon_index);
        if (key == NULL)
        {
            yaml_node_free(mapping);
            return NULL;
        }
        trim_right(key);

        value_part = trim_left(content + colon_index + 1);
        parser->line_index++;

        if (value_part[0] == '\0')
        {
            bool use_indentless_sequence = false;
            size_t lookahead = parser->line_index;

            while (lookahead < parser->line_count)
            {
                char *peek_line = parser->lines[lookahead];
                char *peek_content = trim_left(peek_line);

                if ((*peek_content == '\0') || (*peek_content == '#') ||
                    is_separator_line(peek_content))
                {
                    lookahead++;
                    continue;
                }

                if ((line_indent(peek_line) == indent) && (peek_content[0] == '-') &&
                    ((peek_content[1] == '\0') ||
                     isspace((unsigned char)peek_content[1])))
                {
                    use_indentless_sequence = true;
                }
                break;
            }

            if (use_indentless_sequence)
            {
                parser->line_index = lookahead;
                value_node = yaml_parse_block_sequence(parser, indent);
            }
            else
            {
                value_node = yaml_parse_block_node(parser, indent + 1);
            }

            if (value_node == NULL)
            {
                value_node = yaml_node_new(YAML_NODE_NULL);
            }
        }
        else
        {
            value_node = yaml_parse_inline_value(parser, value_part, indent);
        }

        if (value_node == NULL)
        {
            free(key);
            yaml_node_free(mapping);
            return NULL;
        }

        if (!yaml_mapping_add(mapping, key, value_node))
        {
            free(key);
            yaml_node_free(value_node);
            yaml_node_free(mapping);
            return NULL;
        }

        free(key);
    }

    return mapping;
}

static yaml_node_t *yaml_parse_block_sequence(yaml_line_parser_t *parser, int indent)
{
    yaml_node_t *sequence = yaml_node_new(YAML_NODE_SEQUENCE);

    if (sequence == NULL)
    {
        return NULL;
    }

    while (parser->line_index < parser->line_count)
    {
        char *line;
        char *content;
        int current_indent;

        yaml_skip_empty(parser);
        if (parser->line_index >= parser->line_count)
        {
            break;
        }

        line = parser->lines[parser->line_index];
        current_indent = line_indent(line);
        if (current_indent < indent)
        {
            break;
        }
        if (current_indent != indent)
        {
            yaml_node_free(sequence);
            return NULL;
        }

        content = trim_left(line);
        if ((content[0] != '-') || ((content[1] != '\0') && !isspace((unsigned char)content[1])))
        {
            break;
        }

        content = trim_left(content + 1);

        if (content[0] == '\0')
        {
            yaml_node_t *item;
            parser->line_index++;
            item = yaml_parse_block_node(parser, indent + 1);
            if (item == NULL)
            {
                item = yaml_node_new(YAML_NODE_NULL);
            }
            if (item == NULL)
            {
                yaml_node_free(sequence);
                return NULL;
            }
            if (!yaml_sequence_add(sequence, item))
            {
                yaml_node_free(item);
                yaml_node_free(sequence);
                return NULL;
            }
            continue;
        }

        {
            size_t synthetic_len = (size_t)indent + 2u + strlen(content);
            char *synthetic = (char *)malloc(synthetic_len + 1u);
            yaml_node_t *item;

            if (synthetic == NULL)
            {
                yaml_node_free(sequence);
                return NULL;
            }

            memset(synthetic, ' ', (size_t)indent + 2u);
            memcpy(synthetic + (size_t)indent + 2u, content, strlen(content) + 1u);

            free(parser->lines[parser->line_index]);
            parser->lines[parser->line_index] = synthetic;

            item = yaml_parse_block_node(parser, indent + 1);
            if (item == NULL)
            {
                yaml_node_free(sequence);
                return NULL;
            }

            if (!yaml_sequence_add(sequence, item))
            {
                yaml_node_free(item);
                yaml_node_free(sequence);
                return NULL;
            }
        }
    }

    return sequence;
}

static yaml_node_t *yaml_parse_block_node(yaml_line_parser_t *parser, int min_indent)
{
    char *line;
    char *content;
    int indent;
    int colon_index;

    yaml_skip_empty(parser);
    if (parser->line_index >= parser->line_count)
    {
        return NULL;
    }

    line = parser->lines[parser->line_index];
    indent = line_indent(line);
    if (indent < min_indent)
    {
        return NULL;
    }

    content = trim_left(line);

    if ((content[0] == '-') && ((content[1] == '\0') || isspace((unsigned char)content[1])))
    {
        return yaml_parse_block_sequence(parser, indent);
    }

    colon_index = find_top_level_colon(content);
    if (colon_index >= 0)
    {
        return yaml_parse_block_mapping(parser, indent);
    }

    parser->line_index++;
    return yaml_parse_inline_value(parser, content, indent);
}

static void free_lines(char **lines, size_t count)
{
    for (size_t i = 0u; i < count; i++)
    {
        free(lines[i]);
    }
    free(lines);
}

static char **split_lines(const char *text, size_t *out_count)
{
    char **lines = NULL;
    size_t count = 0u;
    size_t start = 0u;
    size_t len;

    if ((text == NULL) || (out_count == NULL))
    {
        return NULL;
    }

    len = strlen(text);

    for (size_t i = 0u; i <= len; i++)
    {
        if ((i != len) && (text[i] != '\n'))
        {
            continue;
        }

        {
            size_t line_len = i - start;
            char *line = yaml_strndup(text + start, line_len);
            char **new_lines;

            if (line == NULL)
            {
                free_lines(lines, count);
                return NULL;
            }
            trim_right(line);

            new_lines = (char **)realloc(lines, (count + 1u) * sizeof(char *));
            if (new_lines == NULL)
            {
                free(line);
                free_lines(lines, count);
                return NULL;
            }

            lines = new_lines;
            lines[count++] = line;
        }

        start = i + 1u;
    }

    *out_count = count;
    return lines;
}

yaml_node_t *yaml_parse_file(const char *path)
{
    FILE *file;
    long file_size;
    char *text;
    size_t bytes_read;
    yaml_line_parser_t parser;
    yaml_node_t *root;

    if (path == NULL)
    {
        return NULL;
    }

    file = fopen(path, "rb");
    if (file == NULL)
    {
        return NULL;
    }

    if (fseek(file, 0L, SEEK_END) != 0)
    {
        fclose(file);
        return NULL;
    }

    file_size = ftell(file);
    if (file_size < 0)
    {
        fclose(file);
        return NULL;
    }

    if (fseek(file, 0L, SEEK_SET) != 0)
    {
        fclose(file);
        return NULL;
    }

    text = (char *)malloc((size_t)file_size + 1u);
    if (text == NULL)
    {
        fclose(file);
        return NULL;
    }

    bytes_read = fread(text, 1u, (size_t)file_size, file);
    fclose(file);

    if (bytes_read != (size_t)file_size)
    {
        free(text);
        return NULL;
    }

    text[bytes_read] = '\0';

    parser.lines = split_lines(text, &parser.line_count);
    parser.line_index = 0u;
    free(text);

    if (parser.lines == NULL)
    {
        return NULL;
    }

    root = yaml_parse_block_node(&parser, 0);

    yaml_skip_empty(&parser);
    if ((root == NULL) || (parser.line_index != parser.line_count))
    {
        yaml_node_free(root);
        root = NULL;
    }

    free_lines(parser.lines, parser.line_count);
    return root;
}

const yaml_node_t *yaml_mapping_get(const yaml_node_t *node, const char *key)
{
    if ((node == NULL) || (node->type != YAML_NODE_MAPPING) || (key == NULL))
    {
        return NULL;
    }

    for (size_t i = 0u; i < node->as.mapping.count; i++)
    {
        if (strcmp(node->as.mapping.pairs[i].key, key) == 0)
        {
            return node->as.mapping.pairs[i].value;
        }
    }

    return NULL;
}

static const char *yaml_scalar_text(const yaml_node_t *node)
{
    if (node == NULL)
    {
        return NULL;
    }

    if ((node->type == YAML_NODE_STRING) || (node->type == YAML_NODE_INT))
    {
        return node->as.text;
    }

    return NULL;
}

bool yaml_node_parse_u64(const yaml_node_t *node, uint64_t *out_value)
{
    const char *text;
    uint64_t value = 0u;

    if (out_value == NULL)
    {
        return false;
    }

    if ((node != NULL) && (node->type == YAML_NODE_BOOL))
    {
        *out_value = node->as.bool_value ? 1u : 0u;
        return true;
    }

    text = yaml_scalar_text(node);
    if ((text == NULL) || (*text == '\0'))
    {
        return false;
    }

    for (size_t i = 0u; text[i] != '\0'; i++)
    {
        uint8_t digit;
        uint64_t next;

        if (!isdigit((unsigned char)text[i]))
        {
            return false;
        }

        digit = (uint8_t)(text[i] - '0');
        if (value > (UINT64_MAX / 10u))
        {
            return false;
        }

        next = value * 10u;
        if (next > (UINT64_MAX - digit))
        {
            return false;
        }

        value = next + digit;
    }

    *out_value = value;
    return true;
}

bool yaml_node_parse_decimal_le(const yaml_node_t *node, uint8_t *out, size_t out_len)
{
    const char *text;

    if ((out == NULL) || (out_len == 0u))
    {
        return false;
    }

    text = yaml_scalar_text(node);
    if ((text == NULL) || (*text == '\0'))
    {
        return false;
    }

    memset(out, 0, out_len);

    for (size_t i = 0u; text[i] != '\0'; i++)
    {
        uint8_t digit;
        uint32_t carry;

        if (!isdigit((unsigned char)text[i]))
        {
            return false;
        }

        digit = (uint8_t)(text[i] - '0');
        carry = digit;

        for (size_t j = 0u; j < out_len; j++)
        {
            uint32_t value = (uint32_t)out[j] * 10u + carry;
            out[j] = (uint8_t)(value & 0xFFu);
            carry = value >> 8u;
        }

        if (carry != 0u)
        {
            return false;
        }
    }

    return true;
}

static int hex_nibble(char ch)
{
    if ((ch >= '0') && (ch <= '9'))
    {
        return ch - '0';
    }
    if ((ch >= 'a') && (ch <= 'f'))
    {
        return 10 + (ch - 'a');
    }
    if ((ch >= 'A') && (ch <= 'F'))
    {
        return 10 + (ch - 'A');
    }
    return -1;
}

bool yaml_node_parse_hex(const yaml_node_t *node, uint8_t **out_bytes, size_t *out_len)
{
    const char *text;
    size_t text_len;
    size_t byte_len;
    uint8_t *bytes;

    if ((out_bytes == NULL) || (out_len == NULL))
    {
        return false;
    }

    *out_bytes = NULL;
    *out_len = 0u;

    text = yaml_scalar_text(node);
    if ((text == NULL) || (text[0] != '0') || (text[1] != 'x'))
    {
        return false;
    }

    text += 2;
    text_len = strlen(text);
    if ((text_len % 2u) != 0u)
    {
        return false;
    }

    byte_len = text_len / 2u;
    bytes = (uint8_t *)malloc(byte_len == 0u ? 1u : byte_len);
    if (bytes == NULL)
    {
        return false;
    }

    for (size_t i = 0u; i < byte_len; i++)
    {
        int hi = hex_nibble(text[i * 2u]);
        int lo = hex_nibble(text[i * 2u + 1u]);
        if ((hi < 0) || (lo < 0))
        {
            free(bytes);
            return false;
        }
        bytes[i] = (uint8_t)((hi << 4u) | lo);
    }

    *out_bytes = bytes;
    *out_len = byte_len;
    return true;
}
