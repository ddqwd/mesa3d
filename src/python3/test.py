def my_decorator(func):
    print("my_decorator ")
    def wrapper():
        print("Something is happening before the function is called.")
        func()
        print("Something is happening after the function is called.")
    return wrapper

@my_decorator
def say_hello():
    print("Hello!")

# 使用装饰器调用函数
say_hello()


