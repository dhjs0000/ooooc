// ============================================================
//  ooooc.h —— O语言核心
//
//        超 智 慧 编 程 ， 真 实 不 虚
//
//  v4.0 "太初"
//
//  超智慧编程三定律
//  ------------------------------------------------
//  一、能用宏解决的，绝不写运行时代码。
//  二、宏解决不了的，就写进诚实声明。
//  三、真实不虚。
//
//  诚实声明（v4.0）
//  ------------------------------------------------
//  [真] 单继承
//  [真] 虚函数与动态派发
//  [真] RTTI：类名、instanceof、动态转型
//  [真] 接口：多重继承以接口方式真实提供
//  [真] 自动析构：派生先于基类，沿继承链逐级调用
//  [真] 命名空间符号编码（Itanium 风格）
//  [真] GCC / Clang，C99 及以后
//  [假] 多重继承（请用接口；extends(...) 只负责安慰旧代码）
//  [假] 模板（宏就是模板，模板就是宏）
//  [假] 异常安全（见 ooooc_std.h：longjmp 不负责展开栈）
//  [假] 线程安全（RTTI 只读；异常是线程局部的；其余靠你自己）
//
//  对象内存布局
//  ------------------------------------------------
//      +----------------+          +---------------------------+
//      | vtable 指针     |  --->    | meta 指针（类型信息）      |
//      +----------------+          +---------------------------+
//      | 父类子对象/字段 |          | 方法槽 0                   |
//      | ...            |          | 方法槽 1 ...               |
//      +----------------+          +---------------------------+
//
//  虚表第 0 槽是类型信息指针，所以 RTTI 是免费的；
//  方法槽从第 1 槽开始，所以派发和 C++ 单继承一样快。
// ============================================================
#ifndef OOOOC_H
#define OOOOC_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define OOOOC_VERSION_MAJOR 4
#define OOOOC_VERSION_MINOR 0
#define OOOOC_VERSION_PATCH 0
#define OOOOC_VERSION_STRING "4.0.0"

// ============================================================
//  0. 编译器适配
// ============================================================
#if defined(_MSC_VER) && !defined(__clang__)
#  error "O语言 v4 需要 GCC 或 Clang。MSVC 的宏还不够超智慧。"
#endif

#if defined(__GNUC__) || defined(__clang__)
#  define OOOOC_UNUSED           __attribute__((unused))
#  define OOOOC_NORETURN         __attribute__((noreturn))
#  define OOOOC_NOINLINE         __attribute__((noinline))
#  define OOOOC_ALWAYS_INLINE    static inline __attribute__((always_inline))
#  define OOOOC_INLINE           static inline
#  define OOOOC_THREAD_LOCAL     __thread
#  define OOOOC_CLEANUP(fn)      __attribute__((cleanup(fn)))
#else
#  define OOOOC_UNUSED
#  define OOOOC_NORETURN
#  define OOOOC_NOINLINE
#  define OOOOC_ALWAYS_INLINE    static inline
#  define OOOOC_INLINE           static inline
#  define OOOOC_THREAD_LOCAL
#  define OOOOC_CLEANUP(fn)
#endif

// ============================================================
//  1. 类型信息（meta）—— RTTI 的全部秘密
// ============================================================
struct ooooc_class;

// 接口视图表项：接口名 -> 接口子对象偏移 -> 该视图的虚表
// vtable 让构造函数能自动绑定整条继承链上的接口，不必手工初始化。
struct ooooc_cast_entry {
    const char *name;
    ptrdiff_t   offset;
    const void *vtable;
};

struct ooooc_class {
    const char *name;                       // 类名，或 "类::接口" 视图名
    const struct ooooc_class *parent;       // 父类；接口视图指向其实现类
    size_t size;                            // sizeof(类)
    void (*destroy)(void *self);            // 析构函数（无资源也要给个空函数）
    const struct ooooc_cast_entry *ifaces;  // 接口视图表，NULL 结尾
    ptrdiff_t self_offset;                  // 本视图到完整对象的偏移；类视图为 0
    unsigned flags;                         // OOOOC_KIND_*
};

