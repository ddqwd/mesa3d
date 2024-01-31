# 缓冲对象


缓冲对象是OpenGL对象，用于存储由OpenGL上下文（也称为GPU）分配的未格式化内存数组。它们可用于存储顶点数据、从图像或帧缓冲区中检索的像素数据以及各种其他数据。



缓冲对象是OpenGL对象；因此，它们遵循所有常规OpenGL对象的规则。要创建缓冲对象，您需要调用glGenBuffers。要删除它们，可以使用glDeleteBuffers。这些函数使用了标准的生成/删除范式，与大多数OpenGL对象一样。

与标准的OpenGL对象范式一样，这仅仅是创建了对象的名称，即对对象的引用。要设置其内部状态，您必须将其绑定到上下文。您可以使用以下API来实现这一点：

```c
void glBindBuffer(enum target, uint bufferName)
```

target参数定义了您打算如何使用该缓冲对象的绑定。当您只是创建、填充缓冲对象的数据，或两者都在进行时，您所使用的target在技术上并不重要。

缓冲对象保存了一个任意大小的线性内存数组。在将其上传或使用之前，必须分配这块内存。有两种分配缓冲对象存储空间的方式：可变或不可变。为缓冲对象分配不可变的存储空间将改变您与缓冲对象进行交互的方式。


## 不可变存储

与不可变存储纹理类似，缓冲对象的存储空间可以被不可变地分配。在这种情况下，您将无法重新分配该存储空间。您仍然可以使用显式的失效命令或通过映射缓冲来使其无效。但是，如果存储空间是不可变的，您不能使用glBufferData(..., NULL)的技巧来使其无效。

要为缓冲对象分配不可变存储空间，您可以调用以下函数：

```c
void glBufferStorage(GLenum target, GLsizeiptr size, const GLvoid *data, GLbitfield flags);
```

target参数与bindBuffer的参数类似；它指定要修改的绑定缓冲。size表示要在此缓冲对象中分配多少字节的存储空间。

data参数是指向大小为size的字节数组的指针。OpenGL将在初始化时将数据复制到缓冲对象中。您可以将此参数设置为NULL；如果这样做，缓冲的初始内容将是未定义的。如果您希望更新它，可以在分配后清除缓冲。

flags字段建立了您和OpenGL之间的协议，描述了您如何以及不能访问缓冲的内容。


## 不可变存储访问方法


不可变存储缓冲对象允许您与OpenGL建立协议，指定您将如何访问和修改缓冲对象存储的内容。flags​​位域是一系列位，描述了您将操作的限制，如果不遵守这些限制，将导致OpenGL错误。

flags​​位覆盖了用户如何直接从缓冲中读取或写入数据。但这仅限于用户如何直接修改数据存储；"服务器端"对缓冲内容的操作始终可用。

以下操作始终对不可变缓冲有效，无论flags​字段如何：

1. 使用任何渲染管道过程向缓冲写入数据。这包括变换反馈（Transform Feedback）、图像加载存储（Image Load Store）、原子计数器（Atomic Counter）和着色器存储缓冲对象（Shader Storage Buffer Object）等。基本上，任何可以向缓冲写入数据的渲染管道的一部分都将正常工作。
2. 清除缓冲。因为这只传输少量数据，不被视为"客户端"修改。
3. 复制缓冲。这会从一个缓冲复制到另一个缓冲，因此所有操作都在"服务器端"进行。
4. 使缓冲失效。这只会擦除缓冲的内容，因此被视为"服务器端"操作。
5. 异步像素传输到缓冲中。这会设置缓冲中的数据，但只能通过纯OpenGL机制来完成。
6. 使用glGetBufferSubData从缓冲中读取部分数据到CPU。这不是"服务器端"操作，但始终可用。

以下是可以请求的客户端行为以及代表它们的位：

- GL_MAP_READ_BIT：允许用户通过映射缓冲来读取缓冲。如果没有此标志，尝试将缓冲映射为读取将失败。
- GL_MAP_WRITE_BIT：允许用户映射缓冲以进行写入。如果没有此标志，尝试将缓冲映射为写入将失败。
- GL_DYNAMIC_STORAGE_BIT：允许用户使用glBufferSubData来修改存储的内容。如果没有此标志，尝试在此缓冲上调用该函数将失败。
- GL_MAP_PERSISTENT_BIT：允许以一种可在映射时使用的方式映射缓冲对象。如果没有此标志，尝试在映射时对缓冲执行任何操作都将失败。在使用此位时，必须使用其中一个映射位。
- GL_MAP_COHERENT_BIT：允许从持久性缓冲读取和写入数据与OpenGL保持一致，无需显式屏障。如果没有此标志，您必须使用显式屏障来实现一致性。在使用此位时，必须使用GL_PERSISTENT_BIT。

- GL_CLIENT_STORAGE_BIT：这是一种暗示，建议实现缓冲的存储应来自"客户端"内存。

虽然您可以自由地使用这些位的任何合法组合，但有些缓冲对象的用途适合特定的位组合。

- 纯OpenGL缓冲：有时，拥有几乎完全由OpenGL进程拥有的缓冲对象是有用的。缓冲的内容由计算着色器、变换反馈或其他各种机制编写。其他OpenGL进程从中读取，例如通过间接渲染、顶点规范等。

在这种情况下，您只需要能够分配固定大小的存储空间。将flags​​设置为0是处理这种情况的最佳方式。

- 静态数据缓冲：在某些情况下，存储在缓冲对象中的数据一旦上传就不会再次更改。例如，顶点数据可以是静态的：设置一次，多次使用。

对于这些情况，将flags​​设置为0，并使用data​​作为初始上传。从那时起，您只需使用缓冲中的数据。这需要您提前收集所有的静态数据。

- 图像读取缓冲：通过像素缓冲对象（Pixel Buffer Objects），可以使用缓冲作为异步像素传输操作的中介。在这种情况下，缓冲的目的只是使读取异步化。

您可以依赖glGetBufferSubData来执行读取，但通过将缓冲映射为读取，可以实现等效，如果不是更好的性能。因此，flags​​应设置为GL_MAP_READ_BIT。

