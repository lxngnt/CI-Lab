/**************************************************************************
 * C S 429 EEL interpreter
 * 
 * parse.c - This file contains the skeleton of functions to be implemented by
 * you. When completed, it will contain the code used to parse an expression and
 * create an AST.
 * 
 * Copyright (c) 2021. S. Chatterjee, X. Shen, T. Byrd. All rights reserved.
 * May not be used, modified, or copied without permission.
 **************************************************************************/ 

#include "ci.h"

/* Explained in ci.h */
extern lptr_t this_token, next_token;
extern void init_lexer(void);
extern void advance_lexer(void);

/* Valid format specifers */
static const char *VALID_FMTS = "dxXbB";

/* The only reserved identifiers are "true" and "false" */
static const int NUM_RESERVED_IDS = 2;
static const struct {
    char *id;
    token_t t;
} reserved_ids[] = {
    {"true", TOK_TRUE},
    {"false", TOK_FALSE}
};

/* is_binop() - return true if a token represents a binary operator
 * Parameter: Any token
 * Return value: true if t is a binary operator, false otherwise */
bool is_binop(token_t t) {
    return t >= TOK_PLUS && t <= TOK_EQ;
}

/* is_unop() - return true if a token represents a unary operator
 * Parameter: Any token
 * Return value: true if t is a unary operator, false otherwise */
bool is_unop(token_t t) {
    return t >= TOK_UMINUS && t <= TOK_NOT;
}

/* id_is_fmt_spec() - return true if a string is a format specifier
 * Parameter: Any string
 * Return value: true if s is format specifier, false otherwise */
bool id_is_fmt_spec(char *s) {
    return strlen(s) == 1 && strspn(s, VALID_FMTS) == 1;
}

/* check_reserved_ids() - check if a given string is a reserved identifier
 * Parameter: Any string
 * Return value: true if s is a reserved identifier, false otherwise */
static token_t check_reserved_ids(char *s) {
    for (int i = 0; i < NUM_RESERVED_IDS; i++)
        if (strcmp(reserved_ids[i].id, s) == 0) return reserved_ids[i].t;
    return TOK_INVALID;
}

/* build_leaf() - create a leaf node based on this_token and / or next_token
 * Parameter: none
 * Return value: pointer to a leaf node
 * (STUDENT TODO) */
//Self note: Do this first
static node_t *build_leaf(void) {
    //Make empty node
    //memory leaks detected
    nptr_t newNode = (node_t*) calloc(1, sizeof(node_t));
    if (! newNode) {
        // calloc returns NULL if memory allocation fails
        logging(LOG_FATAL, "failed to allocate node");
        return NULL;
    }

    //Set represented input token using the given lexeme, (INT, BOOL, STRING)
    newNode->tok = this_token->ttype;
    //Internal, leaf, root
    newNode->node_type = NT_LEAF;

    //Find the EEL type and set the value
    if(this_token->ttype == TOK_NUM) { 
        newNode->type = INT_TYPE;
        newNode->val.ival = atoi(this_token->repr); //Convert char array to int
    }
    else if(this_token->ttype == TOK_TRUE) {
        newNode->type = BOOL_TYPE;
        newNode->val.bval = true;
    }
    else if(this_token->ttype == TOK_FALSE) {
        newNode->type = BOOL_TYPE;
        newNode->val.bval = false;
    }
    else if(this_token->ttype == TOK_STR) {
        newNode->type = STRING_TYPE;
        //malloc a string and set the sval pointer to it in the node
        newNode->val.sval = calloc(1, sizeof(this_token->repr));
        strcpy(this_token->repr, newNode->val.sval);
    }
    else if(this_token->ttype == TOK_FMT_SPEC) {
        newNode->type = FMT_TYPE;
        newNode->val.fval = this_token->repr[0];
    }
    
    //Allocates memory to array of pointers to children?
        
    return newNode;
}


/* build_exp() - parse an expression based on this_token and / or next_token
 * Make calls to build_leaf() or build_exp() if necessary. 
 * Parameter: none
 * Return value: pointer to an internal node
 * (STUDENT TODO) */