enum ooooc_class_flags {
    OOOOC_KIND_CLASS       = 0,
    OOOOC_KIND_IFACE_VIEW  = 1
};

// 对象偏移 0 处是 vtable 指针；vtable 偏移 0 处是 meta 指针
OOOOC_ALWAYS_INLINE const struct ooooc_class *ooooc_meta_of(const void *obj)
{
    const void *vptr = *(const void *const *)obj;
    return *(const struct ooooc_class *const *)vptr;
}

#define OOOOC_VPTR(obj)      (*(const void *const *)(const void *)(obj))
#define OOOOC_SET_VPTR(obj)  (*(const void **)(void *)(obj))
#define OOOOC_META(obj)      ooooc_meta_of((const void *)(obj))

// ============================================================
//  2. 命名空间与符号编码（v2 遗产，继续超智慧）
// ============================================================
//  名字长度由 oooo.py 扫描源码自动生成，不再手工数数。
//  旧版手写注册表依然兼容：OOOOC_LENOF_<name> <len>
#define namespace(NAME)     enum { NAMESPACE_##NAME##_OPEN };
#define endnamespace(NAME)  enum { NAMESPACE_##NAME##_CLOSE };

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

// ============================================================
//  3. 类声明
// ============================================================
//  根类：OOOOC_ROOT_CLASS(Animal, int age;)
//  派生：OOOOC_CLASS(Duck, Animal, int speed;)
//
//  规则只有一条：父类子对象必须放在最前面，vtable 指针全类共享偏移 0。
//  违反这条规则，向上转型后字段偏移会全部错位 —— 这不是信仰问题，是数学问题。
#define OOOOC_ROOT_CLASS(NAME, ...) \
    struct NAME##_vtable; \
    extern const struct ooooc_class NAME##_meta; \
    typedef struct NAME { \
        const void *vtable; \
        __VA_ARGS__ \
    } NAME; \
    static const struct ooooc_class *const NAME##__parent \
        OOOOC_UNUSED = NULL;

#define OOOOC_CLASS(NAME, PARENT, ...) \
    struct NAME##_vtable; \
    extern const struct ooooc_class NAME##_meta; \
    typedef struct NAME { \
        PARENT base; \
        __VA_ARGS__ \
    } NAME; \
    static const struct ooooc_class *const NAME##__parent \
        OOOOC_UNUSED = &PARENT##_meta;

// ============================================================
//  4. 虚表
// ============================================================
//  槽位声明：OOOOC_VMETHOD(返回类型, 方法名, 参数类型们...)
//  继承来的虚方法必须按父类原顺序重新列出，保证虚表前缀兼容。
//  抄漏一个槽，就是一次行为艺术；oooo.py report 会帮你数。
#define OOOOC_VMETHOD(R, NAME, ...) R (*NAME)(__VA_ARGS__);

#define OOOOC_VTABLE(NAME, ...) \
    struct NAME##_vtable { \
        const struct ooooc_class *meta; \
        __VA_ARGS__ \
    }; \
    extern const struct NAME##_vtable NAME##_vtable_instance;

// 按槽位顺序填入实现函数；继承下来的槽也要重新填。
#define OOOOC_DEFINE_VTABLE(NAME, ...) \
    const struct NAME##_vtable NAME##_vtable_instance = { \
        &NAME##_meta, __VA_ARGS__ \
    };

// ============================================================
//  5. 类型信息定义
// ============================================================
//  每个类恰好定义一次（通常在类的 .c 里）。
//  DTOR 是 void (*)(void *) 函数；没有资源要释放就写 OOOOC_NO_DTOR。
OOOOC_INLINE void ooooc_noop_destroy(void *self) { (void)self; }
#define OOOOC_NO_DTOR ooooc_noop_destroy