- 可修改缓冲：通常情况下，映射缓冲并写入它将与glBufferSubData一样高效。在大多数情况下，速度会更快，特别是如果采用失效和其他缓冲对象流技术。

为了满足这一要求，flags​​应设置为GL_MAP_WRITE_BIT。这会让实现知道您根本不会使用glBufferSubData。

## 可变存储


要为缓冲对象创建可变存储，可以使用以下API：

```c
void glBufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage)
```

target参数与bindBuffer的参数类似；它指定要修改的绑定缓冲。size表示要在此缓冲对象中分配多少字节的存储空间。

data参数是指向将被复制到缓冲对象数据存储的用户内存的指针。如果该值为NULL，则不会发生复制


## 缓冲对象用途


缓冲对象是由OpenGL分配的通用内存存储块，用于多种用途。为了提供实现更大的灵活性，以更好地优化性能，用户需要提供用途提示，以明确指定特定缓冲对象数据存储的使用方式。

使用模式有两个独立的部分：用户将如何从/向缓冲中读取/写入的方式，以及相对于数据使用，用户将多频繁更改它。

数据导致缓冲对象的数据内容发生变化有两种方式。一种方式是用户明确上传一些二进制数据。另一种方式是用户发出OpenGL命令，导致缓冲被填充。例如，如果您希望缓冲存储顶点着色器计算的结果（通过变换反馈），用户不会直接更改缓冲信息。这是后一种更改的情况。

同样，用户可以使用各种命令读取缓冲的数据。或者，用户可以执行OpenGL命令，导致OpenGL读取缓冲的内容并基于它执行操作。存储顶点数据的缓冲在渲染时由OpenGL读取。

用户可以指定数据的三种提示，这些提示基于用户将如何直接读取或写入缓冲的数据来确定。也就是说，用户是否将直接读取或写入缓冲的数据。

- DRAW：用户将向缓冲中写入数据，但用户不会读取它。
- READ：用户不会写入数据，但用户将从中读取数据。
- COPY：用户既不会写入数据也不会读取数据。

DRAW是用于绘制的，正如其名称所示，用户正在上传数据，但只有OpenGL在读取它。通常，存储顶点数据的缓冲被指定为DRAW，尽管也可能有例外情况。

READ在缓冲对象用作OpenGL命令的目标时使用。这可能是渲染到缓冲纹理、对缓冲纹理进行任意写入、将像素传输到缓冲对象、使用变换反馈或其他任何向缓冲对象写入数据的OpenGL操作。

COPY在缓冲对象用于在OpenGL中的不同位置传递数据时使用。例如，您可以将图像数据读入缓冲，然后在绘制调用中使用该图像数据作为顶点数据。您的代码实际上从未直接将数据发送到缓冲，也不会读取数据。您还可以使用变换反馈以更直接的方式实现相同的操作。您将反馈数据传送到缓冲对象，然后使用该缓冲对象作为顶点数据。用户虽然通过渲染命令导致缓冲被更新，但用户在任何时候都不会直接读取或写入缓冲的存储。

用户还可以指定有关用户将多频繁更改缓冲数据的三种提示。

- STATIC：用户将设置数据一次。
- DYNAMIC：用户将偶尔设置数据。
- STREAM：用户将在每次使用后更改数据。或者几乎每次使用。

STREAM相对容易理解：缓冲对象的存储将在几乎每次使用后更新。STATIC也相对容易理解。缓冲对象的内容将被更新一次，然后永不更改。

不清楚DYNAMIC何时变为STREAM或STATIC。这些只是提示，毕竟。在创建后修改STATIC缓冲或从不修改STREAM缓冲是完全合法的OpenGL代码。

对于很少更新的缓冲，是不是更好使用STATIC？对于经常更新但不以STREAM速度更新的缓冲，是不是更好使用DYNAMIC？对于部分更新的缓冲，是不是更好使用DYNAMIC？这些问题只能通过仔细的性能分析来回答。即使如此，答案也只对特定硬件供应商的特定驱动程序版本有效。

在任何情况下，STREAM、STATIC和DYNAMIC可以与READ、DRAW和COPY组合在一起。STREAM_COPY表示您将在几乎每次使用后执行变换反馈写入（或其他基于GL的写入）到缓冲中；它不会使用glBufferSubData或类似函数进行更新。STATIC_READ表示您将从GL填充缓冲，但只会执行一次。

## 数据规范


我们已经看到，glBufferData可用于更新缓冲对象中的数据。但是，这也重新分配了缓冲对象的存储空间。因此，此函数不适用于仅更新分配的内存内容（对于不可变存储缓冲，这是不可能的）。

相反，要更新缓冲对象中的所有或部分数据，可以使用以下API：

```c
void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void *data)
```

offset参数是一个整数偏移量，指示从何处开始更新缓冲对象。size参数表示应从数据中复制多少字节。出于明显的原因，data不能为NULL。

## 清除


可以清除缓冲对象的存储，部分或全部到特定值。这些函数的工作方式与像素传输操作类似，但存在一些重要的区别：

```c
void glClearBufferData(GLenum target, GLenum internalformat, GLenum format, GLenum type, const void *data);
void glClearBufferSubData(GLenum target, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data);
```

glClearBufferData与glClearBufferSubData的操作方式相似，只是它作用于整个缓冲的内容。这两个函数都不会重新分配缓冲对象的存储空间。

target参数与bindBuffer的参数类似；它指定要清除的绑定缓冲。

internalformat必须是一个大小图像格式，但只能是用于缓冲纹理的那种。这定义了OpenGL将如何存储缓冲对象中的数据。format和type对于像素传输操作的操作方式与正常情况下相同。

data是指向单个像素数据的指针，而不是像实际像素传输中使用的行。因此，如果format是GL_RG，type是GL_UNSIGNED_BYTE，那么data应该是一个指向两个GLubyte的数组的指针。

此函数将在缓冲的指定范围内多次复制给定数据。offset必须是internalformat定义的字节大小的倍数，size也必须如此。


## 拷贝


