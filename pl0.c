// pl0 compiler source code

#pragma warning(disable:4996)


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "pl0.h"
#include "set.c"

//////////////////////////////////////////////////////////////////////
// print error message.
void error(int n)
{
	int i;

	printf("      ");
	for (i = 1; i <= cc - 1; i++)
		printf(" ");
	printf("^\n");
	printf("Error %3d: %s\n", n, err_msg[n]);
	err++;
} // error

//////////////////////////////////////////////////////////////////////
/**
 * Reads a character from the input file and stores it in the variable 'ch'.
 * If the end of the file is reached, it prints an error message and exits the program.
 * If the end of the line is reached, it reads the next line from the input file.
 * This function is used to implement the 'getch' operation in the PL/0 compiler.
 */
void getch(void)
{
	if (cc == ll)
	{
		if (feof(infile))
		{
			printf("\nPROGRAM INCOMPLETE\n");
			exit(1);
		}
		ll = cc = 0;
		printf("%5d  ", cx);
		while ((!feof(infile)) && ((ch = getc(infile)) != '\n'))
		{
			printf("%c", ch);
			line[++ll] = ch;
		}
		printf("\n");
		line[++ll] = ' ';
	}
	ch = line[++cc];
} // getch

//////////////////////////////////////////////////////////////////////
// gets a symbol from input stream.
void getsym(void)
{
	int i, k;
	char a[MAXIDLEN + 1];

	while (ch == ' '||ch == '\t')
		getch();

	if (isalpha(ch))
	{ // symbol is a reserved word or an identifier.
		k = 0;
		do
		{
			if (k < MAXIDLEN)
				a[k++] = ch;
			getch();
		}
		while (isalpha(ch) || isdigit(ch));
		a[k] = 0;
		strcpy(id, a);
		word[0] = id;
		i = NRW;
		while (strcmp(id, word[i--]));
		if (++i)
			sym = wsym[i]; // symbol is a reserved word
		else
			sym = SYM_IDENTIFIER;   // symbol is an identifier
	}
	else if (isdigit(ch))
	{ // symbol is a number.
		k = num = 0;
		sym = SYM_NUMBER;
		do
		{
			num = num * 10 + ch - '0';
			k++;
			getch();
		}
		while (isdigit(ch));
		if (k > MAXNUMLEN)
			error(25);     // The number is too great.
	}
	else if (ch == ':')
	{
		getch();
		if (ch == '=')
		{
			sym = SYM_BECOMES; // :=
			getch();
		}
		else if (ch == ':')
		{
			sym = SYM_SCOPE; // ::
			getch();
		}
		else
		{
			sym = SYM_NULL;       // illegal
		}
	}
	else if (ch == '>')
	{
		getch();
		if (ch == '=')
		{
			sym = SYM_GEQ;     // >=
			getch();
		}
		else
		{
			sym = SYM_GTR;     // >
		}
	}
	else if (ch == '<')
	{
		getch();
		if (ch == '=')
		{
			sym = SYM_LEQ;     // <=
			getch();
		}
		else if (ch == '>')
		{
			sym = SYM_NEQ;     // <>
			getch();
		}
		else
		{
			sym = SYM_LES;     // <
		}
	}
	else
	{ // other tokens
		i = NSYM;
		csym[0] = ch;
		while (csym[i--] != ch);
		if (++i)
		{
			sym = ssym[i];
			getch();
		}
		else
		{
			printf("Fatal Error: Unknown character.\n");
			exit(1);
		}
	}
	all_sym[++all_sym_num] = sym;
	num_sym_val[all_sym_num] = num;
} // getsym

//////////////////////////////////////////////////////////////////////
// generates (assembles) an instruction.
void gen(int x, int y, int z)
{
	if (cx > CXMAX)
	{
		printf("Fatal Error: Program too long.\n");
		exit(1);
	}
	code[cx].f = x;
	code[cx].l = y;
	code[cx++].a = z;
	//debug
	// printf("%5d  gen( %3s %d %d )\n", cx - 1, mnemonic[x], y, z);
} // gen

//////////////////////////////////////////////////////////////////////
// tests if error occurs and skips all symbols that do not belongcxs to s1 or s2.
void test(symset s1, symset s2, int n)
{
	symset s;

	if (! inset(sym, s1))
	{
		error(n);
		s = uniteset(s1, s2);
		while(! inset(sym, s))
			getsym();
		destroyset(s);
	}
} // test

