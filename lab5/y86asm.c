#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "y86asm.h"

line_t *y86bin_listhead = NULL;   /* the head of y86 binary code line list*/
line_t *y86bin_listtail = NULL;   /* the tail of y86 binary code line list*/
int y86asm_lineno = 0; /* the current line number of y86 assemble code */

#define err_print(_s, _a ...) do { \
  if (y86asm_lineno < 0) \
    fprintf(stderr, "[--]: "_s"\n", ## _a); \
  else \
    fprintf(stderr, "[L%d]: "_s"\n", y86asm_lineno, ## _a); \
} while (0);

int vmaddr = 0;    /* vm addr */

/* register table */
reg_t reg_table[REG_CNT] = {
    {"%eax", REG_EAX},
    {"%ecx", REG_ECX},
    {"%edx", REG_EDX},
    {"%ebx", REG_EBX},
    {"%esp", REG_ESP},
    {"%ebp", REG_EBP},
    {"%esi", REG_ESI},
    {"%edi", REG_EDI},
};

regid_t find_register(char *name)
{
    int i = 0;
    reg_t *REG;
    /* Error Note: don't put "int i" in "for()" in C code */
    while (i < REG_CNT) 
    {
        REG = &reg_table[i];
        /* Error Note: use strcmp() to cmp two char* in C code */
        if (strncmp(name, REG->name, 4) == 0)
        {
            return REG->id;
        }

        i++;
    }

    return REG_ERR;
}

/* instruction set */
int INSTR_CNT = 32;
instr_t instr_set[] = {
    {"nop", 3,   HPACK(I_NOP, F_NONE), 1 },
    {"halt", 4,  HPACK(I_HALT, F_NONE), 1 },
    {"rrmovl", 6,HPACK(I_RRMOVL, F_NONE), 2 },
    {"cmovle", 6,HPACK(I_RRMOVL, C_LE), 2 },
    {"cmovl", 5, HPACK(I_RRMOVL, C_L), 2 },
    {"cmove", 5, HPACK(I_RRMOVL, C_E), 2 },
    {"cmovne", 6,HPACK(I_RRMOVL, C_NE), 2 },
    {"cmovge", 6,HPACK(I_RRMOVL, C_GE), 2 },
    {"cmovg", 5, HPACK(I_RRMOVL, C_G), 2 },
    {"irmovl", 6,HPACK(I_IRMOVL, F_NONE), 6 },
    {"rmmovl", 6,HPACK(I_RMMOVL, F_NONE), 6 },
    {"mrmovl", 6,HPACK(I_MRMOVL, F_NONE), 6 },
    {"addl", 4,  HPACK(I_ALU, A_ADD), 2 },
    {"subl", 4,  HPACK(I_ALU, A_SUB), 2 },
    {"andl", 4,  HPACK(I_ALU, A_AND), 2 },
    {"xorl", 4,  HPACK(I_ALU, A_XOR), 2 },
    {"jmp", 3,   HPACK(I_JMP, C_YES), 5 },
    {"jle", 3,   HPACK(I_JMP, C_LE), 5 },
    {"jl", 2,    HPACK(I_JMP, C_L), 5 },
    {"je", 2,    HPACK(I_JMP, C_E), 5 },
    {"jne", 3,   HPACK(I_JMP, C_NE), 5 },
    {"jge", 3,   HPACK(I_JMP, C_GE), 5 },
    {"jg", 2,    HPACK(I_JMP, C_G), 5 },
    {"call", 4,  HPACK(I_CALL, F_NONE), 5 },
    {"ret", 3,   HPACK(I_RET, F_NONE), 1 },
    {"pushl", 5, HPACK(I_PUSHL, F_NONE), 2 },
    {"popl", 4,  HPACK(I_POPL, F_NONE),  2 },
    {".byte", 5, HPACK(I_DIRECTIVE, D_DATA), 1 },
    {".word", 5, HPACK(I_DIRECTIVE, D_DATA), 2 },
    {".long", 5, HPACK(I_DIRECTIVE, D_DATA), 4 },
    {".pos", 4,  HPACK(I_DIRECTIVE, D_POS), 0 },
    {".align", 6,HPACK(I_DIRECTIVE, D_ALIGN), 0 },
    {NULL, 1,    0   , 0 } //end
};

/* Error Note: its return is (instr_t*) , not (instr) */
instr_t *find_instr(char *name)
{
    int i = 0;
    instr_t *INSTR;
    while (i < INSTR_CNT)
    {
        INSTR = &instr_set[i];
        if (strncmp(name, INSTR->name, INSTR->len) == 0)
        {
            return INSTR;
        }

        i++;
    }
    /* return end helps code easier */
    return (&instr_set[INSTR_CNT]);
}

/* symbol table (don't forget to init and finit it) */
symbol_t *symtab = NULL;

/*
 * find_symbol: scan table to find the symbol
 * args
 *     name: the name of symbol
 *
 * return
 *     symbol_t: the 'name' symbol
 *     NULL: not exist
 */
symbol_t *find_symbol(char *name)
{
    /* Error Note: symtab itself just records symhead , don't put anything in it */
    symbol_t *p = symtab->next;
    while (p != NULL) 
    {
        if (strcmp(name, p->name) == 0)
        {
            return p;
        }

        p = p->next;
    }
    return NULL;
}

/*
 * add_symbol: add a new symbol to the symbol table
 * args
 *     name: the name of symbol
 *
 * return
 *     0: success
 *     -1: error, the symbol has exist
 */
int add_symbol(char *name)
{    
    /* check duplicate */
    if (find_symbol(name) != NULL)
    {
        err_print("Dup symbol:%s", name);
        return -1;
    }
    /* create new symbol_t (don't forget to free it)*/
    symbol_t *p = (symbol_t *)malloc(sizeof(symbol_t));
    /* Error Note: strlen don't calculate '\0' */
    int name_size = strlen(name) + 1;
    /* Error Note: p->(char *) should malloc, too */
    p->name = (char *)malloc(name_size);
    strcpy(p->name, name);

    p->addr = vmaddr;

    p->next = NULL;
    /* add the new symbol_t to symbol table */
    symbol_t *tail = symtab;
    while (tail->next != NULL) 
    {
        tail = tail->next;
    }
    tail->next = p;
    return 0;
}

/* relocation table (don't forget to init and finit it) */
reloc_t *reltab = NULL;

/*
 * add_reloc: add a new relocation to the relocation table
 * args
 *     name: the name of symbol
 *     bin: the bin to reloc
 *
 * return
 *     None
 */
void add_reloc(char *name, bin_t *bin)
{
    /* create new reloc_t (don't forget to free it)*/
    reloc_t *p = (reloc_t *)malloc(sizeof(reloc_t));
    p->y86bin = bin;

    int name_size = strlen(name) + 1;
    p->name = (char *)malloc(name_size);
    strcpy(p->name, name);

    p->next = NULL;
    /* add the new reloc_t to relocation table */
    reloc_t *tail = reltab;
    while (tail->next != NULL)
    {
        tail = tail->next;
    }
    tail->next = p;
}


/* macro for parsing y86 assembly code */
#define IS_DIGIT(s) ((*(s)>='0' && *(s)<='9') || *(s)=='-' || *(s)=='+')
#define IS_LETTER(s) ((*(s)>='a' && *(s)<='z') || (*(s)>='A' && *(s)<='Z'))
#define IS_COMMENT(s) (*(s)=='#')
#define IS_REG(s) (*(s)=='%')
#define IS_IMM(s) (*(s)=='$')

#define IS_BLANK(s) (*(s)==' ' || *(s)=='\t')
#define IS_END(s) (*(s)=='\0')

#define SKIP_BLANK(s) do {  \
  while(!IS_END(s) && IS_BLANK(s))  \
    (s)++;    \
} while(0);

