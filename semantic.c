# include <stdio.h>
# include <string.h>
# include <stdarg.h>
# include <stdlib.h>
# include "semantic.h"
# include "parser.h"

/*
TODO list
完全没有实现
错误类型8：return语句的返回类型与函数定义的返回类型不匹配 ————已改
错误类型9：过程调用时实参与形参的数目或类型不匹配————已改
错误类型14：访问结构体中未定义过的域。 ————已改
错误类型17：直接使用未定义过的结构体来定义变量。 ————已改


实现了一部分
错误类型 7（操作数不匹配）：要求是“不能相互赋值或运算”。现在只检查了 BASIC 类型是否相同————已改
错误类型 5 & 6（左值与赋值）：目前的 _Exp 只检查了 INT 和 FLOAT 常量。————已改
假设 2（if/while 条件必须为 int）：在 _Stmt 的 case 5 和 case 7 中。检查 _Exp(root->child[2]) 返回的类型是否为 BASIC 且是 INT。————已改

最好将所有报错信息之后都加一条return NULL————已改
*/

// 记录当前正在分析的函数的返回类型
type current_func_return_type = NULL;
// 设定头指针
symbol_node head = NULL;

// 创建变量表
static inline symbol_node createSymbolNode(){
    symbol_node pointer = (symbol_node)malloc(sizeof(struct _SymbolNode));
    pointer->name = (char*)malloc(35); // 直接分配 35 字节
    pointer->symbolType = NULL;
    pointer->next = NULL;
    return pointer;
}

// 初始化变量表
static inline void _initSymbolList(){
    head = createSymbolNode();
}

// 打印变量表
// static inline void _printSymbolList(symbol_node head){
//     head = head->next;
//     while (head){
//         printf("Symbol: %s\n", head->name);
//         head = head->next;
//     }    
// }

// 检查变量表中是否有相关数据，有则返回对应指针，没有返回 NULL
static inline symbol_node _findRecord(symbol_node head, symbol_node node){
    symbol_node p = head->next;
    while (p){
        if (!strcmp(p->name, node->name)){
            return p;
        }
        p = p->next;
    }
    return NULL;
}

// 向符号表中添加记录
static inline void _addRecord(symbol_node sym_record){
    // 头插法添加
    sym_record->next = head->next;
    head->next = sym_record;
}

//外部定义分析 int a
//TODO：结构体类型没有正确加入表
static inline void _ExtDef(Node root){
    // 对全局定义的符号的分析
    type t = _Specifier(root->child[0]);
    if (t == NULL) return;
    // 如果是结构体类型，特别对待一下
    // 结构体就单独处理了，不把它和其他混在一起，要不结构体内的变量也被添加到变量表里
    if (t->kind == STRUCTURE){
        symbol_node temp = createSymbolNode();
        temp->name = root->child[0]->name;
        // 这里要判断是不是结构体初次定义，如果是的话不能报错
        // 这里直接判断前面有没有相关结构体定义，儿子数不是 5 说明不是定义的结构体
        if (!_findRecord(head, temp) && root->child[0]->child[0]->num_child != 5){
            fault = 1;
            printf("Error type 17 at Line %d: variables \"%s\" cannot be defined using undefined structs.\n", root->child[0]->line, root->child[1]->child[0]->child[0]->child[0]->ID_NAME);
            return;
        }
    } else if (!strcmp(root->child[1]->name, "ExtDecList")){
        _ExtDefList(root->child[1], t);
    } else if (!strcmp(root->child[1]->name, "FunDec")){
        _FuncDec(root->child[1], t);
        current_func_return_type = t;
        if (root->num_child == 3) { 
            _semantic(root->child[2]); 
        }
        // type return_type = _Specifier(root->child[0]);
    }
}

// // 对 CompSt 的分析
// static inline type _CompSt(Node root){
//     return _StmtList(root->child[2]);
// }

// 对 StmtList 的分析
// static inline type _StmtList(Node root){
//     while (1){
//         // 获取最后一条语句
//         if (root->child[1]->num_child != 2){
//             // 将 root 更新到 Stmt，即最后一条语句
//             if (root->child[0]->num_child == 3 && _Exp(root->child[0]->child[1])){
//                 return _Exp(root->child[0]->child[1]);
//             } else{
//                 return NULL;
//             }
//         }
//         root = root->child[1];
//     }
// }

//将当前函数名传入temp->name 调用_findRecord函数，
//_findRecord遍历全局链表head 如果没找到返回就是null
//如果没找到（没重名）就调用_addRecord将这个名传入表
// static inline void _FuncDec(Node root, type return_type){
//     symbol_node temp = createSymbolNode();
//     temp->name = root->child[0]->ID_NAME;
//     temp->symbolType = _createType(FUNCTION, 2, return_type, NULL);
//     if (_findRecord(head, temp)){
//         // 这里用的是所有符号的查找，即只要有相同的符号就不能进行命名
//         // 也可以允许和非函数的变量同名，这样需要再写一个函数
//         fault = 1;
//         printf("Error type 4 at Line %d: The function name \"%s\" is duplicated.\n", root->child[0]->line, root->child[0]->ID_NAME);
//     }
//     _addRecord(temp);
//     if (root->num_child == 4){//如果有传递的参数这个数量就是4
//         _VarList(root->child[2]);//child[2] 是整个参数列表的根节点。
//     }
// }

