

# GL Dispatch Reference

## GL Dispatch的复杂性

每个GL应用程序至少有一个称为GL上下文的对象。这个对象是每个GL函数的隐式参数，它存储了应用程序的所有与GL相关的状态。每个纹理、每个缓冲对象、每个开关等等都存储在上下文中。由于一个应用程序可以有多个上下文，因此要使用的上下文是由与窗口系统相关的函数（例如glXMakeContextCurrent）选择的。

在使用GLX实现基于X窗口系统的OpenGL环境中，每个GL函数，包括glXGetProcAddress返回的指针，都是上下文无关的。这意味着无论当前激活的上下文是哪个，都使用相同的glVertex3fv函数。

这就带来了调度的第一个复杂性。一个应用程序可以有两个GL上下文。一个上下文是直接渲染上下文，函数调用直接路由到加载在应用程序地址空间内的驱动程序。另一个上下文是间接渲染上下文，函数调用转换为GLX协议并发送到服务器。同一个glVertex3fv函数必须根据当前的上下文来执行正确的操作。

高度优化的驱动程序或GLX协议实现可能希望根据当前状态更改GL函数的行为。例如，根据是否启用雾化，glFogCoordf的操作方式可能不同。

在多线程环境中，每个线程可能具有不同的当前GL上下文。这意味着可怜的glVertex3fv必须知道在调用它的线程中哪个GL上下文是当前的。

## Mesa 的实现

Mesa使用两个每个线程的指针。第一个指针存储线程中当前上下文的地址，第二个指针存储与该上下文相关的调度表的地址。调度表存储实际实现特定GL函数的函数指针。每当在线程中设置新的上下文为当前上下文时，这些指针就会进行更新。

例如glVertex3fv这样的函数的实现在概念上变得简单：
 
* 获取当前调度表指针。
 
* 从表中获取真正的glVertex3fv函数的指针。
 
* 调用真正的函数。
 
这可以在几行C代码中实现。文件src/mesa/glapi/glapitemp.h中包含了非常类似这种实现的代码。


示例调度函数：
```
void glVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
    const struct _glapi_table * const dispatch = GET_DISPATCH();

    dispatch->Vertex3f(x, y, z);
}
```

这种简单实现的问题在于它为每个GL函数调用增加了大量的开销。

在多线程环境中，GET_DISPATCH()的一个简单实现会涉及调用`_glapi_get_dispatch()`或`_glapi_tls_Dispatch`.

## 优化

多年来，Mesa进行了许多优化，以减少GL调度带来的性能损失。本节将描述这些优化。列出了每种优化的好处以及适用和不适用的情况。

### ELF TLS
从Linux内核2.4.20版本开始，每个线程都被分配了一个用于线程级全局存储的区域。可以使用一些GCC的扩展（称为ELF TLS）将变量放入该区域。通过将调度表指针存储在此区域中，可以避免昂贵的pthread_getspecific调用和`_glapi_Dispatch`的检查。由于我们不支持早于2.4.20版本的Linux内核，所以我们始终可以使用ELF TLS。

调度表指针存储在一个名为`_glapi_tls_Dispatch`的新变量中。使用新变量名是为了使单个libGL可以实现两个接口。这使得libGL可以与使用任一接口的直接渲染驱动程序一起运行。一旦指针被正确声明，GET_DISPATCH就变成了一个简单的变量引用。

TLS GET_DISPATCH 实现

```c
extern __THREAD_INITIAL_EXEC struct _glapi_table *_glapi_tls_Dispatch;

#define GET_DISPATCH() _glapi_tls_Dispatch
```
### 汇编语言调度存根

许多平台在调度存根中难以正确优化尾调用。像x86这样的平台在参数传递时将参数放在堆栈上时似乎更难优化这些例程。所有的调度例程都非常短，因此创建最佳汇编语言版本是微不足道的。使用汇编存根的优化程度因平台和应用程序而异。然而，通过使用汇编存根，许多平台可以使用额外的空间优化（参见下文）。

创建汇编存根的最大障碍是处理访问调度表指针的不同方式。可以使用四种不同的方法：

在非多线程环境中直接使用`_glapi_Dispatch`。
在多线程环境中使用`_glapi_Dispatch`和`_glapi_get_dispatch`。
在启用TLS的多线程环境中直接使用`_glapi_tls_Dispatch`。
对于支持TLS的新平台，希望实现汇编存根的人应该专注于第3种方法。否则，使用第2种方法。不支持多线程的环境是不常见的，也不是非常相关的。
调度表指针访问方法的选择由一些预处理定义来控制。

