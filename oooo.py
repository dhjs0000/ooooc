#!/usr/bin/env python3
# ============================================================
#  oooo.py —— O语言工具链
#
#        超 智 慧 编 程 ， 真 实 不 虚
#
#  这不是编译器，这是编译器的经纪人：
#    · 扫描源码，生成符号长度注册表
#    · 调 gcc/clang 编译
#    · 跑测试、跑基准
#    · 静态检查虚表槽位、继承前缀、接口一致性
#    · 用启发式找“可去虚化”的调用点
#
#  用法：
#    python oooo.py doctor          环境体检
#    python oooo.py test            编译并运行测试
#    python oooo.py demo            编译并运行旗舰演示
#    python oooo.py bench           编译并运行基准
#    python oooo.py report          静态分析
#    python oooo.py all             一条龙验收
#    python oooo.py build a.c       编译任意源文件
#    python oooo.py run a.c         编译并运行
#    python oooo.py registry -o x.h 生成注册表
#    python oooo.py clean           清空 build/
# ============================================================
from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
import time
from pathlib import Path

from ooooc_reg import collect_identifiers, render_registry

ROOT = Path(__file__).resolve().parent
BUILD = ROOT / "build"
VERSION = "4.0.0"
CCCCCC_VERSION = "1.0"
TOOL_VERSION = "1.1"

DEFAULT_REPORT = [
    "demo_zoo.c",
    "tests.c",
    "bench.c",
    "ooooc.h",
    "ooooc_std.h",
]

# 中文扩展包里的宏别名：静态分析不能只认洋文
ALIAS = {
    "root_class":  ("OOOOC_ROOT_CLASS", "类"),
    "class":       ("OOOOC_CLASS", "派生"),
    "vtable":      ("OOOOC_VTABLE", "虚表"),
    "vmethod":     ("OOOOC_VMETHOD", "虚方法"),
    "meta":        ("OOOOC_DEFINE_META", "定义元信息"),
    "vtable_def":  ("OOOOC_DEFINE_VTABLE", "定义虚表"),
    "init":        ("OOOOC_DEFINE_INIT", "定义构造"),
    "interface":   ("OOOOC_INTERFACE", "接口"),
    "imethod":     ("OOOOC_IMETHOD", "接口方法"),
    "ivtable":     ("OOOOC_DEFINE_IVTABLE", "定义接口虚表"),
    "iface":       ("OOOOC_IFACE", "接口项"),
    "send":        ("OOOOC_SEND", "发送"),
    "as":          ("OOOOC_AS", "当作"),
    "new":         ("OOOOC_NEW", "新建"),
}

CJK_RE = re.compile(r"[\u3400-\u9fff\uf900-\ufaff]")
CODE_IDENT_RE = re.compile(r"[^\W\d]\w*", re.UNICODE)
C_KEYWORDS = re.compile(
    r"\b(?:int|void|char|double|float|short|long|struct|union|enum|"
    r"return|if|else|for|while|do|switch|case|break|continue|"
    r"typedef|static|const|extern|sizeof)\b"
)


# ============================================================
#  小工具
# ============================================================
def say(msg):
    print("[oooo] " + msg, flush=True)


def warn(msg):
    print("[oooo] " + msg, file=sys.stderr, flush=True)


def die(msg, code=2):
    print("[oooo] 错误：" + msg, file=sys.stderr)
    raise SystemExit(code)


def rel(path):
    try:
        return str(Path(path).resolve().relative_to(ROOT))
    except ValueError:
        return str(path)


def find_cc():
    cc = os.environ.get("CC") or "gcc"
    if shutil.which(cc) is None:
        die("找不到 C 编译器 '{0}'。请安装 GCC/Clang，或设置环境变量 CC。".format(cc))
    return cc


def read_text(path):
    return Path(path).read_text(encoding="utf-8", errors="replace")


def resolve_source(name):
    p = Path(name)
    if not p.is_absolute():
        p = ROOT / p
    if not p.is_file():
        die("找不到源文件：{0}".format(name))
    return p


def resolve_output(output, stem):
    if output:
        p = Path(output)
        if not p.is_absolute():
            p = ROOT / p
        return p
    suffix = ".exe" if os.name == "nt" else ""
    return BUILD / (stem + suffix)


def unique_paths(paths):
    seen = set()
    out = []
    for p in paths:
        rp = Path(p).resolve()
        if rp not in seen:
            seen.add(rp)
            out.append(rp)
    return out