/* return value from different parse_xxx function */
typedef enum { PARSE_ERR=-1, PARSE_REG, PARSE_DIGIT, PARSE_SYMBOL, 
    PARSE_MEM, PARSE_DELIM, PARSE_INSTR, PARSE_LABEL} parse_t;

/*
 * parse_instr: parse an expected data token (e.g., 'rrmovl')
 * args
 *     ptr: point to the start of string
 *     inst: point to the inst_t within instr_set
 *
 * return
 *     PARSE_INSTR: success, move 'ptr' to the first char after token,
 *                            and store the pointer of the instruction to 'inst'
 *     PARSE_ERR: error, the value of 'ptr' and 'inst' are undefined
 */
parse_t parse_instr(char **ptr, instr_t **inst)
{
    char *p = *ptr;

    /* skip the blank */
    SKIP_BLANK(p);
    /* find_instr and check end */
    instr_t *INSTR = find_instr(p);
    if ((INSTR->name) == NULL)
    {
        return PARSE_ERR;
    }

    else
    {
        p += INSTR->len;
        if (!(IS_END(p) || IS_BLANK(p) || IS_COMMENT(p)))
        {
            return PARSE_ERR;
        }
        /* set 'ptr' and 'inst' */
        *ptr = p;
        *inst = INSTR;
        return PARSE_INSTR;
    }
}

/*
 * parse_delim: parse an expected delimiter token (e.g., ',')
 * args
 *     ptr: point to the start of string
 *
 * return
 *     PARSE_DELIM: success, move 'ptr' to the first char after token
 *     PARSE_ERR: error, the value of 'ptr' and 'delim' are undefined
 */
