

# amdgpu winsys分析

## 


```mermaid
amdgpu_bo_init_functions-.初始化ws->base.buffer_create.-> A

A[amdgpu_bo_create] --> 
B[amdgpu_create_bo]


C[amdgpu_get_new_ib]--> D[amdgpu_ib_new_buffer]
D -->E[buffer_create]

F[si_alloc_resoure]--> B
I[si_buffer_create] --> F
J[si_invalidata_buffer]--> F
K[si_texture_create_object]-->F
L[si_texture_invalidata_storage] --> F

M[si_resource_create] --> I
N[pipe_aligned_buffer_create]--> I

A1[si_buffer_transfer_map]--> J
B1[si_buffer_subdata] --> A1
C1[bufferobj_data] --> B1
D1[st_bufferojb_data] --> C1

E1[buffer_data] --> D1
F1[_mesa_buffer_data]-->E1
H1[_mesa_BufferData_no_error] -->  I1

I1[buffer_data_no_error] --> E1
L1[buffer_data_error] -->E1 
K1[glBufferData] --> M1[_mesa_BufferData]
M1-->F1


G[amdgpu_cs_create] --> C
H[amdgpu_cs_flush]--> C


amdgpu_cs_init_functions -.初始化ws->base._cs_flush, cs_create等.-> H

si_flush_dma_cs --> H
si_flush_gfx_cs --> H

si_create_context -.构造dma_cs, gfx_cs.-> G


```
si_init_screen_buffer_functions.初始化si_screen的b.resource_create,destro
buffer_subdata为si_buffer_subdata.

### 关于`_mesa_BufferData_no_error` 

在OpenGL中，可以通过调用`glGetError`函数来获取OpenGL的错误状态。默认情况下，OpenGL会在出现错误时自动将错误状态设置，并且在出现错误后会导致OpenGL函数调用的立即返回。这样可以帮助开发者快速发现和解决问题。

如果你想在某些情况下禁用OpenGL错误检查，可以使用以下方法：

1. **设置noerror标志**：你可以通过调用`glDebugMessageControl`函数来设置OpenGL的错误检查模式。这将允许你选择哪些类型的错误消息会产生，或者完全禁用错误消息。

   ```c
   // 禁用所有错误消息
   glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_FALSE);
   ```

   在此例中，`GL_DONT_CARE`参数表示不关心特定的消息类型或源类型。最后一个参数设置为`GL_FALSE`表示禁用错误消息。你可以根据需要选择特定的消息类型和源类型。

2. **OpenGL调试上下文**：另一种方法是使用OpenGL调试上下文。在使用OpenGL的调试扩展时，你可以启用或禁用调试输出，包括错误消息。

   ```c
   // 初始化调试上下文
   glEnable(GL_DEBUG_OUTPUT);
   glDebugMessageCallback(DebugMessageCallback, NULL);
   ```

   然后，你可以实现`DebugMessageCallback`回调函数来处理调试消息，并根据需要选择是否处理错误消息。

需要注意的是，禁用OpenGL错误检查可能会导致难以调试的问题，并且不推荐在正式代码中禁用错误检查。在开发阶段，错误检查对于快速发现和解决问题非常有用，因此应该在开发时启用错误检查，并在发布版本中禁用它。



st_init_bufferobj_functions 初始化pipe_screen的screen的BufferData为st_bufferojb_data



### 关于amdgpu_winsys的创建


```mermaid


graph TD
	A0[pipe_loader_create_screen] -->A	
	A[pipe_radeonsi_create_screen]-> B[amdgpu_winsys_create]
```
关于amdgpu_winsys_create 的函数内部流程图

```mermaid
	amdgpu_winsys_create --> "调用drmGetVersion验证版本"
	--> "从dev_table查找winsys"
	--> "初始化amdgpu设备amdgpu_device_initialize"
	-->  A {查找一个winsys}
	A --> |查找成功| ”retaurn ws->base"
	A --> |重新创建一个winsys"
	--> "do_init_winsys 初始化"
	-->  "创建managers"
	-->  "设置 ws->base.回调函数"
	--> "amdgpu_bo_init_functions初始化bo函数ws->base.buffer如buffer_map"
	--> "amdgpu_cs_init_functions初始化命令流函数回调ws->base.cs* 如cs_create为amdgpu_cs_create"
	--> "amdgpu_surface_init_functions初始化surface回调ws->base.surface_init"
	--> "创建屏幕screen_create设置ws->base.screen"
	--> "返回ws->base"
```


