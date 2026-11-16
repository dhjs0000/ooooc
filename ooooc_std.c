// ============================================================
//  ooooc_std.c —— O语言标准库中必须只有一个的那部分
//
//  异常状态、异常投掷、测试计数器、控制台编码。
//  其余东西都在 ooooc_std.h 里内联着。
// ============================================================
#include "ooooc_std.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#  include <windows.h>
#endif

OOOOC_THREAD_LOCAL struct ooooc_try_frame *ooooc__try_top = NULL;
OOOOC_THREAD_LOCAL struct ooooc_exception ooooc__current_exc;
int ooooc__test_failures = 0;

OOOOC_NORETURN void ooooc_throw(const char *type, const char *file, int line,
                                const char *fmt, ...)
{
    va_list ap;
    struct ooooc_try_frame *frame;

    ooooc__current_exc.type = type ? type : "Exception";
    ooooc__current_exc.file = file;
    ooooc__current_exc.line = line;

    va_start(ap, fmt);
    vsnprintf(ooooc__current_exc.msg, sizeof ooooc__current_exc.msg, fmt, ap);
    va_end(ap);

    frame = ooooc__try_top;
    if (!frame) {
        fprintf(stderr, "\n[ooooc] 未捕获的 %s: %s (%s:%d)\n",
                ooooc__current_exc.type,
                ooooc__current_exc.msg,
                file ? file : "?",
                line);
        abort();
    }

    /* 先弹栈再跳：catch 块里再 throw 时会落到外层 */
    ooooc__try_top = frame->prev;
    longjmp(frame->jb, 1);
}

void ooooc_console_utf8(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif
}
