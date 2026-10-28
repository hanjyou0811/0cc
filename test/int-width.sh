#!/bin/sh
set -eu

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
failed=0

check() {
    name=$1
    source=$2
    printf '%s\n' "$source" > "$work/input.c"
    ./0cc "$work/input.c" > "$work/output.s"
    ${CC:-cc} -o "$work/output" "$work/output.s"
    actual=0
    "$work/output" || actual=$?
    if [ "$actual" -ne 1 ]; then
        echo "FAIL: $name (expected 1, got $actual)"
        failed=1
    else
        echo "PASS: $name"
    fi
}

check 'int array store preserves next element' \
    'int main(){int a[2]; a[1]=2; a[0]=1; return a[1]==2;}'
check 'int array load excludes next element' \
    'int main(){int a[2]; a[0]=1; a[1]=2; return a[0]==1;}'
check 'negative int load sign extends' \
    'int main(){int a[2]; a[0]=-1; a[1]=0; return a[0]<0;}'
check 'int pointer store preserves next element' \
    'int main(){int a[2]; int *p=a; a[1]=2; *p=1; return a[1]==2;}'
check 'int pointer load excludes next element' \
    'int main(){int a[2]; int *p=a; a[0]=-1; a[1]=0; return *p<0;}'
check 'global int load excludes next variable' \
    'int a=1; int b=2; int main(){return b==2;}'
check 'global int store preserves next variable' \
    'int a=1; int b=2; int main(){b=3; return a==1;}'
check 'six int parameters retain their values' \
    'int f(int a,int b,int c,int d,int e,int f){return a+b+c+d+e+f;} int main(){return f(-1,2,3,4,5,6)==19;}'
check 'pointer parameters retain full addresses' \
    'int f(int *p){return *p;} int main(){int x=42; return f(&x)==42;}'
check 'char store preserves next element' \
    'int main(){char a[2]; a[1]=2; a[0]=1; return a[1]==2;}'

exit "$failed"