可以将数据从一个缓冲对象复制到另一个缓冲对象，或者从同一缓冲的一个区域复制到另一个区域（不重叠）。要做到这一点，首先将源缓冲和目标缓冲绑定到不同的target​。这些可以是任何target，但GL_COPY_READ_BUFFER和GL_COPY_WRITE_BUFFER没有特殊的语义，因此它们在这方面是有用的目标。在同一缓冲内进行复制时，将相同的缓冲对象绑定到两者。

一旦两者都绑定，使用以下函数：

```c
void glCopyBufferSubData(GLenum readtarget, GLenum writetarget, GLintptr readoffset, GLintptr writeoffset, GLsizeiptr size);
```

readtarget是你绑定源缓冲的地方，所以这是你从哪里复制数据。writetarget是你绑定目标缓冲的地方。readoffset是从源缓冲的开头开始读取的字节偏移量。writeoffset是从目标缓冲的开头开始写入的字节偏移量。size是要读取的字节数。

如果偏移/大小会导致从各自缓冲对象的存储范围之外读取或写入位置，将会出现错误。此外，如果源缓冲和目标缓冲相同，如果要读取的范围与要写入的范围重叠，将会出现错误。

## 映射


glBufferSubData是向缓冲对象提供数据的一种好方法。但是，根据您的使用模式，它的性能可能浪费。

例如，如果您有一个生成数据并希望存储在缓冲对象中的算法，您必须首先分配一些临时内存来存储该数据。然后，您可以使用glBufferSubData将其传输到OpenGL的内存中。类似地，如果要读取数据，getBufferSubData也许并不是您所需要的，尽管这种情况较少。如果您可以直接获得缓冲对象的存储空间指针并直接写入它，那将非常好。

实际上，您可以这样做。为此，您必须映射缓冲。这将为您提供一个指向内存的指针，您可以从中读取或写入，理论上就像任何其他内存一样。当您取消映射缓冲时，这将使指针无效（不要再使用它），并且缓冲对象将更新为您对其所做的更改。

当缓冲被映射时，您可以自由取消绑定该缓冲。但是，除非您映射缓冲保持不变，否则不能在映射时调用任何会导致OpenGL读取、修改或写入该缓冲的函数。因此，调用glBufferData是不允许的，就像使用任何会导致OpenGL从中读取的函数一样（使用VAO进行渲染等）。

要映射缓冲，您可以调用glMapBufferRange。此函数的签名如下：

```c
void *glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);
``` 返回值是指向缓冲对象数据的指针。offset和length参数允许您指定要映射的缓冲内的特定范围；您不必映射整个缓冲。target参数是指您将要绑定到的缓冲对象的特定target。

access参数有点复杂。

您可以告诉OpenGL您打算使用指针做什么。如果您只是向缓冲中添加新数据，那么返回的指针就不需要从中读取。同样，您可能打算从该指针中读取而不更改缓冲；OpenGL可以执行简单的从缓冲的内存到客户地址空间中的临时内存的复制，以

加速读取缓冲（缓冲内存可能没有针对读取进行优化）。

您通过在access参数中设置位标志来指定这一点。此参数必须设置GL_MAP_READ_BIT或GL_MAP_WRITE_BIT之一；它可以同时设置两者（即：内存将适合读取和写入），但不能两者都不设置。还有许多其他可以设置的标志。

取消映射缓冲是在使用指针完成后并希望让OpenGL知道该缓冲可以被使用时进行的。这是使用glUnmapBuffer函数完成的。此函数只接受与所涉及的缓冲绑定的target。在调用此函数后，您不应再使用映射调用返回的指针。



## 缓冲区无效化


当一个缓冲区或其中的一部分被无效化时，意味着该缓冲区的内容现在变得不确定。OpenGL如何处理无效化取决于它自身。需要注意的是，对于缓冲区的任何待处理操作仍将完成。对OpenGL的任何待处理读取操作仍将获取先前的数值，而任何待处理写入操作仍将写入它们的数值（尽管这些写入将被丢弃，因为你已经使缓冲区无效）。

这个概念是，通过使缓冲区或其一部分无效，实现将简单地为以后的操作获取新的内存空间。因此，尽管先前发布的GL命令仍然可以读取缓冲区的原始数据，但你可以通过映射或glBufferSubData填充已无效化的缓冲区以存入新值，而不会引发隐式同步。

缓冲区的无效化可以通过以下三种方式之一发生。

对于非不可变存储的缓冲对象，可以通过调用glBufferData，与之前相同的大小和使用提示以及一个NULL数据参数来使缓冲区无效化。这是一种较旧的无效化方法，应仅在其他方法不可用时使用。

无效化可以在缓冲区映射时通过调用glMapBufferRange并设置GL_MAP_INVALIDATE_BUFFER_BIT映射位来发生。这将导致整个缓冲区的内容被无效化（即使你只映射了部分）。要仅无效化映射的缓冲区部分，可以使用GL_MAP_INVALIDATE_RANGE_BIT。

缓冲区的无效化还可以通过显式调用以下函数之一来触发：

void glInvalidateBufferData(GLuint buffer​);

void glInvalidateBufferSubData(GLuint buffer​, GLintptr offset​, GLsizeiptr length​);

glInvalidateBufferData等效于将offset设置为0和length设置为缓冲区存储的大小来调用glInvalidateBufferSubData。



## 流式传输


流式传输是频繁将数据上传到缓冲对象，然后在某个OpenGL过程中使用该缓冲对象的过程。使这个过程尽可能高效是一个复杂的操作。缓冲对象提供了多种可能的流式传输使用模式，哪种模式最适合并不完全清楚。应该使用感兴趣的硬件进行测试，以确保获得最佳的流式传输性能。

流式传输的关键是并行性。OpenGL规范允许实现延迟执行绘图命令。这允许你绘制大量的东西，然后让OpenGL在自己的时间内处理事务。因此，很有可能，在你调用带有缓冲对象的渲染函数之后，你可能会尝试将顶点数据流式传输到该缓冲对象。如果发生这种情况，OpenGL规范要求线程等待，直到可能受到缓冲对象更新影响的所有绘图命令完成。这显然违背了流式传输的初衷。

