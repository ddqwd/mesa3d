
### statements and expressions


statements do not return values, 
```rust
fn main() {

    let x = (let y = 6);
}

```
在其他语言中 x=y=6
在rust中不返回任何值


```rust
fn main(){
    let y = {
        let x= 3;
        x + 1
    };
    
}
```
在x+1后面没有分号，这值是直接返回给y


## 带有返回值的函数

```
fn five() -> i32 {
    5
}

fn plus_one(x:i32) -> i32 {

    x + 1;
}
fn main() {
    let x = plus_one(5);
    println("The value of x is : {x}");
}

```


## 控制流


### if 表达式


```
fn main() {
    let numbeer = 1;
    if numbeer < 5 {
    } else {

    }

    if number  {
    }

    if number %4 ==0 {
    } else if number %3 == 0 {
    } else if number %2 == 0  {
    } else  

// 在let句子中使用if条件句
    let condition =  true;
    let number = if condition { 5 } else { 6 };
// loop循环语句

    loop {
    }
```


# 4. 理解所有全

##  4.1

