#include <stdio.h>
#include "set.h"
#include "type.c"

#define NRW        12     // number of reserved words 添加新的保留字需要增大NRW
#define TXMAX      500    // length of identifier table 标识符最多数量
#define MAXNUMLEN  14     // maximum number of digits in numbers 数字最大长度
#define NSYM       13     // maximum number of symbols in array ssym and csym;FPT:增加了`[`和`]`，所以要增大NSYM到12
#define MAXIDLEN   10     // length of identifiers 标识符最大长度

#define MAXADDRESS 32767  // maximum address 
#define MAXLEVEL   32     // maximum depth of nesting block 嵌套层数最大值
#define CXMAX      500    // size of code array

#define MAXSYM     30     // maximum number of symbols  

#define STACKSIZE  65536  // maximum storage

#define MAX_DIM    100    //maximum dim of array

/**
 * @enum symtype
 * @brief Represents the different types of symbols in the PL/0 programming language.
 * 
 * This enumeration defines the possible symbol types that can be encountered in a PL/0 program.
 * Each symbol type corresponds to a specific token in the PL/0 language.
 */
enum symtype
{
	SYM_NULL,           // undefined symbol
	SYM_IDENTIFIER,     // identifier
	SYM_NUMBER,         // number
	SYM_SCOPE,          // '::'
	SYM_PLUS,           // '+'
	SYM_MINUS,          // '-'
	SYM_TIMES,          // '*'
	SYM_SLASH,          // '/'
	SYM_ODD,            // 'odd'，判断一个expression是不是奇数
	SYM_EQU,            // '='
	SYM_NEQ,            // '<>'
	SYM_LES,            // '<'
	SYM_LEQ,            // '<='
	SYM_GTR,            // '>'
	SYM_GEQ,            // '>='
	SYM_LPAREN,         // '('
	SYM_RPAREN,         // ')'
	SYM_COMMA,          // ','
	SYM_SEMICOLON,      // ';'
	SYM_PERIOD,         // '.'
	SYM_BECOMES,        // ':='
    SYM_BEGIN,          // 'begin'
	SYM_END,            // 'end'
	SYM_IF,             // 'if'
	SYM_THEN,           // 'then'
	SYM_WHILE,          // 'while'
	SYM_DO,             // 'do'
	SYM_CALL,           // 'call'
	SYM_CONST,          // 'const'
	SYM_VAR,            // 'var'
	SYM_PROCEDURE,      // 'procedure'
	SYM_LMIDPAREN,		//`[`
	SYM_RMIDPAREN,		//`]`
	SYM_PRINT,          // 'print'
	SYM_ADDRESS,        // '&'，取地址符
	SYM_POINTER 		// '*'，指针
};

enum idtype
{
	ID_CONSTANT, ID_VARIABLE, ID_PROCEDURE
};

enum opcode
{
	LIT, OPR, LOD, STO, CAL, INT, JMP, JPC, PRT, STA, LEA, LDA
};

enum oprcode
{
	OPR_RET, OPR_NEG, OPR_ADD, OPR_MIN,
	OPR_MUL, OPR_DIV, OPR_ODD, OPR_EQU,
	OPR_NEQ, OPR_LES, OPR_LEQ, OPR_GTR,
	OPR_GEQ, OPR_SWP, OPR_POP
};


typedef struct
{
	int f; // function code
	int l; // level
	int a; // displacement address
} instruction;

//////////////////////////////////////////////////////////////////////
char* err_msg[] =
{
/*  0 */    "",
/*  1 */    "Found ':=' when expecting '='.",
/*  2 */    "There must be a number to follow '='.",
/*  3 */    "There must be an '=' to follow the identifier.",
/*  4 */    "There must be an identifier to follow 'const', 'var', or 'procedure'.",
/*  5 */    "Missing ',' or ';'.",
/*  6 */    "Incorrect procedure name.",
/*  7 */    "Statement expected.",
/*  8 */    "Follow the statement is an incorrect symbol.",
/*  9 */    "'.' expected.",
/* 10 */    "';' expected.",
/* 11 */    "Undeclared identifier.",
/* 12 */    "Illegal assignment.",
/* 13 */    "':=' expected.",
/* 14 */    "There must be an identifier to follow the 'call'.",
/* 15 */    "A constant or variable can not be called.",
/* 16 */    "'then' expected.",
/* 17 */    "';' or 'end' expected.",
/* 18 */    "'do' expected.",
/* 19 */    "Incorrect symbol.",
/* 20 */    "Relative operators expected.",
/* 21 */    "Procedure identifier can not be in an expression.",
/* 22 */    "Missing ')'.",
/* 23 */    "The symbol can not be followed by a factor.",
/* 24 */    "The symbol can not be as the beginning of an expression.",
/* 25 */    "The number is too great.",
/* 26 */    "Can not locate identifier in a non-procedure block.",
/* 27 */    "Missing ']'",
/* 28 */    "Declaration failed.",
/* 29 */    "Missing size of array", //缺少维度大小
/* 30 */    "Incorrect array dimension analysis", //维度分析错误
/* 31 */    "Missing identifier",
/* 32 */    "There are too many levels.",
/* 33 */	"There are too many array dimensions.", //维数过多
/* 34 */	"Type deduction failed.", //类型推导失败
/* 35 */	"Can not add two pointers.",
/* 36 */	"Procedure can not be an expression.",
/* 37 */	"Can not address" //不能取地址
};