# ============================================================
#  构建
# ============================================================
def write_registry(sources):
    BUILD.mkdir(parents=True, exist_ok=True)
    files = unique_paths(list(sources) + sorted(ROOT.glob("*.h")))
    names = collect_identifiers(files)
    reg = BUILD / "ooooc_registry.h"
    reg.write_text(render_registry(names, origin="oooo.py"), encoding="utf-8")
    say("注册表：{0} 个标识符 -> {1}".format(len(names), rel(reg)))
    return reg


def release_flags(cc):
    """按编译器给优化参数：clang 的 LTO 需要 lld，也没有 -fno-plt。"""
    flags = ["-O3", "-march=native", "-DNDEBUG", "-Wall", "-Wextra"]
    name = Path(cc).stem.lower()
    if "clang" in name:
        if shutil.which("lld"):
            flags += ["-flto", "-fuse-ld=lld"]
    else:
        flags += ["-flto", "-fno-plt"]
    return flags


def build(sources, output=None, debug=False, werror=False,
          no_std=False, extra=None, quiet=False):
    cc = find_cc()
    sources = [resolve_source(s) for s in sources]
    if not sources:
        die("没有源文件可以编译。")

    reg = write_registry(sources)
    flags = ["-std=gnu11", "-I", str(ROOT), "-include", str(reg)]

    if debug:
        flags += ["-O0", "-g3", "-Wall", "-Wextra"]
    else:
        flags += release_flags(cc)
    if werror:
        flags.append("-Werror")
    if extra:
        flags.extend(extra)

    link = list(sources)
    std_src = ROOT / "ooooc_std.c"
    if (not no_std and std_src.is_file()
            and all(s.resolve() != std_src.resolve() for s in link)):
        link.append(std_src)

    out = resolve_output(output, sources[0].stem)
    out.parent.mkdir(parents=True, exist_ok=True)
    cmd = [cc] + flags + [str(s) for s in link] + ["-o", str(out)]

    if not quiet:
        say("编译：" + " ".join(cmd))

    started = time.time()
    result = subprocess.run(cmd)
    elapsed = time.time() - started

    if result.returncode != 0:
        warn("构建失败（{0:.2f}s）".format(elapsed))
        return None
    say("构建成功：{0}（{1:.2f}s）".format(rel(out), elapsed))
    return out


def run_exe(path):
    say("运行：" + rel(path))
    return subprocess.run([str(path)]).returncode


def do_test(debug=False, werror=True, extra=None):
    out = build(["tests.c"], "build/tests.exe", debug=debug,
                werror=werror, extra=extra)
    if not out:
        return 1
    return run_exe(out)


def do_demo(debug=False, extra=None):
    out = build(["demo_zoo.c"], "build/demo_zoo.exe", debug=debug,
                extra=extra)
    if not out:
        return 1
    return run_exe(out)


def do_bench(debug=False, extra=None):
    out = build(["bench.c"], "build/bench.exe", debug=debug,
                extra=extra)
    if not out:
        return 1
    return run_exe(out)


def do_zh(debug=False, extra=None):
    out = build(["demo_zh.c"], "build/demo_zh.exe", debug=debug,
                extra=extra)
    if not out:
        return 1
    return run_exe(out)


# ============================================================
#  轻量静态分析：解析 O语言宏
# ============================================================
def strip_comments(text):
    """把注释替换成空格，保留换行和字符串；长度不变，行号才不会漂。"""
    chars = list(text)
    i = 0
    n = len(chars)
    while i < n:
        c = chars[i]
        if c in ('"', "'"):
            quote = c
            i += 1
            while i < n:
                if chars[i] == "\\":
                    i += 2
                    continue
                if chars[i] == quote:
                    i += 1
                    break
                i += 1
            continue
        if c == "/" and i + 1 < n and chars[i + 1] == "/":
            while i < n and chars[i] not in "\r\n":
                chars[i] = " "
                i += 1
            continue
        if c == "/" and i + 1 < n and chars[i + 1] == "*":
            chars[i] = " "
            chars[i + 1] = " "
            i += 2
            while i < n:
                if chars[i] == "*" and i + 1 < n and chars[i + 1] == "/":
                    chars[i] = " "
                    chars[i + 1] = " "
                    i += 2
                    break
                if chars[i] not in "\r\n":
                    chars[i] = " "
                i += 1
            continue
        i += 1
    return "".join(chars)


