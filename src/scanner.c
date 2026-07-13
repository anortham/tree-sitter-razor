#include "tree_sitter/alloc.h"
#include "tree_sitter/array.h"
#include "tree_sitter/parser.h"

#include <string.h>
#include <wctype.h>

enum TokenType {
    OPT_SEMI,
    INTERPOLATION_REGULAR_START,
    INTERPOLATION_VERBATIM_START,
    INTERPOLATION_RAW_START,
    INTERPOLATION_START_QUOTE,
    INTERPOLATION_END_QUOTE,
    INTERPOLATION_OPEN_BRACE,
    INTERPOLATION_CLOSE_BRACE,
    INTERPOLATION_STRING_CONTENT,
    RAW_STRING_START,
    RAW_STRING_END,
    RAW_STRING_CONTENT,
    HTML_ATTRIBUTE_TAIL_TEXT,
    START_TAG_NAME,
    VOID_TAG_NAME,
    SCRIPT_START_TAG_NAME,
    STYLE_START_TAG_NAME,
    END_TAG_NAME,
    SELF_CLOSING_TAG_DELIMITER,
    RAW_TEXT,
};

typedef enum {
    REGULAR = 1 << 0,
    VERBATIM = 1 << 1,
    RAW = 1 << 2,
} StringType;

typedef struct {
    uint8_t dollar_count;
    uint8_t open_brace_count;
    uint8_t quote_count;
    StringType string_type;
} Interpolation;

typedef Array(char) TagName;

static inline bool is_regular(Interpolation *interpolation) { return interpolation->string_type & REGULAR; }

static inline bool is_verbatim(Interpolation *interpolation) { return interpolation->string_type & VERBATIM; }

static inline bool is_raw(Interpolation *interpolation) { return interpolation->string_type & RAW; }

typedef struct {
    uint8_t quote_count;
    Array(Interpolation) interpolation_stack;
    Array(TagName) tag_stack;
} Scanner;

static inline void advance(TSLexer *lexer) { lexer->advance(lexer, false); }

static inline void skip(TSLexer *lexer) { lexer->advance(lexer, true); }

static inline bool is_tag_name_character(int32_t character) {
    return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') ||
           (character >= '0' && character <= '9') || character == '_' || character == '-' || character == ':' ||
           character == '.';
}

static TagName scan_tag_name(TSLexer *lexer) {
    TagName name = array_new();
    while (is_tag_name_character(lexer->lookahead)) {
        array_push(&name, (char)lexer->lookahead);
        advance(lexer);
    }
    return name;
}

static bool tag_name_is_valid(const TagName *name) {
    if (name->size == 0 || name->contents[0] == '.' || name->contents[name->size - 1] == '.') return false;
    for (unsigned i = 1; i < name->size; i++) {
        if (name->contents[i] == '.' && name->contents[i - 1] == '.') return false;
    }
    return true;
}

static bool tag_name_equals_literal(const TagName *name, const char *literal) {
    size_t length = strlen(literal);
    if (name->size != length) return false;
    for (unsigned i = 0; i < name->size; i++) {
        if (towlower((unsigned char)name->contents[i]) != towlower((unsigned char)literal[i])) return false;
    }
    return true;
}

static bool tag_names_equal(const TagName *left, const TagName *right) {
    if (left->size != right->size) return false;
    for (unsigned i = 0; i < left->size; i++) {
        if (towlower((unsigned char)left->contents[i]) != towlower((unsigned char)right->contents[i])) return false;
    }
    return true;
}

static bool is_void_tag(const TagName *name) {
    static const char *const names[] = {
        "area", "base", "br", "col", "embed", "hr", "img", "input",
        "link", "meta", "param", "source", "track", "wbr",
    };
    for (unsigned i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
        if (tag_name_equals_literal(name, names[i])) return true;
    }
    return false;
}

static void clear_tag_stack(Scanner *scanner) {
    for (unsigned i = 0; i < scanner->tag_stack.size; i++) {
        array_delete(&scanner->tag_stack.contents[i]);
    }
    array_clear(&scanner->tag_stack);
}

static void pop_tag(Scanner *scanner) {
    TagName name = array_pop(&scanner->tag_stack);
    array_delete(&name);
}

