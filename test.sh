#!/bin/bash
assert() {
  expected="$1"
  input="$2"

  ./0cc "$input" > tmp.s
  cc -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert 0 0
assert 42 42
assert 255 255
assert 21 "5+20-4"
assert 0 "1-1"
assert 255 "254+1"
assert 41 " 12 + 34 - 5"
assert 0 " 255 - 255 "
assert 255 " 0 + 255 "

echo OK