def strip_preprocessor(text):
    """把预处理指令行清成空格，避免把 #define 里的宏名当成调用。"""
    out = []
    continuation = False
    for line in text.splitlines(keepends=True):
        if continuation or line.lstrip().startswith("#"):
            continuation = line.rstrip("\r\n").endswith("\\")
            out.append("".join(ch if ch in "\r\n" else " " for ch in line))
        else:
            out.append(line)
    return "".join(out)


def strip_strings(text):
    """把字符串/字符字面量的内容清成空格，保留换行。"""
    chars = list(text)
    i = 0
    n = len(chars)
    while i < n:
        c = chars[i]
        if c in ('"', "'"):
            quote = c
            chars[i] = " "
            i += 1
            while i < n:
                if chars[i] == "\\":
                    chars[i] = " "
                    if i + 1 < n and chars[i + 1] not in "\r\n":
                        chars[i + 1] = " "
                    i += 2
                    continue
                if chars[i] == quote:
                    chars[i] = " "
                    i += 1
                    break
                if chars[i] not in "\r\n":
                    chars[i] = " "
                i += 1
            continue
        i += 1
    return "".join(chars)


def skip_quoted(s, i, quote):
    i += 1
    while i < len(s):
        if s[i] == "\\":
            i += 2
            continue
        if s[i] == quote:
            return i + 1
        i += 1
    return i


def macro_calls(text, name):
    """返回 [(起始偏移, 括号内文本), ...]。"""
    pattern = re.compile(r"\b" + re.escape(name) + r"\s*\(")
    calls = []
    for m in pattern.finditer(text):
        i = m.end()
        start = i
        depth = 1
        while i < len(text) and depth > 0:
            c = text[i]
            if c == '"':
                i = skip_quoted(text, i, '"')
                continue
            if c == "'":
                i = skip_quoted(text, i, "'")
                continue
            if c == "(":
                depth += 1
            elif c == ")":
                depth -= 1
                if depth == 0:
                    calls.append((m.start(), text[start:i]))
                    i += 1
                    break
            i += 1
    return calls


def calls_any(text, names):
    """收集一组互为别名的宏调用，按出现位置排序。"""
    found = []
    seen = set()
    for name in names:
        for start, body in macro_calls(text, name):
            if start not in seen:
                seen.add(start)
                found.append((start, body))
    found.sort(key=lambda item: item[0])
    return found


def split_args(body):
    """按顶层逗号切参数，忽略嵌套括号和字符串里的逗号。"""
    parts = []
    cur = []
    depth = 0
    i = 0
    n = len(body)

    while i < n:
        c = body[i]
        if c == '"':
            j = skip_quoted(body, i, '"')
            cur.append(body[i:j])
            i = j
            continue
        if c == "'":
            j = skip_quoted(body, i, "'")
            cur.append(body[i:j])
            i = j
            continue
        if c in "([{":
            depth += 1
        elif c in ")]}":
            depth -= 1
        if c == "," and depth == 0:
            parts.append("".join(cur).strip())
            cur = []
        else:
            cur.append(c)
        i += 1

    if cur:
        parts.append("".join(cur).strip())
    return parts


def line_of(text, pos):
    return text.count("\n", 0, pos) + 1


def top_level_blocks(text):
    """返回文件里所有顶层 {...} 的 (起点, 终点)。

    O语言源码在宏展开前没有顶层结构体定义；顶层花括号基本就是函数体。
    去虚化建议只在同一个函数体内成立，所以需要这个粗略的切分。
    """
    blocks = []
    depth = 0
    start = None
    i = 0
    n = len(text)

    while i < n:
        c = text[i]
        if c == '"':
            i = skip_quoted(text, i, '"')
            continue
        if c == "'":
            i = skip_quoted(text, i, "'")
            continue
        if c == "{":
            if depth == 0:
                start = i
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0 and start is not None:
                blocks.append((start, i))
                start = None
            if depth < 0:
                depth = 0
        i += 1
    return blocks


def block_index(blocks, pos):
    for idx, (start, end) in enumerate(blocks):
        if start <= pos <= end:
            return idx
    return -1


def add_unique(target, key, value, warnings, what):
    if key in target:
        warnings.append("{0} '{1}' 在多个文件里出现，只保留第一次".format(what, key))
        return
    target[key] = value