static bool scan_raw_text(Scanner *scanner, TSLexer *lexer) {
    if (scanner->tag_stack.size == 0) return false;

    const TagName *name = array_back(&scanner->tag_stack);
    const char *delimiter = tag_name_equals_literal(name, "script") ? "</script" : "</style";
    unsigned delimiter_index = 0;
    bool did_advance = false;
    lexer->mark_end(lexer);

    while (lexer->lookahead) {
        if (towlower(lexer->lookahead) == delimiter[delimiter_index]) {
            delimiter_index++;
            if (delimiter[delimiter_index] == '\0') break;
            advance(lexer);
        } else {
            delimiter_index = 0;
            advance(lexer);
            did_advance = true;
            lexer->mark_end(lexer);
        }
    }

    lexer->result_symbol = RAW_TEXT;
    return did_advance;
}

static bool scan_markup_tag(Scanner *scanner, TSLexer *lexer, const bool *valid_symbols) {
    TagName name = scan_tag_name(lexer);
    if (!tag_name_is_valid(&name)) {
        array_delete(&name);
        return false;
    }

    if (valid_symbols[END_TAG_NAME]) {
        if (scanner->tag_stack.size > 0 && tag_names_equal(array_back(&scanner->tag_stack), &name)) {
            pop_tag(scanner);
            array_delete(&name);
            lexer->result_symbol = END_TAG_NAME;
            return true;
        }
        array_delete(&name);
        return false;
    }

    if (is_void_tag(&name)) {
        array_delete(&name);
        lexer->result_symbol = VOID_TAG_NAME;
        return true;
    }

    if (tag_name_equals_literal(&name, "script")) {
        lexer->result_symbol = SCRIPT_START_TAG_NAME;
    } else if (tag_name_equals_literal(&name, "style")) {
        lexer->result_symbol = STYLE_START_TAG_NAME;
    } else {
        lexer->result_symbol = START_TAG_NAME;
    }
    array_push(&scanner->tag_stack, name);
    return true;
}

void *tree_sitter_razor_external_scanner_create() {
    Scanner *scanner = ts_calloc(1, sizeof(Scanner));
    array_init(&scanner->interpolation_stack);
    array_init(&scanner->tag_stack);
    return scanner;
}

void tree_sitter_razor_external_scanner_destroy(void *payload) {
    Scanner *scanner = (Scanner *)payload;
    clear_tag_stack(scanner);
    array_delete(&scanner->tag_stack);
    array_delete(&scanner->interpolation_stack);
    ts_free(scanner);
}

unsigned tree_sitter_razor_external_scanner_serialize(void *payload, char *buffer) {
    Scanner *scanner = (Scanner *)payload;

    if (scanner->interpolation_stack.size > UINT8_MAX ||
        scanner->interpolation_stack.size * 4 + 3 > TREE_SITTER_SERIALIZATION_BUFFER_SIZE) {
        return 0;
    }

    unsigned size = 0;

    buffer[size++] = (char)scanner->quote_count;
    buffer[size++] = (char)scanner->interpolation_stack.size;

    for (unsigned i = 0; i < scanner->interpolation_stack.size; i++) {
        Interpolation interpolation = scanner->interpolation_stack.contents[i];
        buffer[size++] = (char)interpolation.dollar_count;
        buffer[size++] = (char)interpolation.open_brace_count;
        buffer[size++] = (char)interpolation.quote_count;
        buffer[size++] = (char)interpolation.string_type;
    }

    unsigned tag_count_index = size++;
    unsigned first_tag = scanner->tag_stack.size;
    unsigned tag_bytes = 0;
    while (first_tag > 0 && scanner->tag_stack.size - first_tag < UINT8_MAX) {
        unsigned name_length = scanner->tag_stack.contents[first_tag - 1].size;
        if (name_length > UINT8_MAX) name_length = UINT8_MAX;
        if (size + tag_bytes + name_length + 1 > TREE_SITTER_SERIALIZATION_BUFFER_SIZE) break;
        tag_bytes += name_length + 1;
        first_tag--;
    }

    buffer[tag_count_index] = (char)(scanner->tag_stack.size - first_tag);
    for (unsigned i = first_tag; i < scanner->tag_stack.size; i++) {
        unsigned name_length = scanner->tag_stack.contents[i].size;
        if (name_length > UINT8_MAX) name_length = UINT8_MAX;
        buffer[size++] = (char)name_length;
        memcpy(&buffer[size], scanner->tag_stack.contents[i].contents, name_length);
        size += name_length;
    }

    return size;
}

