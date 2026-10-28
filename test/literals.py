"""Compile and run literal regressions, including malformed input diagnostics."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile


cases = [
    ("character", "int main(){return 'a';}", 97, b""),
    ("character type", "int main(){return sizeof 'a';}", 4, b""),
    ("escaped quote", r"int main(){return '\'';}", 39, b""),
    ("escaped backslash", r"int main(){return '\\';}", 92, b""),
    ("octal character", r"int main(){return '\101';}", 65, b""),
    ("hex character", r"int main(){return '\x41';}", 65, b""),
    ("high byte character", r"int main(){return '\xff';}", 255, b""),
    ("high byte string", r'int main(){char *p="\377"; return p[0];}', 255, b""),
    ("octal stops after three digits", r'int main(){char *p="\1012"; return p[1];}', 50, b""),
    ("hex uppercase digits", r"int main(){return '\x4A';}", 74, b""),
    ("empty string", 'int main(){return sizeof "";}', 1, b""),
    ("string size", r'int main(){return sizeof "a\nb";}', 4, b""),
    ("string pointer", 'int main(){char *p="abc"; return p[1];}', 98, b""),
    ("string pointer assignment", 'int main(){char *p; p="abc"; return p[2];}', 99, b""),
    ("literal index", 'int main(){return "abc"[1];}', 98, b""),
    ("literal dereference", 'int main(){return *"abc";}', 97, b""),
    ("literal array address", 'int main(){return (*(&"abc"))[1];}', 98, b""),
    ("embedded NUL", r'int main(){char *p="a\0b"; return p[2];}', 98, b""),
    ("empty string terminator", 'int main(){char *p=""; return p[0];}', 0, b""),
    ("adjacent strings", 'int main(){char *p="ab" "cd"; return p[3];}', 100, b""),
    ("adjacent strings with NUL", r'int main(){char *p="a\0" "bc"; return p[3];}', 99, b""),
    ("adjacent strings size", r'int main(){return sizeof "a\0" "bc";}', 5, b""),
    ("direct argument", 'int main(){puts("hello"); return 0;}', 0, b"hello\n"),
    ("escaped output", r'int main(){puts("a\n\t\"\\b"); return 0;}', 0, b'a\n\t"\\b\n'),
    ("numeric string escapes", r'int main(){puts("\101\x42"); return 0;}', 0, b"AB\n"),
    ("array terminator", 'int main(){char a[4]="abc"; return a[3];}', 0, b""),
    ("array zero fill", 'int main(){char a[6]="a"; return a[5];}', 0, b""),
    ("array embedded NUL", r'int main(){char a[4]="a\0b"; return a[2];}', 98, b""),
    ("array exact fit", 'int main(){char a[3]="abc"; return a[2];}', 99, b""),
    ("array inferred size", 'int main(){char a[]="abc"; return sizeof a;}', 4, b""),
    ("empty inferred array", 'int main(){char a[]=""; return sizeof a;}', 1, b""),
    ("empty array zero fill", 'int main(){char a[8]=""; return a[7];}', 0, b""),
    ("inferred array after scalar", 'int main(){int x=42; char a[]="abcdefghijklmn"; return x;}', 42, b""),
    ("array after scalar", 'int main(){int x=42; char a[16]="abcdefghijklmn"; return x;}', 42, b""),
    ("literal expression statement", 'int main(){"abc"; return 42;}', 42, b""),
    ("multiple literals", 'int main(){puts("one"); puts("two"); return 0;}', 0, b"one\ntwo\n"),
]
for escape, value in [("a", 7), ("b", 8), ("t", 9), ("n", 10),
                      ("v", 11), ("f", 12), ("r", 13), ("0", 0),
                      ('"', 34), ("?", 63)]:
    cases.append((f"escape {escape}", "int main(){return '\\" + escape + "';}", value, b""))

invalid = [
    ("empty character", "int main(){return '';}", "文字"),
    ("multiple characters", "int main(){return 'ab';}", "文字"),
    ("unclosed character", "int main(){return 'a;}", "文字"),
    ("newline in character", "int main(){return '\n';}", "文字"),
    ("trailing backslash", 'int main(){puts("abc' + '\\', "文字列"),
    ("unclosed string", 'int main(){puts("abc);}', "文字列"),
    ("newline in string", 'int main(){puts("a\nb");}', "文字列"),
    ("unknown escape", r'int main(){puts("\q");}', "エスケープ"),
    ("hex missing digits", r"int main(){return '\x';}", "エスケープ"),
    ("hex out of range", r"int main(){return '\x100';}", "エスケープ"),
    ("hex consumes all digits", r'int main(){puts("\x41B");}', "エスケープ"),
    ("octal out of range", r"int main(){return '\400';}", "エスケープ"),
    ("array overflow", 'int main(){char a[2]="abc"; return 0;}', "初期化"),
    ("non char array", 'int main(){int a[4]="abc"; return 0;}', "初期化"),
    ("array assignment", 'int main(){char a[4]; a="abc"; return 0;}', "配列"),
    ("inferred array without initializer", 'int main(){char a[]; return 0;}', "配列"),
    ("inferred array without string", 'int main(){char a[]=1; return 0;}', "初期化"),
]

failed = 0
with tempfile.TemporaryDirectory(prefix="0cc-literals-") as directory:
    root = Path(directory)
    for name, source, expected, stdout in cases:
        (root / "input.c").write_text(source + "\n")
        result = subprocess.run(["./0cc", str(root / "input.c")], capture_output=True, timeout=5)
        try:
            assert result.returncode == 0, result.stderr.decode()
            (root / "output.s").write_bytes(result.stdout)
            subprocess.run(shlex.split(os.environ.get("CC", "cc")) +
                           ["-o", str(root / "output"), str(root / "output.s")],
                           check=True, capture_output=True, timeout=10)
            result = subprocess.run([str(root / "output")], capture_output=True, timeout=5)
            assert (result.returncode, result.stdout) == (expected, stdout), repr((result.returncode, result.stdout))
            print(f"PASS: {name}")
        except (AssertionError, subprocess.SubprocessError) as error:
            failed += 1
            print(f"FAIL: {name}: {error}")
    for name, source, diagnostic in invalid:
        (root / "input.c").write_text(source + "\n")
        result = subprocess.run(["./0cc", str(root / "input.c")], capture_output=True, timeout=5)
        if result.returncode != 1 or diagnostic.encode() not in result.stderr:
            failed += 1
            print(f"FAIL: {name}: expected diagnostic {diagnostic!r}, got {result.returncode}, {result.stderr!r}")
        else:
            print(f"PASS: {name}")
raise SystemExit(bool(failed))