//////////////////////////////////////////////////////////////////////
int dx;  // data allocation index

// enter object(constant, variable or procedre) into table.
void enter(int kind, type *type)
{
	sym_num++;
	strcpy(sym_tab[sym_num].name, id);
	sym_tab[sym_num].kind = kind;
	switch (kind)
	{
	case ID_CONSTANT:
		if (num > MAXADDRESS)
		{
			error(25); // The number is too great.
			num = 0;
		}
		const_tab[const_num] = num;
		sym_tab[sym_num].attr = &const_tab[const_num++];
		break;
	case ID_VARIABLE:
		// all identifiers declared by 'var' are variables
		var_tab[var_num] = dx;
		dx += type_size(type);
		sym_tab[sym_num].type = type;
		sym_tab[sym_num].level = level;
		sym_tab[sym_num].attr = &var_tab[var_num++];
		break;
	case ID_PROCEDURE:
		// 过程的入口地址在生成代码的时候回填
		proc_tab[proc_num].active = 1;
		sym_tab[sym_num].level = level;
		sym_tab[sym_num].attr = &proc_tab[proc_num++];
		break;
	} // switch
} // enter

//////////////////////////////////////////////////////////////////////
// locates identifier in symbol table.
int position(char* id)
{
	int i;
	strcpy(sym_tab[0].name, id);
	i = sym_num + 1;
	while (strcmp(sym_tab[--i].name, id) != 0);
	return i;
} // position

// locates identifier in current block.
int locate(char* id, int proc)
{
	proc_attr *p;
	int i;
	p = sym_tab[proc].attr;
	if (sym_tab[proc].kind != ID_PROCEDURE)
		error(26); // Error: Can not locate identifier in a non-procedure block.
	if (!p->active)
		return 0;
	for (i = proc + 1; i <= sym_num; i++)
	{
		if (sym_tab[i].level > sym_tab[proc].level + 1)
			break;
		if (strcmp(sym_tab[i].name, id) == 0)
			return i;
	}
	return 0;
} // locate

//////////////////////////////////////////////////////////////////////
void constdeclaration()
{
	if (sym == SYM_IDENTIFIER)
	{
		getsym();
		if (sym == SYM_EQU || sym == SYM_BECOMES)
		{
			if (sym == SYM_BECOMES)
				error(1); // Found ':=' when expecting '='.
			getsym();
			if (sym == SYM_NUMBER)
			{
				enter(ID_CONSTANT, NULL);
				getsym();
			}
			else
			{
				error(2); // There must be a number to follow '='.
			}
		}
		else
		{
			error(3); // There must be an '=' to follow the identifier.
		}
	} else	error(4);
	 // There must be an identifier to follow 'const', 'var', or 'procedure'.
} // constdeclaration

/**
 * This function determines the type of a variable based on its declaration.
 * It takes two pointers, `l` and `r`, which represent the left and right boundaries of the declaration.
 * The function first checks if the declaration is enclosed in parentheses and removes them if present.
 * Then it checks if the declaration is an array by looking for the right square bracket symbol.
 * If it is an array, it validates the array declaration syntax and creates a new array type.
 * If it is a pointer, it creates a new pointer type.
 * If it is a simple identifier, it returns the default integer type.
 * If the declaration is invalid or missing an identifier, it throws an error and returns NULL.
 *
 * @param l A pointer to the left boundary of the declaration.
 * @param r A pointer to the right boundary of the declaration.
 * @return A pointer to the determined type of the variable, or NULL if the declaration is invalid.
 */
type *var_type(int *l, int *r, type *t)
{
	while(*l == SYM_LPAREN && *(r-1) == SYM_RPAREN){
		l++;
		r--;
	}
	if(r-l < 1){
		error(28);
		return NULL;
	}
	if(*l == SYM_TIMES){
		return var_type(l+1, r, new_type(T_PTR, t, 0));
	}
	if(*(r-1) == SYM_RMIDPAREN){
		if (r-l<4 || *(r-2)!=SYM_NUMBER || *(r-3)!=SYM_LMIDPAREN) {
			error(28);
			return NULL;
		}
		return var_type(l, r-3, new_type(T_ARR, t, num_sym_val[r-2-all_sym]));
	}
	if(r-l != 1 || *l != SYM_IDENTIFIER){
		error(28);
		return NULL;
	}
	return t;
}