parse_t parse_delim(char **ptr, char delim)
{
    char *p = *ptr;

    /* skip the blank and check */
    SKIP_BLANK(p);

    if ((*p) == delim)
    {
        p++;
    }

    else
    {
        if (delim == ',')
        {
            err_print("Invalid '%c'", delim);
        }
        return PARSE_ERR; 
    } 

    /* set 'ptr' */
    *ptr = p;
    return PARSE_DELIM;
}

/*
 * parse_reg: parse an expected register token (e.g., '%eax')
 * args
 *     ptr: point to the start of string
 *     regid: point to the regid of register
 *
 * return
 *     PARSE_REG: success, move 'ptr' to the first char after token, 
 *                         and store the regid to 'regid'
 *     PARSE_ERR: error, the value of 'ptr' and 'regid' are undefined
 */
parse_t parse_reg(char **ptr, regid_t *regid)
{
    char *p = *ptr;

    /* skip the blank and check */
    SKIP_BLANK(p);
    if (!IS_REG(p))
    {
        return PARSE_ERR;
    }

    /* find register */
    else
    {
        regid_t reg = find_register(p);
        if (reg == REG_ERR)
        {
            err_print("Invalid REG");
            return PARSE_ERR;
        }

        else
        {
            p += 4;
            /* set 'ptr' and 'regid' */
            *ptr = p;
            *regid = reg;
            return PARSE_REG;
        }
    }
}

/*
 * parse_symbol: parse an expected symbol token (e.g., 'Main')
 * args
 *     ptr: point to the start of string
 *     name: point to the name of symbol (should be allocated in this function)
 *
 * return
 *     PARSE_SYMBOL: success, move 'ptr' to the first char after token,
 *                               and allocate and store name to 'name'
 *     PARSE_ERR: error, the value of 'ptr' and 'name' are undefined
 */
parse_t parse_symbol(char **ptr, char **name)
{
    char *p = *ptr;

    /* skip the blank and check */
    SKIP_BLANK(p);
    if (!IS_LETTER(p))
    {
        return PARSE_ERR;
    }

    /* allocate name and copy to it */
    int len = 0;
    while (IS_LETTER(p) || IS_DIGIT(p))
    {
        len++;
        p++;
    }

    char *sym_name = (char *)malloc(len + 1);  
    memcpy(sym_name, *ptr, len);
    *(sym_name + len) = '\0';

    /* set 'ptr' and 'name' */
    *ptr = p;
    *name = sym_name;
    return PARSE_SYMBOL;
}

/*
 * parse_digit: parse an expected digit token (e.g., '0x100')
 * args
 *     ptr: point to the start of string
 *     value: point to the value of digit
 *
 * return
 *     PARSE_DIGIT: success, move 'ptr' to the first char after token
 *                            and store the value of digit to 'value'
 *     PARSE_ERR: error, the value of 'ptr' and 'value' are undefined
 */
parse_t parse_digit(char **ptr, long *value)
{
    char *p = *ptr;

    /* skip the blank and check */
    SKIP_BLANK(p);
    if (!IS_DIGIT(p)) 
    {
        return PARSE_ERR;
    }

    /* calculate the digit, (NOTE: see strtoll()) */
    long num = strtoll(*ptr, &p, 0);

    /* set 'ptr' and 'value' */
    *ptr = p;
    *value = num;
    return PARSE_DIGIT;
}

/*
 * parse_imm: parse an expected immediate token (e.g., '$0x100' or 'STACK')
 * args
 *     ptr: point to the start of string
 *     name: point to the name of symbol (should be allocated in this function)
 *     value: point to the value of digit
 *
 * return
 *     PARSE_DIGIT: success, the immediate token is a digit,
 *                            move 'ptr' to the first char after token,
 *                            and store the value of digit to 'value'
 *     PARSE_SYMBOL: success, the immediate token is a symbol,
 *                            move 'ptr' to the first char after token,
 *                            and allocate and store name to 'name' 
 *     PARSE_ERR: error, the value of 'ptr', 'name' and 'value' are undefined
 */
parse_t parse_imm(char **ptr, char **name, long *value)
{
    char *p = *ptr;
    /* skip the blank and check */
    SKIP_BLANK(p);
    if (IS_END(p))
    {
        return PARSE_ERR;
    }

    else
    {
        /* if IS_IMM, then parse the digit */
        if (IS_IMM(p))
        {
            p++;
            if (parse_digit(&p, value) == PARSE_DIGIT)
            {
                *ptr = p;
                return PARSE_DIGIT;
            }

            else
            {
                err_print("Invalid Immediate");
                return PARSE_ERR;
            }
        }

        /* if IS_LETTER, then parse the symbol */
        else if (IS_LETTER(p))
        {
            if(parse_symbol(&p, name) == PARSE_SYMBOL)
            {
                *ptr = p;
                return PARSE_SYMBOL;
            }
        }
    }

    return PARSE_ERR;
}

