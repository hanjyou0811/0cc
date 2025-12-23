#!/bin/bash
assert() {
  expected="$1"
  input="$2"

  ./0cc "$input" > tmp.s
  cc -o tmp tmp.s
  ./tmp
  actual="$(./tmp)"

  if [ "$actual" = "$expected" ]; then
    echo "'$input' :=> $actual"
  else
    echo "'$input' :=> $expected expected, but got $actual"
    exit 1
  fi
}

assert '12345678910->11:ascii(48):str(0)' "int mod(int x, int y){
	return x - (x / y * y);
}
int pow(int x, int y){
	int ret = 1;
	for(int i=0;i<y;i+=1){
		ret *= x;
	}
	return ret;
}
int print_num(int x){
	if(x == 0){
		char c = x + 48;
		write(1, &c, 1);
		return 0;
	}
	int i = 0, num = x;
	while(num){
		i += 1;
		num /= 10;
	}
	for(num = x;i > 0;i -= 1){
		int p = pow(10, i-1);
		char tmp = num / p + 48;
		write(1, &tmp, 1);
		num -= num / p * p;
	}
}
int main(){
  int x = 10;
  for(int i=0;i<x;i+=1){
    print_num(i + 1);
  }
  char str[12] = \"->11:ascii(\";
  write(1, str, 11);
  int num = 48;
  print_num(num);
  char c[6] = \"):str(\";
  write(1, &c, 6);
  write(1, &num, 1);
  c = \")\";
  write(1, &c, 1);
  return 42;
}"

echo OK