//////////////////////////////////////////////////////////////////////
char ch;         // last character read
int  sym;        // last symbol read
char id[MAXIDLEN + 1]; // last identifier read
int  num;        // last number read
int  cc;         // character count
int  ll;         // line length
int  kk; 	   // program text index
int  err; 	  // error flag
int  cx;         // index of current instruction to be generated.当前要生成第几条中间代码
int  level = 0; // current depth of block nesting

char line[80];
int all_sym_num, all_sym[TXMAX]; // all symbols in the PL/0 source program
int num_sym_val[TXMAX]; // the value of symbols, if it is a number

// typedef struct array_attribute
// {
// 	short address;//数组的基地址(即数组第一个元素在栈中的位置)
// 	int size;//数组大小
// 	int dim;//总维数 
// 	int dim_size[MAX_DIM + 1];//每一维的范围大小
// } array_attribute;
// array_attribute array_table[TXMAX];//数组信息表
// struct array_attribute* Last_Array; //指向最后读到的数组
// int  arr_tx = 0; // 当前读到的数组在数组表中的索引

instruction code[CXMAX];

char* word[NRW + 1] =
{
	"", /* place holder */
	"begin", "call", "const", "do", "end","if",
	"odd", "procedure", "then", "var", "while", "print"
};

/**
 * Array of symbols representing reserved words in PL/0 language.
 */
int wsym[NRW + 1] =
{
	SYM_NULL, SYM_BEGIN, SYM_CALL, SYM_CONST, SYM_DO, SYM_END,
	SYM_IF, SYM_ODD, SYM_PROCEDURE, SYM_THEN, SYM_VAR, SYM_WHILE, SYM_PRINT
};

/**
 * Array of symbols representing special characters in PL/0 language.
 */
int ssym[NSYM + 1] =
{
	SYM_NULL, SYM_PLUS, SYM_MINUS, SYM_TIMES, SYM_SLASH,
	SYM_LPAREN, SYM_RPAREN, SYM_EQU, SYM_COMMA, SYM_PERIOD, SYM_SEMICOLON,
	//FPT:增加`[`和`]`
	SYM_LMIDPAREN,SYM_RMIDPAREN, SYM_ADDRESS
};

char csym[NSYM + 1] =
{
	' ', '+', '-', '*', '/', '(', ')', '=', ',', '.', ';', '[', ']', '&'
};

#define MAXINS   12
char* mnemonic[MAXINS] =
{
	"LIT", "OPR", "LOD", "STO", "CAL", "INT", "JMP", "JPC", "PRT", "STA", "LEA", "LDA"
};

// 符号表
typedef struct
{
	char name[MAXIDLEN + 1]; // 名字
	type *type; // 类型
	int kind; // const = 0, var = 1, proc = 2
	int level; // 所在层
	void *attr; // 符号在自己类型的属性表中的表项位置
} sym_attr;
int sym_num;
sym_attr sym_tab[TXMAX];

// 常量表
typedef int const_attr;
int const_num;
const_attr const_tab[TXMAX]; // 常量的值

// 变量表
typedef int var_attr;
int var_num;
var_attr var_tab[TXMAX]; // 变量的相对地址

// 过程表
typedef struct
{
	int address; // 过程的入口地址
	int active; // 过程是否在作用域内
} proc_attr;
int proc_num;
proc_attr proc_tab[TXMAX]; // 过程的入口地址


FILE* infile;

type *prim_scope(symset fsys, int proc);
type *scope(symset fsys);
type *factor(symset fsys);
type *array_term(symset fsys, int cal_addr);
type *unary_term(symset fsys, int cal_addr);
type *term(symset fsys, int cal_addr);
type *expression(symset fsys, int cal_addr);

void condition(symset fsys);

void pointer();
void direct_declarator();
void declarator(symset fsys);
void statement(symset fsys);
void vardeclaration();
void constdeclaration();
void block(symset fsys);

// EOF PL0.h