有效的流式传输的关键是同步。更具体地说，是尽量避免同步。

## **通用用途**


大多数缓冲对象的使用涉及将它们绑定到特定目标，然后其他OpenGL操作将检测已绑定的缓冲对象，然后以某种方式使用该缓冲对象中存储的数据，无论是读取还是以明确定义的格式写入值。

以下是不同缓冲对象目标及其相关用途：

- **GL_ARRAY_BUFFER**
  - 缓冲区将用作顶点数据的源，但连接仅在调用`glVertexAttribPointer`时进行。此函数的指针字段被视为从当前绑定到此目标的缓冲区的开头的字节偏移量。

- **GL_ELEMENT_ARRAY_BUFFER**
  - 所有形式为`gl*Draw*Elements*`的渲染函数将使用指针字段作为从绑定到此目标的缓冲对象的开头的字节偏移量。用于索引渲染的索引将来自缓冲对象。请注意，此绑定目标是顶点数组对象（Vertex Array Objects）状态的一部分，因此在绑定缓冲区之前必须绑定VAO。

- **GL_COPY_READ_BUFFER** 和 **GL_COPY_WRITE_BUFFER**
  - 这些没有特定的语义。因为它们没有实际意义，它们是使用`glCopyBufferSubData`复制缓冲对象数据的有用目标。在复制时不必使用这些目标，但通过使用它们，您可以避免干扰具有实际语义的缓冲目标。

- **GL_PIXEL_UNPACK_BUFFER** 和 **GL_PIXEL_PACK_BUFFER**
  - 这些用于执行异步像素传输操作。如果一个缓冲区绑定到**GL_PIXEL_UNPACK_BUFFER**，那么`glTexImage*`、`glTexSubImage*`、`glCompressedTexImage*` 和 `glCompressedTexSubImage*`将受到影响。这些函数将从绑定的缓冲对象中读取其数据，而不是从客户指针指向的位置。类似地，如果一个缓冲区绑定到**GL_PIXEL_PACK_BUFFER**，那么`glGetTexImage` 和 `glReadPixels`将把它们的数据存储到绑定的缓冲对象中，而不是从客户指针指向的位置。

- **GL_QUERY_BUFFER**
  - 这用于从异步查询直接写入缓冲对象内存。如果一个缓冲区绑定到**GL_QUERY_BUFFER**，那么所有`getGetQueryObject[ui64v]`函数调用将将结果写入绑定的缓冲区对象的偏移量。

- **GL_TEXTURE_BUFFER**
  - 此目标没有特殊语义。

- **GL_TRANSFORM_FEEDBACK_BUFFER**
  - 用于Transform Feedback操作中使用的缓冲区的索引绑定。

- **GL_UNIFORM_BUFFER**
  - 用作统一块存储的缓冲区的索引绑定。

- **GL_DRAW_INDIRECT_BUFFER**
  - 绑定到此目标的缓冲区将用作执行间接渲染时的间接数据源。这需要OpenGL 4.0或ARB_draw_indirect。

- **GL_ATOMIC_COUNTER_BUFFER**
  - 用作原子计数器存储的缓冲区的索引绑定。这需要OpenGL 4.2或ARB_shader_atomic_counters。

- **GL_DISPATCH_INDIRECT_BUFFER**
  - 绑定到此目标的缓冲区将用作执行间接计算分发操作的间接数据源，通过`glDispatchComputeIndirect`。这需要OpenGL 4.3或ARB_compute_shader。

- **GL_SHADER_STORAGE_BUFFER**
  - 用作着色器存储块存储的缓冲区的索引绑定。这需要OpenGL 4.3或ARB_shader_storage_buffer_object。

**绑定索引目标**
如上所述，某些缓冲目标是索引的。这用于绑定执行类似操作的多个缓冲区。例如，GLSL程序可以使用多个不同的统一缓冲区。

要将缓冲对象绑定到索引位置，可以使用以下函数：

```c
void glBindBufferRange(GLenum target​, GLuint index​, GLuint buffer​, GLintptr offset​, GLsizeiptr size​);
```

这将导致缓冲区被绑定到目标位置`target​`上的缓冲区。`target​`的唯一有效值是索引目标（请参阅上文）。

`index​`的有效值取决于要绑定的`target​`的类型。有效的`target​`值，以及它们关联的索引限制，如下所示：

- **GL_TRANSFORM_FEEDBACK_BUFFER**：限制GL_MAX_TRANSFORM_FEEDBACK_BUFFERS。
- **GL_UNIFORM_BUFFER**：限制GL_MAX_UNIFORM_BUFFER_BINDINGS。
- **GL_ATOMIC_COUNTER_BUFFER**：限制GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS。
- **GL_SHADER_STORAGE_BUFFER**：限制GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS。

`offset​`是用于绑定的缓冲区的字节偏移，`size​`是用于此缓冲对象使用的有效字节数。这允许您绑定缓冲对象的子部分。如果`buffer​`为零，则此索引未绑定。

请注意，这不会取代使用`glBindBuffer`进行标准缓冲绑定。实际上，它会将缓冲区绑定到`target​`参数，从而解绑定到该目标的任何内容。但通常，当使用`glBindBufferRange`时，您更倾向于要使用缓冲区而不仅仅是修改它。

将`glBindBufferRange`视为将缓冲区绑定到两个位置：特定的索引和`target​`。`glBindBuffer`仅绑定到`target​

`，而不是索引。

有一种更有限的函数`glBindBufferBase`，它将整个缓冲区绑定到索引。它仅省略了`offset​`和`size​`字段，但与`glBindBufferRange`有一个主要区别。如果使用`glBufferData`重新指定缓冲区的数据存储，则通过`glBindBufferBase`绑定的所有索引将采用新的大小（对于任何后续的渲染命令）。使用`glBindBufferRange`时，如果更改缓冲区的数据大小，则不会更新绑定的位置。

注意：通常不建议重新指定具有更大或更小大小的缓冲区数据存储。建议不依赖于`glBindBufferBase`的调整大小特性。