// 该函数遍历 Args 节点，返回一个 symbol_node 链表，每个节点的 symbolType 存储实参类型
static inline symbol_node _getArgsType(Node root) {
    if (root == NULL) return NULL;

    symbol_node head_arg = createSymbolNode();
    // 计算当前表达式 Exp 的类型
    head_arg->symbolType = _Exp(root->child[0]);

    if (root->num_child == 3) { // Args -> Exp COMMA Args
        head_arg->next = _getArgsType(root->child[2]);
    }
    return head_arg;
}







static inline void _FuncDec(Node root, type return_type){
    symbol_node temp = createSymbolNode();
    temp->name = root->child[0]->ID_NAME;
    
    // 初始参数列表设为 NULL
    symbol_node param_list = NULL;
    if (root->num_child == 4){ // 有参数 ID ( VarList )
        param_list = _VarList(root->child[2]); 
    }
    
    // 关键：将参数列表存入 FUNCTION 类型的 par_type 中
    temp->symbolType = _createType(FUNCTION, 2, return_type, param_list);
    
    if (_findRecord(head, temp)){
        fault = 1;
        printf("Error type 4 at Line %d: Redefined function \"%s\".\n", root->child[0]->line, temp->name);
    }
    _addRecord(temp);
}
// static inline void _VarList(Node root){
//     while(root->num_child != 1){
//         _ParamDec(root->child[0]);
//         root = root->child[2];
//     }
//     _ParamDec(root->child[0]);
// }
//返回参数链表的头指针
static inline symbol_node _VarList(Node root){
    symbol_node head_param = NULL;
    symbol_node last_param = NULL;

    Node curr = root;
    while(1){
        // 解析当前参数
        symbol_node p = _ParamDec(curr->child[0]);
        
        if (head_param == NULL) {
            head_param = p;
        } else {
            last_param->next = p;
        }
        last_param = p;

        if (curr->num_child == 1) break; // VarList -> ParamDec
        curr = curr->child[2]; // VarList -> ParamDec COMMA VarList
    }
    return head_param;
}
// 对 ParamDec 的分析
// static inline void _ParamDec(Node root){
//     type t = _Specifier(root->child[0]);
//     _VarDec(root->child[1], t);
// }
//返回一个填充好数据的 symbol_node
static inline symbol_node _ParamDec(Node root){
    type t = _Specifier(root->child[0]);
    // _VarDec 本质上是把变量存入全局符号表
    // 但为了 Error 9，我们需要一个该变量的副本作为函数参数信息的存储
    Node id_node = root->child[1];
    while(id_node->child[0]) id_node = id_node->child[0];
    
    symbol_node param = createSymbolNode();
    strcpy(param->name, id_node->ID_NAME);
    param->symbolType = t; // 这里简化处理，暂不考虑数组参数的维度转换
    
    // 依然调用 _VarDec，确保函数内部可以直接使用这些参数变量
    _VarDec(root->child[1], t); 
    
    return param;
}
// 由ExtDef调用，判断是全局变量后进入此函数，_VarDec用于提取名字
static inline void _ExtDefList(Node root, type var_type){
    _VarDec(root->child[0], var_type);
}