//////////////////////////////////////////////////////////////////////
void vardeclaration()
{
	int l, r;
	l = all_sym_num;
	while(sym != SYM_SEMICOLON && sym != SYM_COMMA)
		getsym();
	r = all_sym_num;
	enter(ID_VARIABLE, var_type(all_sym+l, all_sym+r, &int_type));
} // vardeclaration

//////////////////////////////////////////////////////////////////////
void listcode(int from, int to)
{
	int i;
	
	printf("\n");
	for (i = from; i < to; i++)
	{
		printf("%5d %s\t%d\t%d\n", i, mnemonic[code[i].f], code[i].l, code[i].a);
	}
	printf("\n");
} // listcode

//////////////////////////////////////////////////////////////////////
/*
prim_scope -> ident
			  | prim_scope::ident
*/
//未完成
type *prim_scope(symset fsys, int proc)
{
	type *t;
	int i;
	const_attr *c;
	var_attr *v;
	proc_attr *p;
	if (sym == SYM_IDENTIFIER)
	{
		if (proc)
			i = locate(id, proc);
		else
			i = position(id);
		if (!i)
			error(11); // Undeclared identifier.
	}
	else
		error(31);
	getsym();
	if (sym == SYM_SCOPE)
	{
		getsym();
		t = prim_scope(fsys, i);
	}
	else
	{
		switch (sym_tab[i].kind)
		{
		case ID_CONSTANT:
			c = sym_tab[i].attr;
			gen(LIT, 0, -1); // address placeholder
			gen(LIT, 0, *c); // value
			t = &int_type;
			break;
		case ID_VARIABLE:
			v = sym_tab[i].attr;
			t = copy_type(sym_tab[i].type);
			gen(LEA, level - sym_tab[i].level, *v); // address
			if (t->tag == T_ARR)
				gen(LEA, level - sym_tab[i].level, *v); // value of array is address
			else
				gen(LOD, level - sym_tab[i].level, *v); // value of variable and pointer is value
			break;
		case ID_PROCEDURE:
			error(21);
			break;
		} // switch
	}
	return t;
}

//////////////////////////////////////////////////////////////////////
/*
scope -> prim_scope
		 | ::prim_scope
*/
type *scope(symset fsys)
{
	type *t;
	int i;
	if (sym == SYM_SCOPE)
	{
		getsym();
		t = prim_scope(fsys, 1);
	}
	else
	{
		t = prim_scope(fsys, 0);
	}
	return t;
}

//////////////////////////////////////////////////////////////////////
/*
fact -> scope
		| number 
		| -fact 
		| (expr)
*/
type *factor(symset fsys)
{
	int i;
	symset set;
	type *t;
	
	test(facbegsys, fsys, 24); // The symbol can not be as the beginning of an expression.

	if (inset(sym, facbegsys))
	{
		if (sym == SYM_IDENTIFIER || sym == SYM_SCOPE)
		{
			t = scope(fsys);
		}
		else if (sym == SYM_NUMBER)
		{
			if (num > MAXADDRESS)
			{
				error(25); // The number is too great.
				num = 0;
			}
			gen(LIT, 0, -1); // address placeholder
			gen(LIT, 0, num); // value
			getsym();
			t = &int_type;
		}
		else if (sym == SYM_LPAREN)
		{
			getsym();
			set = uniteset(createset(SYM_RPAREN, SYM_NULL), fsys);
			t = expression(set, 0);
			gen(LIT, 0, -1); // address placeholder
			gen(OPR, 0, OPR_SWP); // swap value and address
			destroyset(set);
			if (sym == SYM_RPAREN)
			{
				getsym();
			}
			else
			{
				error(22); // Missing ')'.
			}
		}
		else if(sym == SYM_MINUS) // UMINUS,  Expr -> '-' Expr
		{  
			getsym();
			t = factor(fsys);
			if (t->tag != T_INT)
				error(34);
			gen(OPR, 0, OPR_NEG);
		}
		test(fsys, createset(SYM_LPAREN, SYM_NULL), 23);
	} // if
	return t;
} // factor