# mesa18分析


//stack tracker
```
st_init_bufferobject_functions(struct pipe_screen *screen,
                               struct dd_function_table *functions)
{
   functions->NewBufferObject = st_bufferobj_alloc;
   functions->DeleteBuffer = st_bufferobj_free;
   functions->BufferData = st_bufferobj_data;
   functions->BufferDataMem = st_bufferobj_data_mem;
   functions->BufferSubData = st_bufferobj_subdata;
   functions->GetBufferSubData = st_bufferobj_get_subdata;
   functions->MapBufferRange = st_bufferobj_map_range;
   functions->FlushMappedBufferRange = st_bufferobj_flush_mapped_range;
   functions->UnmapBuffer = st_bufferobj_unmap;
   functions->CopyBufferSubData = st_copy_buffer_subdata;
   functions->ClearBufferSubData = st_clear_buffer_subdata;
   functions->BufferPageCommitment = st_bufferobj_page_commitment;

   if (screen->get_param(screen, PIPE_CAP_INVALIDATE_BUFFER))
      functions->InvalidateBufferSubData = st_bufferobj_invalidate;
}


```
// driver

```

void si_init_buffer_functions(struct si_context *sctx)
{
	sctx->b.invalidate_resource = si_invalidate_resource;
	sctx->b.transfer_map = u_transfer_map_vtbl;
	sctx->b.transfer_flush_region = u_transfer_flush_region_vtbl;
	sctx->b.transfer_unmap = u_transfer_unmap_vtbl;
	sctx->b.texture_subdata = u_default_texture_subdata;
	sctx->b.buffer_subdata = si_buffer_subdata;
	sctx->b.resource_commit = si_resource_commit;
}
void si_init_compute_blit_functions(struct si_context *sctx)
{
	sctx->b.clear_buffer = si_pipe_clear_buffer;
}

```
## 缓冲创建绑定

```
_mesa_GenBuffers_no_error(GLsizei n, GLuint *buffers)
   create_buffers(ctx, n, buffers, dsa=false);
        GLuint first;
        struct gl_buffer_object *buf;

        first = _mesa_HashFindFreeKeyBlock(ctx->Shared->BufferObjects, n);
        for (int i = 0; i < n; i++) {
            buf = &DummyBufferObject;
        }

_mesa_CreateBuffers(GLsizei n, GLuint *buffers)
   create_buffers(ctx, n, buffers, dsa=true);



create_buffers(struct gl_context *ctx, GLsizei n, GLuint *buffers, bool dsa)
   for (int i = 0; i < n; i++) {
      buffers[i] = first + i;
      if (dsa) {
         assert(ctx->Driver.NewBufferObject);
         buf = ctx->Driver.NewBufferObject(ctx, buffers[i]);
         if (!buf) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCreateBuffers");
            _mesa_HashUnlockMutex(ctx->Shared->BufferObjects);
            return;
         }
      }
      else
         buf = &DummyBufferObject;





void glBindBuffer(enum target, uint bufferName)
_mesa_BindBuffer_no_error(GLenum target, GLuint buffer)
   struct gl_buffer_object **bindTarget = get_buffer_target(ctx, target);
        switch (target) {
           case GL_ARRAY_BUFFER_ARB:
              return &ctx->Array.ArrayBufferObj;
           ...
        }
   bind_buffer_object(ctx, bindTarget, buffer);

bind_buffer_object(struct gl_context *ctx,
                   struct gl_buffer_object **bindTarget, GLuint buffer)

   struct gl_buffer_object *oldBufObj;
   struct gl_buffer_object *newBufObj = NULL;

   newBufObj = _mesa_lookup_bufferobj(ctx, buffer);
   _mesa_handle_bind_buffer_gen(ctx, buffer,&newBufObj, "glBindBuffer")
      if (!buf || buf == &DummyBufferObject) 
          buf = ctx->Driver.NewBufferObject(ctx, buffer);
 
   /* bind new buffer */
   _mesa_reference_buffer_object(ctx, bindTarget, newBufObj);


// stack_tracker

st_bufferobj_alloc(struct gl_context *ctx, GLuint name)
   struct st_buffer_object *st_obj = ST_CALLOC_STRUCT(st_buffer_object);
   _mesa_initialize_buffer_object(ctx, &st_obj->Base, name);
   return &st_obj->Base;
```



**



## 分配内存数据


