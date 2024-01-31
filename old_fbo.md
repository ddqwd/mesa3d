


# 9.1 概览
fb和fbo不是一个概念。
fbo的每一像素的位组成位层。
这些位层组成逻辑buffers。
这些buffers又分为
颜色， 累积， 深度，模板。
累积accum  只在兼容模式出现。
颜色buffer由许多buffer组成，
在默认fbo和fbo又有所不同。
在默认buffer中， 颜色buffer分为 front left, front right, back left ,\
其中在兼容模式下， 还有back right buffer， 以及其他辅助buffer.
 单眼 显示front left, 立体显示front left 和front right.
 所有的颜色buffer的位层数量必须相同。 
 如果没有默认的fbo， framebuffer是不完全的。
 在默认fb中， fbo和颜色buffer是不可见的。
 fb如果要使用o必须是完全的。
fb在各个buffer的位层数量是不同的。
默认fb中， 位层数量是固定的。
对于fbo来说， 位平面的数量可以会随着图像附着点改变而改变。
fb有两个激活的fb， draw和read,
默认fb初始化draw和read.


# 9.2 管理fbo

离屏渲染允许renderbuffer的图像附着到fb上
渲染到纹理允许纹理图像附着到fb。

默认fb的操作被窗口系统管理。

glBindFramebuffer  导致fbo被创建：
fbo这个状态容器由各种状态组成，可查。

glBindFramebuffer 也可以用来绑定一个存在的buffer

glCreateFramebuffers也可以创建fbo

fbo和默认fb有以下不同：

1. 帧缓冲对象具有可修改的附件点：与默认帧缓冲不同，帧缓冲对象具有每个逻辑缓冲区的可修改附件点。这意味着你可以将可附加的图像附加到这些附件点上，也可以将它们从附件点上分离。这些附件点是帧缓冲对象的一部分，它们用于定义帧缓冲中的不同逻辑缓冲区，如颜色缓冲、深度缓冲和模板缓冲。

2. 图像的大小和格式由OpenGL控制：帧缓冲对象中附加的图像的大小和格式是由OpenGL接口完全控制的。这意味着它们不受窗口系统事件的影响，例如像素格式选择、窗口调整大小和显示模式更改。与默认帧缓冲不同，帧缓冲对象允许你独立地定义和控制附加的图像，而无需担心窗口系统的影响。

此外还有：
1. 像素所有权测试总是成功。

2. 没有可见的颜色buffer位平面。这意味这没有后，前， 左或唷欧颜色位平面。

3. 唯一的颜色缓冲位平面是由帧缓冲附件点命名位 COLOR_ATTACHMENT0 -N 定义的位平面。 

4. 唯一的深度缓冲位平面是DEPTH_ATTACHMENHT

5. 唯一的模板缓冲位平面是STENCIL_ATTACHMENT

