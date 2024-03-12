
https://en.wikipedia.org/wiki/Direct_Rendering_Infrastructure
# software architecture

* 

*  the X server privides an X11 protocol extension

 X server use dri driver to hardware accelerated 3D rendering

*


# DRI1



# DRI2

instead of a single shared buffer ,every DRI client gets its own private back buffer  .

DRI2 first use  TMM  memory manger , and later use GEM .
* DRI2 clients no longer lock the entire DRM device while using it for rendering .
* DRI2 clients can allocate their own buffers in the video memory 

* DRI clients retri     

# DRI3

consist of two different extensions

A better performace is also achieved because

the DRI3 extension3 no longer needs to modified to support new paricular buffer 
Allowss  of two different extension
DRI3 consist of two different extensions teh DRI3 adn the Present extension

purpose  to share direct rendered buffers between DRI clients and the X server.

DRI client allocate and use GEM buffers objects as rendering targets, while the X Server represents these render buffer using a type of X11 Object called "pixmap " 
In these DRI3 operations GEM buffer Objects are passed as DMA-buf file desc iptors intead of GEM names. DRI3 also provides a way to share synchronization objects  between the DRI client and the X Server. 
Screen updates have to done at the proper time . 

Presetn accepts any X pixmap as the source for a screen updaste. Since pixmaps are standard X objects, Present can be used not only DRI3 clients performing direct rendering . ,but also by any X client rendering 
Present brings other advantages .DRI3 graphics 