//////////////////////////////////////////////////////////////////////
/*
array_term -> fact
			  | array_term[expr]
*/
type *array_term(symset fsys, int cal_addr)
{
	type *t1, *t2;
	symset set;
	set = uniteset(fsys, createset(SYM_LMIDPAREN, SYM_NULL));
	t1 = factor(set);
	// top is value, top-1 is address
	if (sym != SYM_LMIDPAREN && cal_addr)
		t1 = new_type(T_PTR, t1, 0);
	else
		gen(OPR, 0, OPR_SWP);
	gen(OPR, 0, OPR_POP);
	while (sym == SYM_LMIDPAREN) {
		getsym();
		t2 = expression(createset(SYM_RMIDPAREN, SYM_NULL), 0);
		getsym();
		if (t1->tag == T_ARR)
			t1->tag = T_PTR;
		if (t2->tag == T_ARR)
			t2->tag = T_PTR;
		if (t1->tag == T_PTR) {
			if (t2->tag == T_PTR) {
				// ptr[ptr]
				error(35);
			} else {
				// ptr[int]
				gen(LIT, 0, type_size(t1->inner_type));
				gen(OPR, 0, OPR_MUL);
				gen(OPR, 0, OPR_ADD);
				// if cal_addr, do not resolve address in last dimension
				if (!cal_addr || sym == SYM_LMIDPAREN) {
					if (t1->inner_type->tag != T_ARR)
						gen(LDA, 0, 0);
					t1 = t1->inner_type;
				}
			}
		} else {
			if (t2->tag == T_PTR) {
				// int[ptr]
				gen(OPR, 0, OPR_SWP);
				gen(LIT, 0, type_size(t2->inner_type));
				gen(OPR, 0, OPR_MUL);
				gen(OPR, 0, OPR_ADD);
				t1 = t2;
				if (!cal_addr || sym == SYM_LMIDPAREN) {
					if (t1->inner_type->tag != T_ARR)
						gen(LDA, 0, 0);
					t1 = t1->inner_type;
				}
			} else {
				// int[int]
				error(34);
			}
		}
	}
	destroyset(set);
	//debug
	// printf("Arr end; sym: %d; type: ", sym);print_type(t1);printf("\n");
	return t1;
}

//////////////////////////////////////////////////////////////////////
/*
unary_term -> array_term
			  | &unary_term
			  | *unary_term
*/
type *unary_term(symset fsys, int cal_addr)
{
	type *t;
	if (sym == SYM_ADDRESS) {
		getsym();
		if (cal_addr)
			error(37);
		t = unary_term(fsys, 1);
	} else if (sym == SYM_TIMES) {
		getsym();
		t = unary_term(fsys, 0);
		if (t->tag != T_PTR && t->tag != T_ARR)
			error(34);
		if (!cal_addr) {
			if (t->inner_type->tag != T_ARR)
				gen(LDA, 0, 0);
			t = t->inner_type;
		}
	} else {
		t = array_term(fsys, cal_addr);
	}
	return t;
}

//////////////////////////////////////////////////////////////////////
/*
term ->  unary_term 
		| term*unary_term 
		| term/unary_term
*/
type *term(symset fsys, int cal_addr)
{
	int mulop;
	symset set;
	type *t1,*t2;
	
	set = uniteset(fsys, createset(SYM_TIMES, SYM_SLASH, SYM_NULL));
	t1 = unary_term(set, cal_addr);
	while (sym == SYM_TIMES || sym == SYM_SLASH)
	{
		mulop = sym;
		getsym();
		if (cal_addr)
			error(37);
		t2 = unary_term(set, cal_addr);
		if (t1->tag != T_ARR || t2->tag != T_ARR)
			error(34);
		if (mulop == SYM_TIMES)
		{
			gen(OPR, 0, OPR_MUL);
		}
		else
		{
			gen(OPR, 0, OPR_DIV);
		}
	} // while
	destroyset(set);
	return t1;
} // term

