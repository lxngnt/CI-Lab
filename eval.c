/**************************************************************************
 * C S 429 EEL interpreter
 * 
 * eval.c - This file contains the skeleton of functions to be implemented by
 * you. When completed, it will contain the code used to evaluate an expression
 * based on its AST.
 * 
 * Copyright (c) 2021. S. Chatterjee, X. Shen, T. Byrd. All rights reserved.
 * May not be used, modified, or copied without permission.
 **************************************************************************/ 

#include "ci.h"

extern bool is_binop(token_t);
extern bool is_unop(token_t);
char *strrev(char *str);

/* infer_type() - set the type of a non-root node based on the types of children
 * Parameter: A node pointer, possibly NULL.
 * Return value: None.
 * Side effect: The type field of the node is updated.
 * (STUDENT TODO)
 */

static void infer_type(node_t *nptr) {
    if(nptr == NULL) {
        return;
    }
    if(nptr->node_type == NT_LEAF) {
        return;
    }
    if(nptr->tok == TOK_UMINUS || nptr->tok == TOK_NOT) { //need to infer the child node, throw an error if not compatible
        return;
    }
    if(nptr->tok == TOK_STR) {
        return;
    }

    //infer the left
    infer_type(nptr->children[0]);
    //infer the right
    infer_type(nptr->children[1]);
    //if both compatible, set the value of this node to their value
    if(nptr->children[0]->type == nptr->children[1]->type) {
        nptr->type = nptr->children[0]->type;
    }
    //singular case for string * number, try to generalize
    else if (nptr->children[0]->type == STRING_TYPE && nptr->children[1]->type == INT_TYPE) {
        nptr->type = nptr->children[0]->type;
    }
    
    else {
        handle_error(ERR_TYPE);
    }

}


/* infer_root() - set the type of the root node based on the types of children
 * Parameter: A pointer to a root node, possibly NULL.
 * Return value: None.
 * Side effect: The type field of the node is updated. 
 */

static void infer_root(node_t *nptr) {
    if (nptr == NULL) return;
    // check running status
    if (terminate || ignore_input) return;
    // check for assignment
    if (nptr->type == ID_TYPE) {
        infer_type(nptr->children[1]);
    } else {
        for (int i = 0; i < 3; ++i) {
            infer_type(nptr->children[i]);
        }
        if (nptr->children[0] == NULL) {
            logging(LOG_ERROR, "failed to find child node");
            return;
        }
        nptr->type = nptr->children[0]->type;
    }
    return;
}

/* eval_node() - set the value of a non-root node based on the values of children
 * Parameter: A node pointer, possibly NULL.
 * Return value: None.
 * Side effect: The val field of the node is updated.
 * (STUDENT TODO) 
 */