/*
 * parse_mem: parse an expected memory token (e.g., '8(%ebp)')
 * args
 *     ptr: point to the start of string
 *     value: point to the value of digit
 *     regid: point to the regid of register
 *
 * return
 *     PARSE_MEM: success, move 'ptr' to the first char after token,
 *                          and store the value of digit to 'value',
 *                          and store the regid to 'regid'
 *     PARSE_ERR: error, the value of 'ptr', 'value' and 'regid' are undefined
 */
parse_t parse_mem(char **ptr, long *value, regid_t *regid)
{
    char *p = *ptr;
    long num = 0;
    regid_t rA;

    /* skip the blank and check */
    SKIP_BLANK(p);
    if (parse_digit(&p, &num) != PARSE_ERR && 
        parse_delim(&p, '(') != PARSE_ERR  &&
        parse_reg(&p, &rA) != PARSE_ERR    &&
        parse_delim(&p, ')') != PARSE_ERR)
        
    {
        /* calculate the digit and register, (ex: (%ebp) or 8(%ebp)) */
        *value = num;
        *regid = rA;
    }
    
    else if (parse_delim(&p, '(') != PARSE_ERR  &&
             parse_reg(&p, &rA) != PARSE_ERR    &&
             parse_delim(&p, ')') != PARSE_ERR)
    {
        /* calculate the digit and register, (ex: (%ebp) or 8(%ebp)) */
        *value = num;
        *regid = rA;
    }

    else
    {
        err_print("Invalid MEM");
        return PARSE_ERR;
    }

    /* set 'ptr'*/
    *ptr = p;
    return PARSE_MEM;
}

/*
 * parse_data: parse an expected data token (e.g., '0x100' or 'array')
 * args
 *     ptr: point to the start of string
 *     name: point to the name of symbol (should be allocated in this function)
 *     value: point to the value of digit
 *
 * return
 *     PARSE_DIGIT: success, data token is a digit,
 *                            and move 'ptr' to the first char after token,
 *                            and store the value of digit to 'value'
 *     PARSE_SYMBOL: success, data token is a symbol,
 *                            and move 'ptr' to the first char after token,
 *                            and allocate and store name to 'name' 
 *     PARSE_ERR: error, the value of 'ptr', 'name' and 'value' are undefined
 */
parse_t parse_data(char **ptr, char **name, long *value)
{
    char *p = *ptr;

    /* skip the blank and check */
    SKIP_BLANK(p);
    if (IS_END(p))
    {
        return PARSE_ERR;
    }

    /* if IS_DIGIT, then parse the digit */
    if (IS_DIGIT(p))
    {
        if (parse_digit(&p, value) != PARSE_ERR)
        {
            *ptr = p;
            return PARSE_DIGIT;
        }

        else
        {
            return PARSE_ERR;
        }
    }

    /* if IS_LETTER, then parse the symbol */
    else if (IS_LETTER(p))
    {
        if (parse_symbol(&p, name) != PARSE_ERR)
        {
            *ptr = p;
            return PARSE_SYMBOL;
        }

        else
        {
            return PARSE_ERR;
        }
    }

    return PARSE_ERR;
}

/*
 * parse_label: parse an expected label token (e.g., 'Loop:')
 * args
 *     ptr: point to the start of string
 *     name: point to the name of symbol (should be allocated in this function)
 *
 * return
 *     PARSE_LABEL: success, move 'ptr' to the first char after token
 *                            and allocate and store name to 'name'
 *     PARSE_ERR: error, the value of 'ptr' is undefined
 */
parse_t parse_label(char **ptr, char **name)
{
    char *p = *ptr;
    /* skip the blank and check */
    SKIP_BLANK(p);

    /* allocate name and copy to it */
    if (parse_symbol(&p, name) != PARSE_ERR)
    {
        if (parse_delim(&p, ':') != PARSE_ERR)
        {
            /* set 'ptr' and 'name' */
            *ptr = p;
            return PARSE_LABEL;
        }
    }

    return PARSE_ERR;
}

/*
 * parse_line: parse a line of y86 code (e.g., 'Loop: mrmovl (%ecx), %esi')
 * (you could combine above parse_xxx functions to do it)
 * args
 *     line: point to a line_t data with a line of y86 assembly code
 *
 * return
 *     PARSE_XXX: success, fill line_t with assembled y86 code
 *     PARSE_ERR: error, try to print err information (e.g., instr type and line number)
 */
 int count = 0;