//  接口表：OOOOC_DEFINE_META(Duck, Duck__destroy, OOOOC_IFACE(Duck, ISwimmer, swimmer))
#define OOOOC_DEFINE_META(NAME, DTOR, ...) \
    static const struct ooooc_cast_entry NAME##__ifaces_storage[] = { \
        __VA_ARGS__ { NULL, 0, NULL } \
    }; \
    const struct ooooc_class NAME##_meta = { \
        #NAME, NAME##__parent, sizeof(NAME), (DTOR), \
        NAME##__ifaces_storage, 0, OOOOC_KIND_CLASS \
    };

//  析构函数写法（签名固定，self 是完整对象的地址）：
//      OOOOC_DEFINE_DTOR(Duck) {
//          Duck *self = (Duck *)ooooc_self_;
//          free(self->name);
//      }
#define OOOOC_DEFINE_DTOR(NAME) \
    void NAME##__destroy(void *ooooc_self_)

#define OOOOC_DECLARE_DTOR(NAME) \
    void NAME##__destroy(void *ooooc_self_)

// ============================================================
//  6. 接口 —— 多重继承的真实形态
// ============================================================
//  声明：OOOOC_INTERFACE(ISwimmer, OOOOC_IMETHOD(void, swim, ISwimmer *))
//  对象里的接口子对象是一个只有 vptr 的小结构，偏移由编译器算。
//
//  类实现接口：
//      OOOOC_CLASS(Duck, Animal, ..., OOOOC_IMPL(ISwimmer, swimmer))
//      OOOOC_DEFINE_IVTABLE(Duck, ISwimmer, swimmer, Duck__swim)   // 先于 META
//      OOOOC_DEFINE_META(Duck, Duck__destroy, OOOOC_IFACE(Duck, ISwimmer, swimmer))
//
//  构造函数会自动绑定本类及祖先类的全部接口视图；
//  OOOOC_INIT_IMPL 仅作为特殊场合的手工补绑工具保留。
#define OOOOC_IMETHOD(R, NAME, ...) R (*NAME)(__VA_ARGS__);

#define OOOOC_INTERFACE(NAME, ...) \
    struct NAME##_vtable; \
    typedef struct NAME { const void *vptr; } NAME; \
    struct NAME##_vtable { \
        const struct ooooc_class *meta; \
        __VA_ARGS__ \
    };

#define OOOOC_IMPL(IFACE, MEMBER) IFACE MEMBER;

#define OOOOC_IFACE(CLASS, IFACE, MEMBER) \
    { #IFACE, (ptrdiff_t)offsetof(CLASS, MEMBER), \
      (const void *)&CLASS##__##IFACE##__vtable },

#define OOOOC_DEFINE_IVTABLE(CLASS, IFACE, MEMBER, ...) \
    static const struct ooooc_class CLASS##__##IFACE##__meta = { \
        #CLASS "::" #IFACE, &CLASS##_meta, sizeof(CLASS), NULL, NULL, \
        (ptrdiff_t)offsetof(CLASS, MEMBER), OOOOC_KIND_IFACE_VIEW \
    }; \
    const struct IFACE##_vtable CLASS##__##IFACE##__vtable = { \
        &CLASS##__##IFACE##__meta, __VA_ARGS__ \
    };

#define OOOOC_DECLARE_IVTABLE(CLASS, IFACE) \
    extern const struct IFACE##_vtable CLASS##__##IFACE##__vtable

