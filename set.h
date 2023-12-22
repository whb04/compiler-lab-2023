#ifndef SET_H
#define SET_H

typedef struct snode
{
	int elem;
	struct snode* next;
} snode, *symset;

/**
 * @brief Declaration of symbol sets.
 * 
 * This file declares the following symbol sets:
 * - phi: an empty symbol set
 * - declbegsys: symbol set for the beginning of a declaration
 * - statbegsys: symbol set for the beginning of a statement
 * - facbegsys: symbol set for the beginning of a factor
 * - relset: symbol set for relational operators
 */
symset phi, declbegsys, statbegsys, facbegsys, relset;

symset createset(int data, .../* SYM_NULL */);
void destroyset(symset s);
symset uniteset(symset s1, symset s2);
int inset(int elem, symset s);

#endif
// EOF set.h
