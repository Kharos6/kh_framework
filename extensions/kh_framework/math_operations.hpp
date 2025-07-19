#ifndef MATH_OPERATIONS_HPP
#define MATH_OPERATIONS_HPP

/* Token types for expression parser */
typedef enum {
    TOKEN_NUMBER,
    TOKEN_OPERATOR,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_COMMA,
    TOKEN_FUNCTION,
    TOKEN_END,
    TOKEN_ERROR
} token_type_t;

/* Token structure - fully dynamic allocation */
typedef struct {
    token_type_t type;
    char* value;
    int value_capacity;  /* Current allocated capacity for value */
    double number;
    int pos;
    int allocation_failed;
} token_t;

/* Mathematical function definition */
typedef struct {
    const char* name;
    int arg_count;
    double (*func_ptr_1)(double);
    double (*func_ptr_2)(double, double);
    double (*func_ptr_3)(double, double, double);
} math_function_t;

/* Expression parser context - using dynamic allocation */
typedef struct {
    const char* expr;
    int pos;
    int length;
    token_t current_token;
    char* error_msg;
    int error_msg_capacity;
    int recursion_depth;     /* Track recursion depth */
    int operation_count;     /* Track number of operations */
    int allocation_failed;
} parser_context_t;

/* Safe mathematical function implementations */
static inline double kh_safe_sqrt(double x) {
    return (x < 0) ? 0.0 : sqrt(x);
}

static inline double kh_safe_log(double x) {
    return (x <= 0) ? 0.0 : log(x);
}

static inline double kh_safe_log10(double x) {
    return (x <= 0) ? 0.0 : log10(x);
}

static inline double kh_safe_asin(double x) {
    return (x < -1.0 || x > 1.0) ? 0.0 : asin(x);
}

static inline double kh_safe_acos(double x) {
    return (x < -1.0 || x > 1.0) ? 0.0 : acos(x);
}

static inline double kh_degrees_to_radians(double degrees) {
    return degrees * MATH_PI / 180.0;
}

static inline double kh_radians_to_degrees(double radians) {
    return radians * 180.0 / MATH_PI;
}

/* Wrapper functions that apply cleanup for trigonometric functions */
static inline double kh_sin_cleaned(double x) {
    return kh_cleanup_near_zero(sin(x));
}

static inline double kh_cos_cleaned(double x) {
    return kh_cleanup_near_zero(cos(x));
}

static inline double kh_tan_cleaned(double x) {
    return kh_cleanup_near_zero(tan(x));
}

static inline double kh_sind(double degrees) {
    return kh_cleanup_near_zero(sin(kh_degrees_to_radians(degrees)));
}

static inline double kh_cosd(double degrees) {
    return kh_cleanup_near_zero(cos(kh_degrees_to_radians(degrees)));
}

static inline double kh_tand(double degrees) {
    return kh_cleanup_near_zero(tan(kh_degrees_to_radians(degrees)));
}

static inline double kh_math_min(double a, double b) {
    return (a < b) ? a : b;
}

static inline double kh_math_max(double a, double b) {
    return (a > b) ? a : b;
}