//////////////////////////////////////////////////////////////////////
/*
expr ->  term
		| expr+term
    	| expr-term
*/
// cal_addr 为1时计算地址，为0时计算值
type *expression(symset fsys, int cal_addr)
{
	int addop;
	symset set;
	type *t1,*t2;

	set = uniteset(fsys, createset(SYM_PLUS, SYM_MINUS, SYM_NULL));
	
	t1 = term(set, cal_addr);
	while (sym == SYM_PLUS || sym == SYM_MINUS)
	{
		addop = sym;
		getsym();
		if (cal_addr)
			error(37);
		t2 = term(set, cal_addr);
		if (t1->tag == T_ARR)
			t1->tag = T_PTR;
		if (t2->tag == T_ARR)
			t2->tag = T_PTR;
		if (t1->tag == T_PTR) {
			if (t2->tag == T_PTR) {
				// ptr+ptr, ptr-ptr
				if (addop == SYM_PLUS)
					error(35);
				if (!type_equal(t1->inner_type, t2->inner_type))
					error(34);
				gen(OPR, 0, OPR_MIN);
			} else {
				// ptr+int, ptr-int
				gen(LIT, 0, type_size(t1->inner_type));
				gen(OPR, 0, OPR_MUL);
				if (addop == SYM_PLUS)
					gen(OPR, 0, OPR_ADD);
				else
					gen(OPR, 0, OPR_MIN);
			}
		} else {
			if (t2->tag == T_PTR) {
				// int+ptr, int-ptr
				if (addop == SYM_MINUS)
					error(34);
				gen(OPR, 0, OPR_SWP);
				gen(LIT, 0, type_size(t2->inner_type));
				gen(OPR, 0, OPR_MUL);
				gen(OPR, 0, OPR_ADD);
				t1 = t2;
			} else {
				// int+int, int-int
				if (addop == SYM_PLUS)
					gen(OPR, 0, OPR_ADD);
				else
					gen(OPR, 0, OPR_MIN);
			}
		}
	} // while

	destroyset(set);
	return t1;
} // expression

//////////////////////////////////////////////////////////////////////
void condition(symset fsys)
{
	int relop;
	symset set;

	if (sym == SYM_ODD)
	{
		getsym();
		expression(fsys, 0);
		gen(OPR, 0, 6);
	}
	else
	{
		set = uniteset(relset, fsys);
		expression(set, 0);
		destroyset(set);
		if (! inset(sym, relset))
		{
			error(20); // Relational operator expected.
		}
		else
		{
			relop = sym;
			getsym();
			expression(fsys, 0);
			switch (relop)
			{
			case SYM_EQU:
				gen(OPR, 0, OPR_EQU);
				break;
			case SYM_NEQ:
				gen(OPR, 0, OPR_NEQ);
				break;
			case SYM_LES:
				gen(OPR, 0, OPR_LES);
				break;
			case SYM_GEQ:
				gen(OPR, 0, OPR_GEQ);
				break;
			case SYM_GTR:
				gen(OPR, 0, OPR_GTR);
				break;
			case SYM_LEQ:
				gen(OPR, 0, OPR_LEQ);
				break;
			} // switch
		} // else
	} // else
} // condition

//////////////////////////////////////////////////////////////////////
void statement(symset fsys)
{
	int i, cx1, cx2;
	symset set1, set;
	sym_attr *s;
	proc_attr *p;

	if (sym == SYM_CALL)
	{ // procedure call
		getsym();
		if (sym != SYM_IDENTIFIER)
		{
			error(14); // There must be an identifier to follow the 'call'.
		}
		else
		{
			if (! (i = position(id)))
			{
				error(11); // Undeclared identifier.
			}
			else if (sym_tab[i].kind == ID_PROCEDURE)
			{
				s = &sym_tab[i];
				p = s->attr;
				gen(CAL, level - s->level, p->address);
			}
			else
			{
				error(15); // A constant or variable can not be called. 
			}
			getsym();
		}
	} 
	else if (sym == SYM_IF)
	{ // if statement
		getsym();
		set1 = createset(SYM_THEN, SYM_DO, SYM_NULL);
		set = uniteset(set1, fsys);
		condition(set);
		destroyset(set1);
		destroyset(set);
		if (sym == SYM_THEN)
		{
			getsym();
		}
		else
		{
			error(16); // 'then' expected.
		}
		cx1 = cx;
		gen(JPC, 0, 0);
		statement(fsys);
		code[cx1].a = cx;	
	}
	else if (sym == SYM_BEGIN)
	{ // block
		getsym();
		set1 = createset(SYM_SEMICOLON, SYM_END, SYM_NULL);
		set = uniteset(set1, fsys);
		statement(set);
		while (sym == SYM_SEMICOLON || inset(sym, statbegsys))
		{
			if (sym == SYM_SEMICOLON)
			{
				getsym();
			}
			else
			{
				error(10); // ';' expected.
			}
			statement(set);
		} // while
		destroyset(set1);
		destroyset(set);
		if (sym == SYM_END)
		{
			getsym();
		}
		else
		{
			error(17); // ';' or 'end' expected.
		}
	}
	else if (sym == SYM_WHILE)
	{ // while statement
		cx1 = cx;
		getsym();
		set1 = createset(SYM_DO, SYM_NULL);
		set = uniteset(set1, fsys);
		condition(set);
		destroyset(set1);
		destroyset(set);
		cx2 = cx;
		gen(JPC, 0, 0);
		if (sym == SYM_DO)
		{
			getsym();
		}
		else
		{
			error(18); // 'do' expected.
		}
		statement(fsys);
		gen(JMP, 0, cx1);
		code[cx2].a = cx;
	}
	else if (sym == SYM_PRINT)
	{ // print statement
		getsym();
		if (sym != SYM_LPAREN)
		{
			error(22); // Missing '('.
		}
		do
		{
			getsym();
			set1 = createset(SYM_RPAREN, SYM_COMMA, SYM_NULL);
			set = uniteset(set1, fsys);
			expression(set, 0);
			destroyset(set1);
			destroyset(set);
			gen(PRT, 0, sym == SYM_COMMA ? ' ' : '\n');
		} while (sym == SYM_COMMA);
		if (sym == SYM_RPAREN)
		{
			getsym();
		}
		else
		{
			error(22); // Missing ')'.
		}
	} else if (sym == SYM_END) {
		// do nothing
	} else { // variable assignment
		type *t1, *t2;
		set1 = createset(SYM_BECOMES, SYM_NULL);
		set = uniteset(set1, fsys);
		t1 = expression(set, 1);
		if (t1->tag != T_PTR && t1->tag != T_ARR)
			error(37);
		if (sym != SYM_BECOMES)
			error(13);
		getsym();
		t2 = expression(fsys, 0);
		if (!type_compatible(t1->inner_type, t2))
			error(12);
		gen(STA, 0, 0);
	}
	test(fsys, phi, 19);
} // statement
			