type_t parse_line(line_t *line)
{

/* when finish parse an instruction or lable, we still need to continue check 
* e.g., 
*  Loop: mrmovl (%ebp), %ecx
*           call SUM  #invoke SUM function */
    //count++;
    //printf("%d\n", count);

    char *p = line->y86asm;
    char *name;
    instr_t *inst;
    regid_t rA;
    regid_t rB;
    long num;
    parse_t ret;

    /* skip blank and check IS_END */
    SKIP_BLANK(p);
    if (IS_END(p))
    {
        line->type = TYPE_COMM;
        return TYPE_COMM; /* default is COMM */
    }

    /* is a comment ? */
    if (IS_COMMENT(p))
    {
        line->type = TYPE_COMM;
        return TYPE_COMM;
    }

    /* is a label ? */
    if (parse_label(&p, &name) != PARSE_ERR)
    {
        if (add_symbol(name) < 0)
        {
            line->type = TYPE_ERR; /* use the same label */ 
            /* Warning Note: should free the mem */
            free(name);
            return line->type;
        }

        else
        {
            line->type = TYPE_INS;
            line->y86bin.addr = vmaddr;
            line->y86bin.bytes = 0;
            free(name);
        }
    }

    /* is an instruction ? */
    if (parse_instr(&p, &inst) != PARSE_ERR)
    {
        //printf("%s\n", inst->name);
        /* parse the rest of instruction according to the itype */
        /* 0 byte inst */
        if (strcmp(inst->name, ".pos") == 0)
        {
            ret = parse_digit(&p, &num);
            if (ret != PARSE_ERR)
            {
                line->type = TYPE_INS;
                vmaddr = (int)num;
            }

            else
            {
                line->type = TYPE_ERR;
            }
        }

        else if (strcmp(inst->name, ".align") == 0)
        {
            ret = parse_digit(&p, &num);
            if (ret != PARSE_ERR)
            {
                if (num != 2 && num != 4 && num != 8)
                {
                    line->type = TYPE_ERR;
                }

                else
                {
                    int val = (int)num;
                    line->type = TYPE_INS;
                    if (vmaddr % val != 0)
                    {
                        int asign = val - vmaddr % val;
                        vmaddr += asign;
                    }
                }
            }

            else
            {
                line->type = TYPE_ERR;
            }
        }
        /* .byte */
        else if (strcmp(inst->name, ".byte") == 0)
        {
            ret = parse_data(&p, &name, &num);
            if (ret != PARSE_ERR)
            {
                line->type = TYPE_INS;
                line->y86bin.addr = vmaddr;
                line->y86bin.bytes = inst->bytes;
                if (ret == PARSE_DIGIT)
                {
                    memcpy(line->y86bin.codes, &num, 1);
                }

                else
                {
                    add_reloc(name, &(line->y86bin));
                }
            }

            else
            {
                line->type = TYPE_ERR;
            }
        }
        /* 1 byte inst except .byte */
        else if (strcmp(inst->name, "nop") == 0  || 
                 strcmp(inst->name, "halt") == 0 || 
                 strcmp(inst->name, "ret") == 0)
        {
            line->y86bin.codes[0] = inst->code;
            line->type = TYPE_INS;
        }
        /* 2 bytes inst except .word / pushl / popl */
        /* Error Note: pushl/popl only has one reg */
        else if (strcmp(inst->name, "rrmovl") == 0 || 
                 strcmp(inst->name, "cmovle") == 0 || 
                 strcmp(inst->name, "cmovl") == 0  ||
                 strcmp(inst->name, "cmove") == 0  || 
                 strcmp(inst->name, "cmovne") == 0 || 
                 strcmp(inst->name, "cmovge") == 0 ||
                 strcmp(inst->name, "cmovg") == 0  || 
                 strcmp(inst->name, "addl") == 0   || 
                 strcmp(inst->name, "subl") == 0   ||
                 strcmp(inst->name, "andl") == 0   ||
                 strcmp(inst->name, "xorl") == 0)
        {
            if (parse_reg(&p, &rA) != PARSE_ERR   && 
                parse_delim(&p, ',') != PARSE_ERR &&
                parse_reg(&p, &rB) != PARSE_ERR)
            {
                line->y86bin.codes[0] = inst->code;
                line->y86bin.codes[1] = HPACK(rA, rB);
                line->type = TYPE_INS;
            }

            else
            {
                line->type = TYPE_ERR;
            }
        }
        /* .word */
        else if (strcmp(inst->name, ".word") == 0)
        {
            ret = parse_data(&p, &name, &num);
            if (ret != PARSE_ERR)
            {
                line->type = TYPE_INS;
                line->y86bin.addr = vmaddr;
                line->y86bin.bytes = inst->bytes;
                if (ret == PARSE_DIGIT)
                {
                    memcpy(line->y86bin.codes, &num, 2);
                }

                else
                {
                    add_reloc(name, &(line->y86bin));
                }
            }

            else
            {
                line->type = TYPE_ERR;
            }
        }
        /* popl and pushl */
        else if (strcmp(inst->name, "pushl") == 0 || 
                 strcmp(inst->name, "popl") == 0) 
        {
            if (parse_reg(&p, &rA) != PARSE_ERR)
            {
                line->y86bin.codes[0] = inst->code;
                line->y86bin.codes[1] = HPACK(rA, REG_NONE);
                line->type = TYPE_INS;
            }

            else
            {
                line->type = TYPE_ERR;
            }
        }
        /* 4 bytes inst .long */
        else if (strcmp(inst->name, ".long") == 0)
        {
            ret = parse_data(&p, &name, &num);
            if (ret != PARSE_ERR)
            {
                line->type = TYPE_INS;
                line->y86bin.addr = vmaddr;
                line->y86bin.bytes = inst->bytes;
                if (ret == PARSE_DIGIT)
                {
                    memcpy(line->y86bin.codes, &num, 4);
                }

                else
                {
                    add_reloc(name, &(line->y86bin));
                }
            }

            else
            {
                line->type = TYPE_ERR;
            }
        }
        /* 5 bytes inst */
        else if (strcmp(inst->name, "jmp") == 0 || 
                 strcmp(inst->name, "jle") == 0 || 
                 strcmp(inst->name, "jl") == 0  ||
                 strcmp(inst->name, "je") == 0  || 
                 strcmp(inst->name, "jne") == 0 || 
                 strcmp(inst->name, "jge") == 0 ||
                 strcmp(inst->name, "jg") == 0  || 
                 strcmp(inst->name, "call") == 0)
        {
            ret = parse_data(&p, &name, &num);
            if (ret == PARSE_SYMBOL)
            {
                line->y86bin.codes[0] = inst->code;
                line->type = TYPE_INS;
                line->y86bin.addr = vmaddr;
                line->y86bin.bytes = inst->bytes;
                add_reloc(name, &(line->y86bin));
            }

            else
            {
                err_print("Invalid DEST");
                line->type = TYPE_ERR;
            }
        }
        /* 6 bytes inst */
        /* irmovl */
        else if (strcmp(inst->name, "irmovl") == 0)
        {
            ret = parse_imm(&p, &name, &num);
            if (ret != PARSE_ERR)
            {
                if (parse_delim(&p, ',') != PARSE_ERR &&
                    parse_reg(&p, &rB) != PARSE_ERR)
                {
                    line->y86bin.codes[0] = inst->code;
                    line->y86bin.codes[1] = HPACK(REG_NONE, rB);
                    line->type = TYPE_INS;
                    line->y86bin.addr = vmaddr;
                    line->y86bin.bytes = inst->bytes;
                    if (ret == PARSE_DIGIT)
                    {
                        memcpy(line->y86bin.codes + 2, &num, 4);
                    }

                    else
                    {
                        add_reloc(name, &(line->y86bin));
                    }
                }

                else
                {
                    line->type = TYPE_ERR;
                }
            }

            else
            {
                line->type = TYPE_ERR;
            }
        }
        /* rmmovl */
        else if (strcmp(inst->name, "rmmovl") == 0)
        {
            if (parse_reg(&p, &rA) != PARSE_ERR   &&
                parse_delim(&p, ',') != PARSE_ERR &&
                parse_mem(&p, &num, &rB) != PARSE_ERR)
            {
                line->type = TYPE_INS;
                line->y86bin.codes[0] = inst->code;
                line->y86bin.codes[1] = HPACK(rA, rB);
                memcpy(line->y86bin.codes + 2, &num, 4);
            }

            else 
            {
                line->type = TYPE_ERR;
            }
        }
        /* mrmovl */
        else if (strcmp(inst->name, "mrmovl") == 0)
        {
            if (parse_mem(&p, &num, &rB) != PARSE_ERR   &&
                parse_delim(&p, ',') != PARSE_ERR       &&
                parse_reg(&p, &rA) != PARSE_ERR)
            {
                line->type = TYPE_INS;
                line->y86bin.codes[0] = inst->code;
                line->y86bin.codes[1] = HPACK(rA, rB);
                memcpy(line->y86bin.codes + 2, &num, 4);
            }

            else 
            {
                line->type = TYPE_ERR;
            }
        }

        else
        {
            line->type = TYPE_ERR;
        }

        /* set some of y86bin states */
        line->y86bin.addr = vmaddr;
        line->y86bin.bytes = inst->bytes;

        /* update vmaddr */  
        vmaddr += inst->bytes; 
    }
    /* parse the rest */
    SKIP_BLANK(p);
    if (!(IS_END(p) || IS_COMMENT(p)))
    {
        line->type = TYPE_ERR;
    }
    return line->type;
}