def parse_project(files):
    data = {
        "classes": {},
        "vtables": {},
        "vtable_defs": {},
        "metas": {},
        "meta_ifaces": {},
        "inits": {},
        "interfaces": {},
        "ivtable_defs": {},
        "sends": [],
        "cast_count": 0,
        "new_count": 0,
        "new_sites": [],
        "files": list(files),
        "line_count": 0,
        "warnings": [],
    }

    new_re = re.compile(
        r"(?<!\w)([^\W\d]\w*)\s*\*\s*([^\W\d]\w*)\s*=\s*"
        r"(?:OOOOC_NEW|新建)\(\s*([^\W\d]\w*)\s*\)",
        re.UNICODE,
    )

    for path in files:
        raw = read_text(path)
        if raw:
            data["line_count"] += raw.count("\n") + (0 if raw.endswith("\n") else 1)

        text = strip_preprocessor(strip_comments(raw))
        scopes = top_level_blocks(text)
        warnings = data["warnings"]

        for pos, body in calls_any(text, ALIAS["root_class"]):
            args = split_args(body)
            if args:
                name = args[0]
                add_unique(data["classes"], name,
                           {"parent": None, "file": path,
                            "line": line_of(text, pos), "vtable": None},
                           warnings, "类")

        for pos, body in calls_any(text, ALIAS["class"]):
            args = split_args(body)
            if len(args) >= 2:
                name, parent = args[0], args[1]
                add_unique(data["classes"], name,
                           {"parent": parent, "file": path,
                            "line": line_of(text, pos), "vtable": None},
                           warnings, "类")

        for pos, body in calls_any(text, ALIAS["vtable"]):
            args = split_args(body)
            if not args:
                continue
            class_name = args[0]
            slots = []
            for piece in args[1:]:
                for _, mbody in calls_any(piece, ALIAS["vmethod"]):
                    margs = split_args(mbody)
                    if len(margs) >= 2:
                        ret = " ".join(margs[0].split())
                        mname = margs[1]
                        params = " ".join(margs[2].split()) if len(margs) > 2 else ""
                        slots.append((ret, mname, params))
            add_unique(data["vtables"], class_name, slots, warnings, "虚表")
            if class_name in data["classes"]:
                data["classes"][class_name]["vtable"] = slots

        for pos, body in calls_any(text, ALIAS["vtable_def"]):
            args = split_args(body)
            if not args:
                continue
            name = args[0]
            data["vtable_defs"][name] = {
                "count": max(0, len(args) - 1),
                "file": path,
                "line": line_of(text, pos),
            }

        for pos, body in calls_any(text, ALIAS["meta"]):
            args = split_args(body)
            if not args:
                continue
            name = args[0]
            data["metas"][name] = {"file": path, "line": line_of(text, pos)}
            entries = set()
            for piece in args[2:]:
                for _, ibody in calls_any(piece, ALIAS["iface"]):
                    iargs = split_args(ibody)
                    if len(iargs) >= 2:
                        entries.add(iargs[1])
            data["meta_ifaces"][name] = entries

        for pos, body in calls_any(text, ALIAS["init"]):
            args = split_args(body)
            if args:
                data["inits"][args[0]] = {
                    "file": path, "line": line_of(text, pos)
                }

        for pos, body in calls_any(text, ALIAS["interface"]):
            args = split_args(body)
            if not args:
                continue
            iface_name = args[0]
            methods = []
            for piece in args[1:]:
                for _, mbody in calls_any(piece, ALIAS["imethod"]):
                    margs = split_args(mbody)
                    if len(margs) >= 2:
                        ret = " ".join(margs[0].split())
                        mname = margs[1]
                        params = " ".join(margs[2].split()) if len(margs) > 2 else ""
                        methods.append((ret, mname, params))
            data["interfaces"][iface_name] = {
                "methods": methods,
                "file": path,
                "line": line_of(text, pos),
            }

        for pos, body in calls_any(text, ALIAS["ivtable"]):
            args = split_args(body)
            if len(args) >= 3:
                key = (args[0], args[1])
                data["ivtable_defs"][key] = {
                    "count": max(0, len(args) - 3),
                    "member": args[2],
                    "file": path,
                    "line": line_of(text, pos),
                }

        for pos, body in calls_any(text, ALIAS["send"]):
            data["sends"].append({
                "file": path,
                "line": line_of(text, pos),
                "block": block_index(scopes, pos),
                "body": body.strip(),
            })

        data["cast_count"] += len(calls_any(text, ALIAS["as"]))
        data["new_count"] += len(re.findall(r"(?<!\w)(?:OOOOC_NEW|新建)\s*\(", text))

        for m in new_re.finditer(text):
            typ, var, newcls = m.group(1), m.group(2), m.group(3)
            if typ == newcls:
                data["new_sites"].append({
                    "file": path,
                    "line": line_of(text, m.start()),
                    "block": block_index(scopes, m.start()),
                    "type": typ,
                    "var": var,
                })

    return data