```

_mesa_BufferData_no_error(GLenum target, GLsizeiptr size, const GLvoid *data,
                          GLenum usage)

   struct gl_buffer_object **bufObj = get_buffer_target(ctx, target);
   buffer_data_no_error(ctx, *bufObj, target, size, data, usage,
      buffer_data(ctx, bufObj, target, size, data, usage, func, true);
          ...

          bufObj->Written = GL_TRUE;
             if (!ctx->Driver.BufferData(ctx, target, size, data, usage,
                                         GL_MAP_READ_BIT |
                                         GL_MAP_WRITE_BIT |
                                         GL_DYNAMIC_STORAGE_BIT,
                                         bufObj)) {
                        functions->BufferData = st_bufferobj_data;
 


_mesa_BufferSubData_no_error(GLenum target, GLintptr offset,
                             GLsizeiptr size, const GLvoid *data)
   buffer_sub_data(target, 0, offset, size, data, false, true,
                   "glBufferSubData");
         struct gl_buffer_object *bufObj = *get_buffer_target(ctx, target);
         if (no_error || validate_buffer_sub_data(ctx, bufObj, offset, size, func))
            _mesa_buffer_sub_data(ctx, bufObj, offset, size, data);
                ctx->Driver.BufferSubData(ctx, offset, size, data, bufObj);
                    functions->BufferSubData = st_bufferobj_subdata;



st_bufferobj_subdata(struct gl_context *ctx,
                     GLintptrARB offset,
                     GLsizeiptrARB size,
                     const void * data, struct gl_buffer_object *obj)
   ...
   pipe_buffer_write(st_context(ctx)->pipe,
                     st_obj->buffer,
                     offset, size, data);

void glBufferStorage(GLenum target, GLsizeiptr size, const GLvoid *data, GLbitfield flags);
    _mesa_BufferStorage_no_error(GLenum target, GLsizeiptr size,const GLvoid *data, GLbitfield flags)
       inlined_buffer_storage(target, 0, size, data, flags, GL_NONE, 0,
                          false, false, true, "glBufferStorage");

       buffer_storage(ctx, bufObj, memObj, target, size, data, flags, offset, func);
            bufObj->Written = GL_TRUE;
            bufObj->Immutable = GL_TRUE;
            bufObj->MinMaxCacheDirty = true;
    
      if(!memObj ) 
            res = ctx->Driver.BufferData(ctx, target, size, data, GL_DYNAMIC_DRAW,
                                   flags, bufObj);
                    
                functions->BufferData = st_bufferobj_data;
        

st_bufferobj_data(struct gl_context *ctx,
    return bufferobj_data(ctx, target, size, data, NULL, 0, usage, storageFlags, obj);

bufferobj_data(struct gl_context *ctx,
               GLenum target,
               GLsizeiptrARB size,
               const void *data,
               struct gl_memory_object *memObj,
               GLuint64 offset,
               GLenum usage,
               GLbitfield storageFlags,
               struct gl_buffer_object *obj)

    // 设定存储标志
   st_obj->Base.Size = size;
   st_obj->Base.Usage = usage;
   st_obj->Base.StorageFlags = storageFlags;
    
    // 如果非首次bufferdata
   if (target != GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD &&...)
        if (data) {
         pipe->buffer_subdata(pipe, st_obj->buffer,
                              PIPE_TRANSFER_DISCARD_WHOLE_RESOURCE,
                              0, size, data);
         return GL_TRUE;
         }
    
      
    memset(&buffer, 0, sizeof buffer);
    buffer.target = PIPE_BUFFER;
    
    if(!st_mem_obj && target != GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD) {
         st_obj->buffer = screen->resource_create(screen, &buffer);
	        sscreen->b.resource_create = si_resource_create;

         if (st_obj->buffer && data)
            pipe_buffer_write(pipe, st_obj->buffer, 0, size, data);
            
    } else {
        // st_obj->buffer.is_usr_ptr= true
         st_obj->buffer =
            screen->resource_from_user_memory(screen, &buffer, (void*)data);
    }


pipe_buffer_write(struct pipe_context *pipe,
   /* Don't set any other usage bits. Drivers should derive them. */
   pipe->buffer_subdata(pipe, buf, PIPE_TRANSFER_WRITE, offset, size, data);
	    sctx->b.buffer_subdata = si_buffer_subdata;


// driver radeonsi
static void si_buffer_subdata(struct pipe_context *ctx,
			      struct pipe_resource *buffer,
			      unsigned usage, unsigned offset =0,
{
	struct pipe_transfer *transfer = NULL;
	struct pipe_box box;
	uint8_t *map = NULL;

	u_box_1d(offset, size, &box);
        box->x = offset
        box->width = size
        box->height = 1;
        box->depth = 1;

	map = si_buffer_transfer_map(ctx, buffer, 0,
				       PIPE_TRANSFER_WRITE |
				       PIPE_TRANSFER_DISCARD_RANGE |
				       usage,
				       &box, &transfer);
	struct r600_resource* s = (struct r600_resource*)buffer;
	if (!map)
		return;

	memcpy(map, data, size);
	si_buffer_transfer_unmap(ctx, transfer);
}

```

## 映射 

```

_mesa_MapBuffer_no_error(GLenum target, GLenum access)
   struct gl_buffer_object *bufObj = *get_buffer_target(ctx, target);
   return map_buffer_range(ctx, bufObj, 0, bufObj->Size, accessFlags,
                           "glMapBuffer");

_mesa_MapBufferRange_no_error(GLenum target, GLintptr offset,

   struct gl_buffer_object *bufObj = *get_buffer_target(ctx, target);
   return map_buffer_range(ctx, bufObj, offset, length, access,
                           "glMapBufferRange");
        void *map = ctx->Driver.MapBufferRange(ctx, offset, length, access, bufObj,
                                              MAP_USER);

st_bufferobj_map_range(struct gl_context *ctx,
                       GLintptr offset, GLsizeiptr length, GLbitfield access,
                       struct gl_buffer_object *obj,
                       gl_map_buffer_index index)
   const enum pipe_transfer_usage transfer_flags =
      st_access_flags_to_transfer_flags(access,
                                        offset == 0 && length == obj->Size);
   obj->Mappings[index].Pointer = pipe_buffer_map_range(pipe,
                                                        st_obj->buffer,
                                                        offset, length,
                                                        transfer_flags,
                                                        &st_obj->transfer[index]);

pipe_buffer_map_range(struct pipe_context *pipe,
                      struct pipe_resource *buffer,
                      unsigned offset,
                      unsigned length,
                      unsigned access,
                      struct pipe_transfer **transfer)
   struct pipe_box box;
   void *map;
   u_box_1d(offset, length, &box);
   map = pipe->transfer_map(pipe, buffer, 0, access, &box, transfer);
	    sctx->b.transfer_map = u_transfer_map_vtbl;
            struct u_resource *ur = u_resource(resource = buffer);
            return ur->vtbl->transfer_map(context, resource, level, usage, box,
                                          transfer);
   return map;

```


## 拷贝数据