/*
 * assemble: assemble an y86 file (e.g., 'asum.ys')
 * args
 *     in: point to input file (an y86 assembly file)
 *
 * return
 *     0: success, assmble the y86 file to a list of line_t
 *     -1: error, try to print err information (e.g., instr type and line number)
 */
int assemble(FILE *in)
{
    static char asm_buf[MAX_INSLEN]; /* the current line of asm code */
    line_t *line;
    int slen;
    char *y86asm;

    /* read y86 code line-by-line, and parse them to generate raw y86 binary code list */
    while (fgets(asm_buf, MAX_INSLEN, in) != NULL) {
        slen  = strlen(asm_buf);
        if ((asm_buf[slen-1] == '\n') || (asm_buf[slen-1] == '\r')) { 
            asm_buf[--slen] = '\0'; /* replace terminator */
        }

        /* store y86 assembly code */
        y86asm = (char *)malloc(sizeof(char) * (slen + 1)); // free in finit
        strcpy(y86asm, asm_buf);

        line = (line_t *)malloc(sizeof(line_t)); // free in finit
        memset(line, '\0', sizeof(line_t));

        /* set defualt */
        line->type = TYPE_COMM;
        line->y86asm = y86asm;
        line->next = NULL;

        /* add to y86 binary code list */
        y86bin_listtail->next = line;
        y86bin_listtail = line;
        y86asm_lineno ++;

        /* parse */
        if (parse_line(line) == TYPE_ERR)
            return -1;
    }

    /* skip line number information in err_print() */
    y86asm_lineno = -1;
    return 0;
}

