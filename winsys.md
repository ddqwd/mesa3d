

bo_create也会在slab标志启用时候进行调用。

有下图可见bo_create也会在资源提交的时候调用， 而si_resource_commit是在测试GL_ARB_sparse_buffer 扩展时候调用， 这个piglit测试的bin程序在arb_sparse_buffer-basic程序中， 为简化 winsys的测试程序， 暂时不用这个接口程序进行测试,等以后需要完善piglit测试程序的时候可以加上这个逻辑进行测试.  
``` mermaid 
graph TD
A[buffer_create]
B[pb_slab_alloc]
C[si_resource_commit]
style A fill:#99ccff, stroke:#0066cc, stroke-width:2px;
style B fill:#99ccff, stroke:#0066cc, stroke-width:2px;
style C fill:#99ccff, stroke:#0066cc, stroke-width:2px;

A --> amdgpu_bo_create
amdgpu_bo_create-->  |未设置flags 为RADEON_FLAG_NO_SUBALOC| B --> |pb_slabs_init -.slabs->slba_alloc |amdgpu_bo_slab_alloc --> amdgpu_bo_create
C --> buffer_commit--> amdgpu_bo_sparse_commit--> sparse_backing_alloc --> amdgpu_bo_create

C --> si_flush_gfx_cs
C --> si_flush_dma_cs

glBufferPageCommmitmentARB -->  |测试GL_ARB_sparse_buffer| _mesa_BufferPageCommitmentARB --> buffer_page_commitment--> BufferPageCommitment --> st_bufferobj_page_commitment--> resource_commit --> C

_mesa_NamedBufferPageCommitmentARB --> buffer_page_commitment
```




