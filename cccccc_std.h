// ============================================================
//  cccccc_std.h —— CCCCCC 扩展包 · 中文标准库
//
//  给 ooooc_std.h 里的东西起中文名。
//  文化自信从命名开始：arena 叫竞技场，vector 叫动态数组。
//
//  用法：
//      #include "cccccc_std.h"
//      竞技场 *场 = 新建竞技场(4096);
//      动态数组(动物 *) 动物园;
//      数组初始化(&动物园);
//      数组压入(&动物园, 某动物);
// ============================================================
#ifndef CCCCCC_STD_H
#define CCCCCC_STD_H

#include "cccccc.h"
#include "ooooc_std.h"

// ============================================================
//  竞技场（arena）
// ============================================================
#define 竞技场            ooooc_arena
#define 新建竞技场        ooooc_arena_new
#define 释放竞技场        ooooc_arena_free
#define 竞技场分配        ooooc_arena_alloc
#define 竞技场清零分配    ooooc_arena_calloc
#define 竞技场重置        ooooc_arena_reset
#define 竞技场字节        ooooc_arena_bytes
#define 竞技场字符串      ooooc_arena_strdup
#define 新建于(类型, 场地)  OOOOC_ARENA_NEW(类型, 场地)
#define 竞技场内字串(场地, 文本)  OOOOC_ARENA_STRDUP(场地, 文本)

// ============================================================
//  动态数组（vector）
// ============================================================
#define 动态数组(类型)    OOOOC_VEC(类型)
#define 数组初始化        OOOOC_VEC_INIT
#define 数组预留          OOOOC_VEC_RESERVE
#define 数组压入          OOOOC_VEC_PUSH
#define 数组弹出          OOOOC_VEC_POP
#define 数组取            OOOOC_VEC_AT
#define 数组清空          OOOOC_VEC_CLEAR
#define 数组遍历          OOOOC_VEC_EACH
#define 数组释放          OOOOC_VEC_FREE
#define 数组长度(数组)    ((数组)->len)

// ============================================================
//  字串（string builder）
// ============================================================
#define 字串              ooooc_str
#define 字串初始化(串)    OOOOC_STR_INIT(串)
#define 字串释放(串)      OOOOC_STR_FREE(串)
#define 字串添加(串, 文本) OOOOC_STR_APPEND(串, 文本)
#define 字串格式化(串, ...) OOOOC_STR_APPENDF(串, __VA_ARGS__)
#define 字串内容(串)      OOOOC_STR_CSTR(串)
#define 字串清空(串)      ooooc_str_clear(&(串))
#define 字串长度(串)      ((串).len)

// ============================================================
//  异常
// ============================================================
#define 尝试              OOOOC_TRY
#define 捕获              OOOOC_CATCH
#define 结束尝试          OOOOC_ENDTRY
#define 抛出              OOOOC_THROW

// ============================================================
//  测试框架
// ============================================================
#define 测试              OOOOC_TEST
#define 运行测试          OOOOC_RUN
#define 断言              OOOOC_EXPECT
#define 断言整数          OOOOC_EXPECT_INT
#define 断言字串          OOOOC_EXPECT_STR
#define 测试总结          OOOOC_TEST_SUMMARY

#endif // CCCCCC_STD_H