/*
 * relocate: relocate the raw y86 binary code with symbol address
 *
 * return
 *     0: success
 *     -1: error, try to print err information (e.g., addr and symbol)
 */
int relocate(void)
{
    reloc_t *rtmp = reltab->next;
    symbol_t *stmp = NULL;
    
    while (rtmp != NULL) {
        /* find symbol */
        stmp = find_symbol(rtmp->name);
        //printf("reloc: %s\n", rtmp->name);
        if (stmp == NULL)
        {
            err_print("Unknown symbol:'%s'", rtmp->name);
            return -1;
        }

        /* relocate y86bin according itype */
        int bytes = rtmp->y86bin->bytes;
        if (bytes == 6) /* irmovl */
        {
            memcpy(rtmp->y86bin->codes + 2, &(stmp->addr), 4);
        }

        else if (bytes == 5) /* jxx / call */
        {
            memcpy(rtmp->y86bin->codes + 1, &(stmp->addr), 4);
        }

        else if (bytes == 4) /* .long */
        {
            memcpy(rtmp->y86bin->codes, &(stmp->addr), 4);
        }

        else if (bytes == 2) /* .word */
        {
            memcpy(rtmp->y86bin->codes, &(stmp->addr), 2);
        }

        else if (bytes == 1) /* .byte */
        {
            memcpy(rtmp->y86bin->codes, &(stmp->addr), 1);
        }

        else
        {
            return -1;
        }

        /* next */
        rtmp = rtmp->next;
    }
    return 0;
}

/*
 * binfile: generate the y86 binary file
 * args
 *     out: point to output file (an y86 binary file)
 *
 * return
 *     0: success
 *     -1: error
 */
int binfile(FILE *out)
{
    /* prepare image with y86 binary code */
    /* Error Note: should add the blank to .pos/.align.... */
    line_t *head = y86bin_listhead->next;
    int real_size = 0;
    line_t *p = head;
    while (p != NULL) 
    {
        if (p->type == TYPE_INS && p->y86bin.bytes > 0) 
        {
            if (real_size < p->y86bin.addr + p->y86bin.bytes)
            {
                real_size = p->y86bin.addr + p->y86bin.bytes;
            }       
        }

        p = p->next;
    }

    byte_t *image = (byte_t *)malloc(real_size);

    p = head;
    while (p != NULL) 
    {
        if (p->type == TYPE_INS && p->y86bin.bytes > 0) 
        {
            memcpy(image + p->y86bin.addr, p->y86bin.codes, p->y86bin.bytes);       
        }

        p = p->next;
    }
    /* binary write y86 code to output file (NOTE: see fwrite()) */
    fwrite(image, real_size, 1, out);
    free(image);
    return 0;
}


