// ============================================================
//  ooooc.h v3 —— 超智慧编程，真实不虚
//
//  诚实声明：单继承、无 RTTI、全部成员公有。
//  多重继承仍由 extends(...) 以信仰方式提供。
// ============================================================

#include <stdlib.h>   // OOOOC_NEW 需要 malloc

// ==================== v1: 命名空间 ====================
#define namespace(NAME)     enum { NAMESPACE_##NAME##_OPEN };
#define endnamespace(NAME)  enum { NAMESPACE_##NAME##_CLOSE };

// ==================== v2: 符号编码 ====================
#define OOOOC_LEN(tok)   OOOOC_LEN_I(tok)
#define OOOOC_LEN_I(tok) OOOOC_LENOF_##tok

#define N_MED(NS_SUM, NS_NAME, MED_SUM, MED_NAME, ARGS) \
    _ZN##NS_SUM##NS_NAME##MED_SUM##MED_NAME##E##ARGS
#define N_MED_MID(NS_SUM, NS_NAME, MED_SUM, MED_NAME, ARGS) \
    N_MED(NS_SUM, NS_NAME, MED_SUM, MED_NAME, ARGS)
#define N_MED_AUTO(NS, MED, ARGS) \
    N_MED_MID(OOOOC_LEN(NS), NS, OOOOC_LEN(MED), MED, ARGS)
#define N_MED3(A, B, MED, ARGS) \
    N_MED3_MID(OOOOC_LEN(A), A, OOOOC_LEN(B), B, OOOOC_LEN(MED), MED, ARGS)
#define N_MED3_MID(AS, A, BS, B, MS, M, ARGS) \
    N_MED3_I(AS, A, BS, B, MS, M, ARGS)
#define N_MED3_I(AS, A, BS, B, MS, M, ARGS) \
    _ZN##AS##A##BS##B##MS##M##E##ARGS

// ==================== v3: 面向对象 ====================

// ---- 类声明 ----
// 根类：OOOOC_ROOT_CLASS(动物, int 年龄;)
#define OOOOC_ROOT_CLASS(NAME, ...) \
    struct NAME##_vtable; \
    typedef struct NAME { \
        const void *vtable; \
        __VA_ARGS__ \
    } NAME;

// 派生类：OOOOC_CLASS(鸭子, 动物, int 速度;)
// 继承 = 在最前面内嵌父类子对象；vtable 指针全类共享根对象偏移 0 的那一个，
// 派生类不得再添加自己的 vtable 字段，否则向上转型后字段偏移全部错位
#define OOOOC_CLASS(NAME, PARENT, ...) \
    struct NAME##_vtable; \
    typedef struct NAME { \
        PARENT base; \
        __VA_ARGS__ \
    } NAME;

// ---- 虚表声明 ----
// 槽位声明：OOOOC_VMETHOD(返回类型, 方法名, 参数类型们...)
// 注意：从父类继承的虚方法必须按原顺序重新列出，保证虚表前缀布局兼容
#define OOOOC_VMETHOD(R, NAME, ...) R (*NAME)(__VA_ARGS__);

#define OOOOC_VTABLE(NAME, ...) \
    struct NAME##_vtable { __VA_ARGS__ }; \
    extern const struct NAME##_vtable NAME##_vtable_instance;

// ---- 虚表实例化（用户唯一的"写vtable"工作） ----
// 按槽位顺序填入实现函数指针，支持重写（override）
#define OOOOC_DEFINE_VTABLE(NAME, ...) \
    const struct NAME##_vtable NAME##_vtable_instance = { __VA_ARGS__ };

// ---- 构造函数 ----
// __VA_ARGS__ 里写字段初始化语句
// 虚表指针约定固定在对象偏移 0，根类如此，派生类亦复如是
#define OOOOC_VPTR(obj)      (*(const void * const *)(const void *)(obj))
#define OOOOC_SET_VPTR(obj)  (*(const void **)(void *)(obj))

#define OOOOC_DEFINE_INIT(NAME, ...) \
    NAME *NAME##_init(NAME *self) { \
        OOOOC_SET_VPTR(self) = &NAME##_vtable_instance; \
        __VA_ARGS__ \
        return self; \
    }

#define OOOOC_NEW(NAME)  (NAME##_init(malloc(sizeof(NAME))))
#define OOOOC_DELETE(p)  free(p)

// ---- 消息发送（动态分发） ----
// GNU typeof 推出对象静态类型的虚表结构，从 void* vtable 字段强转后调用
#define OOOOC_SEND(CLASS, obj, meth, ...) \
    (((const struct CLASS##_vtable *)OOOOC_VPTR(obj))->meth((obj), ##__VA_ARGS__))

// ---- 转型 ----
#define OOOOC_UPCAST(obj, PARENT)    ((PARENT *)(obj))
#define OOOOC_DOWNCAST(obj, CHILD)   ((CHILD *)(obj))   /* 不检查，真实不虚 */
#define OOOOC_SUPER(obj)             ((obj).base)

// ---- OOP 其余特性（与 v2 保持一致的真实不虚标准） ----
#define extends(...)     /* 已完美继承，无需生成任何代码 */
#define virtual          /* O语言没有虚函数，一切皆是实函数 */

// ==================== 使用示例见 demo_oop.c ====================