static inline double kh_math_clamp(double value, double min_val, double max_val) {
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

/* Whitelist of allowed mathematical functions - SECURITY CRITICAL */
static const math_function_t KH_ALLOWED_MATH_FUNCTIONS[] = {
    /* Single argument functions */
    {"sqrt", 1, kh_safe_sqrt, NULL, NULL},
    {"cbrt", 1, cbrt, NULL, NULL},
    {"abs", 1, fabs, NULL, NULL},
    {"fabs", 1, fabs, NULL, NULL},
    {"sin", 1, kh_sin_cleaned, NULL, NULL},
    {"cos", 1, kh_cos_cleaned, NULL, NULL},
    {"tan", 1, kh_tan_cleaned, NULL, NULL},
    {"asin", 1, kh_safe_asin, NULL, NULL},
    {"acos", 1, kh_safe_acos, NULL, NULL},
    {"atan", 1, atan, NULL, NULL},
    {"sind", 1, kh_sind, NULL, NULL},
    {"cosd", 1, kh_cosd, NULL, NULL},
    {"tand", 1, kh_tand, NULL, NULL},
    {"log", 1, kh_safe_log, NULL, NULL},
    {"log10", 1, kh_safe_log10, NULL, NULL},
    {"exp", 1, exp, NULL, NULL},
    {"floor", 1, floor, NULL, NULL},
    {"ceil", 1, ceil, NULL, NULL},
    {"round", 1, round, NULL, NULL},
    {"trunc", 1, trunc, NULL, NULL},
    {"deg", 1, kh_radians_to_degrees, NULL, NULL},
    {"rad", 1, kh_degrees_to_radians, NULL, NULL},
    
    /* Two argument functions */
    {"pow", 2, NULL, pow, NULL},
    {"atan2", 2, NULL, atan2, NULL},
    {"fmod", 2, NULL, fmod, NULL},
    {"min", 2, NULL, kh_math_min, NULL},
    {"max", 2, NULL, kh_math_max, NULL},
    
    /* Three argument functions */
    {"clamp", 3, NULL, NULL, kh_math_clamp}
};

static const int KH_MATH_FUNCTION_COUNT = sizeof(KH_ALLOWED_MATH_FUNCTIONS) / sizeof(math_function_t);

/* Check safety limits to prevent crashes */
static inline int kh_check_parser_limits(parser_context_t* ctx) {
    if (!ctx) return 0;
    
    // Check for allocation failures first
    if (ctx->allocation_failed || ctx->current_token.allocation_failed) {
        kh_set_parser_error(ctx, "MEMORY ALLOCATION FAILED DURING PARSING");
        return 0;
    }
    
    if (ctx->recursion_depth >= MAX_MATH_RECURSION_DEPTH) {
        kh_set_parser_error(ctx, "MAXIMUM RECURSION DEPTH EXCEEDED");
        return 0;
    }
    
    if (ctx->operation_count >= MAX_MATH_PARSE_OPERATIONS) {
        kh_set_parser_error(ctx, "MAXIMUM OPERATIONS EXCEEDED");
        return 0;
    }
    
    return 1;
}

/* Initialize token with dynamic allocation */
static inline int kh_init_token(token_t* token) {
    if (!token) return 0;
    
    // Initialize all fields first
    memset(token, 0, sizeof(token_t));
    
    token->value = (char*)malloc(INITIAL_MATH_TOKEN_SIZE);
    if (!token->value) {
        token->allocation_failed = 1;
        return 0;
    }
    
    token->value_capacity = INITIAL_MATH_TOKEN_SIZE;
    token->value[0] = '\0';
    token->type = TOKEN_END;
    token->number = 0.0;
    token->pos = 0;
    token->allocation_failed = 0;
    return 1;
}

/* Resize token value buffer if needed */
static inline int kh_resize_token_value(token_t* token, int required_size) {
    if (!token || required_size <= 0 || token->allocation_failed) return 0;
    
    if (required_size <= token->value_capacity) return 1;
    
    int new_capacity = token->value_capacity;
    while (new_capacity < required_size) {
        new_capacity *= 2;
        // Prevent integer overflow
        if (new_capacity < token->value_capacity) {
            token->allocation_failed = 1;
            return 0;
        }
    }
    
    // Attempt realloc with fallback
    char* new_value = (char*)realloc(token->value, (size_t)new_capacity);
    if (!new_value) {
        // Don't modify original on failure
        token->allocation_failed = 1;
        return 0;
    }
    
    token->value = new_value;
    token->value_capacity = new_capacity;
    return 1;
}

/* Set token value with automatic resizing */
static inline int kh_set_token_value(token_t* token, const char* value) {
    if (!token || !value) return 0;
    
    int required_size = (int)strlen(value) + 1;
    if (!kh_resize_token_value(token, required_size)) return 0;
    
    strcpy_s(token->value, (size_t)token->value_capacity, value);
    return 1;
}

/* Set token value with automatic resizing (formatted) */
static inline int kh_set_token_value_f(token_t* token, const char* format, ...) {
    if (!token || !format) return 0;
    
    va_list args;
    va_start(args, format);
    
    /* Calculate required size */
    int required_size = _vscprintf(format, args) + 1;
    va_end(args);
    
    if (!kh_resize_token_value(token, required_size)) return 0;
    
    va_start(args, format);
    _vsnprintf_s(token->value, (size_t)token->value_capacity, _TRUNCATE, format, args);
    va_end(args);
    
    return 1;
}

/* Free token resources */
static inline void kh_free_token(token_t* token) {
    if (token && token->value) {
        free(token->value);
        token->value = NULL;
    }
}

/* Initialize parser context with dynamic allocation */
static inline int kh_init_parser_context(parser_context_t* ctx, const char* expr) {
    if (!ctx || !expr) return 0;
    
    // Initialize everything to safe defaults first
    memset(ctx, 0, sizeof(parser_context_t));
    
    ctx->expr = expr;
    ctx->length = (int)strlen(expr);
    ctx->error_msg_capacity = 512;
    
    // Allocate error message buffer
    ctx->error_msg = (char*)malloc((size_t)ctx->error_msg_capacity);
    if (!ctx->error_msg) {
        ctx->allocation_failed = 1;
        return 0;
    }
    ctx->error_msg[0] = '\0';
    
    // Initialize token with proper error checking
    if (!kh_init_token(&ctx->current_token)) {
        free(ctx->error_msg);
        ctx->error_msg = NULL;
        ctx->allocation_failed = 1;
        return 0;
    }
    
    return 1;
}

/* Set error message with automatic resizing */
static inline int kh_set_parser_error(parser_context_t* ctx, const char* format, ...) {
    if (!ctx || !format || ctx->allocation_failed) return 0;
    
    va_list args;
    va_start(args, format);
    
    int required_size = _vscprintf(format, args) + 1;
    va_end(args);
    
    // Handle allocation failure gracefully
    if (required_size > ctx->error_msg_capacity) {
        int new_capacity = ctx->error_msg_capacity;
        while (new_capacity < required_size && new_capacity > 0) {
            new_capacity *= 2;
        }
        
        if (new_capacity <= 0) { // Integer overflow check
            ctx->allocation_failed = 1;
            return 0;
        }
        
        char* new_error_msg = (char*)realloc(ctx->error_msg, (size_t)new_capacity);
        if (!new_error_msg) {
            // Fall back to existing buffer with truncation
            ctx->allocation_failed = 1;
            va_start(args, format);
            _vsnprintf_s(ctx->error_msg, (size_t)ctx->error_msg_capacity, _TRUNCATE, format, args);
            va_end(args);
            return 0;
        }
        
        ctx->error_msg = new_error_msg;
        ctx->error_msg_capacity = new_capacity;
    }
    
    va_start(args, format);
    _vsnprintf_s(ctx->error_msg, (size_t)ctx->error_msg_capacity, _TRUNCATE, format, args);
    va_end(args);
    
    return 1;
}

/* Free parser context resources */
static inline void kh_free_parser_context(parser_context_t* ctx) {
    if (ctx) {
        kh_free_token(&ctx->current_token);
        if (ctx->error_msg) {
            free(ctx->error_msg);
            ctx->error_msg = NULL;
        }
        // Reset allocation failure flag
        ctx->allocation_failed = 0;
    }
}

/* Helper functions for parsing */
static inline void kh_skip_whitespace(parser_context_t* ctx) {
    if (!ctx) return;
    while (ctx->pos < ctx->length && isspace(ctx->expr[ctx->pos])) {
        ctx->pos++;
    }
}

static int kh_read_number(parser_context_t* ctx, double* number) {
    if (!ctx || !number) return 0;
    
    int initial_capacity = 32;
    char* num_str = (char*)malloc((size_t)initial_capacity);
    if (!num_str) return 0;
    
    int num_pos = 0;
    int has_dot = 0;
    int has_e = 0;  // Track scientific notation
    int capacity = initial_capacity;
    
    while (ctx->pos < ctx->length) {
        char c = ctx->expr[ctx->pos];
        
        /* Resize buffer if needed */
        if (num_pos >= capacity - 1) {
            capacity *= 2;
            char* new_str = (char*)realloc(num_str, (size_t)capacity);
            if (!new_str) {
                free(num_str);
                return 0;
            }
            num_str = new_str;
        }
        
        if (isdigit(c)) {
            num_str[num_pos++] = c;
            ctx->pos++;
        } else if (c == '.' && !has_dot && !has_e) {
            has_dot = 1;
            num_str[num_pos++] = c;
            ctx->pos++;
        } else if ((c == 'e' || c == 'E') && !has_e && num_pos > 0) {
            /* Scientific notation support */
            has_e = 1;
            num_str[num_pos++] = c;
            ctx->pos++;
            
            /* Check for optional + or - after e/E */
            if (ctx->pos < ctx->length && (ctx->expr[ctx->pos] == '+' || ctx->expr[ctx->pos] == '-')) {
                if (num_pos >= capacity - 1) {
                    capacity *= 2;
                    char* new_str = (char*)realloc(num_str, (size_t)capacity);
                    if (!new_str) {
                        free(num_str);
                        return 0;
                    }
                    num_str = new_str;
                }
                num_str[num_pos++] = ctx->expr[ctx->pos];
                ctx->pos++;
            }
        } else {
            break;
        }
    }
    
    int result = 0;
    if (num_pos > 0) {
        num_str[num_pos] = '\0';
        *number = atof(num_str);
        result = 1;
    }
    
    free(num_str);
    return result;
}

static int kh_read_identifier(parser_context_t* ctx, char** identifier) {
    if (!ctx || !identifier) return 0;
    
    *identifier = NULL;
    
    if (ctx->pos >= ctx->length || (!isalpha(ctx->expr[ctx->pos]) && ctx->expr[ctx->pos] != '_')) {
        return 0;
    }
    
    int initial_capacity = 32;
    char* id_str = (char*)malloc((size_t)initial_capacity);
    if (!id_str) return 0;
    
    int id_pos = 0;
    int capacity = initial_capacity;
    
    while (ctx->pos < ctx->length) {
        char c = ctx->expr[ctx->pos];
        
        if (isalnum(c) || c == '_') {
            /* Resize buffer if needed */
            if (id_pos >= capacity - 1) {
                capacity *= 2;
                char* new_str = (char*)realloc(id_str, (size_t)capacity);
                if (!new_str) {
                    free(id_str);
                    return 0;
                }
                id_str = new_str;
            }
            
            id_str[id_pos++] = c;
            ctx->pos++;
        } else {
            break;
        }
    }
    
    if (id_pos > 0) {
        id_str[id_pos] = '\0';
        *identifier = id_str;
        return 1;
    }
    
    free(id_str);
    return 0;
}

static int kh_read_operator(parser_context_t* ctx, char** op) {
    if (!ctx || !op) return 0;
    
    *op = NULL;
    
    if (ctx->pos >= ctx->length) return 0;
    
    char c1 = ctx->expr[ctx->pos];
    char c2 = (ctx->pos + 1 < ctx->length) ? ctx->expr[ctx->pos + 1] : '\0';
    
    *op = (char*)malloc(3); /* Maximum operator length is 2 + null terminator */
    if (!*op) return 0;
    
    /* Two-character operators */
    if ((c1 == '=' && c2 == '=') || (c1 == '!' && c2 == '=') ||
        (c1 == '<' && c2 == '=') || (c1 == '>' && c2 == '=') ||
        (c1 == '&' && c2 == '&') || (c1 == '|' && c2 == '|')) {
        (*op)[0] = c1;
        (*op)[1] = c2;
        (*op)[2] = '\0';
        ctx->pos += 2;
        return 1;
    }
    
    /* Single-character operators */
    if (c1 == '+' || c1 == '-' || c1 == '*' || c1 == '/' || c1 == '%' ||
        c1 == '<' || c1 == '>' || c1 == '!') {
        (*op)[0] = c1;
        (*op)[1] = '\0';
        ctx->pos++;
        return 1;
    }
    
    free(*op);
    *op = NULL;
    return 0;
}

/* Find function in whitelist - SECURITY CRITICAL */
static inline const math_function_t* kh_find_math_function(const char* name) {
    if (!name) return NULL;
    
    int i;
    for (i = 0; i < KH_MATH_FUNCTION_COUNT; i++) {
        if (strcmp(KH_ALLOWED_MATH_FUNCTIONS[i].name, name) == 0) {
            return &KH_ALLOWED_MATH_FUNCTIONS[i];
        }
    }
    return NULL;
}

/* Get next token from expression - enhanced with better memory management */
static void kh_get_next_token(parser_context_t* ctx) {
    if (!ctx || !ctx->current_token.value) return;
    
    /* Check limits before proceeding */
    if (!kh_check_parser_limits(ctx)) {
        ctx->current_token.type = TOKEN_ERROR;
        return;
    }
    
    ctx->operation_count++;
    
    kh_skip_whitespace(ctx);
    
    if (ctx->pos >= ctx->length) {
        ctx->current_token.type = TOKEN_END;
        return;
    }
    
    char c = ctx->expr[ctx->pos];
    
    /* Check for parentheses and comma */
    if (c == '(') {
        ctx->current_token.type = TOKEN_LPAREN;
        if (!kh_set_token_value(&ctx->current_token, "(")) {
            ctx->current_token.type = TOKEN_ERROR;
            kh_set_parser_error(ctx, "MEMORY ALLOCATION FAILED");
            return;
        }
        ctx->pos++;
        return;
    }
    
    if (c == ')') {
        ctx->current_token.type = TOKEN_RPAREN;
        if (!kh_set_token_value(&ctx->current_token, ")")) {
            ctx->current_token.type = TOKEN_ERROR;
            kh_set_parser_error(ctx, "MEMORY ALLOCATION FAILED");
            return;
        }
        ctx->pos++;
        return;
    }
    
    if (c == ',') {
        ctx->current_token.type = TOKEN_COMMA;
        if (!kh_set_token_value(&ctx->current_token, ",")) {
            ctx->current_token.type = TOKEN_ERROR;
            kh_set_parser_error(ctx, "MEMORY ALLOCATION FAILED");
            return;
        }
        ctx->pos++;
        return;
    }
    
    /* Check for numbers */
    double number;
    int old_pos = ctx->pos;
    if (kh_read_number(ctx, &number)) {
        ctx->current_token.type = TOKEN_NUMBER;
        ctx->current_token.number = number;
        if (!kh_set_token_value_f(&ctx->current_token, "%.6g", number)) {
            ctx->current_token.type = TOKEN_ERROR;
            kh_set_parser_error(ctx, "MEMORY ALLOCATION FAILED");
            return;
        }
        return;
    }
    ctx->pos = old_pos;
    
    /* Check for function names/identifiers */
    char* identifier = NULL;
    if (kh_read_identifier(ctx, &identifier)) {
        /* Check for boolean values and reject them */
        if (strcmp(identifier, "true") == 0 || strcmp(identifier, "false") == 0) {
            ctx->current_token.type = TOKEN_ERROR;
            kh_set_parser_error(ctx, "BOOLEAN VALUE NOT SUPPORTED");
            free(identifier);
            return;
        }
        
        if (kh_find_math_function(identifier) != NULL) {
            ctx->current_token.type = TOKEN_FUNCTION;
            if (!kh_set_token_value(&ctx->current_token, identifier)) {
                ctx->current_token.type = TOKEN_ERROR;
                kh_set_parser_error(ctx, "MEMORY ALLOCATION FAILED");
            }
            free(identifier);
            return;
        } else {
            ctx->current_token.type = TOKEN_ERROR;
            kh_set_parser_error(ctx, "UNKNOWN FUNCTION '%s' AT POSITION %d", 
                     identifier, (int)(ctx->pos - strlen(identifier)));
            free(identifier);
            return;
        }
    }
    
    /* Check for operators */
    char* op = NULL;
    if (kh_read_operator(ctx, &op)) {
        ctx->current_token.type = TOKEN_OPERATOR;
        if (!kh_set_token_value(&ctx->current_token, op)) {
            ctx->current_token.type = TOKEN_ERROR;
            kh_set_parser_error(ctx, "MEMORY ALLOCATION FAILED");
        }
        free(op);
        return;
    }
    
    /* Unknown character */
    ctx->current_token.type = TOKEN_ERROR;
    kh_set_parser_error(ctx, "UNEXPECTED CHARACTER '%c' AT POSITION %d", c, ctx->pos);
}

/* Forward declarations for recursive descent parser */
static double kh_parse_expression(parser_context_t* ctx);
static double kh_parse_logical_or(parser_context_t* ctx);
static double kh_parse_logical_and(parser_context_t* ctx);
static double kh_parse_equality(parser_context_t* ctx);
static double kh_parse_relational(parser_context_t* ctx);
static double kh_parse_additive(parser_context_t* ctx);
static double kh_parse_multiplicative(parser_context_t* ctx);
static double kh_parse_unary(parser_context_t* ctx);
static double kh_parse_primary(parser_context_t* ctx);
static double kh_parse_function_call(parser_context_t* ctx, const char* func_name);

/* Parse function call with arguments - enhanced error handling */
static double kh_parse_function_call(parser_context_t* ctx, const char* func_name) {
    if (!ctx || !func_name) return 0.0;
    
    if (!kh_check_parser_limits(ctx)) return 0.0;
    
    const math_function_t* func = kh_find_math_function(func_name);
    double* args = NULL;
    int arg_count = 0;
    double result = 0.0;
    
    if (!func) {
        kh_set_parser_error(ctx, "UNKNOWN FUNCTION '%s'", func_name);
        return 0.0;
    }
    
    /* Allocate memory for arguments */
    args = (double*)malloc(MAX_MATH_FUNCTION_ARGS * sizeof(double));
    if (!args) {
        kh_set_parser_error(ctx, "MEMORY ALLOCATION FAILED");
        return 0.0;
    }
    
    /* Expect opening parenthesis */
    if (ctx->current_token.type != TOKEN_LPAREN) {
        kh_set_parser_error(ctx, "EXPECTED '(' AFTER FUNCTION '%s'", func_name);
        free(args);
        return 0.0;
    }
    
    kh_get_next_token(ctx); /* consume '(' */
    
    /* Parse arguments */
    if (ctx->current_token.type != TOKEN_RPAREN) {
        args[arg_count++] = kh_parse_expression(ctx);
        
        while (ctx->current_token.type == TOKEN_COMMA && arg_count < MAX_MATH_FUNCTION_ARGS) {
            kh_get_next_token(ctx); /* consume ',' */
            if (arg_count < MAX_MATH_FUNCTION_ARGS) {
                args[arg_count++] = kh_parse_expression(ctx);
            }
        }
    }
    
    /* Expect closing parenthesis */
    if (ctx->current_token.type != TOKEN_RPAREN) {
        kh_set_parser_error(ctx, "EXPECTED ')' IN FUNCTION CALL '%s'", func_name);
        free(args);
        return 0.0;
    }
    
    kh_get_next_token(ctx); /* consume ')' */
    
    /* Check argument count */
    if (arg_count != func->arg_count) {
        kh_set_parser_error(ctx, "FUNCTION '%s' EXPECTS %d ARGUMENTS, GOT %d", 
                 func_name, func->arg_count, arg_count);
        free(args);
        return 0.0;
    }
    
    /* Call the appropriate function */
    switch (func->arg_count) {
        case 1: result = func->func_ptr_1(args[0]); break;
        case 2: result = func->func_ptr_2(args[0], args[1]); break;
        case 3: result = func->func_ptr_3(args[0], args[1], args[2]); break;
        default:
            kh_set_parser_error(ctx, "INVALID ARGUMENT COUNT FOR FUNCTION '%s'", func_name);
            break;
    }
    
    free(args);
    return result;
}

/* Parse primary expressions (numbers, parentheses, function calls) */
static double kh_parse_primary(parser_context_t* ctx) {
    if (!ctx) return 0.0;
    
    if (!kh_check_parser_limits(ctx)) return 0.0;
    
    if (ctx->current_token.type == TOKEN_NUMBER) {
        double value = ctx->current_token.number;
        kh_get_next_token(ctx);
        return value;
    }
    
    if (ctx->current_token.type == TOKEN_FUNCTION) {
        char* func_name = (char*)malloc(strlen(ctx->current_token.value) + 1);
        if (!func_name) {
            _snprintf_s(ctx->error_msg, 512, _TRUNCATE, "MEMORY ALLOCATION FAILED");
            return 0.0;
        }
        
        strcpy_s(func_name, strlen(ctx->current_token.value) + 1, ctx->current_token.value);
        kh_get_next_token(ctx);
        double result = kh_parse_function_call(ctx, func_name);
        free(func_name);
        return result;
    }
    
    if (ctx->current_token.type == TOKEN_LPAREN) {
        kh_get_next_token(ctx); /* consume '(' */
        double value = kh_parse_expression(ctx);
        
        if (ctx->current_token.type != TOKEN_RPAREN) {
            _snprintf_s(ctx->error_msg, 512, _TRUNCATE, "EXPECTED ')' AT POSITION %d", ctx->pos);
            return 0.0;
        }
        
        kh_get_next_token(ctx); /* consume ')' */
        return value;
    }
    
    _snprintf_s(ctx->error_msg, 512, _TRUNCATE, "EXPECTED NUMBER, FUNCTION, OR '(' AT POSITION %d", ctx->pos);
    return 0.0;
}

/* Parse unary expressions (!, -, +) */
static double kh_parse_unary(parser_context_t* ctx) {
    if (!ctx) return 0.0;
    
    if (!kh_check_parser_limits(ctx)) return 0.0;
    ctx->recursion_depth++;
    
    double result = 0.0;
    
    if (ctx->current_token.type == TOKEN_OPERATOR) {
        if (strcmp(ctx->current_token.value, "-") == 0) {
            kh_get_next_token(ctx);
            result = -kh_parse_unary(ctx);
        } else if (strcmp(ctx->current_token.value, "+") == 0) {
            kh_get_next_token(ctx);
            result = kh_parse_unary(ctx);
        } else if (strcmp(ctx->current_token.value, "!") == 0) {
            kh_get_next_token(ctx);
            double value = kh_parse_unary(ctx);
            result = (value == 0.0) ? 1.0 : 0.0;
        } else {
            result = kh_parse_primary(ctx);
        }
    } else {
        result = kh_parse_primary(ctx);
    }
    
    ctx->recursion_depth--;
    return result;
}

/* Parse multiplicative expressions (*, /, %) */
static double kh_parse_multiplicative(parser_context_t* ctx) {
    if (!ctx) return 0.0;
    
    if (!kh_check_parser_limits(ctx)) return 0.0;
    ctx->recursion_depth++;
    
    double left = kh_parse_unary(ctx);
    
    while (ctx->current_token.type == TOKEN_OPERATOR) {
        char op = ctx->current_token.value[0];
        
        if (op == '*') {
            kh_get_next_token(ctx);
            left *= kh_parse_unary(ctx);
        } else if (op == '/') {
            kh_get_next_token(ctx);
            double right = kh_parse_unary(ctx);
            if (right == 0.0) {
                _snprintf_s(ctx->error_msg, 512, _TRUNCATE, "DIVISION BY ZERO");
                ctx->recursion_depth--;
                return 0.0;
            }
            left /= right;
        } else if (op == '%') {
            kh_get_next_token(ctx);
            double right = kh_parse_unary(ctx);
            if (right == 0.0) {
                _snprintf_s(ctx->error_msg, 512, _TRUNCATE, "MODULO BY ZERO");
                ctx->recursion_depth--;
                return 0.0;
            }
            left = fmod(left, right);
        } else {
            break;
        }
    }
    
    ctx->recursion_depth--;
    return left;
}

/* Parse additive expressions (+, -) */
static double kh_parse_additive(parser_context_t* ctx) {
    if (!ctx) return 0.0;
    
    if (!kh_check_parser_limits(ctx)) return 0.0;
    ctx->recursion_depth++;
    
    double left = kh_parse_multiplicative(ctx);
    
    while (ctx->current_token.type == TOKEN_OPERATOR) {
        char op = ctx->current_token.value[0];
        
        if (op == '+') {
            kh_get_next_token(ctx);
            left += kh_parse_multiplicative(ctx);
        } else if (op == '-') {
            kh_get_next_token(ctx);
            left -= kh_parse_multiplicative(ctx);
        } else {
            break;
        }
    }
    
    ctx->recursion_depth--;
    return left;
}

/* Parse relational expressions (<, >, <=, >=) */
static double kh_parse_relational(parser_context_t* ctx) {
    if (!ctx) return 0.0;
    
    if (!kh_check_parser_limits(ctx)) return 0.0;
    ctx->recursion_depth++;
    
    double left = kh_parse_additive(ctx);
    
    while (ctx->current_token.type == TOKEN_OPERATOR) {
        if (strcmp(ctx->current_token.value, "<") == 0) {
            kh_get_next_token(ctx);
            left = (left < kh_parse_additive(ctx)) ? 1.0 : 0.0;
        } else if (strcmp(ctx->current_token.value, ">") == 0) {
            kh_get_next_token(ctx);
            left = (left > kh_parse_additive(ctx)) ? 1.0 : 0.0;
        } else if (strcmp(ctx->current_token.value, "<=") == 0) {
            kh_get_next_token(ctx);
            left = (left <= kh_parse_additive(ctx)) ? 1.0 : 0.0;
        } else if (strcmp(ctx->current_token.value, ">=") == 0) {
            kh_get_next_token(ctx);
            left = (left >= kh_parse_additive(ctx)) ? 1.0 : 0.0;
        } else {
            break;
        }
    }
    
    ctx->recursion_depth--;
    return left;
}

/* Parse equality expressions (==, !=) */
static double kh_parse_equality(parser_context_t* ctx) {
    if (!ctx) return 0.0;
    
    if (!kh_check_parser_limits(ctx)) return 0.0;
    ctx->recursion_depth++;
    
    double left = kh_parse_relational(ctx);
    
    while (ctx->current_token.type == TOKEN_OPERATOR) {
        if (strcmp(ctx->current_token.value, "==") == 0) {
            kh_get_next_token(ctx);
            double right = kh_parse_relational(ctx);
            left = (fabs(left - right) < 1e-9) ? 1.0 : 0.0;
        } else if (strcmp(ctx->current_token.value, "!=") == 0) {
            kh_get_next_token(ctx);
            double right = kh_parse_relational(ctx);
            left = (fabs(left - right) >= 1e-9) ? 1.0 : 0.0;
        } else {
            break;
        }
    }
    
    ctx->recursion_depth--;
    return left;
}

/* Parse logical AND expressions (&&) */
static double kh_parse_logical_and(parser_context_t* ctx) {
    if (!ctx) return 0.0;
    
    if (!kh_check_parser_limits(ctx)) return 0.0;
    ctx->recursion_depth++;
    
    double left = kh_parse_equality(ctx);
    
    while (ctx->current_token.type == TOKEN_OPERATOR && 
           strcmp(ctx->current_token.value, "&&") == 0) {
        kh_get_next_token(ctx);
        double right = kh_parse_equality(ctx);
        left = (left != 0.0 && right != 0.0) ? 1.0 : 0.0;
    }
    
    ctx->recursion_depth--;
    return left;
}

/* Parse logical OR expressions (||) */
static double kh_parse_logical_or(parser_context_t* ctx) {
    if (!ctx) return 0.0;
    
    if (!kh_check_parser_limits(ctx)) return 0.0;
    ctx->recursion_depth++;
    
    double left = kh_parse_logical_and(ctx);
    
    while (ctx->current_token.type == TOKEN_OPERATOR && 
           strcmp(ctx->current_token.value, "||") == 0) {
        kh_get_next_token(ctx);
        double right = kh_parse_logical_and(ctx);
        left = (left != 0.0 || right != 0.0) ? 1.0 : 0.0;
    }
    
    ctx->recursion_depth--;
    return left;
}

/* Parse full expression */
static inline double kh_parse_expression(parser_context_t* ctx) {
    if (!ctx) return 0.0;
    return kh_parse_logical_or(ctx);
}

/* Check if expression contains boolean operators */
static inline int kh_is_boolean_expression(const char* expr) {
    if (!expr) return 0;
    return (strstr(expr, "==") || strstr(expr, "!=") || 
            strstr(expr, "<=") || strstr(expr, ">=") ||
            strstr(expr, "<") || strstr(expr, ">") ||
            strstr(expr, "&&") || strstr(expr, "||") ||
            (expr[0] == '!' && expr[1] != '='));
}

/* Main math operation processing function - enhanced with better memory management */
static int kh_process_math_operation(char* output, int output_size, const char** argv, int argc) {
    if (!output || output_size <= 0 || !argv || argc < 1 || !argv[0]) {
        if (output && output_size > 0) {
            kh_set_error(output, output_size, "INVALID PARAMETERS");
        }
        return 1;
    }
    
    parser_context_t ctx;
    char* clean_expr = NULL;
    double result;
    int ret = 1;
    
    /* Allocate memory for clean expression */
    clean_expr = (char*)malloc(strlen(argv[0]) + 1);
    if (!clean_expr) {
        kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
        return 1;
    }
    
    kh_clean_string(argv[0], clean_expr, (int)strlen(argv[0]) + 1);
    
    if (strlen(clean_expr) == 0) {
        kh_set_error(output, output_size, "EMPTY EXPRESSION");
        free(clean_expr);
        return 1;
    }
    
    /* Initialize parser context */
    if (!kh_init_parser_context(&ctx, clean_expr)) {
        kh_set_error(output, output_size, "FAILED TO INITIALIZE PARSER");
        free(clean_expr);
        return 1;
    }
    
    /* Parse expression */
    kh_get_next_token(&ctx);
    
    if (ctx.current_token.type == TOKEN_ERROR) {
        kh_set_error(output, output_size, ctx.error_msg);
        goto cleanup;
    }
    
    result = kh_parse_expression(&ctx);
    
    if (strlen(ctx.error_msg) > 0) {
        kh_set_error(output, output_size, ctx.error_msg);
        goto cleanup;
    }
    
    if (ctx.current_token.type != TOKEN_END) {
        kh_set_error(output, output_size, "UNEXPECTED TOKEN AT END OF EXPRESSION");
        goto cleanup;
    }
    
    /* Format result */
    if (result == 0.0 && kh_is_boolean_expression(clean_expr)) {
        strcpy_s(output, (size_t)output_size, "false");
    } else if (result == 1.0 && kh_is_boolean_expression(clean_expr)) {
        strcpy_s(output, (size_t)output_size, "true");
    } else {
        _snprintf_s(output, (size_t)output_size, _TRUNCATE, "%.6g", result);
    }
    
    ret = 0;

cleanup:
    kh_free_parser_context(&ctx);
    free(clean_expr);
    return ret;
}

#endif /* MATH_OPERATIONS_HPP */