```
void glCopyBufferSubData(GLenum readtarget, GLenum writetarget, GLintptr readoffset, GLintptr writeoffset, GLsizeiptr size);
   _mesa_CopyBufferSubData_no_error(GLenum readTarget, GLenum writeTarget,
                                    GLintptr readOffset, GLintptr writeOffset,
                                    GLsizeiptr size)
   struct gl_buffer_object *src =*get_buffer_target(ctx, readTarget);
   struct gl_buffer_object **dst = *get_buffer_target(ctx, writeTarget);
   ctx->Driver.CopyBufferSubData(ctx, src, dst, readOffset, writeOffset,
                                 size);
        functions->CopyBufferSubData = st_copy_buffer_subdata;
            u_box_1d(readOffset, size, &box);
            pipe->resource_copy_region(pipe, dstObj->buffer, 0, writeOffset, 0, 0,
                                       srcObj->buffer, 0, &box);
	            sctx->b.resource_copy_region = si_resource_copy_region;



void si_resource_copy_region(struct pipe_context *ctx,
			     struct pipe_resource *dst,
			     unsigned dst_level,
			     unsigned dstx, unsigned dsty, unsigned dstz,
			     struct pipe_resource *src,
			     unsigned src_level,
			     const struct pipe_box *src_box)

	struct pipe_box sbox, dstbox;
	if (dst->target == PIPE_BUFFER && src->target == PIPE_BUFFER) {
		si_copy_buffer(sctx, dst, src, dstx, src_box->x, src_box->width);
		return;
	}

	/* Copy. */ // see blitter.pdf
	si_blitter_begin(sctx, SI_COPY);
	util_blitter_blit_generic(sctx->blitter, dst_view, &dstbox,
				  src_view, src_box, src_width0, src_height0,
				  PIPE_MASK_RGBAZS, PIPE_TEX_FILTER_NEAREST, NULL,
				  false);
	si_blitter_end(sctx);

```
* 
* 关于ur->vtbl
`
```
	rbuffer->b.vtbl = &si_buffer_vtbl;

static const struct u_resource_vtbl si_buffer_vtbl =
{
	NULL,				/* get_handle */
	si_buffer_destroy,		/* resource_destroy */
	si_buffer_transfer_map,	/* transfer_map */
	si_buffer_flush_region,	/* transfer_flush_region */
	si_buffer_transfer_unmap,	/* transfer_unmap */
};

```
* 关于st_buffer_object的Mappings

```c
typedef enum
{
   MAP_USER,
   MAP_INTERNAL,
   MAP_COUNT
} gl_map_buffer_index;

st_buffer_object {
   struct gl_buffer_mapping Mappings[MAP_COUNT=2];
} 

struct gl_buffer_mapping
{
   GLbitfield AccessFlags; /**< Mask of GL_MAP_x_BIT flags */
   GLvoid *Pointer;        /**< User-space address of mapping */
   GLintptr Offset;        /**< Mapped offset */
}