void tree_sitter_razor_external_scanner_deserialize(void *payload, const char *buffer, unsigned length) {
    Scanner *scanner = (Scanner *)payload;

    scanner->quote_count = 0;
    array_clear(&scanner->interpolation_stack);
    clear_tag_stack(scanner);
    unsigned size = 0;

    if (length > 0) {
        if (length < 3) return;
        scanner->quote_count = (unsigned char)buffer[size++];
        unsigned interpolation_count = (unsigned char)buffer[size++];
        if (size + interpolation_count * 4 + 1 > length) return;
        scanner->interpolation_stack.size = interpolation_count;
        array_reserve(&scanner->interpolation_stack, scanner->interpolation_stack.size);

        for (unsigned i = 0; i < scanner->interpolation_stack.size; i++) {
            Interpolation interpolation = {0};
            interpolation.dollar_count = buffer[size++];
            interpolation.open_brace_count = buffer[size++];
            interpolation.quote_count = buffer[size++];
            interpolation.string_type = (unsigned char)buffer[size++];
            scanner->interpolation_stack.contents[i] = interpolation;
        }

        unsigned tag_count = (unsigned char)buffer[size++];
        array_reserve(&scanner->tag_stack, tag_count);
        for (unsigned i = 0; i < tag_count; i++) {
            if (size >= length) {
                clear_tag_stack(scanner);
                return;
            }
            unsigned name_length = (unsigned char)buffer[size++];
            if (size + name_length > length) {
                clear_tag_stack(scanner);
                return;
            }
            TagName name = array_new();
            array_reserve(&name, name_length);
            name.size = name_length;
            memcpy(name.contents, &buffer[size], name_length);
            size += name_length;
            array_push(&scanner->tag_stack, name);
        }
    }
}

