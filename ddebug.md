# si_debug 功能分析


这篇文档主要介绍了关于GALLIUM radeonsi中命令流、Shader、描述符列表寄存器相关信息的打印调试功能分析。  
打印通过一个log_context记录当前的chunk信息  
si_context创建时会将log_context进行注册。   
当dd_context_create时会将dd_context中的log绑定到si_context的log  
每次u_log_flush时都会重置当前的log_context   

si对log chunk的打印分为三种， shader,cs, descriptor_list   
log chunk记录了传输给CP的命令流，当前shader，draw的信息，描述列表。

```c

static struct u_log_chunk_type si_log_chunk_type_shader = {
	.destroy = si_log_chunk_shader_destroy,
	.print = si_log_chunk_shader_print,
};


static const struct u_log_chunk_type si_log_chunk_type_cs = {
	.destroy = si_log_chunk_type_cs_destroy,
	.print = si_log_chunk_type_cs_print,
};

static const struct u_log_chunk_type si_log_chunk_type_descriptor_list = {
	.destroy = si_log_chunk_desc_list_destroy,
	.print = si_log_chunk_desc_list_print,
};

```

每次输出调试信息都会调用这里的print函数。

## print的设置流程

```mermaid 
graph TD 
dd_context_create --> si_set_log_context -->
u_log_add_auto_logger-->
si_auto_log_cs -->
si_log_cs -.add si_log_chunk_type_cs.-> u_log_chunk

si_draw_vbo -.current_saved_cs exits .-> si_log_draw_state
--> si_dump_framebuffer
si_log_draw_state --> si_dump_gfx_shader 
si_dump_gfx_shader -.add si_log_chunk_type_shader.-> u_log_chunk 
si_log_draw_state --> si_dump_descriptor_list
si_log_draw_state --> si_dump_gfx_descriptors
si_dump_gfx_descriptors --> si_dump_descriptor_list 
si_dump_descriptor_list -.add si_log_chunk_type_descriptor_list .-> u_log_chunk 


```

## 触发print

有两种情况会触发:

* 当gpu hang的时候会打印
* dump 记录的时候

不过都需要开启debug_mode,  这个通过设置GALLIUM_DDEBUG环境变量开启。 默认dump路径是$HOME/ddebug_dumps

关于具体选项设置可以首先通过设置GALLIUM_DDEBUG=help 后会自动显示相关帮助信息

### 触发流程

```mermaid
graph TD

dd_thread_main --> dd_maybe_dump_record
dd_thread_main --> dd_report_hang
dd_report_hang --> dd_write_record
dd_maybe_dump_record  --> dd_write_record 
dd_write_record -->u_log_page_print-.遍历所有chunk type .-> print

```
所以可以看出当dump log时也能打印shader编译信息


### 关于dd_thread_main

dd_context_create会创将dd_thread_main线程,保存在dd_context中的thread.

`dd_thread_main`是一个用于管线挂起的检测线程。 每次绘制调用时，都会有一个基于内存的栅栏被clear_buffer递增。不使用驱动程序栅栏。在每次绘制调用后，都会创建一个新的dd_draw_record，其中包含所有状态的副本，pipe_context::dump_debug_state的输出， 并且它具有分配的栅栏号。这样做是不知道该绘制调用是否有问题的。记录将添加到所有记录的列表中。独立的、单独的线程循环遍历记录列表并检查它们的栅栏。已信号标记的记录将被释放。
在栅栏超时时，该线程会转储正在进行的绘制的记录。

## IB 的来源流程

si_flush_dma_cs会将si_context中的dma_cs 保存  
si_flush_gfx_cs 会将gfx_cs 拷贝。

如下图所示

```mermaid
graph TD
	si_flush_dma_cs --> si_save_cs 
	si_flush_gfx_cs --> si_save_cs

```
`si_saved_cs` 通过参数拷贝si_context中gfx_cs的IB chunks, 通过winsys得到buffer list保存在saved 的bo_list中。


## si_log_chunk_type_cs_print 打印命令流


```mermaid
 
graph TD

A[si_log_chunk_type_cs_print] --> buffer_map(amdgpu_bo_cs ) 
A --> | si_context init config 等| C 
A --> B{si_saved_cs flushed}
B --> |true | C[ac_parse_ib]
B --> |false | D[ac_parse_current_ib]
A --> | chunk-> dump_bo_list exits | E[ si_dump_bo_list]
C --> F[ac_parse_ib_chunk]
F --> G[ac_do_parse_ib]
G --> | type-3 packet | H[ac_parse_packet3]
H --> I{ parse OP }
I --> | PTK3_SET_CONTEXT_REG | J[ac_parse_set_reg_packet]
I --> | ... | K[ ... ]


```




## si_log_chunk_type_shader 打印shader

这块比较简单就是调用了si_dump_shader， 之前文档有说明


##  si_log_chunk_type_descriptor_list

根据dw_size dump出不同资源的描述符。
```mermaid
graph TD
A[si_log_chunk_type_descriptor_list] --> B{chunk->element_dw_size}
B --> |case 4: R_008F00_SQ_BUF_RSRC_WORD0 | C[ac_dump_reg]
B --> |case 8: R_008F10_SQ_IMG_RSRC_WORD0 | C
B --> | ......  | C


``` 