// 创建一个 type
static inline type _createType(Kind kind, int num, ...){
    type t = (type)malloc(sizeof(struct _Type));
    t->kind = kind;
    va_list tlist;
    va_start(tlist, num);
    switch(kind){
        // 处理 BASIC
        case BASIC:
            t->data.basic = va_arg(tlist, basic_type);
            break;
        // 处理 ARRAY
        case ARRAY:
            t->data.arr.size = va_arg(tlist, int);
            t->data.arr.arr_type = va_arg(tlist, type);
            break;
        // 处理 STRUCTURE
        case STRUCTURE:
           t->data.struct_pointer = va_arg(tlist, symbol_node);
           break;
        // 对函数进行创建，第一个量为返回值，第二个量为参数值
        case FUNCTION:
            t->data.func_type.return_type = va_arg(tlist, type);
            t->data.func_type.par_type = va_arg(tlist, symbol_node);
    }
    va_end(tlist);
    return t;
}
/* 深度比较两个类型是否完全一致 */
static inline int _checkTypeEqual(type t1, type t2) {
    if (t1 == NULL || t2 == NULL) return 0;
    if (t1->kind != t2->kind) return 0; // 种类不同肯定不相等

    switch (t1->kind) {
        case BASIC:
            // 检查是 INT 还是 FLOAT[cite: 6]
            return t1->data.basic == t2->data.basic;
        
        case ARRAY:
            // 数组相等要求：基类型一致（递归检查）[cite: 3]
            // 注意：CMM 语言通常不要求数组长度一致，只需基类型和维度一致
            return _checkTypeEqual(t1->data.arr.arr_type, t2->data.arr.arr_type);
            
        case STRUCTURE:
            // 结构体相等要求：成员列表完全一致（结构等价）或名字一致（名等价）
            // 在 CMM 中通常采用“结构等价”：即成员的数量、类型、顺序必须完全相同[cite: 3]
            {
                symbol_node p1 = t1->data.struct_pointer->next; // 跳过哑头节点[cite: 6]
                symbol_node p2 = t2->data.struct_pointer->next;
                while (p1 != NULL && p2 != NULL) {
                    if (!_checkTypeEqual(p1->symbolType, p2->symbolType)) return 0;
                    p1 = p1->next;
                    p2 = p2->next;
                }
                return (p1 == NULL && p2 == NULL); // 确保长度也一致
            }
            
        case FUNCTION:
            // 函数比对（虽然在本实验中较少直接比对函数类型，但为了严谨可以写上）
            // 要求返回值类型一致，且参数列表一致
            if (!_checkTypeEqual(t1->data.func_type.return_type, t2->data.func_type.return_type)) return 0;
            {
                symbol_node p1 = t1->data.func_type.par_type;
                symbol_node p2 = t2->data.func_type.par_type;
                while (p1 != NULL && p2 != NULL) {
                    if (!_checkTypeEqual(p1->symbolType, p2->symbolType)) return 0;
                    p1 = p1->next;
                    p2 = p2->next;
                }
                return (p1 == NULL && p2 == NULL);
            }
    }
    return 0;
}
//类型说明符（Specifier）
static inline type _Specifier(Node root){
    if (!strcmp(root->child[0]->name, "TYPE")){
        if (!strcmp(root->child[0]->ID_NAME, "int")) {
            return _createType(BASIC, 1, INT);
        } else if (!strcmp(root->child[0]->ID_NAME, "float")){
            return _createType(BASIC, 1, FLOAT);
        }
    } else if(!strcmp(root->child[0]->name, "StructSpecifier")) {
        Node ss = root->child[0];
        // 情况 A：这是定义 struct A { ... }
        if (ss->num_child > 2 && !strcmp(ss->child[1]->name, "OptTag")) {
            // _StructSpecifier(ss); // 执行定义逻辑，存入符号表
            // return _createType(STRUCTURE, 1, NULL); // 这里可以简化
            return _StructSpecifier(ss);
        } 
        // 情况 B：这是引用 struct A x;
        else if (!strcmp(ss->child[1]->name, "Tag")) {
            // 重点：调用 _Tag 去符号表里把真正的“档案”取出来
            symbol_node res = _Tag(ss->child[1]); 
            if (res) return res->symbolType; // 这样返回的类型里就带着成员列表了！
            return NULL; 
        }
    }
    return NULL;
}

// 解析结构体对象
static inline type _StructSpecifier(Node root) {
    if (!strcmp(root->child[1]->name, "OptTag")) {
        // --- 场景 A：结构体定义 (STRUCT OptTag LC DefList RC) ---
        
        // 1. 获取成员链表
        symbol_node fields = _DefList(root->child[3]); 
        
        // // 2. 检查成员重名 (Error 15)
        // if (_hasDuplicateName(fields)) {
        //     fault = 1;
        //     // 注意：OptTag 内部可能为空，这里打印名字要小心处理
        //     printf("Error type 15 at Line %d: Redefinition of field in struct.\n", root->child[1]->line);
        //     return NULL; 
        // }

        // 3. 创建该结构体的类型对象
        type new_type = (type)malloc(sizeof(struct _Type));
        new_type->kind = STRUCTURE;
        new_type->data.struct_pointer = fields; // 挂载成员链表

        // 4. 处理结构体自身的名字 (OptTag) 并存入符号表
        // 注意：_OptTag 是 void 类型，它只是负责把 new_type 关联到名字上
        _OptTag(root->child[1], new_type); 

        // 5. 必须返回这个新创建的类型
        return new_type; 
    } 
    else if (!strcmp(root->child[1]->name, "Tag")) {
        // --- 场景 B：结构体引用 (STRUCT Tag) ---
        
        // 1. _Tag 应该返回它在符号表中查到的节点
        symbol_node res = _Tag(root->child[1]); 
        
        if (res == NULL) {
            // Error 17: 使用了未定义的结构体名字 (通常在 _Tag 内部打印)
            return NULL;
        }
        
        // 2. 返回查到的类型
        return res->symbolType; 
    }
    
    return NULL; // 兜底返回，消除 Warning
}

