#include "../../headers/parser.h"
#include <stdint.h>
#include <string.h>

ASTNode *parse_array_node(Parser *parser, DataType datatype, CodegenContext *context, uint32_t count) {
    consume(parser, TOKEN_LBRACE, "Expected '{' to open array.");

    bool is_dynamic = (count > 0) ? false: true;
    uint32_t capacity = (count > 0) ? count : 8;
    uint32_t parsed_count = 0;
    ASTNode **elements = (ASTNode **)arena_alloc(context->a, sizeof(ASTNode *) * capacity);

    while (parser->current.type != TOKEN_RBRACE && parser->current.type != TOKEN_EOF) {
        if (parsed_count >= capacity) {
            uint32_t old_capacity = capacity;
            capacity *= 2;
            ASTNode **new_elements = (ASTNode **)arena_alloc(context->a, sizeof(ASTNode *) * capacity);
            memcpy(new_elements, elements, sizeof(ASTNode *) * old_capacity);
            elements = new_elements;
        }

        ASTNode *elem = parse_logical_or(parser, context);
        elements[parsed_count++] = elem;

        if (parser->current.type == TOKEN_COMMA) {
            advance(parser);
        } else {
            break;
        }
    }

    consume(parser, TOKEN_RBRACE, "Expected '}' to close array.");

    uint32_t final_capacity = (count > 0) ? count : parsed_count;
    return create_array_node(elements, parsed_count, final_capacity, is_dynamic, datatype, context->a);
}

ASTNode *parse_index_expr(Parser *parser, CodegenContext *context, ASTNode *target) {
    consume(parser, TOKEN_LSQUARE_BRACE, "Expected '[' for array indexing.");
    ASTNode *index = parse_logical_or(parser, context);
    consume(parser, TOKEN_RSQUARE_BRACE, "Expected ']' after array index expression.");

    return create_index_expr_node(target, index, TYPE_NULL, context->a);
}