//////////////////////////////////////////////////////////////////////
void block(symset fsys)
{
	int cx0; // initial code index
	int block_dx;
	int savedSn; // saved procedure symbol number
	symset set1, set;
	proc_attr *p, *q;

	dx = 3; // data allocation index
	block_dx = dx; // useless? because dx will be reseted by next block_dx = dx;
	savedSn = sym_num;
	p = sym_tab[sym_num].attr;
	p->address = cx;
	// first instruction is JMP, jump to the main program
	gen(JMP, 0, 0); // address placeholder, will be backpatched later
	if (level > MAXLEVEL)
	{
		error(32); // There are too many levels.
	}
	do
	{
		if (sym == SYM_CONST)
		{ // constant declarations
			getsym();
			do
			{
				constdeclaration();
				while (sym == SYM_COMMA)
				{
					getsym();
					constdeclaration();
				}
				if (sym == SYM_SEMICOLON)
				{
					getsym();
				}
				else
				{
					error(5); // Missing ',' or ';'.
				}
			}
			while (sym == SYM_IDENTIFIER);
		} // if

		while (sym == SYM_VAR)
		{ // variable declarations
			do
			{
				getsym();
				vardeclaration();
			} while (sym == SYM_COMMA);
			if (sym == SYM_SEMICOLON)
				getsym();
			else
				error(5); // Missing ',' or ';'.
		} // if
		block_dx = dx; //save dx before handling procedure call!
		while (sym == SYM_PROCEDURE)
		{ // procedure declarations
			getsym();
			if (sym == SYM_IDENTIFIER)
			{
				enter(ID_PROCEDURE, NULL);
				getsym();
			}
			else
			{
				error(4); // There must be an identifier to follow 'const', 'var', or 'procedure'.
			}


			if (sym == SYM_SEMICOLON)
			{
				getsym();
			}
			else
			{
				error(5); // Missing ',' or ';'.
			}

			level++;
			savedSn = sym_num;
			set1 = createset(SYM_SEMICOLON, SYM_NULL);
			set = uniteset(set1, fsys);
			block(set);
			destroyset(set1);
			destroyset(set);
			sym_num = savedSn;
			q = sym_tab[sym_num].attr;
			q->active = 0;
			level--;

			if (sym == SYM_SEMICOLON)
			{
				getsym();
				set1 = createset(SYM_IDENTIFIER, SYM_PROCEDURE, SYM_NULL);
				set = uniteset(statbegsys, set1);
				test(set, fsys, 6);
				destroyset(set1);
				destroyset(set);
			}
			else
			{
				error(5); // Missing ',' or ';'.
			}
		} // while
		dx = block_dx; //restore dx after handling procedure call!
		set1 = createset(SYM_IDENTIFIER, SYM_NULL);
		set = uniteset(statbegsys, set1);
		test(set, declbegsys, 7);
		destroyset(set1);
		destroyset(set);
	}
	while (inset(sym, declbegsys));

	code[p->address].a = cx; // backpatch jump address of procedure
	p->address = cx; // procedure address, is index of instruction INT
	cx0 = cx;
	gen(INT, 0, block_dx); // allocate space for this procedure
	set1 = createset(SYM_SEMICOLON, SYM_END, SYM_NULL);
	set = uniteset(set1, fsys);
	statement(set); // compile statement
	destroyset(set1);
	destroyset(set);
	gen(OPR, 0, OPR_RET); // return
	test(fsys, phi, 8); // test for error: Follow the statement is an incorrect symbol.
	listcode(cx0, cx);
} // block