#define OOOOC_INIT_IMPL(CLASS, IFACE, MEMBER, SELF) \
    ((SELF)->MEMBER.vptr = (const void *)&CLASS##__##IFACE##__vtable)

// 从接口子对象指针取回实现类指针（接口不在偏移 0 时也正确）
#define OOOOC_SELF(CLASS, MEMBER, SELF) \
    ((CLASS *)(void *)((char *)(SELF) - offsetof(CLASS, MEMBER)))

// ============================================================
//  7. 对象生命周期
// ============================================================
OOOOC_INLINE void *ooooc_alloc(size_t n)
{
    void *p = malloc(n ? n : 1);
    if (!p) {
        fputs("[ooooc] 内存耗尽：超智慧也需要 RAM\n", stderr);
        abort();
    }
    return p;
}

// 把本类及祖先类的接口视图 vptr 全部绑好。
// 先绑祖先、后绑本类：同一位置若都声明了同一接口，派生类胜出。
OOOOC_INLINE void ooooc__bind_ifaces(void *obj, const struct ooooc_class *m)
{
    const struct ooooc_cast_entry *e;

    if (!m) {
        return;
    }
    ooooc__bind_ifaces(obj, m->parent);
    for (e = m->ifaces; e && e->name; ++e) {
        *(const void **)((char *)obj + e->offset) = e->vtable;
    }
}

// 构造函数由 OOOOC_DEFINE_INIT 生成；接口绑定自动完成
#define OOOOC_NEW(NAME)  (NAME##_init((NAME *)ooooc_alloc(sizeof(NAME))))

#define OOOOC_DEFINE_INIT(NAME, ...) \
    NAME *NAME##_init(NAME *self) { \
        OOOOC_SET_VPTR(self) = &NAME##_vtable_instance; \
        ooooc__bind_ifaces((void *)self, &NAME##_meta); \
        __VA_ARGS__ \
        return self; \
    }

// 多编译单元时，在头文件里声明构造函数
#define OOOOC_DECLARE_INIT(NAME) NAME *NAME##_init(NAME *self)

// 析构：派生先于基类，沿继承链逐级调用
OOOOC_INLINE void ooooc__destroy_chain(const struct ooooc_class *m, void *self)
{
    while (m) {
        if (m->destroy) {
            m->destroy(self);
        }
        m = m->parent;
    }
}

// 只析构，不释放内存：用于栈对象、arena 对象
OOOOC_INLINE void ooooc_destroy(void *obj)
{
    const struct ooooc_class *m;
    void *complete;

    if (!obj) {
        return;
    }
    m = ooooc_meta_of(obj);
    complete = (char *)obj - m->self_offset;
    ooooc__destroy_chain(
        (m->flags & OOOOC_KIND_IFACE_VIEW) ? m->parent : m,
        complete);
}

// 析构并释放：用于 OOOOC_NEW 出来的对象
OOOOC_INLINE void ooooc_delete(void *obj)
{
    const struct ooooc_class *m;
    void *complete;

    if (!obj) {
        return;
    }
    m = ooooc_meta_of(obj);
    complete = (char *)obj - m->self_offset;
    ooooc__destroy_chain(
        (m->flags & OOOOC_KIND_IFACE_VIEW) ? m->parent : m,
        complete);
    free(complete);
}

#define OOOOC_DESTROY(obj)  ooooc_destroy((void *)(obj))
#define OOOOC_DELETE(obj)   ooooc_delete((void *)(obj))

// ============================================================
//  8. 消息发送（动态派发）
// ============================================================
//  OOOOC_SEND(Animal, a, breathe)
//  OOOOC_SEND(Duck, d, quack, 42)
#define OOOOC_SEND(CLASS, obj, meth, ...) \
    (((const struct CLASS##_vtable *)OOOOC_VPTR(obj))->meth((obj), ##__VA_ARGS__))

// 去虚化：类型已知时直接调用实现函数，省一次间接跳转。
// 这是超智慧优化，不是作弊。oooo.py report 会告诉你哪里可以这么干。
#define OOOOC_DEVIRT(IMPL, ...) IMPL(__VA_ARGS__)

// ============================================================
//  9. 转型与 RTTI
// ============================================================
// GCC 的 -Wclobbered 会把“内联进含 setjmp 的函数”的局部变量误报为
// 可能被 longjmp 破坏。GCC 下让 dyncast 不内联，换一个干净编译；
// Clang 不报这个警告，继续内联。
#if defined(__GNUC__) && !defined(__clang__)
#  define OOOOC_RTTI_FN static __attribute__((noinline, unused))
#else
#  define OOOOC_RTTI_FN OOOOC_INLINE
#endif

// 先比首字符：类型名不同首字母时，连 strcmp 都不用进。
OOOOC_INLINE int ooooc__name_eq(const char *a, const char *b)
{
    return a && b && a[0] == b[0] && strcmp(a, b) == 0;
}

OOOOC_RTTI_FN void *ooooc__dyncast(const void *obj, const char *type)
{
    const struct ooooc_class *m;
    const struct ooooc_class *cm;
    const void *complete;

    if (!obj || !type) {
        return NULL;
    }
    m = ooooc_meta_of(obj);
    complete = (const char *)obj - m->self_offset;
    cm = (m->flags & OOOOC_KIND_IFACE_VIEW) ? m->parent : m;
    if (!cm) {
        return NULL;
    }

    // 先看类继承链（单继承，全部偏移 0）
    for (const struct ooooc_class *k = cm; k; k = k->parent) {
        if (ooooc__name_eq(k->name, type)) {
            return (void *)complete;
        }
    }
    // 再看接口视图（可能在本类，也可能在祖先类）
    for (const struct ooooc_class *k = cm; k; k = k->parent) {
        for (const struct ooooc_cast_entry *e = k->ifaces; e && e->name; ++e) {
            if (ooooc__name_eq(e->name, type)) {
                return (char *)complete + e->offset;
            }
        }
    }
    return NULL;
}

OOOOC_INLINE int ooooc_instanceof(const void *obj, const char *type)
{
    return ooooc__dyncast(obj, type) != NULL;
}

OOOOC_INLINE const char *ooooc_typename(const void *obj)
{
    const struct ooooc_class *m;

    if (!obj) {
        return "?";
    }
    m = ooooc_meta_of(obj);
    return (m && m->name) ? m->name : "?";
}

OOOOC_INLINE const char *ooooc_classname(const void *obj)
{
    const struct ooooc_class *m;

    if (!obj) {
        return "?";
    }
    m = ooooc_meta_of(obj);
    if (m && (m->flags & OOOOC_KIND_IFACE_VIEW)) {
        m = m->parent;
    }
    return (m && m->name) ? m->name : "?";
}

OOOOC_INLINE int ooooc_is_exact(const void *obj, const char *type)
{
    return ooooc__name_eq(ooooc_classname(obj), type);
}

#define OOOOC_AS(TYPE, obj) \
    ((TYPE *)ooooc__dyncast((const void *)(obj), #TYPE))
#define OOOOC_INSTANCEOF(obj, TYPE) \
    (ooooc__dyncast((const void *)(obj), #TYPE) != NULL)
#define OOOOC_TYPENAME(obj)   ooooc_typename((const void *)(obj))
#define OOOOC_CLASSNAME(obj)  ooooc_classname((const void *)(obj))
#define OOOOC_IS_EXACT(obj, TYPE) ooooc_is_exact((const void *)(obj), #TYPE)

// 向上转型免费（偏移 0）
#define OOOOC_UPCAST(obj, PARENT)   ((PARENT *)(obj))
// 向下转型不检查，真实不虚；要检查请用 OOOOC_AS
#define OOOOC_DOWNCAST(obj, CHILD)  ((CHILD *)(obj))
// 父类视图指针
#define OOOOC_SUPER(obj)            (&(obj)->base)

// ============================================================
//  10. OOP 氛围组（保留 v3 的经典笑话）
// ============================================================
#define extends(...)     /* v4 已用接口真实实现；这里保留，用来安慰旧代码 */
#define virtual          /* 在 O语言里，所有函数生而平等 */

#endif // OOOOC_H