/* whether print the readable output to screen or not ? */
bool_t screen = TRUE; 

static void hexstuff(char *dest, int value, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        char c;
        int h = (value >> 4*i) & 0xF;
        c = h < 10 ? h + '0' : h - 10 + 'a';
        dest[len-i-1] = c;
    }
}

void print_line(line_t *line)
{
    char buf[26];

    /* line format: 0xHHH: cccccccccccc | <line> */
    if (line->type == TYPE_INS) {
        bin_t *y86bin = &line->y86bin;
        int i;
        
        strcpy(buf, "  0x000:              | ");
        
        hexstuff(buf+4, y86bin->addr, 3);
        if (y86bin->bytes > 0)
            for (i = 0; i < y86bin->bytes; i++)
                hexstuff(buf+9+2*i, y86bin->codes[i]&0xFF, 2);
    } else {
        strcpy(buf, "                      | ");
    }

    printf("%s%s\n", buf, line->y86asm);
}

/* 
 * print_screen: dump readable binary and assembly code to screen
 * (e.g., Figure 4.8 in ICS book)
 */
void print_screen(void)
{
    line_t *tmp = y86bin_listhead->next;
    
    /* line by line */
    while (tmp != NULL) {
        print_line(tmp);
        tmp = tmp->next;
    }
}

/* init and finit */
void init(void)
{
    reltab = (reloc_t *)malloc(sizeof(reloc_t)); // free in finit
    memset(reltab, 0, sizeof(reloc_t));

    symtab = (symbol_t *)malloc(sizeof(symbol_t)); // free in finit
    memset(symtab, 0, sizeof(symbol_t));

    y86bin_listhead = (line_t *)malloc(sizeof(line_t)); // free in finit
    memset(y86bin_listhead, 0, sizeof(line_t));
    y86bin_listtail = y86bin_listhead;
    y86asm_lineno = 0;
}

void finit(void)
{
    reloc_t *rtmp = NULL;
    do {
        rtmp = reltab->next;
        if (reltab->name) 
            free(reltab->name);
        free(reltab);
        reltab = rtmp;
    } while (reltab);
    
    symbol_t *stmp = NULL;
    do {
        stmp = symtab->next;
        if (symtab->name) 
            free(symtab->name);
        free(symtab);
        symtab = stmp;
    } while (symtab);

    line_t *ltmp = NULL;
    do {
        ltmp = y86bin_listhead->next;
        if (y86bin_listhead->y86asm) 
            free(y86bin_listhead->y86asm);
        free(y86bin_listhead);
        y86bin_listhead = ltmp;
    } while (y86bin_listhead);
}

static void usage(char *pname)
{
    printf("Usage: %s [-v] file.ys\n", pname);
    printf("   -v print the readable output to screen\n");
    exit(0);
}

int main(int argc, char *argv[])
{
    int rootlen;
    char infname[512];
    char outfname[512];
    int nextarg = 1;
    FILE *in = NULL, *out = NULL;
    
    if (argc < 2)
        usage(argv[0]);
    
    if (argv[nextarg][0] == '-') {
        char flag = argv[nextarg][1];
        switch (flag) {
          case 'v':
            screen = TRUE;
            nextarg++;
            break;
          default:
            usage(argv[0]);
        }
    }

    /* parse input file name */
    rootlen = strlen(argv[nextarg])-3;
    /* only support the .ys file */
    if (strcmp(argv[nextarg]+rootlen, ".ys"))
        usage(argv[0]);
    
    if (rootlen > 500) {
        err_print("File name too long");
        exit(1);
    }


    /* init */
    init();

    
    /* assemble .ys file */
    strncpy(infname, argv[nextarg], rootlen);
    strcpy(infname+rootlen, ".ys");
    in = fopen(infname, "r");
    if (!in) {
        err_print("Can't open input file '%s'", infname);
        exit(1);
    }
    
    if (assemble(in) < 0) {
        err_print("Assemble y86 code error");
        fclose(in);
        exit(1);
    }
    fclose(in);


    /* relocate binary code */
    if (relocate() < 0) {
        err_print("Relocate binary code error");
        exit(1);
    }


    /* generate .bin file */
    strncpy(outfname, argv[nextarg], rootlen);
    strcpy(outfname+rootlen, ".bin");
    out = fopen(outfname, "wb");
    if (!out) {
        err_print("Can't open output file '%s'", outfname);
        exit(1);
    }

    if (binfile(out) < 0) {
        err_print("Generate binary file error");
        fclose(out);
        exit(1);
    }
    fclose(out);
    
    /* print to screen (.yo file) */
    if (screen)
       print_screen(); 

    /* finit */
    finit();
    return 0;
}


