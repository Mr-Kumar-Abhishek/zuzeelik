# zuzeelik

[![Join the chat at https://gitter.im/Mr-Kumar-Abhishek/zuzeelik](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/Mr-Kumar-Abhishek/zuzeelik?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge) [![Build Status](https://travis-ci.org/Mr-Kumar-Abhishek/zuzeelik.svg?branch=master)](https://travis-ci.org/Mr-Kumar-Abhishek/zuzeelik)

A programming language similar to lisp.

####Cloning:

Remember to get the submodules while cloning.
```
git clone --recursive <git repository url here>
```
Already have a copy of the repository locally without submodules ? Get them with:
```
git submodule update --init
```

####Compilation:

* **For Linux:**
Compile using c99. Libraries that are needed to be linked while compilation are *math* library and *editline* library.

``` 
cc -std=c99 -Wall zuzeelik.c mpc/mpc.c -ledit -lm -o zuzeelik
```
Or this could be simply done by using make.

```
make
```

####Syntax and operators:

Zuzeelik follows [polish notation](http://en.wikipedia.org/wiki/Polish_notation) while reading inputs. Those who are not familiar with it , try prefixing operators, such as:

* `1 + 4 + 9` becomes `+ 1 4 9`
* `8 - ( 5 * 6) ` becomes  `- 8 (* 5 6)`
* `(10 / 5) * (10 /2)` becomes `* (/ 10 5) ( / 10 2)` 

Currently it supports `+` , `-`, `/` , `*`, `%` and `^` symbolic operators. In textual operators it supports `add`, `sub`, `mul`, `div`, `mod`, `min`, `max` and `pow`. Syntax is same for using symbolic and textual operators. 

#####Examples:
* input: 
 ```
 zuzeelik> + 4 ( / 4 2)
 ```
 
* output:
```

Abstract Syntax Tree:

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


Evaluated output: 6
```
