# compiler-lab-2023

**进度**

- [x] 实现 print
- [ ] 实现数组
- [ ] 实现指针
- [ ] 实现作用域
- [ ] *(其他，请输入文字）*



---

### 一个可能的综合指针、数组、：：的文法

```
expr ->  term
		| expr+term
    	| expr-term
term ->  unary_term 
		| term*unary_term 
		| term/unary_term
unary_term -> array_term
			  | &unary_term
			  | *unary_term
array_term -> fact
			  | array_term[expr]
fact -> scope
		| number 
		| -fact 
		| (expr)
scope -> S 
		| ::S
S -> id 
	| S::id
```

### 指针与数组声明的文法

#### 可以实现复杂的指针与数组嵌套的版本

复杂的指针与数组嵌套指类似var * (*q[10])[10]这样的。

```
block -> const_declaration block
		 | variable_declaration block
		 | procedure_declaration block
		 | statement
variable_declaration -> "var" declarator
declarator -> direct_declarator
			  | pointer direct_declarator
pointer -> *
		   | *pointer
direct_declarator -> ident
					 | direct_declarator[number]
					 | (declarator)
```

#### 简化的版本

只需删去最后一行。可以实现实验文档中所有的例子。

```
block -> const_declaration block
		 | variable_declaration block
		 | procedure_declaration block
		 | statement
variable_declaration -> "var" declarator
declarator -> direct_declarator
			  | pointer direct_declarator
pointer -> *
		   | *pointer
direct_declarator -> ident
					 | direct_declarator[number]
```

### 指针

#### Lexical Analysis
添加符号` SYM_ADDRESS`代表取地址符`& `

指针算符与乘法运算符相同

*To do...*

#### Parsing

*To do...*

#### Semantic Analysis

*To do...*

#### Code Generation

*To do...*

### 数组

#### Lexical Analysis

*To do...*

#### Parsing

定义数组的文法：

```
S -> var arrs;
arrs -> arr | arrs,arr
arr -> ptr | arr[size]
ptr -> id | *ptr
```

*To do...*

#### Semantic Analysis

*To do...*

#### Code Generation

*To do...*

### 作用域

#### Lexical Analysis

添加符号 `SYM_SCOPE` 代表 `::`

#### Parsing

计划再分解 factor 到 word

```
expr -> term | expr+term | expr-term
term -> fact | term*fact | term/fact
fact -> word | number | -fact | (expr)
word -> ident | word::ident
```

#### Semantic Analysis

*To do...*

#### Code Generation

*To do...*

### 输出

#### Lexical Analysis

添加符号 `SYM_PRINT` 代表 `print`

`print` 为保留字，用于输出若干个表达式

#### Parsing

修改语句(statement)的文法, 新增

```
stat -> print(exprs)
exprs -> expr | expr,exprs
```

#### Semantic Analysis

依次处理括号中的表达式，生成将值放到栈顶的代码，然后生成 `PRT` 指令以输出

一个 print 语句中的表达式输出时以空格隔开，结尾输出换行

#### Code Generation

添加指令 `PRT(c)` , 输出栈顶的值和一个字符 `c`

生成代码为 `gen(PRT, 0, c)`

同时移除原指令集中 `STO` 中的 printf 语句