// 判断是否有相同的数据
static inline int _hasDuplicateName(symbol_node head){
    if (head == NULL || head->next == NULL){
        // 如果链表为空或者只有一个节点，则不可能存在重复名称的节点
        return 0;
    }
    symbol_node current = head->next;
    // 遍历链表
    while (current != NULL) {
        symbol_node runner = current->next;
        // 在当前节点之后查找是否有相同名称的节点
        while (runner != NULL) {
            // 如果找到相同名称的节点，则返回 true
            if (!strcmp(current->name, runner->name)) {
                return 1;
            }
            runner = runner->next;
        }
        // 移动到链表的下一个节点
        current = current->next;
    }
    // 如果没有找到相同名称的节点，则返回 false
    return 0;
}


// 对 DefList 的处理
// static inline symbol_node _DefList(Node root){
//     // 获取 Def
//     symbol_node head_pointer = createSymbolNode();
//     head_pointer->next = NULL;
//     // 如果一个儿子都没有，直接返回，不然会出空指针异常
//     if (root->num_child == 0){
//         return head_pointer;
//     }
//     while (1){
//         symbol_node temp = createSymbolNode();
//         type t = _Specifier(root->child[0]->child[0]);
//         temp->symbolType = t;
//         // 如果出现结构体内就初始化的，报错
//         if(root->child[0]->child[1]->child[0]->num_child != 1){
//             fault = 1;
//             printf("Error type unknow at Line %d: The struct \"%s\" contains initialized internal variable definitions.\n", root->child[0]->line, root->child[0]->child[1]->child[0]->child[0]->child[0]->ID_NAME);
//             return NULL;
//         }
//         temp->name = root->child[0]->child[1]->child[0]->child[0]->child[0]->ID_NAME;
//         // 头插法构建
//         temp->next = head_pointer->next;
//         head_pointer->next = temp;
//         root = root->child[1];
//         if (root->num_child == 0){
//             break;
//         }
//     }
//     return head_pointer;
// }
static inline symbol_node _DefList(Node root){
    symbol_node head_pointer = createSymbolNode();
    head_pointer->next = NULL;
    if (root->num_child == 0) return head_pointer;

    Node curr_deflist = root;
    while (1){
        Node def_node = curr_deflist->child[0]; // 对应 Def
        type t = _Specifier(def_node->child[0]);
        
        // --- 修改点：不要在这里直接提取 field_name，要处理 DecList ---
        Node dec_list = def_node->child[1]; // 对应 DecList
        
        while(dec_list) {
            Node dec = dec_list->child[0]; // 对应 Dec
            
            // 找到真正的 ID 节点以获取精准行号[cite: 3]
            // 因为 Dec -> VarDec -> ... -> ID
            Node var_dec = dec->child[0];
            Node id_node = var_dec;
            while (id_node->num_child != 0) {
                id_node = id_node->child[0];
            }
            
            char* field_name = id_node->ID_NAME;
            int error_line = id_node->line; // 这里就是你要的精确行号[cite: 3]

            // --- 检查重名 ---
            symbol_node check_p = head_pointer->next;
            int is_duplicate = 0;
            while (check_p) {
                if (!strcmp(check_p->name, field_name)) {
                    fault = 1;
                    // 使用 id_node->line 而不是 def_node->line
                    printf("Error type 15 at Line %d: Redefinition of field \"%s\".\n", error_line, field_name);
                    is_duplicate = 1;
                    break;
                }
                check_p = check_p->next;
            }

            if (!is_duplicate) {
                symbol_node temp = createSymbolNode();
                temp->symbolType = t;
                strcpy(temp->name, field_name);
                
                // 结构体内初始化检查
                if(dec->num_child != 1){ // Dec -> VarDec ASSIGNOP Exp
                    fault = 1;
                    printf("Error type unknow at Line %d: Struct fields cannot be initialized.\n", error_line);
                }

                temp->next = head_pointer->next;
                head_pointer->next = temp;
            }

            // 处理同一行定义多个变量：int a, b, c;
            if (dec_list->num_child == 3) {
                dec_list = dec_list->child[2];
            } else {
                break;
            }
        }

        curr_deflist = curr_deflist->child[1];
        if (curr_deflist->num_child == 0) break;
    }
    return head_pointer;
}
// 定义结构体时调用，如果名字不为空就存入符号表，不应该返回type
static inline void _OptTag(Node root, symbol_node var){
    if (!strcmp(root->child[0]->name, "ID")){
        symbol_node temp = createSymbolNode();
        temp->name = root->child[0]->ID_NAME;
        temp->symbolType = _createType(STRUCTURE, 1, var);
        if (_findRecord(head, temp)){
            // 对结构体重复定义进行审查
            fault = 1;
            printf("Error type 16 at Line %d: The struct \"%s\" has been previously defined and cannot be redefined.\n", root->child[0]->line, root->child[0]->ID_NAME);
            return;
        }
        // 这里还没有写 structure 后面跟的属性值
        temp->symbolType->data.struct_pointer = var;
        _addRecord(temp);
    } else{
        return;
    }
}

