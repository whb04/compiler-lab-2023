# compiler-lab-2023

**进度**

- [x] 实现 print
- [x] 实现数组与指针
  - [x] 数组与指针的声明
  - [x] 类型推导
  - [x] 指针相关运算
  - [x] 数组相关运算
  - [x] 运算及赋值的类型检查
- [x] 实现作用域

实验要求及汇报ppt见 `/doc` 目录

问题与改进建议

- 取地址运算的设计有缺陷，现有的设计会在处理 `fact -> (expr)` 时丢失地址。可以修改栈的结构，使其同时存储表达式的值和地址。

- 处理数组声明可以使用流式处理，只需实现 `replace_inner_type`
- 词法分析、文法分析、代码生成、代码解释混杂在 `pl0.c` 中，可以写在多个文件中

---



### 指针与数组

#### Lexical Analysis
添加符号` SYM_ADDRESS`代表取地址符`& `

指针算符与乘法运算符相同

添加符号 `SYM_LMIDPAREN` 代表 `[`, `SYM_RMIDPAREN` 代表 `]`

#### Parsing

声明指针与数组的文法:

```
var_dec -> var arrs;
arrs -> arr | arrs,arr
arr -> ptr | arr[size]
ptr -> id | *ptr | (arr)
```

含指针和数组的表达式的文法:

```
expr -> term | expr+term | expr-term
term -> unary | term*unary | term/unary
unary -> arr | &unary | *unary
arr -> fact | arr[expr]
fact -> scope | num | -fact | (expr)
scope -> pscope | ::pscope
pscope -> id | pscope::id
```

赋值操作的文法:

```
assign -> lexpr:=expr
```

其中 lexpr 文法同 expr, 但运行时计算的不是值而是地址

#### Semantic Analysis

声明的文法不是 LL(1) 文法，故存储整个 var_dec 进行类型推导，从两侧向中间解析，直至遇到标识符时解析完成。详见 `var_type`

`*`, `&`, `[]` 运算及类型检查详见文法名对应的函数

`&` 运算的设计缺陷已在上文提及

#### Code Generation

详见代码

### 作用域

#### Lexical Analysis

添加符号 `SYM_SCOPE` 代表 `::`

#### Parsing

见表达式文法中的 scope

#### Semantic Analysis

过程的(声明)嵌套关系可视为一棵树

一个 scope 若不以 `::` 开头, 则第一个符号在树的当前过程及其祖先中找(优先在近的祖先中找), 见 `position`

若以 `::` 开头, 则在 main 的直接子树中找.

后续的 `::` 都在之前找到的符号(必然是过程符号)的直接子树中找, 见 `locate`

注意到符号表是动态的, 在离开一个过程的声明范围后其中的符号随即销毁, 因此实现 `position` 可以直接倒序遍历符号表.

#### Code Generation

作用域解析在编译期完成, 不生成代码.

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
