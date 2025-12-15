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

assert 0   "main(){return 0;}"
assert 42  "main(){return 42;}"
assert 255 "main(){return 255;}"
assert 21  "main(){return 5+20-4;}"
assert 0   "main(){return 1-1;}"
assert 255 "main(){return 254+1;}"
assert 41  "main(){return  12 + 34 - 5;}"
assert 0   "main(){ return 255 - 255 ;}"
assert 255 "main(){ return 0 + 255 ;}"
assert 0   "main(){ return 0 * 1;}"
assert 5   "main(){ return 15 / 3;}"
assert 7   "main(){ return 1 + 2 * 3;}"
assert 18  "main(){return (2+4) * (4 - 1);}"
assert 255 "main(){return 2 * 128 - 1;}"
assert 47  "main(){return 5+6*7;}"
assert 15  "main(){return 5*(9-6);}"
assert 4   "main(){return (3+5)/2;}"
assert 0   "main(){return -12 + 12;}"
assert 10  "main(){return -10 + 20;}"
assert 1   "main(){return 0 == 0;}"
assert 0   "main(){return 0 == 1;}"
assert 1   "main(){return 0 != 1;}"
assert 0   "main(){return 0 != 0;}"
assert 0   "main(){return 0 < 0;}"
assert 1   "main(){return 0 >= 0;}"
assert 0   "main(){return (1 + 3) > (2 * 6);}"
assert 1   "main(){return 255 >= (254 + 1);}"
assert 3   "main(){a = 3; return a;}"
assert 7   "main(){a=1, b=2, c=3; return a + b * c;}"
assert 10  "main(){ a = 3; b = 2; return (a + 2) * b;}"
assert 21  "main(){a = 3, b = 7; return (a * b);}"
assert 1   "main(){if (1) return 1;}"
assert 42  "main(){if (0) return 1; else return (42);}"
assert 1   "main(){return a = 1, b = 0; if(a > b) return 1;else return 42;}"
assert 42  "main(){if(0) return 1; else if(0) return 1; else if (0) return 1; if(1) return 42;}"
assert 1   "main(){if(0) return 42; else if(0) return 42; else if (0) return 42; else return 1;}"
assert 42  "main(){if(0) return 1; else if(1) return 42; else if (0) return 1; else return 1;}"
assert 1   "main(){if(1) if(1) if(1) if(1) if(1) return 1;}"
assert 0   "main(){if(1) if(1) if(1) if(1) if(0) return 1;else return 0;}"
assert 4  "main(){ a = 10; i = 0;while(a > 0) {a = a/2; i = i + 1;} return i;}"
assert 55  "main(){ret = 0;for(a=1;a<11;a = a + 1) ret = ret + a; return ret;}"
assert 10  "main(){ret = 0; for(;ret < 10;ret = ret + 1) ret; return ret;}"
assert 30  "main(){ret = 0; for(a = 1; a< 11; a = a + 1) if((a - (a / 2) * 2) == 0) ret = ret + a; return ret;}"
assert 35  "main(){ret = 0; for(a = 1; a< 11; a = a + 1) if((a - (a / 2) * 2) == 0) ret = ret + a; else ret = ret + 1;return ret;}"
assert 0   "main(){{1;2;3;}}"
assert 4   "main(){{a = 1, b = 2; return a * ( b + b);}}"
assert 1   "main(){{{{{{{{{{return 1;}}}}}}}}}}"
assert 6   "calcmod(x, y) {return (x - (x / y) * y);} main(){x = 16, y= 10; return calcmod(x, y);}"

echo OK