// 对 Tag 的处理
// static inline void _Tag(Node root){
//     symbol_node temp = createSymbolNode();
//     temp->name = root->child[0]->ID_NAME;
//     temp->symbolType = _createType(STRUCTURE, 1, NULL);
//     // 这里还没有写 structure 后面跟的属性值
//     _addRecord(temp);
// }
//Tag变成一个完全的查询者，只负责查询，不存，必须返回找到了symbol_node
static inline symbol_node _Tag(Node root){
    symbol_node temp = createSymbolNode();
    temp->name = root->child[0]->ID_NAME;
    
    // 从全局表里找这个结构体的定义[cite: 3]
    symbol_node res = _findRecord(head, temp);
    if (!res || res->symbolType->kind != STRUCTURE) {
        // 这里顺便处理了错误类型 17：直接使用未定义过的结构体[cite: 2]
        fault = 1;
        printf("Error type 17 at Line %d: Undefined struct \"%s\".\n", root->child[0]->line, temp->name);
        return NULL;
    }
    return res; // 返回找到的节点，它里面存有成员信息[cite: 3]
}
// 对 DecList 的分析
static inline void _DecList(Node root, type var_type){
    // printf("已经进入DecList函数\n");
    // printf("即将进入Dec函数\n");
    _Dec(root->child[0], var_type);//处理当前遇到的第一个变量
    // 如果还有变量就递归处理
    // if (root->child[1]){ //chile[1]是逗号
    //     _DecList(root->child[2], var_type);
    // } 
    if(root->num_child == 3){ // DecList -> Dec COMMA DecList
        _DecList(root->child[2], var_type);
    }
    else {
        return;
    }
}

// 对 Dec 的分析
static inline void _Dec(Node root, type var_type){
    // printf("已经进入Dec函数\n");
    // printf("即将进入VarDec函数\n");
    _VarDec(root->child[0], var_type);
}

// 用于提取ID名字，将名字和类型一起存入符号表
static inline void _VarDec(Node root, type var_type){
    // printf("已经进入VarDec函数,VarDec负责提取变量名并构造数组类型\n"); 
    // 获取 ID 节点
    Node id = root->child[0];
    // while (id->child[0]){
    //     id = id->child[0];
    // }
    while (id->num_child > 0) {
        id = id->child[0];
    }
    // 初始化 node 节点的数据并设置
    symbol_node temp = createSymbolNode();
    strcpy(temp->name, id->ID_NAME);
    // printf("提取到变量名：%s\n", temp->name);
    // 非数组元素
    if (!strcmp(root->child[0]->name, "ID")){
        // printf("root->child[0] 的名字是 'ID'\n");
        temp->symbolType = var_type;
    } else {
        // printf("root->child[0] 的名字不是 'ID'\n");
        // 数组元素沿着child[0]一路向下，直到找到一个没有child[0]的也就是ID叶子节点
        Node vardec_node = root;
        // printf("vardec_node 代表 i[10]");
        // printf("一共有%d个子节点\n", vardec_node->num_child);
        // 获取数组的类型和 size
        while (vardec_node->num_child > 1){
            if (vardec_node->child[2] == NULL) {
                // printf("Panic: vardec_node->child[2] is NULL! Check parser tree construction.\n");
                return;
            }
            // printf("通过了第一个if检查，vardec_node->child[2] 不为 NULL\n");
            // printf("DEBUG: child[2] 的内存地址是: %p\n", (void*)vardec_node->child[2]);
            if (vardec_node->child[2]->ID_NAME == NULL) {
                // printf("Panic: INT node has NULL ID_NAME!\n");
                // 这里可以尝试改用数值字段，比如 vardec_node->child[2]->val_int
                return; 
            }
            // printf("DEBUG: Array size string is:");
            // printf(" %d\n", vardec_node->child[2]->INT_NUM);
            int array_size = vardec_node->child[2]->INT_NUM;
            // printf("DEBUG: Parsed array size is: %d\n", array_size);
            temp->symbolType = _createType(ARRAY, 2, array_size, var_type);
            vardec_node = vardec_node->child[0];
        }
    }
    // 向记录表中进行添加
    if (_findRecord(head, temp)){
        fault = 1;
        printf("Error type 3 at Line %d: Redefined variable \"%s\".\n", root->line, temp->name);
    } else {
        _addRecord(temp);
    }
}