如果定义了HAVE_PTHREAD，则使用第2种方法。
如果上述都未定义，则使用第1种方法。
为处理不同情况，使用了两种不同的技术。在x86和SPARC上，使用了一个名为GL_STUB的宏。在汇编源文件的前言部分，根据定义的预处理变量选择不同实现的宏。然后，汇编代码由一系列宏的调用组成，例如：

`GL_STUB(Color3fv, _gloffset_Color3fv)`

这种技术的好处在于调用模式的更改（例如，添加新的调度表指针访问方法）仅需要在汇编代码中修改较少的行。

然而，此技术仅适用于函数实现不基于传递给函数的参数而变化的平台。例如，由于x86将所有参数传递到堆栈上，不需要在调用pthread_getspecific时保存和恢复函数参数的额外代码。由于x86-64在寄存器中传递参数，需要插入不同量的代码来保存和恢复GL函数的参数。

另一种技术适用于像x86-64这样无法使用第一种技术的平台，即在每个函数的汇编实现中插入#ifdef。这使得汇编文件变得更大（例如，glapi_x86-64.S的行数为29,332行，而glapi_x86.S的行数为1,155行），并且使得对函数实现的简单更改产生许多不同行的差异。由于汇编文件通常是由脚本生成的，所以这不是一个重大问题。

创建新的汇编文件后，必须将其添加到构建系统中。这包括两个步骤。首先，必须将文件添加到src/mesa/sources中，以便进行构建和链接。第二步是在src/mesa/glapi/glapi_dispatch.c中添加正确的#ifdef魔术代码，以防止构建C版本的调度函数。

## 固定长度的调度存根

为了实现glXGetProcAddress，Mesa存储了一个将函数名与函数指针关联的表格。该表格存储在src/mesa/glapi/glprocs.h中。出于不同平台的不同原因，存储所有这些指针是低效的。在大多数平台上，包括所有已知支持TLS的平台，我们可以避免这种额外开销。

如果汇编存根的大小都相同，就不需要为每个函数存储指针。而是通过将函数在表格中的偏移量乘以调度存根的大小来计算函数的位置。然后将该值加上第一个调度存根的地址。
通过在src/mesa/glapi/glapi.c中包含正确的#ifdef魔术代码，可以启用这个路径。


# 调试 

下面是glapi_x86-64.S中对glClear函数的实现部分，对GLX_USE_TLS 和 HAVE_PTHREAD 调用不同的dispatch处理，调用libglapi中的get_dispatch从而获得`_glapi_table`地址后根据glclear在表中的偏移地址获得实际glClear内部实现函数的地址， 这个函数就是`_mesa_Clear`, 

```asm


_x86_64_get_dispatch:
	movq	_glapi_tls_Dispatch@GOTTPOFF(%rip), %rax
	movq	%fs:(%rax), %rax
	ret
	.size	_x86_64_get_dispatch, .-_x86_64_get_dispatch

#elif defined(HAVE_PTHREAD)

	.extern	_glapi_Dispatch
	.extern	_gl_DispatchTSD
	.extern	pthread_getspecific

	.p2align	4,,15
_x86_64_get_dispatch:
	movq	_gl_DispatchTSD@GOTPCREL(%rip), %rax
	movl	(%rax), %edi
	jmp	pthread_getspecific@PLT

#else

	.extern	_glapi_get_dispatch

    ......
    ......

    
	.p2align	4,,15
	.globl	GL_PREFIX(Clear)
	.type	GL_PREFIX(Clear), @function
GL_PREFIX(Clear):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1624(%rax), %r11
	jmp	*%r11
#elif defined(HAVE_PTHREAD)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	1624(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1624(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	1624(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Clear), .-GL_PREFIX(Clear)

    ```

```

使用gdb 追踪glClear的调用，在glClear处打断点可以发现，这里使用的_glapi_Dispatch， 变量值为0x27381.由于glClear实现的文件为汇编语言无法显示具体源文件。

```
(gdb) disassemble 
Dump of assembler code for function glClear:
=> 0x00007ffff7f91c60 <+0>:                                                                                                                                                                               mov    0x27381(%rip),%rax        # 0x7ffff7fb8fe8
   0x00007ffff7f91c67 <+7>:                                                                                                                                                                               mov    %fs:(%rax),%r11
   0x00007ffff7f91c6b <+11>:                                                                                                                                                                              jmp    *0x658(%r11)
   0x00007ffff7f91c72 <+18>:                                                                                                                                                                              data16 cs nopw 0x0(%rax,%rax,1)
   0x00007ffff7f91c7d <+29>:                                                                                                                                                                              nopl   (%rax)
End of assembler dump.


```

继续单步进入即可进入_mesa_Clear函数内部