bool tree_sitter_razor_external_scanner_scan(void *payload, TSLexer *lexer, const bool *valid_symbols) {
    Scanner *scanner = (Scanner *)payload;

    uint8_t brace_advanced = 0;
    uint8_t quote_count = 0;
    bool did_advance = false;

    if (valid_symbols[OPT_SEMI] && valid_symbols[INTERPOLATION_REGULAR_START]) {
        return false;
    }

    if (valid_symbols[RAW_TEXT] && !valid_symbols[START_TAG_NAME] && !valid_symbols[END_TAG_NAME]) {
        return scan_raw_text(scanner, lexer);
    }

    if (valid_symbols[SELF_CLOSING_TAG_DELIMITER]) {
        while (iswspace(lexer->lookahead)) skip(lexer);
        if (lexer->lookahead != '/') return false;
        advance(lexer);
        if (lexer->lookahead != '>') return false;
        advance(lexer);
        if (scanner->tag_stack.size > 0) pop_tag(scanner);
        lexer->result_symbol = SELF_CLOSING_TAG_DELIMITER;
        return true;
    }

    if (valid_symbols[START_TAG_NAME] || valid_symbols[VOID_TAG_NAME] || valid_symbols[SCRIPT_START_TAG_NAME] ||
        valid_symbols[STYLE_START_TAG_NAME] || valid_symbols[END_TAG_NAME]) {
        return scan_markup_tag(scanner, lexer, valid_symbols);
    }

    if (valid_symbols[HTML_ATTRIBUTE_TAIL_TEXT] && iswspace(lexer->lookahead)) {
        do {
            advance(lexer);
        } while (!lexer->eof(lexer) && lexer->lookahead != '\n' && lexer->lookahead != '\r' &&
                 lexer->lookahead != '<' && lexer->lookahead != '>' && lexer->lookahead != '"' &&
                 lexer->lookahead != '\'' && lexer->lookahead != '@');
        lexer->result_symbol = HTML_ATTRIBUTE_TAIL_TEXT;
        return true;
    }

    if (valid_symbols[OPT_SEMI]) {
        lexer->result_symbol = OPT_SEMI;
        if (lexer->lookahead == ';') {
            advance(lexer);
        }
        return true;
    }

    if (valid_symbols[RAW_STRING_START]) {
        while (iswspace(lexer->lookahead)) {
            skip(lexer);
        }

        if (lexer->lookahead == '"') {
            while (lexer->lookahead == '"') {
                advance(lexer);
                quote_count++;
            }

            if (quote_count >= 3) {
                lexer->result_symbol = RAW_STRING_START;
                scanner->quote_count = quote_count;
                return true;
            }
        }
    }

    if (valid_symbols[RAW_STRING_END] && lexer->lookahead == '"') {
        while (lexer->lookahead == '"') {
            advance(lexer);
            quote_count++;
        }

        if (quote_count == scanner->quote_count) {
            lexer->result_symbol = RAW_STRING_END;
            scanner->quote_count = 0;
            return true;
        }

        did_advance = quote_count > 0;
    }

    if (valid_symbols[RAW_STRING_CONTENT]) {
        while (lexer->lookahead) {
            if (lexer->lookahead == '"') {
                lexer->mark_end(lexer);
                quote_count = 0;

                while (lexer->lookahead == '"') {
                    advance(lexer);
                    quote_count++;
                }

                if (quote_count == scanner->quote_count) {
                    lexer->result_symbol = RAW_STRING_CONTENT;
                    return true;
                }
            }
            advance(lexer);
            did_advance = true;
        }
        lexer->mark_end(lexer);
        lexer->result_symbol = RAW_STRING_CONTENT;
        return true;
    }

    if (valid_symbols[INTERPOLATION_REGULAR_START] || valid_symbols[INTERPOLATION_VERBATIM_START] ||
        valid_symbols[INTERPOLATION_RAW_START]) {
        while (iswspace(lexer->lookahead)) {
            skip(lexer);
        }

        uint8_t dollar_advanced = 0;

        bool is_verbatim = false;

        if (lexer->lookahead == '@') {
            is_verbatim = true;
            advance(lexer);
        }

        while (lexer->lookahead == '$' && quote_count == 0) {
            advance(lexer);
            dollar_advanced++;
        }

        if (dollar_advanced > 0 && (lexer->lookahead == '"' || lexer->lookahead == '@')) {
            lexer->result_symbol = INTERPOLATION_REGULAR_START;
            Interpolation interpolation = {
                .dollar_count = dollar_advanced,
                .open_brace_count = 0,
                .quote_count = 0,
                .string_type = 0,
            };

            if (is_verbatim || lexer->lookahead == '@') {
                if (lexer->lookahead == '@') {
                    advance(lexer);
                    is_verbatim = true;
                }
                lexer->result_symbol = INTERPOLATION_VERBATIM_START;
                interpolation.string_type = VERBATIM;
            }

            lexer->mark_end(lexer);
            advance(lexer);

            if (lexer->lookahead == '"' && !is_verbatim) {
                advance(lexer);
                if (lexer->lookahead == '"') {
                    lexer->result_symbol = INTERPOLATION_RAW_START;
                    interpolation.string_type |= RAW;
                    array_push(&scanner->interpolation_stack, interpolation);
                }
            } else {
                interpolation.string_type |= REGULAR;
                array_push(&scanner->interpolation_stack, interpolation);
            }

            return true;
        }
    }

    if (valid_symbols[INTERPOLATION_START_QUOTE] && scanner->interpolation_stack.size > 0) {
        Interpolation *current_interpolation = array_back(&scanner->interpolation_stack);

        if (is_verbatim(current_interpolation) || is_regular(current_interpolation)) {
            if (lexer->lookahead == '"') {
                advance(lexer);
                current_interpolation->quote_count++;
            }
        } else {
            while (lexer->lookahead == '"') {
                advance(lexer);
                current_interpolation->quote_count++;
            }
        }

        lexer->result_symbol = INTERPOLATION_START_QUOTE;
        return current_interpolation->quote_count > 0;
    }

    if (valid_symbols[INTERPOLATION_END_QUOTE] && scanner->interpolation_stack.size > 0) {
        Interpolation *current_interpolation = array_back(&scanner->interpolation_stack);

        while (lexer->lookahead == '"') {
            advance(lexer);
            quote_count++;
        }

        if (quote_count == current_interpolation->quote_count) {
            lexer->result_symbol = INTERPOLATION_END_QUOTE;
            array_pop(&scanner->interpolation_stack);
            return true;
        }

        did_advance = quote_count > 0;
    }

    if (valid_symbols[INTERPOLATION_OPEN_BRACE] && scanner->interpolation_stack.size > 0) {
        Interpolation *current_interpolation = array_back(&scanner->interpolation_stack);

        while (lexer->lookahead == '{' && brace_advanced < current_interpolation->dollar_count) {
            advance(lexer);
            brace_advanced++;
        }

        if (brace_advanced > 0 && brace_advanced == current_interpolation->dollar_count &&
            (brace_advanced == 0 || lexer->lookahead != '{')) {
            current_interpolation->open_brace_count = brace_advanced;
            lexer->result_symbol = INTERPOLATION_OPEN_BRACE;
            return true;
        }
    }

    if (valid_symbols[INTERPOLATION_CLOSE_BRACE] && scanner->interpolation_stack.size > 0) {
        uint8_t brace_advanced = 0;
        Interpolation *current_interpolation = array_back(&scanner->interpolation_stack);

        while (iswspace(lexer->lookahead)) {
            advance(lexer);
        }

        while (lexer->lookahead == '}') {
            advance(lexer);
            brace_advanced++;

            if (brace_advanced == current_interpolation->open_brace_count) {
                current_interpolation->open_brace_count = 0;
                lexer->result_symbol = INTERPOLATION_CLOSE_BRACE;
                return true;
            }
        }

        return false;
    }

    if (valid_symbols[INTERPOLATION_STRING_CONTENT] && scanner->interpolation_stack.size > 0) {
        lexer->result_symbol = INTERPOLATION_STRING_CONTENT;
        Interpolation *current_interpolation = array_back(&scanner->interpolation_stack);

        while (lexer->lookahead) {
            if (is_raw(current_interpolation)) {
                if (lexer->lookahead == '"') {
                    lexer->mark_end(lexer);
                    advance(lexer);
                    if (lexer->lookahead == '"') {
                        advance(lexer);
                        uint8_t quote_advanced = 2;
                        while (lexer->lookahead == '"') {
                            quote_advanced++;
                            advance(lexer);
                        }
                        if (quote_advanced == current_interpolation->quote_count) {
                            return did_advance;
                        }
                    }
                }

                if (lexer->lookahead == '{') {
                    lexer->mark_end(lexer);

                    while (lexer->lookahead == '{' && brace_advanced < current_interpolation->open_brace_count) {
                        advance(lexer);
                        brace_advanced++;
                    }

                    if (brace_advanced == current_interpolation->open_brace_count &&
                        (brace_advanced == 0 || lexer->lookahead != '{')) {
                        return did_advance;
                    }
                }
            } else if (is_verbatim(current_interpolation)) {
                if (lexer->lookahead == '"') {
                    lexer->mark_end(lexer);
                    advance(lexer);
                    if (lexer->lookahead == '"') {
                        advance(lexer);
                        continue;
                    }
                    return did_advance;
                }

                if (lexer->lookahead == '{') {
                    lexer->mark_end(lexer);

                    while (lexer->lookahead == '{' && brace_advanced < current_interpolation->open_brace_count) {
                        advance(lexer);
                        brace_advanced++;
                    }

                    if (brace_advanced == current_interpolation->open_brace_count &&
                        (brace_advanced == 0 || lexer->lookahead != '{')) {
                        return did_advance;
                    }
                }
            } else if (is_regular(current_interpolation)) {
                if (lexer->lookahead == '\\' || lexer->lookahead == '\n' || lexer->lookahead == '"') {
                    lexer->mark_end(lexer);
                    return did_advance;
                }

                if (lexer->lookahead == '{') {
                    lexer->mark_end(lexer);

                    while (lexer->lookahead == '{' && brace_advanced < current_interpolation->open_brace_count) {
                        advance(lexer);
                        brace_advanced++;
                    }

                    if (brace_advanced == current_interpolation->open_brace_count &&
                        (brace_advanced == 0 || lexer->lookahead != '{')) {
                        return did_advance;
                    }
                }
            }

            if (lexer->lookahead != '{') {
                brace_advanced = 0;
            }
            advance(lexer);
            did_advance = true;
        }

        lexer->mark_end(lexer);
        return did_advance;
    }

    return false;
}
