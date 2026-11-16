#!/usr/bin/env python3
# ooooc_reg.py —— 注册表生成器
# 用法: python3 ooooc_reg.py 名字1 名字2 ... > ooooc_registry.h
import sys
names = sys.argv[1:]
out = ["// 本文件由 ooooc_reg.py", "#pragma once"]
for n in names:
    if not n.isidentifier():
        sys.exit(f"错误: '{n}' 不是合法标识符，O语言拒绝")
    out.append(f"#define OOOOC_LENOF_{n} {len(n)}")
print("\n".join(out))