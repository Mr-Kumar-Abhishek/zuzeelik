# zuzeelik
A programming language similar to lisp.

* compilation: `cc -std=c99 -Wall zuzeelik.c mpc/mpc.c -ledit -lm -o zuzeelik`
* cloning: `git clone --recursive https://github.com/Mr-Kumar-Abhishek/zuzeelik.git`

#####Examples
* input: `zuzeelik> + 4 ( / 4 2)`
* output:
```
> 
  regex 
  operator|char:1:1 '+'
  expression|number|regex:1:3 '4'
  expression|> 
    char:1:5 '('
    operator|char:1:7 '/'
    expression|number|regex:1:9 '4'
    expression|number|regex:1:11 '2'
    char:1:12 ')'
  regex 
```