6. (兼容模式） 没有累积buffer, 没有辅助buffer

7. 如果附件的大小不完全相同， 则渲染的结果金在
8. 如果附件的大小不完全相同，则渲染的结果仅在所有附件都能容纳的最大区域内定义。该区域被定义为每个附件的左下角为(0, 0)，右上角为(width, height)的矩形的交集。在渲染命令执行后，此区域之外的附件内容是未定义的（如第2.4节中定义）。

9. 如果没有附件，渲染将受限于一个矩形，其左下角为(0, 0)，右上角为(width, height)，其中width和height是帧缓冲对象的默认宽度和高度。

10. 如果每个附件的层数不完全相同，渲染将受限于任何附件的最小层数。如果没有附件，则层数将从帧缓冲对象的默认层数中取得。


 ## 创建fbo

```c
 void GenFramebuffers(sizei n , *framebuffers);
```
## 删除fbo
```c
void DeleteFramebuffers(sizei n ,const uint *framebuffers);

```

* `IsFramebuffer`用来判断framebuffers是不是一个fbo，0不是一个fbo

* `void FramebufferParameteri(enum target, enum pname,  int param); `和 `void NamedFramebufferParameteri(uint framebuffer , enum pname, int param);`是用设定 fbo的参数。

* FramebufferParameteri 的target是DRAW_FRAMEBUFFER, READ_FRAMEBUFFER, 或FRAMEBUFFER， FRAMEBUFFER等同DRAW。
* NamedFramebufferParameteri的taraget是fbo的名字。
* 当fb有多个附件的时候， 宽度，高度， 层数， 采样数， 采样位置模式 继承fb附件的属性。 当fb没有附件时候， 这些属性 是从fb参数获取。
* 参数有 FRAMEBUFFER_DEFAULT_WIDTH, FRAMEBUFFER_DEFAULT_HEIGHT, FRAMEBUFFER_DEFAULT_LAYERS, FRAMEBUFFER_DEFAULT_SAMPLES, or FRAMEBUFFER_DEFAULT_FIXED_SAMPLE_LOCATIONS
* 当fb没有附件时，  


* 图像附件时指 Framebuffer-attachable.
* 默认fb 的图像附件无法更改。
* 单个可附着图像可以附着到多个fbo，比卖年数据拷贝， 减少内存消费。
* 每个逻辑缓冲， 一个fbo 存储了一系列定义逻辑缓冲附着点的状态。
### 可附着图像的类型有：

* renderbuffer。，总是二维的。
* 单个一维纹理， 被对待作为二维纹理使用。
* 单级而未多重采样或矩形纹理。
* 立方体纹理的一个面。
* 一维或二级纹理数组的一个层
* 立方体数组的一个层。

* 查询fbo参数 通过`void GetFramebufferParameteriv(enum target, enum pname, int *params)` 和 `GetNamedFramebufferParameteriv`

* fbo的附件 或这**默认fb的缓冲** 可以通过` GetFramebufferAttachmentParameteriv GetNamedFramebufferAttachmentParameteriv`进行参数查询


## 多重采样查询

* `SAMPLE_BUFFERS`和`SAMPLERS`控制多重采样怎样被操作的。 
* 如果fbo不是fb完全的话， SAMPLERS_BUFFERS和SAMPLERS是未定义的。 否则SAMPLERS等同RENDERBUFFER_SAMPLERS或者纹理采样。依赖附件图像的类型。

## Renderbuffer objects

* renderbuffer 是一个保存可渲染内部个格式的一个数据存储对象。 
* rbo创建可以通过BindRenderbuffer， 创建，也克罗伊通过 `CreateRenderbuffers`创建。

* GenRenderbuffers 用来返回在renderbuffers中没有使用的rbo名字。
* DeleteRenderbuffers删除rbo
* IsRenderbuffer用来判断是否是rbo

* 图像存储， 格式， dimensions， rbo图像采样的数量通过renderbufferStorageMultisample, NamedRenderbufferStorageMultisample建立

* renderbufferStorageMultisample的target必须是RENDERBUFFER , samplers是0 ，RENDERBUFFER_SIZE被设置为0 ，  否则采样代表**最小请求采样数**, 具体采样大小由实现决定。
* renderbufferstorage 和 NamedRenderbufferStorage 等同于之前
* 通过FramebfferRenderbuffer可以将rbo变成fbo的附件。一个rbo
* 如果删除rbo， 那么绑定到fbo的这个rbo 会依次删除


* fbo 通过CopyTexImage\* 和 CopyTexSubImage\* 将fb的渲染部分拷贝到一个纹理 对象。 额外的， GL支持直接渲染纹理对象图像。
* 为了直接渲染到一个纹理图像， 一个纹理对象的 ** 明确等级** 需要确定。 通过`FramebufferTexture`和`NamedFramebufferTexture`确定。
* 如果纹理是三维纹理类的，那么这个附件被认为是分层的。
* 

8.2 Sampler 对象

当一个采样对象绑定到一个纹理单元它的状态
* 采样器的参数设置通过` SamplerParameter `来设置。
* 删除采样其DeleteSampelr
* 判断是否是采样IsSampler
Pixel Rectangle 
Pixel Store*, PixelTransfer*, and  

1. Texture 

压缩纹理图像

Immutable-Format Texture Images
An alternative set of commands is provideed ofr specify  ing th prooperites of all levels of a texture at  once ,Once a texture is specified with such a command, the format and dimensiions of all levels becomes immutable , unless it is a proxy textu re ;.
否则， 纹理单元操作在一个正常的Let Dt be the depth texture value and St be the stencil index component . If there is no stencil componnent 

# 9,4 fb 完整性

* 如果默认fb 存在，那么它总是完整的。
* 如果fbo的所有附件图像

* fb 附件完整性 ， 如果 FRAMEBUFFER_ATTACHMET——OBJECT——TYPE 的值不为i0  ， 那么也就是说fb 可附着图像 是附着在这个附着点上面去
* 如果FRAMEBUFFER-ATTACH——MENT——OBJECT——TYPE 为0  那么称这个fb附着点附件是fb附着完全的。如果不为0的时候哦满足下面的条件为真。

1.  图像"image"是与"FRAMEBUFFER_ATTACHMENT_OBJECT_NAME"指定的名称相关联的现有对象的一部分，且其类型与"FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE"指定的类型相匹配。
2. image的宽度和高度必须在一个范围之类
3. 如果图像时三位的话， 
4. 如果图像时三维的话，
5. 如果图像由多个采样的，它的采样数 必须在一定的 范围和
6. 如果图像不是一个不可变纹理的话，它的选择等级必须在一定的范围内。
7. 如果图像图像不是一个不可变纹理的话，
8. 如果附件是color附件 ，图像必须有一个可颜色渲染的内部格式。
9. 如果附件是depth， 图像必须由一个可深度渲染的内部格式
10. 如果附件是stencil, 图像必须有一个可模板渲染的内部它格式‘、
：
## 附件完整性

1.  IsFramebuffer



# opengl fbo wiki 


# 语义

帧缓冲对象是一组附件的集合。为了更好地解释，让我们明确定义一些术语。

## 图像

在本文中，图像是一组像素的二维数组。它对这些像素有特定的格式。

## 分层图像

在本文中，分层图像是具有特定大小和格式的一系列图像。分层图像来自于某些纹理类型的单一mipmap级别。

## 纹理

在本文中，纹理是一个包含一定数量图像的对象，如上所述。所有图像具有相同的格式，但它们的大小可以不同（例如，不同的mipmaps）。通过各种方法，着色器可以从纹理中访问这些图像。

## 渲染缓冲

渲染缓冲是包含单个图像的对象。渲染缓冲不能以任何方式被着色器访问。除了创建它之外，处理渲染缓冲的唯一方式是将它放入帧缓冲对象中。

## 附加

连接一个对象到另一个对象。这个术语在OpenGL中被广泛使用，但帧缓冲对象最充分地利用了这个概念。附加不同于绑定。对象被绑定到上下文中，而对象被附加到彼此之间。

## 附着点

帧缓冲对象内的一个命名位置，可以将可附加图像或分层图像附着到其中。附着点限制了附着到它们的图像的通用图像格式。

## 帧缓冲可附加图像

可以附着到帧缓冲对象的任何图像，如前所述。

## 帧缓冲可附加的分层图像

可以附着到帧缓冲对象的任何分层图像，如前所述。