```

## si缓冲传输

```
static void *si_buffer_transfer_map(struct pipe_context *ctx,
				    struct pipe_resource *resource (st_obj->buffer),
				    unsigned level(0),
				    unsigned usage(PIPE_TRANSFER_WRITE  | DISCARD_RANGE),
				    const struct pipe_box *box,
				    struct pipe_transfer **ptransfer)

	struct si_context *sctx = (struct si_context*)ctx;
	struct r600_resource *rbuffer = r600_resource(resource);
	uint8_t *data;

	if (rbuffer->b.is_user_ptr) ... 

	/* See if the buffer range being mapped has never been initialized,
	 * in which case it can be mapped unsynchronized. */
	if (!(usage & (PIPE_TRANSFER_UNSYNCHRONIZED |
		       TC_TRANSFER_MAP_NO_INFER_UNSYNCHRONIZED)) &&
	    usage & PIPE_TRANSFER_WRITE &&
	    !rbuffer->b.is_shared &&
	    !util_ranges_intersect(&rbuffer->valid_buffer_range, box->x, box->x + box->width)) {
		usage |= PIPE_TRANSFER_UNSYNCHRONIZED;
	}



	/* If discarding the entire range, discard the whole resource instead. */
	if (usage & PIPE_TRANSFER_DISCARD_RANGE &&
	    box->x == 0 && box->width == resource->width0) {
		usage |= PIPE_TRANSFER_DISCARD_WHOLE_RESOURCE;


	data = si_buffer_map_sync_with_rings(sctx, rbuffer, usage);

	data += box->x;
	return si_buffer_get_transfer(ctx, resource, usage, box,
					ptransfer, data, staging=NULL,offset=0);
	    struct si_transfer *transfer;
		transfer = slab_alloc(&sctx->pool_transfers);
	    pipe_resource_reference(&transfer->b.b.resource, resource);
	    *ptransfer = &transfer->b.b;
	    return data;


void *si_buffer_map_sync_with_rings(struct si_context *sctx,
				    struct r600_resource *resource,
				    unsigned usage)
	if (usage & PIPE_TRANSFER_UNSYNCHRONIZED) {
		return sctx->ws->buffer_map(resource->buf, NULL, usage);
            amdgpu_bo_map
    }
```


## 清理缓冲数据


```
void glClearBufferData(GLenum target, GLenum internalformat, GLenum format, GLenum type, const void *data);
    _mesa_ClearBufferData_no_error(GLenum target, GLenum internalformat,
                               GLenum format, GLenum type, const GLvoid *data)
         struct gl_buffer_object **bufObj = get_buffer_target(ctx, target);
         clear_buffer_sub_data_no_error(ctx, *bufObj, internalformat, 0,
                                        (*bufObj)->Size, format, type, data,
                                        "glClearBufferData", false);
                clear_buffer_sub_data(ctx, bufObj, internalformat, offset, size, format,
                                 type, data, func, subdata, true);
                    ctx->Driver.ClearBufferSubData(ctx, offset, size,
                                                   clearValue, clearValueSize, bufObj);




void glClearBufferSubData(GLenum target, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data);
     _mesa_ClearBufferSubData_no_error(GLenum target, GLenum internalformat,
        struct gl_buffer_object **bufObj = get_buffer_target(ctx, target);
        clear_buffer_sub_data_no_error(ctx, *bufObj, internalformat, offset, size,
                                       format, type, data, "glClearBufferSubData", true);
             clear_buffer_sub_data(ctx, bufObj, internalformat, offset, size, format,
                                   type, data, func, subdata, true);



st_clear_buffer_subdata(struct gl_context *ctx,
                        GLintptr offset, GLsizeiptr size,
                        const void *clearValue,
                        GLsizeiptr clearValueSize,
                        struct gl_buffer_object *bufObj)

   if (!pipe->clear_buffer) {
      _mesa_ClearBufferSubData_sw(ctx, offset, size,
                                  clearValue, clearValueSize, bufObj);
      return;
   }
   pipe->clear_buffer(pipe, buf->buffer, offset, size,
                      clearValue, clearValueSize);




static void si_pipe_clear_buffer(struct pipe_context *ctx,
				 struct pipe_resource *dst,
				 unsigned offset, unsigned size,
				 const void *clear_value,
				 int clear_value_size)
	si_clear_buffer((struct si_context*)ctx, dst, offset, size, (uint32_t*)clear_value,
			clear_value_size, coher);



void si_clear_buffer(struct si_context *sctx, struct pipe_resource *dst,
		     uint64_t offset, uint64_t size, uint32_t *clear_value,
		     uint32_t clear_value_size, enum si_coherency coher)

	/* Use transform feedback for 12-byte clears. */
	/* TODO: Use compute. */
	if (clear_value_size == 12) {
		union pipe_color_union streamout_clear_value;

		memcpy(&streamout_clear_value, clear_value, clear_value_size);
		si_blitter_begin(sctx, SI_DISABLE_RENDER_COND);
		util_blitter_clear_buffer(sctx->blitter, dst, offset,
					  size, clear_value_size / 4,
					  &streamout_clear_value);
		si_blitter_end(sctx);
		return;
	}

    // use compute
 	uint64_t aligned_size = size & ~3ull;
	if (aligned_size >= 4) {
		/* Before GFX9, CP DMA was very slow when clearing GTT, so never
		 * use CP DMA clears on those chips, because we can't be certain
		 * about buffer placements.
		 */
		if (clear_value_size > 4 ||
		    (clear_value_size == 4 &&
		     offset % 4 == 0 &&
		     (size > 32*1024 || sctx->chip_class <= VI))) {
			si_compute_do_clear_or_copy(sctx, dst, offset, NULL, 0,
						    aligned_size, clear_value,
						    clear_value_size, coher);
		} else {
			assert(clear_value_size == 4);
			si_cp_dma_clear_buffer(sctx, dst, offset,
					       aligned_size, *clear_value, coher,
					       get_cache_policy(sctx, coher, size));
		}
	}

	/* Handle non-dword alignment. */
	if (size) {
		pipe_buffer_write(&sctx->b, dst, offset, size, clear_value);
	}




```


## 绑定缓冲范围


```
_mesa_BindBufferRange_no_error(GLenum target, GLuint index, GLuint buffer,
                                GLintptr offset, GLsizeiptr size)
    bind_buffer_range(target, index, buffer, offset, size, true);
        
         if (buffer == 0) {
            bufObj = ctx->Shared->NullBufferObj;
         } else {
            bufObj = _mesa_lookup_bufferobj(ctx, buffer);
            if (!_mesa_handle_bind_buffer_gen(ctx, buffer,
                                              &bufObj, "glBindBufferRange"))
               return;
        }
        switch (target) {
        case GL_TRANSFORM_FEEDBACK_BUFFER:
           _mesa_bind_buffer_range_xfb(ctx, ctx->TransformFeedback.CurrentObject,
                                       index, bufObj, offset, size);
           return;
        case GL_UNIFORM_BUFFER:
           bind_buffer_range_uniform_buffer(ctx, index, bufObj, offset, size);
           return;
        case GL_SHADER_STORAGE_BUFFER:
           bind_buffer_range_shader_storage_buffer(ctx, index, bufObj, offset,
                                                   size);
           return;
        case GL_ATOMIC_COUNTER_BUFFER:
           bind_buffer_range_atomic_buffer(ctx, index, bufObj, offset, size);
           return;
        default:
           unreachable("invalid BindBufferRange target with KHR_no_error");
        }
 

```
* 对于这几个目标的分析见相应专门文档



## 无效化buffer


```
void glInvalidateBufferData(GLuint buffer​);
_mesa_InvalidateBufferSubData_no_error(GLuint buffer, GLintptr offset,
                                       GLsizeiptr length)
   struct gl_buffer_object *bufObj = _mesa_lookup_bufferobj(ctx, buffer);
   invalidate_buffer_subdata(ctx, bufObj, offset, length);
      if (ctx->Driver.InvalidateBufferSubData)
         ctx->Driver.InvalidateBufferSubData(ctx, bufObj, offset, length);



st_bufferobj_invalidate(struct gl_context *ctx,
                        struct gl_buffer_object *obj,
                        GLintptr offset,
                        GLsizeiptr size)
   pipe->invalidate_resource(pipe, st_obj->buffer);





static void si_invalidate_resource(struct pipe_context *ctx,

	if (resource->target == PIPE_BUFFER)
		(void)si_invalidate_buffer(sctx, rbuffer);


si_invalidate_buffer(struct si_context *sctx,
		     struct r600_resource *rbuffer)
	/* Shared buffers can't be reallocated. */
	if (rbuffer->b.is_shared)
		return false;

	/* Sparse buffers can't be reallocated. */
	if (rbuffer->flags & RADEON_FLAG_SPARSE)
		return false;

	/* In AMD_pinned_memory, the user pointer association only gets
	 * broken when the buffer is explicitly re-allocated.
	 */
	if (rbuffer->b.is_user_ptr)
		return false;

	/* Check if mapping this buffer would cause waiting for the GPU. */
	if (si_rings_is_buffer_referenced(sctx, rbuffer->buf, RADEON_USAGE_READWRITE) ||
	    !sctx->ws->buffer_wait(rbuffer->buf, 0, RADEON_USAGE_READWRITE)) {
		uint64_t old_va = rbuffer->gpu_address;

		/* Reallocate the buffer in the same pipe_resource. */
		si_alloc_resource(sctx->screen, rbuffer);
		si_rebind_buffer(sctx, &rbuffer->b.b, old_va);
	} else {
		util_range_set_empty(&rbuffer->valid_buffer_range);
	}

	return true;



/* Update all resource bindings where the buffer is bound, including
 * all resource descriptors. This is invalidate_buffer without
 * the invalidation. */
void si_rebind_buffer(struct si_context *sctx, struct pipe_resource *buf,
		      uint64_t old_va)

```