def fmt_slot(slot):
    ret, name, params = slot
    return "{0} {1}({2})".format(ret, name, params)


def fmt_slots(slots):
    return "[" + ", ".join(fmt_slot(s) for s in slots) + "]"


def run_report(sources, strict=False):
    files = [resolve_source(s) for s in sources] if sources else None
    if not files:
        files = [ROOT / name for name in DEFAULT_REPORT if (ROOT / name).is_file()]
    files = unique_paths(files)

    if not files:
        die("没有可分析的文件。")

    data = parse_project(files)
    classes = data["classes"]
    vtables = data["vtables"]
    interfaces = data["interfaces"]
    ivtable_defs = data["ivtable_defs"]

    total_methods = sum(len(slots) for slots in vtables.values())
    print("=" * 60)
    print("  O语言静态分析 · 超智慧编译器 v" + VERSION)
    print("=" * 60)
    print("扫描：{0} 个文件，{1} 行".format(len(files), data["line_count"]))
    print("类：{0} 个    接口：{1} 个    虚方法：{2} 个".format(
        len(classes), len(interfaces), total_methods))
    print("OOOOC_SEND/发送：{0} 处    OOOOC_AS/当作：{1} 处    OOOOC_NEW/新建：{2} 处".format(
        len(data["sends"]), data["cast_count"], data["new_count"]))

    # ---- 类层次 ----
    print("\n类层次")
    if not classes:
        print("  （没有发现类）")
    else:
        children = {}
        roots = []
        for name, info in classes.items():
            parent = info["parent"]
            if parent and parent in classes:
                children.setdefault(parent, []).append(name)
            else:
                roots.append(name)

        def walk(name, prefix):
            kids = sorted(children.get(name, []))
            for idx, kid in enumerate(kids):
                last = idx == len(kids) - 1
                slots = len(vtables.get(kid, []))
                print("  {0}{1}{2} : {3}  ({4} 槽)".format(
                    prefix, "`- " if last else "|- ", kid,
                    classes[kid]["parent"], slots))
                walk(kid, prefix + ("   " if last else "|  "))

        for root in sorted(roots):
            slots = len(vtables.get(root, []))
            print("  {0} [root]  ({1} 槽)".format(root, slots))
            walk(root, "  ")

    # ---- 接口 ----
    print("\n接口")
    if not interfaces:
        print("  （没有发现接口）")
    else:
        impls = {}
        for (cls, iface) in ivtable_defs:
            impls.setdefault(iface, set()).add(cls)
        for name in sorted(interfaces):
            methods = interfaces[name]["methods"]
            who = ", ".join(sorted(impls.get(name, set()))) or "-"
            print("  {0}  ({1} 槽) <- {2}".format(name, len(methods), who))

    # ---- 检查 ----
    issues = []

    for name in sorted(classes):
        info = classes[name]
        if name not in vtables:
            issues.append("{0}: 缺少 OOOOC_VTABLE".format(name))
        if name not in data["vtable_defs"]:
            issues.append("{0}: 缺少 OOOOC_DEFINE_VTABLE".format(name))
        if name not in data["metas"]:
            issues.append("{0}: 缺少 OOOOC_DEFINE_META（链接会失败）".format(name))
        if name not in data["inits"]:
            issues.append("{0}: 缺少 OOOOC_DEFINE_INIT（无法 OOOOC_NEW）".format(name))
        if name in vtables and name in data["vtable_defs"]:
            want = len(vtables[name])
            got = data["vtable_defs"][name]["count"]
            if want != got:
                issues.append(
                    "{0}: 虚表有 {1} 槽，只填了 {2} 个实现".format(name, want, got))

        parent = info["parent"]
        if parent and parent not in classes:
            issues.append("{0}: 父类 {1} 不在扫描范围".format(name, parent))
        if parent in vtables and name in vtables:
            pslots = vtables[parent]
            cslots = vtables[name]
            if cslots[:len(pslots)] != pslots:
                issues.append(
                    "{0}: 虚表前缀与 {1} 不兼容。父类 {2}，子类 {3}".format(
                        name, parent, fmt_slots(pslots), fmt_slots(cslots)))

    for (cls, iface), info in sorted(ivtable_defs.items()):
        if iface not in interfaces:
            issues.append("{0}: 实现了未知接口 {1}".format(cls, iface))
        else:
            want = len(interfaces[iface]["methods"])
            if info["count"] != want:
                issues.append(
                    "{0}::{1}: 接口有 {2} 个方法，虚表填了 {3} 个".format(
                        cls, iface, want, info["count"]))
        if iface not in data["meta_ifaces"].get(cls, set()):
            issues.append(
                "{0}: 有 {1} 的接口虚表，但 META 里没有 OOOOC_IFACE({0}, {1}, ...)".format(
                    cls, iface))

    for cls, ifaces in data["meta_ifaces"].items():
        for iface in sorted(ifaces):
            if (cls, iface) not in ivtable_defs:
                issues.append(
                    "{0}: META 声明了 {1}，但没有 OOOOC_DEFINE_IVTABLE".format(cls, iface))

    print("\n检查")
    if not issues:
        print("  [OK] 类定义、虚表槽位、继承前缀、接口绑定全部一致")
    else:
        for msg in issues:
            print("  [WARN] " + msg)
        print("  共 {0} 条提示（有些类可能只是抽象声明，不一定是错误）".format(len(issues)))

    # ---- 去虚化建议 ----
    hints = []
    seen_hints = set()
    for site in data["new_sites"]:
        for send in data["sends"]:
            if send["file"] != site["file"]:
                continue
            # 只信同一函数体：跨函数同名变量是误报重灾区
            if send["block"] != site["block"]:
                continue
            args = split_args(send["body"])
            if (len(args) >= 2 and args[0] == site["type"]
                    and args[1] == site["var"]):
                key = (send["file"], send["line"], send["body"])
                if key in seen_hints:
                    continue
                seen_hints.add(key)
                hints.append((send, site))

    print("\n去虚化建议（启发式）")
    if not hints:
        print("  （没有发现“对象类型可静态确定”的 OOOOC_SEND）")
    else:
        print("  共 {0} 处可考虑 OOOOC_DEVIRT（省一次间接跳转）".format(len(hints)))
        for send, site in hints[:5]:
            call = "OOOOC_SEND(" + send["body"] + ")"
            if len(call) > 64:
                call = call[:61] + "..."
            print("  · {0}:{1}  {2}  <- {3} 由 OOOOC_NEW({3}) 创建".format(
                rel(send["file"]), send["line"], call, site["type"]))
        if len(hints) > 5:
            print("  · ... 其余 {0} 处略".format(len(hints) - 5))

    if data["warnings"]:
        print("\n解析提示")
        for msg in data["warnings"][:10]:
            print("  [WARN] " + msg)

    sys.stdout.flush()
    return 1 if (strict and issues) else 0