//Self note: Do this second, is a recursive function
static node_t *build_exp(void) {
    // check running status
    if (terminate || ignore_input) return NULL;
    
    token_t t;

    // The case of a leaf node is handled for you
    if (this_token->ttype == TOK_NUM)
        return build_leaf();
    if (this_token->ttype == TOK_STR)
        return build_leaf();
    // handle the reserved identifiers, namely true and false
    if (this_token->ttype == TOK_ID) {
        if ((t = check_reserved_ids(this_token->repr)) != TOK_INVALID) {
            this_token->ttype = t;
        }
        return build_leaf();
    } else {
        // (STUDENT TODO) implement the logic for internal nodes
        //Make new node, set type to internal
            node_t *intNode = (node_t*) calloc(1, sizeof(node_t)); 
                if (! intNode) {
                // calloc returns NULL if memory allocation fails
                logging(LOG_FATAL, "failed to allocate node");
                return NULL;
                }
            intNode->node_type = NT_INTERNAL;
            //Logic when encountering a left parenthesis
            if(this_token->ttype == TOK_LPAREN) {
                advance_lexer();
                //left subtree
                intNode->children[0] = build_exp();
                    if(is_binop(next_token->ttype)) {
                        intNode -> tok = next_token->ttype; //set value
                        advance_lexer();
                        advance_lexer();
                }
            //right subtree
            intNode->children[1] = build_exp();
            advance_lexer();
            //throw error for rparenth
            if(this_token ->ttype != TOK_RPAREN) {
                handle_error(ERR_SYNTAX);
                return NULL;
            }
        }
        return intNode;
    }
} 
        

/* build_root() - construct the root of the AST for the current input
 * This function is provided to you. Use it as a reference for your code
 * Parameter: none
 * Return value: the root of the AST */
static node_t *build_root(void) {
    // check running status
    if (terminate || ignore_input) return NULL;

    // allocate memory for the root node
    node_t *ret = calloc(1, sizeof(node_t));
    if (! ret) {
        // calloc returns NULL if memory allocation fails
        logging(LOG_FATAL, "failed to allocate node");
        return NULL;
    }

    // set the node struct's fields
    ret->node_type = NT_ROOT;
    ret->type = NO_TYPE;

    // (EEL-2) check for variable assignment
    if (this_token->ttype == TOK_ID && next_token->ttype == TOK_ASSIGN) {
        if (check_reserved_ids(this_token->repr) != TOK_INVALID) {
            logging(LOG_ERROR, "variable name is reserved");
            return ret;
        }
        ret->type = ID_TYPE;
        ret->children[0] = build_leaf();
        advance_lexer();
        advance_lexer();
        ret->children[1] = build_exp();
        if (next_token->ttype != TOK_EOL) {
            handle_error(ERR_SYNTAX);
        }
        return ret;
    }
    
    // build an expression based on the current token
    // this will be where the majority of the tree is recursively constructed
    ret->children[0] = build_exp();

    // if the next token is End of Line, we're done
    if (next_token->ttype == TOK_EOL)
        return ret;
    else {                                     
    /* At this point, we've finished building the main expression. The only
     * syntactically valid tokens that could remain would be format specifiers */    
        
        // check that our next token is a format specifier
        if (next_token->ttype != TOK_SEP) {
            handle_error(ERR_SYNTAX);
            return ret;
        }

        advance_lexer();
        
        // check that there is an ID following the format specifier
        if (next_token->ttype != TOK_ID) {
            handle_error(ERR_SYNTAX);
            return ret;
        }

        // check that the ID is a format specifier ID
        if (id_is_fmt_spec(next_token->repr))
            next_token->ttype = TOK_FMT_SPEC;
        if (next_token->ttype != TOK_FMT_SPEC) {
            handle_error(ERR_SYNTAX);
            return ret;
        }

        advance_lexer();

        // build the leaf for the format specifier. 
        // if any tokens besides EOL remain, the syntax is not valid
        ret->children[1] = build_leaf();
        if (next_token->ttype != TOK_EOL) {
            handle_error(ERR_SYNTAX);
            return ret;
        }
        return ret;
    }

    // this return statement will only be reached if there was a syntax error
    handle_error(ERR_SYNTAX);
    return ret;
}

/* read_and_parse - return the root of an AST representing the current input
 * Parameter: none
 * Return value: the root of the AST */
node_t *read_and_parse(void) {
    init_lexer();
    return build_root();
}

/* cleanup() - given the root of an AST, free all associated memory
 * Parameter: The root of an AST
 * Return value: none
 * (STUDENT TODO) */
//Self note: Do this last
void cleanup(node_t *nptr) {
    // Is it enough to free the node the function called upon?
    free(nptr);
    return;
    //reminder: if the node is a string, need to free the char array
}