// 局部变量定义处理
static inline void _Def(Node root){
    type Type = _Specifier(root->child[0]);
    // printf("进入_Def函数,已获取变量类型\n");
    // printf("变量类型是：%d\n", Type->data.basic);// 0: BASIC, 1: ARRAY, 2: STRUCTURE, 3: FUNCTION
    // printf("即将进入_DefList函数\n");
    _DecList(root->child[1], Type);
}
/* 判断一个 Exp 节点是否为左值 */
static inline int _isLValue(Node exp) {
    // 情况 1: Exp -> ID (变量)
    if (exp->num_child == 1 && !strcmp(exp->child[0]->name, "ID")) {
        return 1;
    }
    // 情况 2: Exp -> Exp LB Exp RB (数组访问)
    if (exp->num_child == 4 && !strcmp(exp->child[1]->name, "LB")) {
        return 1;
    }
    // 情况 3: Exp -> Exp DOT ID (结构体访问)
    if (exp->num_child == 3 && !strcmp(exp->child[1]->name, "DOT")) {
        return 1;
    }
    // 其他情况（如常量、算术表达式、函数调用）均不是左值
    return 0;
}
// 对表达式进行语义检查，返回表达式类型，若表达式有误则返回 NULL
//表达式可以是常量，变量，数组，结构体，赋值表达式，算术表达式，函数调用等
static inline type _Exp(Node root){
    // 如果是 ID, INT, FLOAT
    if (!strcmp(root->child[0]->name, "INT")){
        return _createType(BASIC, 1, INT);
    } 
    else if (!strcmp(root->child[0]->name, "FLOAT")){
        // printf("进入FLOAT分支\n");
        return _createType(BASIC, 1, FLOAT);
    } 
    else if (!strcmp(root->child[0]->name, "ID")){
        //可能是单独的变量引用，也可能是函数调用
        if (root->num_child == 1){ //单独的标识符x
            // 对 ID 的分析
            // 这里注意的是，变量表中的 node 和树中的 node 是不一样的类型
            // 所以要转换一下
            symbol_node s = createSymbolNode();
            s->name = root->child[0]->ID_NAME;
            if (!_findRecord(head, s)){
                fault = 1;
                printf("Error type 1 at Line %d: Undefined variable \"%s\".\n", root->child[0]->line, root->child[0]->ID_NAME);
                return NULL;
            } 
            else {
                return _findRecord(head, s)->symbolType;
            }
        } 
        /* 修改 semantic_4.c 中 _Exp 函数 ID 调用的 else 分支 */
        else { //函数调用ID
            // 1. 获取函数符号（注意：这里不能直接用 createSymbolNode 查，要用实名查）
            symbol_node s = createSymbolNode();
            s->name = root->child[0]->ID_NAME;
            symbol_node func_node = _findRecord(head, s); // s 是你之前创建的带名字的 node
            
            if (func_node == NULL) {
                // Error 2: 函数未定义（你已经写了）
                fault = 1;
                printf("Error type 2 at Line %d: \"%s\" is undefined.\n", root->child[0]->line, s->name);
                return NULL;
            } 
            else if (func_node->symbolType->kind != FUNCTION) {
                // Error 11: 非函数被调用（你已经写了）
                fault = 1;
                printf("Error type 11 at Line %d: \"%s\" is not a function.\n", root->child[0]->line, s->name);
                return NULL;
            } 
            else {
                // --- 这里就是你要添加的代码段 (Error 9 集结地) ---
                
                // 1. 获取预期的形参列表 (档案)
                symbol_node expect_args = func_node->symbolType->data.func_type.par_type; 
                
                // 2. 解析当前调用的实参类型 (现场)
                symbol_node real_args = NULL;
                if (root->num_child == 4 && !strcmp(root->child[2]->name, "Args")) {
                    real_args = _getArgsType(root->child[2]); 
                }

                // 3. 逐个比对 (你没看懂的那段循环)
                symbol_node p1 = expect_args; // 档案指针
                symbol_node p2 = real_args;   // 实参指针
                int type_mismatch = 0;

                while (p1 != NULL && p2 != NULL) {
                    // 调用深度比对函数，检查每一个参数类型是否匹配[cite: 3]
                    if (!_checkTypeEqual(p1->symbolType, p2->symbolType)) {
                        type_mismatch = 1;
                        break;
                    }
                    p1 = p1->next;
                    p2 = p2->next;
                }

                // 4. 判断结果：数量不对(p1或p2没走完) 或 类型不对(mismatch)
                if (p1 != NULL || p2 != NULL || type_mismatch) {
                    fault = 1;
                    printf("Error type 9 at Line %d: Function \"%s\" is not applicable for arguments.\n", 
                            root->child[0]->line, func_node->name);
                }

                // 5. 返回定义的返回类型，保证外层表达式能继续检查
                return func_node->symbolType->data.func_type.return_type;
            }
        }
    }
    else if (!strcmp(root->child[0]->name, "Exp")){
//这个分支没有返回值
        // 对数组元素展开分析
        // printf("进入Exp分支\n");
        if (!strcmp(root->child[1]->name, "LB")){//数组下标运算 LB（即 Exp [ Exp ]）
            type t1 = _Exp(root->child[0]);// 左子表达式，应为数组
            type t2 = _Exp(root->child[2]); // 下标表达式，应为整型
            // printf("%d\n", t2->kind);
            if (!t1 || !t2){
                return NULL;
            }
            if (t1->kind != ARRAY){
                fault = 1;
                printf("Error type 10 at Line %d: \"%s\" is not an array.\n", root->child[0]->line, root->child[0]->child[0]->ID_NAME);
                return NULL;
            } else if (t2->kind != BASIC || t2->data.basic != INT){
                // printf("即将进入12错误 分支\n");
                fault = 1;
//TODO 使用%f打印FLOAT_NUM，但是这里可能不是一个常量而是一个表达式————已改
                // printf("Error type 12 at Line %d: \"%f\" is not an integer.\n", root->child[0]->line, root->child[2]->child[0]->FLOAT_NUM);
                printf("Error type 12 at Line %d: Array index is not an integer.\n", root->child[0]->line);
                return NULL;
            }
            if (t1 && t1->kind == ARRAY) {
                return t1->data.arr.arr_type; 
            }
            return NULL;
        } 
        else if (!strcmp(root->child[1]->name, "DOT")){
            // 对非结构体变量使用 '.' 进行审查
            type t = _Exp(root->child[0]);
            if (t == NULL) return NULL; // 必须加这一行
            if (t->kind != STRUCTURE){ //对非结构体使用 . 
                fault = 1;
                printf("Error type 13 at Line %d: \"%s\" is not a struct type, cannot be accessed using DOT notation.\n", root->child[0]->line, root->child[0]->child[0]->ID_NAME);
                return NULL;
            } 
            else {
                // 错误类型 14：访问结构体中未定义过的域
                // 这里的 t->data.struct_pointer 存储了该结构体定义时的成员链表
                symbol_node field_list = t->data.struct_pointer;
                char* field_name = root->child[2]->ID_NAME; // 获取 DOT 右边的 ID 名字[cite: 3]
                
                // 遍历结构体内部的成员链表[cite: 3]
                symbol_node p = field_list->next; // 假设 head 是头结点[cite: 3]
                while (p) {
                    if (!strcmp(p->name, field_name)) {
                        // 找到了，返回该成员的类型[cite: 3]
                        return p->symbolType;
                    }
                    p = p->next;
                }
                
                // 遍历完没找到，说明域不存在
                fault = 1;
                printf("Error type 14 at Line %d: Non-existent field \n", root->child[2]->line);
                return NULL;
            }
        } 
        else if (!strcmp(root->child[1]->name, "ASSIGNOP")){
            // 对于有关 ID 的赋值操作
            type t1 = _Exp(root->child[0]); // 左值表达式
            type t2 = _Exp(root->child[2]); // 右值表达式
            // 先对不能为左值的数据进行审查
            if (!_isLValue(root->child[0])){
//只判断了是否是常量————已改
                fault = 1;
                printf("Error type 6 at Line %d: The left-hand side of an assignment must be a variable.\n", root->child[0]->line);
                return NULL;
            } else if (!strcmp(root->child[0]->child[0]->name, "ID")){
                if (!t1 || !t2){
                    return NULL;
                }
                if (!_checkTypeEqual(t1, t2)){
//对于数组元素赋值，左侧类型是 ARRAY 的元素类型，右侧应与元素类型一致，但这里完全没有处理————已改
                    fault = 1;
                    printf("Error type 5 at Line %d: Type mismatched for assignment.\n", root->child[0]->line);
                    return NULL;
                }
            }
        } 
        else {
            // 对其他运算操作如 +、-、*、/进行审查
            type t1 = _Exp(root->child[0]);
            type t2 = _Exp(root->child[2]);
            if (!t1 || !t2){
                return NULL;
            }
//错误七分为两类：操作数类型不匹配和操作数类型与操作符不匹配————已改
//当前只实现了第一种      
/*
是否可以这样实现？
*/
            if (t1->kind != BASIC || t2->kind != BASIC) {
                fault = 1;
                printf("Error type 7 at Line %d: Operand(s) not numeric (array/struct) for arithmetic/relational operation.\n",  root->child[0]->line);
                return NULL;
            }


            if (t1->data.basic != t2->data.basic){
                fault = 1;
                printf("Error type 7 at Line %d: Type mismatched for operands.\n", root->child[0]->line);
                return NULL;
            }
            if (!strcmp(root->child[1]->name, "RELOP")) {
                return _createType(BASIC, 1, INT);
            }
            return t1; // 算术运算结果类型与操作数一致
        }
    }
    return NULL;
}