# ============================================================
#  命令实现
# ============================================================
def cmd_build(args):
    sources = args.sources or ["demo_zoo.c"]
    out = build(sources, args.output, debug=args.debug, werror=args.werror,
                no_std=args.no_std, extra=args.extra)
    return 0 if out else 1


def cmd_run(args):
    sources = args.sources or ["demo_zoo.c"]
    out = build(sources, args.output, debug=args.debug, werror=args.werror,
                no_std=args.no_std, extra=args.extra)
    if not out:
        return 1
    return run_exe(out)


def cmd_test(args):
    return do_test(debug=args.debug, werror=not args.no_werror,
                   extra=args.extra)


def cmd_demo(args):
    return do_demo(debug=args.debug, extra=args.extra)


def cmd_bench(args):
    return do_bench(debug=args.debug, extra=args.extra)


def cmd_zh(args):
    return do_zh(debug=args.debug, extra=args.extra)


def cmd_confidence(args):
    files = [p for p in sorted(ROOT.glob("*.c")) + sorted(ROOT.glob("*.h"))]
    if not files:
        die("没有可以检查的源文件。")

    total_cjk = 0
    ident_hits = 0
    ident_names = set()
    comment_lines = 0
    keywords = 0
    board = []

    for path in files:
        raw = read_text(path)
        cjk = len(CJK_RE.findall(raw))
        total_cjk += cjk

        code = strip_strings(strip_comments(raw))
        names = [m.group(0) for m in CODE_IDENT_RE.finditer(code)
                 if CJK_RE.search(m.group(0))]
        ident_hits += len(names)
        ident_names.update(names)

        for line in raw.splitlines():
            if (CJK_RE.search(line)
                    and ("//" in line or "/*" in line or line.lstrip().startswith("*"))):
                comment_lines += 1

        keywords += len(C_KEYWORDS.findall(code))
        board.append((len(names), cjk, path.name))

    pack = all((ROOT / name).is_file()
               for name in ("cccccc.h", "cccccc_std.h"))
    demo = (ROOT / "demo_zh.c").is_file()

    print("=" * 60)
    print("  CCCCCC 文化自信检查")
    print("=" * 60)
    print("  扩展包        : " + ("已装载" if pack else "未装载"))
    print("  中文演示      : " + ("已就绪" if demo else "未找到"))
    print("  源文件        : {0} 个".format(len(files)))
    print("  中文字符      : {0} 个".format(total_cjk))
    print("  中文标识符    : {0} 次（{1} 个不同的名字）".format(
        ident_hits, len(ident_names)))
    print("  含中文注释行  : {0} 行".format(comment_lines))
    print("  洋文 C 关键字 : {0} 个（编译器的签证，不是思想问题）".format(keywords))

    print("\n  汉化榜：")
    for hits, cjk, name in sorted(board, reverse=True)[:5]:
        print("    {0:<18} 中文标识符 {1:>3} 次，中文字符 {2:>4} 个".format(
            name, hits, cjk))

    print("\n  结论：")
    print("    中文能当标识符，编译器不查护照。")
    print("    脊梁骨：挺直。")
    print("\n  （本检查是玩笑：文化自信不是排斥英文，")
    print("    是让中文也能当第一等公民。）")
    return 0


