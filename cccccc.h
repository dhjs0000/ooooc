// ============================================================
//  cccccc.h —— CCCCCC 扩展包
//
//      Chinese C Coder's Cultural Confidence Committee
//      中华 C 语言程序员文化自信委员会 · 官方指定扩展包
//
//  宗旨：让中文也能当标识符；编译器不查护照。
//
//  诚实声明
//  ------------------------------------------------
//  · 这是宏层汉化，不是新编译器。
//  · #include / #define 等预处理指令仍是英文——那是编译器的签证。
//  · 宏会展开；调试器里看到的是展开后的名字。
//  · 中文标识符依赖 GCC/Clang 的 UTF-8 支持；C 标准层面是 UCN。
//  · 文化自信不是排斥英文，而是中文也能当第一等公民。
//
//  用法：
//      #include "cccccc.h"
//      类(动物, 整数 年龄;)
//      ...
//
//  六个 C 的意思：
//      Chinese · C · Coder's · Cultural · Confidence · Committee
// ============================================================
#ifndef CCCCCC_H
#define CCCCCC_H

#include "ooooc.h"

// ============================================================
//  一、基础词汇
// ============================================================
#define 整数          int
#define 长整数        long
#define 短整数        short
#define 字符          char
#define 浮点          double
#define 单精度浮点    float
#define 空            void
#define 布尔          _Bool
#define 无符号        unsigned
#define 有符号        signed
#define 常量          const
#define 静态          static
#define 外部          extern
#define 内联          inline
#define 易变          volatile
#define 结构          struct
#define 联合          union
#define 枚举          enum
#define 定义类型      typedef
#define 大小          sizeof

#define 真            1
#define 假            0
#define 空指针        NULL

#define 字符指针      char *
#define 常量字符指针  const char *

// ============================================================
//  二、流程控制
// ============================================================
#define 如果          if
#define 否则          else
#define 当            while
#define 对于          for
#define 做            do
#define 开关          switch
#define 情况          case
#define 默认          default
#define 跳出          break
#define 继续          continue
#define 返回          return
#define 转到          goto

// 运算符也可以有中文名：C 的运算符是符号，不是单词
#define 并且          &&
#define 或者          ||
#define 不是          !
#define 等于          ==
#define 不等于        !=

// ============================================================
//  三、O语言面向对象
// ============================================================
#define 类            OOOOC_ROOT_CLASS
#define 派生          OOOOC_CLASS
#define 虚表          OOOOC_VTABLE
#define 虚方法        OOOOC_VMETHOD

#define 定义元信息    OOOOC_DEFINE_META
#define 定义虚表      OOOOC_DEFINE_VTABLE
#define 无析构        OOOOC_NO_DTOR

#define 新建          OOOOC_NEW
#define 删除          OOOOC_DELETE
#define 析构          OOOOC_DESTROY
#define 发送          OOOOC_SEND

#define 当作          OOOOC_AS
#define 是            OOOOC_INSTANCEOF
#define 精确是        OOOOC_IS_EXACT
#define 取类名        OOOOC_CLASSNAME
#define 取类型名      OOOOC_TYPENAME
#define 向上转型      OOOOC_UPCAST
#define 向下转型      OOOOC_DOWNCAST
#define 取父类        OOOOC_SUPER
#define 去虚化        OOOOC_DEVIRT

// 中文的父类字段名：自己->父类.年龄
#define 父类          base

// 构造/析构用中文参数名“自己”，这是本包与核心宏唯一不同之处
#define 定义构造(名字, ...) \
    名字 *名字##_init(名字 *自己) { \
        OOOOC_SET_VPTR(自己) = &名字##_vtable_instance; \
        ooooc__bind_ifaces((void *)自己, &名字##_meta); \
        __VA_ARGS__ \
        return 自己; \
    }

#define 定义析构(名字) \
    void 名字##__destroy(void *自己)

// 多编译单元时的声明
#define 声明构造      OOOOC_DECLARE_INIT
#define 声明析构      OOOOC_DECLARE_DTOR
#define 声明接口虚表  OOOOC_DECLARE_IVTABLE

// ============================================================
//  四、接口（多重继承的中文形态）
// ============================================================
#define 接口            OOOOC_INTERFACE
#define 接口方法        OOOOC_IMETHOD
#define 实现接口        OOOOC_IMPL
#define 定义接口虚表    OOOOC_DEFINE_IVTABLE
#define 接口项          OOOOC_IFACE
#define 取自身          OOOOC_SELF
#define 初始化接口      OOOOC_INIT_IMPL

// ============================================================
//  五、命名空间与符号编码
// ============================================================
#define 命名空间        namespace
#define 结束命名空间    endnamespace

// ============================================================
//  六、随手可用的中文函数名
// ============================================================
#define 打印            printf
#define 打印行          puts
#define 读入            scanf
#define 格式化          snprintf
#define 比较字串        strcmp
#define 主函数          main
#define 中文控制台      ooooc_console_utf8

#endif // CCCCCC_H