//////////////////////////////////////////////////////////////////////
int base(int stack[], int currentLevel, int levelDiff)
{
	int b = currentLevel;
	
	while (levelDiff--)
		b = stack[b];
	return b;
} // base

//////////////////////////////////////////////////////////////////////
// interprets and executes codes.
void interpret()
{
	int pc;        // program counter
	int stack[STACKSIZE];
	int top;       // top of stack
	int b;         // program, base, and top-stack register
	instruction i; // instruction register

	printf("Begin executing PL/0 program.\n");

	pc = 0;
	b = 1;
	top = 3;
	stack[1] = stack[2] = stack[3] = 0;
	do
	{
		i = code[pc++];
		switch (i.f)
		{
		case LIT:
			stack[++top] = i.a;
			break;
		case OPR:
			switch (i.a) // operator
			{
			case OPR_RET:
				top = b - 1;
				pc = stack[top + 3];
				b = stack[top + 2];
				break;
			case OPR_NEG:
				stack[top] = -stack[top];
				break;
			case OPR_ADD:
				top--;
				stack[top] += stack[top + 1];
				break;
			case OPR_MIN:
				top--;
				stack[top] -= stack[top + 1];
				break;
			case OPR_MUL:
				top--;
				stack[top] *= stack[top + 1];
				break;
			case OPR_DIV:
				top--;
				if (stack[top + 1] == 0)
				{
					fprintf(stderr, "Runtime Error: Divided by zero.\n");
					fprintf(stderr, "Program terminated.\n");
					continue;
				}
				stack[top] /= stack[top + 1];
				break;
			case OPR_ODD:
				stack[top] %= 2;
				break;
			case OPR_EQU:
				top--;
				stack[top] = stack[top] == stack[top + 1];
				break;
			case OPR_NEQ:
				top--;
				stack[top] = stack[top] != stack[top + 1];
				break;
			case OPR_LES:
				top--;
				stack[top] = stack[top] < stack[top + 1];
				break;
			case OPR_GEQ:
				top--;
				stack[top] = stack[top] >= stack[top + 1];
				break;
			case OPR_GTR:
				top--;
				stack[top] = stack[top] > stack[top + 1];
				break;
			case OPR_LEQ:
				top--;
				stack[top] = stack[top] <= stack[top + 1];
				break;
			case OPR_SWP:
				// swap top two elements on stack
				int tmp;
				tmp = stack[top];
				stack[top] = stack[top - 1];
				stack[top - 1] = tmp;
				break;
			case OPR_POP:
				top--;
				break;
			} // switch
			break;
		case LOD:
			stack[++top] = stack[base(stack, b, i.l) + i.a];
			break;
		case STO:
			stack[base(stack, b, i.l) + i.a] = stack[top];
			// printf("%d\n", stack[top]);
			top--;
			break;
		/**
		 * Executes the CAL instruction.
		 * 
		 * This instruction is used to call a procedure or function. It generates a new block mark,
		 * updates the base pointer, and sets the program counter to the address specified in the instruction.
		 * 
		 * @param stack The stack used for execution.
		 * @param b The base pointer.
		 * @param l The address specified in the instruction.
		 */
		case CAL:
			stack[top + 1] = base(stack, b, i.l);
			// generate new block mark
			stack[top + 2] = b;
			stack[top + 3] = pc;
			b = top + 1;
			pc = i.a;
			break;

		/**
		 * Executes the INT instruction.
		 * 
		 * This instruction is used to increment the top pointer of the stack by a given value.
		 * 
		 * @param stack The stack used for execution.
		 * @param a The value to increment the top pointer by.
		 */
		case INT:
			top += i.a;
			break;

		/**
		 * Executes the JMP instruction.
		 * 
		 * This instruction is used to set the program counter to the address specified in the instruction.
		 * 
		 * @param a The address specified in the instruction.
		 */
		case JMP:
			pc = i.a;
			break;

		/**
		 * Executes the JPC instruction.
		 * 
		 * This instruction is used to conditionally set the program counter to the address specified in the instruction.
		 * If the value at the top of the stack is 0, the program counter is set to the address; otherwise, it continues to the next instruction.
		 * 
		 * @param stack The stack used for execution.
		 * @param a The address specified in the instruction.
		 */
		case JPC:
			if (stack[top] == 0)
				pc = i.a;
			top--;
			break;
		case PRT: // print and pop stack top
			printf("%d", stack[top]);
			if (i.a)
				putchar(i.a);
			top--;
			break;
		case STA: //(STA,--,--),将位于栈顶单元的内容，存入到次栈顶单元内容所代表的栈单元里，然后弹出栈顶和次栈顶
			//debug
			// printf("stack[%d]=%d\n", stack[top - 1], stack[top]);
			stack[stack[top - 1]] = stack[top];
			top = top - 2;
			break;
		case LEA: //(LEA,层次差,相对偏移),取绝对地址压入栈顶
			//debug
			// printf("push(%d)\n", base(stack, b, i.l) + i.a);

			stack[++top] = base(stack, b, i.l) + i.a;
			break;
		case LDA: //(LDA,--,--),以当前栈顶单元的内容为地址来读取相应单元的值，并将该值存储到原先的栈顶单元中
			//debug
			// printf("stack[top] -> stack[%d]=%d\n", stack[top], stack[stack[top]]);

			stack[top] = stack[stack[top]];
			break;
		} // switch
	}
	while (pc);

	printf("End executing PL/0 program.\n");
} // interpret