def cmd_report(args):
    return run_report(args.sources, strict=args.strict)


def cmd_registry(args):
    if args.sources:
        files = [resolve_source(s) for s in args.sources]
    else:
        files = [p for p in sorted(ROOT.glob("*.c")) + sorted(ROOT.glob("*.h"))]
    files = unique_paths(files)
    if not files:
        die("没有可以扫描的文件。")
    files = unique_paths(files + sorted(ROOT.glob("*.h")))
    names = collect_identifiers(files)
    text = render_registry(names, origin="oooo.py")

    if args.output:
        out = Path(args.output)
        if not out.is_absolute():
            out = ROOT / out
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(text, encoding="utf-8")
        say("注册表：{0} 个标识符 -> {1}".format(len(names), rel(out)))
    else:
        sys.stdout.write(text)
    return 0


def cmd_clean(args):
    if BUILD.exists():
        if BUILD.name != "build" or BUILD.resolve().parent != ROOT.resolve():
            die("拒绝删除可疑目录：" + str(BUILD))
        shutil.rmtree(BUILD)
        say("build/ 已清空。超智慧也需要定期清理。")
    else:
        say("build/ 本来就很干净。")
    return 0


def cmd_doctor(args):
    cc = find_cc()
    print("=" * 60)
    print("  O语言环境体检 · 超智慧编程，真实不虚")
    print("=" * 60)
    print("  Python      : " + sys.version.split()[0])
    print("  项目根目录  : " + str(ROOT))

    version = subprocess.run([cc, "--version"], capture_output=True,
                             text=True, errors="replace")
    lines = (version.stdout or version.stderr).strip().splitlines()
    print("  编译器      : " + (lines[0] if lines else cc))

    sources = sorted(p.name for p in ROOT.glob("*.c"))
    headers = sorted(p.name for p in ROOT.glob("*.h"))
    print("  源文件      : " + (", ".join(sources) if sources else "（无）"))
    print("  头文件      : " + (", ".join(headers) if headers else "（无）"))
    print("  build/      : " + ("存在" if BUILD.exists() else "不存在"))
    pack = all((ROOT / name).is_file()
               for name in ("cccccc.h", "cccccc_std.h"))
    print("  CCCCCC      : " + ("已装载 v" + CCCCCC_VERSION if pack else "未装载"))

    probe = subprocess.run(
        [cc, "-std=gnu11", "-Wall", "-Wextra", "-x", "c", "-", "-fsyntax-only"],
        input="int main(void) { return 0; }",
        text=True, capture_output=True)
    print("  编译自检    : " + ("OK" if probe.returncode == 0 else "FAILED"))
    if probe.returncode != 0:
        print(probe.stderr.strip())

    print("\n  提示：python oooo.py all 一条龙验收。")
    return 0


def cmd_all(args):
    print("=" * 60, flush=True)
    print("  超智慧全流程验收", flush=True)
    print("=" * 60, flush=True)

    say("---- 1/5 测试 ----")
    code = do_test(debug=args.debug, werror=not args.no_werror)
    if code != 0:
        return code

    say("---- 2/5 静态分析 ----")
    run_report([], strict=False)

    say("---- 3/5 旗舰演示 ----")
    code = do_demo(debug=args.debug)
    if code != 0:
        return code

    say("---- 4/5 中文演示 ----")
    code = do_zh(debug=args.debug)
    if code != 0:
        return code

    say("---- 5/5 性能基准 ----")
    return do_bench(debug=args.debug)


