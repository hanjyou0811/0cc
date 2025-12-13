#!/bin/bash
assert() {
  expected="$1"
  input="$2"

  ./0cc "$input" > tmp.s
  cc -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "'$input' :=> $actual"
  else
    echo "'$input' :=> $expected expected, but got $actual"
    exit 1
  fi
}

assert 0 "0;"
assert 42 "42;"
assert 255 "255;"
assert 21 "5+20-4;"
assert 0 "1-1;"
assert 255 "254+1;"
assert 41 " 12 + 34 - 5;"
assert 0 " 255 - 255 ;"
assert 255 " 0 + 255 ;"
assert 0 " 0 * 1;"
assert 5 "15 / 3;"
assert 7 "1 + 2 * 3;"
assert 18 "(2+4) * (4 - 1);"
assert 255 "2 * 128 - 1;"
assert 47 '5+6*7;'
assert 15 '5*(9-6);'
assert 4 '(3+5)/2;'
assert 0 '-12 + 12;'
assert 10 '-10 + 20;'
assert 1 '0 == 0;'
assert 0 '0 == 1;'
assert 1 '0 != 1;'
assert 0 '0 != 0;'
assert 0 'return 0 < 0;'
assert 1 '0 >= 0;'
assert 0 '(1 + 3) > (2 * 6);'
assert 1 '255 >= (254 + 1);'
assert 3 'a = 3; return a;'
assert 7 'a=1, b=2, c=3; return a + b * c;'
assert 10 'a = 3; b = 2; return (a + 2) * b;'
assert 21 'a = 3, b = 7; return (a * b);'
assert 1 'if (1) 1;'
assert 42 'if (0) 1; else return (42);'
assert 1 'a = 1, b = 0; if(a > b) return 1;else return 42;'
assert 42 'if(0) 1; else if(0) 1; else if (0) 1; if(1) 42;' 
assert 1 'if(0) 42; else if(0) 42; else if (0) 42; else 1;'
assert 42 'if(0) 1; else if(1) 42; else if (0) 1; else 1;'
assert 1 'if(1) if(1) if(1) if(1) if(1) 1;'
assert 0 'if(1) if(1) if(1) if(1) if(0) 1;else 0;'
assert 42 'while(0) 1; 42;'
assert 55 ' ret = 0;for(a=1;a<11;a = a + 1) ret = ret + a; return ret;'
assert 10 ' ret = 0; for(;ret < 10;ret = ret + 1) ret; return ret;'
assert 30 ' ret = 0; for(a = 1; a< 11; a = a + 1) if((a - (a / 2) * 2) == 0) ret = ret + a; return ret;'
assert 35 ' ret = 0; for(a = 1; a< 11; a = a + 1) if((a - (a / 2) * 2) == 0) ret = ret + a; else ret = ret + 1;return ret;'
assert 3 '{1;2;3;}'
assert 4 '{a = 1, b = 2; return a * ( b + b);}'

echo OK