// 对 Args 的分析
static inline void _Args(Node root, symbol_node func_symbol){
    // type type_temp = func_symbol->symbolType;
    // while (type_temp){
    // }
    
}

// 通过儿子数量判断是什么语句类型
//目前所有情况只检查了表达式，应该检查if while条件是否为int以及return是否匹配
//正常来说_Exp应该返回结构体type类型，但是主要为了检查，不接收返回值（直接丢弃）就可以
static inline void _Stmt(Node root){
    switch (root->num_child){
    case 2: //赋值语句 exp ； child[0]是表达式，child[1]是分号
        _Exp(root->child[0]);
        break;
    case 3: //return exp ； child[0]是return ,child[1]是表达式
        if (!strcmp(root->child[0]->name, "RETURN")) {
            type actual_return_type = _Exp(root->child[1]); // 获取 return 后面表达式的类型
            
            if (actual_return_type && current_func_return_type) {
                // 检查类型是否匹配（假设 1：INT 和 FLOAT 不能互换）
                if (actual_return_type->kind != current_func_return_type->kind ||
                    (actual_return_type->kind == BASIC && 
                     actual_return_type->data.basic != current_func_return_type->data.basic)) {
                    
                    fault = 1;
                    printf("Error type 8 at Line %d: Type mismatched for return.\n", root->child[0]->line);
                }
            }
        }
        // _Exp(root->child[1]);
        break;
    case 4://{int a;a=1} child[2]是表达式
//这里可能有误，不清楚为什么会出现case 4
        _Exp(root->child[2]);
        break;
    case 7: //IF 左括号 EXP 右括号 ；ELSE Stmt，child[2]是表达式
        _Exp(root->child[2]);
        break;
    case 5://WHILE(0) LP(1) Exp(2) RP(3) Stmt(4) 或者IF(0) LP(1) Exp(2) RP(3) Stmt(4)
        _Exp(root->child[2]);
        break;
    } 
}