static void eval_node(node_t *nptr) {
    if(nptr == NULL) {
        return;
    }
    if(nptr->node_type == NT_LEAF) {
        return;
    }

    //eval the left
    eval_node(nptr->children[0]);
    //eval the right
    eval_node(nptr->children[1]);
    //if both compatible, eval this node
    switch(nptr->tok) {
        case(TOK_PLUS):
        //TODO
        if(nptr->children[0]->type != nptr->children[1]->type) {
            handle_error(ERR_TYPE);
        }
        if(nptr->type == INT_TYPE) {
            nptr->val.ival = nptr->children[0]->val.ival + nptr->children[1]->val.ival; 
        }
        if(nptr->type == STRING_TYPE) { //EDGE CASE: string length overflow
            nptr->val.sval = calloc(1, strlen(nptr->children[0]->val.sval) + strlen(nptr->children[1]->val.sval));
            strcat(nptr->children[0]->val.sval, nptr->children[1]->val.sval); 
            strcpy(nptr->val.sval, nptr->children[0]->val.sval);
        }
        break;

        case(TOK_BMINUS):
        nptr->val.ival = nptr->children[0]->val.ival - nptr->children[1]->val.ival; 
        break;

        case(TOK_TIMES):
        if(nptr->type == INT_TYPE) {
            nptr->val.ival = nptr->children[0]->val.ival * nptr->children[1]->val.ival; 
        }
        if(nptr->type == STRING_TYPE) { //EDGE CASE: string and number are switched positions
            nptr->val.sval = calloc(1, strlen(nptr->children[0]->val.sval) * nptr->children[1]->val.ival);
            for(int i = 0; i < nptr->children[1]->val.ival; i++) {
                strcat(nptr->val.sval, nptr->children[0]->val.sval); 
            }
        }
        break;

        case(TOK_DIV): //int division, division by zero fails
        if(nptr->children[1]->val.ival == 0) {
            handle_error(ERR_EVAL);
        }
        else {
            nptr->val.ival = nptr->children[0]->val.ival / nptr->children[1]->val.ival; 
        }
        break;

        case(TOK_MOD):
               if(nptr->children[1]->val.ival == 0) {
            handle_error(ERR_EVAL);
        }
        else {
            nptr->val.ival = nptr->children[0]->val.ival % nptr->children[1]->val.ival; 
        }
        break;

        case(TOK_STR): //handle cases: adding strings, multiplying strings by int, negating string, 
        //prone to memory errors probably
        nptr->val.sval = calloc(1, sizeof(this_token->repr));
        strcpy(nptr->val.sval, nptr->children[0]->val.sval);
        //strcat(nptr->val.sval, nptr->children[1]->val.sva l);
        break;

        case(TOK_AND):
        if(nptr->children[0]->type != nptr->children[1]->type) {
            handle_error(ERR_TYPE);
        }
        nptr->val.bval = nptr->children[0]->val.bval & nptr->children[1]->val.bval;
        break;

        case(TOK_OR):
        nptr->val.bval = nptr->children[0]->val.bval | nptr->children[1]->val.bval;
        break;

        //TODO: LT and GT type inference hard-coded, else portions may not be necessary either
        case(TOK_LT):
        if(nptr->children[0]->type != INT_TYPE || nptr->children[1]->type != INT_TYPE) {
            handle_error(ERR_TYPE);
        }
        else {
            nptr->val.bval = nptr->children[0]->val.bval < nptr->children[1]->val.bval;
        }
        break;

        case(TOK_GT):
        if(nptr->children[0]->type != INT_TYPE || nptr->children[1]->type != INT_TYPE) {
            handle_error(ERR_TYPE);
        }
        else {
            nptr->val.bval = nptr->children[0]->val.bval > nptr->children[1]->val.bval;
        }
        break;

        case(TOK_EQ):
                if(nptr->children[0]->type != INT_TYPE) {
            handle_error(ERR_TYPE);
        }
        nptr->val.bval = nptr->children[0]->val.bval = nptr->children[1]->val.bval;
        break;

        case(TOK_UMINUS):
        if(nptr->children[0]->type != INT_TYPE) {
            handle_error(ERR_TYPE);
        }
        else {
            nptr->val.ival = nptr->children[0]->val.ival * -1;
        }
        break;


        //bitwise & boolean op
        case(TOK_NOT):
        if(nptr->children[0]->type == BOOL_TYPE) {
            nptr->val.bval = !nptr->children[0]->val.bval;
        }
        else {
            handle_error(ERR_TYPE);
        }
        break;


        
        default:
        break;
    }
    //if not, throw an error
    
    return;
}

/* eval_root() - set the value of the root node based on the values of children 
 * Parameter: A pointer to a root node, possibly NULL.
 * Return value: None.
 * Side effect: The val dield of the node is updated. 
 */

void eval_root(node_t *nptr) {
    if (nptr == NULL) return;
    // check running status
    if (terminate || ignore_input) return;

    // check for assignment
    if (nptr->type == ID_TYPE) {
        eval_node(nptr->children[1]);
        if (terminate || ignore_input) return;
        
        if (nptr->children[0] == NULL) {
            logging(LOG_ERROR, "failed to find child node");
            return;
        }
        put(nptr->children[0]->val.sval, nptr->children[1]);
        return;
    }

    for (int i = 0; i < 2; ++i) {
        eval_node(nptr->children[i]);
    }
    if (terminate || ignore_input) return;
    
    if (nptr->type == STRING_TYPE) {
        (nptr->val).sval = (char *) malloc(strlen(nptr->children[0]->val.sval) + 1);
        if (! nptr->val.sval) {
            logging(LOG_FATAL, "failed to allocate string");
            return;
        }
        strcpy(nptr->val.sval, nptr->children[0]->val.sval);
    } else {
        nptr->val.ival = nptr->children[0]->val.ival;
    }
    return;
}

/* infer_and_eval() - wrapper for calling infer() and eval() 
 * Parameter: A pointer to a root node.
 * Return value: none.
 * Side effect: The type and val fields of the node are updated. 
 */

void infer_and_eval(node_t *nptr) {
    infer_root(nptr);
    eval_root(nptr);
    return;
}

/* strrev() - helper function to reverse a given string 
 * Parameter: The string to reverse.
 * Return value: The reversed string. The input string is not modified.
 * (STUDENT TODO)
 */

char *strrev(char *str) {
    return NULL;
}