def cmd_version(args):
    print("超智慧编程 · O语言 v" + VERSION)
    print("oooo.py v" + TOOL_VERSION)
    print("CCCCCC 扩展包 v" + CCCCCC_VERSION + "（中文关键字 · 文化自信检查）")
    print()
    print("  超智慧编程三定律")
    print("  一、能用宏解决的，绝不写运行时代码。")
    print("  二、宏解决不了的，就写进诚实声明。")
    print("  三、真实不虚。")
    return 0


# ============================================================
#  入口
# ============================================================
def make_parser():
    parser = argparse.ArgumentParser(
        prog="oooo",
        description="O语言工具链：构建、测试、基准、静态分析。超智慧编程，真实不虚。")
    parser.add_argument("--version", action="store_true", help="显示版本")
    sub = parser.add_subparsers(dest="command")

    p = sub.add_parser("build", help="编译源文件")
    p.add_argument("sources", nargs="*", help="源文件（默认 demo_zoo.c）")
    p.add_argument("-o", "--output", help="输出文件")
    p.add_argument("--debug", action="store_true", help="调试构建：-O0 -g3")
    p.add_argument("--werror", action="store_true", help="警告即错误")
    p.add_argument("--no-std", action="store_true", help="不自动链接 ooooc_std.c")
    p.add_argument("--extra", action="append", default=[], metavar="FLAG",
                   help="追加编译器参数，可多次")
    p.set_defaults(func=cmd_build)

    p = sub.add_parser("run", help="编译并运行")
    p.add_argument("sources", nargs="*", help="源文件（默认 demo_zoo.c）")
    p.add_argument("-o", "--output", help="输出文件")
    p.add_argument("--debug", action="store_true")
    p.add_argument("--werror", action="store_true")
    p.add_argument("--no-std", action="store_true")
    p.add_argument("--extra", action="append", default=[], metavar="FLAG")
    p.set_defaults(func=cmd_run)

    p = sub.add_parser("test", help="编译并运行测试")
    p.add_argument("--debug", action="store_true")
    p.add_argument("--no-werror", action="store_true", help="不把警告当错误")
    p.add_argument("--extra", action="append", default=[], metavar="FLAG")
    p.set_defaults(func=cmd_test)

    p = sub.add_parser("demo", help="编译并运行动物园演示")
    p.add_argument("--debug", action="store_true")
    p.add_argument("--extra", action="append", default=[], metavar="FLAG")
    p.set_defaults(func=cmd_demo)

    p = sub.add_parser("bench", help="编译并运行性能基准")
    p.add_argument("--debug", action="store_true")
    p.add_argument("--extra", action="append", default=[], metavar="FLAG")
    p.set_defaults(func=cmd_bench)

    p = sub.add_parser("zh", aliases=["中文"],
                       help="编译并运行 CCCCCC 中文演示")
    p.add_argument("--debug", action="store_true")
    p.add_argument("--extra", action="append", default=[], metavar="FLAG")
    p.set_defaults(func=cmd_zh)

    p = sub.add_parser("confidence", aliases=["自信"],
                       help="文化自信检查（玩笑）")
    p.set_defaults(func=cmd_confidence)

    p = sub.add_parser("report", help="静态分析类、接口、虚表")
    p.add_argument("sources", nargs="*", help="要分析的文件（默认核心文件）")
    p.add_argument("--strict", action="store_true", help="有提示时返回非零")
    p.set_defaults(func=cmd_report)

    p = sub.add_parser("registry", help="生成符号长度注册表")
    p.add_argument("sources", nargs="*")
    p.add_argument("-o", "--output", help="写入文件；省略则输出到 stdout")
    p.set_defaults(func=cmd_registry)

    p = sub.add_parser("clean", help="清空 build/")
    p.set_defaults(func=cmd_clean)

    p = sub.add_parser("doctor", help="检查环境")
    p.set_defaults(func=cmd_doctor)

    p = sub.add_parser("all", help="测试 + 分析 + 演示 + 中文演示 + 基准")
    p.add_argument("--debug", action="store_true")
    p.add_argument("--no-werror", action="store_true")
    p.set_defaults(func=cmd_all)

    p = sub.add_parser("version", help="显示版本")
    p.set_defaults(func=cmd_version)

    return parser


def main(argv):
    parser = make_parser()
    args = parser.parse_args(argv[1:])

    if args.version or not getattr(args, "command", None):
        return cmd_version(args)

    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