//////////////////////////////////////////////////////////////////////
void main ()
{
	FILE* hbin;
	char s[80];
	int i;
	symset set, set1, set2;

	printf("Please input source file name: "); // get file name to be compiled
	scanf("%s", s);
	if ((infile = fopen(s, "r")) == NULL)
	{
		printf("File %s can't be opened.\n", s);
		exit(1);
	}

	phi = createset(SYM_NULL);
	relset = createset(SYM_EQU, SYM_NEQ, SYM_LES, SYM_LEQ, SYM_GTR, SYM_GEQ, SYM_NULL);
	
	// create begin symbol sets
	declbegsys = createset(SYM_CONST, SYM_VAR, SYM_PROCEDURE, SYM_NULL);
	statbegsys = createset(SYM_BEGIN, SYM_CALL, SYM_IF, SYM_WHILE, SYM_PRINT, SYM_NULL);
	facbegsys = createset(SYM_IDENTIFIER, SYM_NUMBER, SYM_LPAREN, SYM_MINUS, SYM_SCOPE, SYM_NULL);

	err = cc = cx = ll = 0; // initialize global variables
	ch = ' ';
	kk = MAXIDLEN;

	getsym();

	set1 = createset(SYM_PERIOD, SYM_NULL);
	set2 = uniteset(declbegsys, statbegsys);
	set = uniteset(set1, set2);
	id[0] = 0; // name of main program is empty string ""
	enter(ID_PROCEDURE, NULL); // enter the main program into 
	sym_tab[1].level = -1; // set the level of main program to -1
	block(set);
	destroyset(set1);
	destroyset(set2);
	destroyset(set);
	destroyset(phi);
	destroyset(relset);
	destroyset(declbegsys);
	destroyset(statbegsys);
	destroyset(facbegsys);

	if (sym != SYM_PERIOD)
		error(9); // '.' expected.
	if (err == 0)
	{
		hbin = fopen("hbin.txt", "w");
		for (i = 0; i < cx; i++)
			fwrite(&code[i], sizeof(instruction), 1, hbin);
		fclose(hbin);
	}
	if (err == 0)
		interpret();
	else
		printf("There are %d error(s) in PL/0 program.\n", err);
	listcode(0, cx);
} // main

//////////////////////////////////////////////////////////////////////
// eof pl0.c