//从这里进入语义分析器
//这里的 root 是指当前正在处理的语法子树的根节点，而不是整个程序的根节点。
void semantic(Node root){
    _initSymbolList();
    _semantic(root);
    // _printSymbolList(head);
}

//创建头结点之后进入 语义分析函数
static inline void _semantic(Node root){
    if (root == NULL){
        return;
    }
   
    if (!strcmp(root->name, "ExtDef")){//相等返回0
        _ExtDef(root); //外部定义
        return;//——————————————这里要是直接返回，后面的for就不执行了
    } else if (!strcmp(root->name, "Def")){
        // printf("即将进入Def分支\n");
        _Def(root); //局部定义
        // printf("进入Def分支\n");
        return;
    } else if (!strcmp(root->name, "Stmt")){//如果是赋值或者条件判断
        // 这里用 Stmt 对所有可能的 Exp 进行调用
        // 防止重复调用 Exp 引发的问题
        _Stmt(root); 
        return;
    }
   
    for (int i = 0; i < root->num_child; i++){
    //处理完当前节点就不断往下走
    _semantic(root->child[i]);
    }
    
    
    // _printSymbolList(head); 
}
/*
semantic (建表)
_semantic (DFS 遍历树)
遇到 ExtDef -> _Specifier (找类型) -> _FuncDec (存函数名、存参数)
遇到 Def -> _Specifier (找类型) -> _VarDec (存变量名、查重复)
遇到 Stmt -> _Exp (算术检查、查未定义、类型匹配)
遍历结束：所有错误通过 printf 打印完毕。
*/


/*
例子 A：变量定义（ExtDef）
	当输入 int a,b; 时，语法树的结构如下：
	root (ExtDef)
		child[0] (Specifier): 代表类型 int 。 
		child[1] (ExtDecList): 代表变量名列表 。
			child[0] (VarDec): 进一步展开找到 ID 节点 "a" ,
			child[1] (COMMA): 逗号 ,
			child[2] (ExtDecList): 剩余列表。
				child[0] (VarDec): 变量 b
例子 B：函数定义（FunDec）
	当输入func(int x) 时，_FuncDec 函数操作的树长这样：
	root (FunDec)
		child[0] (ID): 存储函数名 "func" 。  
		child[1] (LP): 左括号 ( 。  
		child[2] (VarList): 参数列表（里面包含 int x） 。  
		child[3] (RP): 右括号 ) 。 
		注意：代码中通过 root->num_child == 4 来判断该函数是否有参数 。
例子 C：算术表达式（Exp）
	当输入 a + 1 时，_Exp 处理的树结构：
	root (Exp)
		child[0] (Exp): 变量 a 。 
		child[1] (PLUS): 运算符 + 。 
		child[2] (Exp): 常量 1 
例子 D：数组访问（Exp）
	当输入 data[5] 时，树的层级变得复杂：
	root (Exp)
		child[0] (Exp): 数组名 data 。
		child[1] (LB): 左方括号 [ 。 
		child[2] (Exp): 索引值 5 。 
		child[3] (RB): 右方括号 ] 。
		代码中 _Exp 通过判断 child[1]->name 是否为 "LB" 来确定这是一个数组访问操作 。  
*/