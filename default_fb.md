
**Written by Sj.yu**


# defalut fb的创建

默认fb在gl_context中以 WinSysDrawBuffer, WinSysReadBuffer 成员存在。, 在默认fb 这两个成员指向同一个。


在gl_context  置为当前context的时候， 会判断fb是否存在， 如果没有， 对这两个gl_framebuffer进行创建。


基本流程：

* 调用st_framebuffer_create 创建st_framebuffer,  内部调用 \_mesa_initialize_window_framebuffer,进行简单的fb初步设置。


* 在st_framebuffer_create 内部，然后对颜色0，深度， 累积附件,在st_framebuffer_add_renderbuffer 内部创建st_renderbuffer作为附件加入fb, 该接口返回一个gl_renderbuffer。

* 在st_framebuffer_add_renderbuffer内部， 创建renderbuffer之后， 使用\_mesa_attach_and_own_rb 将rb作为附件附加到fb上, _mesa_attach_and_own_rb会将该附件完整性置为真。
* 

* 更新附件st_framebuffer_update_attachments, 实际上就是将附件保存到st_framebuffer的statts ,

* 验证framebuffer,在 dri_st_framebuffer_validate ,  会根据申请statts数量的纹理资源， 申请纹理会调用addrlib 进行初始化tex->surf， 其中包含了关于纹理的相关设置， 如cmask, fmask.

* 接下来对每一个附件的纹理， 创建`st_renderbuffer-> surface`表面， 间接调用driver层si_create_surface ,同时将该表面所用的纹理指向刚才的纹理， 将宽高设置surface宽高。 

* 将gl_context置为当前context ,更新WinSysDrawBuffer, WinSysReadBuffer 为之前创建的drawbuffer.
