
#  Mip Tree

"Mip Tree"（Mipmap Tree，简称 Mip Tree）是一种用于优化纹理映射的数据结构。它是一系列不同分辨率的纹理图像，这些图像逐渐减小，以适应物体在场景中的远近变化。Mip Tree 主要用于提高渲染性能和图像质量。

Mip Tree 中的每个级别都被称为 "mipmap level" 或 "mip level"。具体来说，一个 Mip Tree 包含了原始纹理的完整分辨率图像（称为基本级别或第 0 级），以及一系列尺寸逐渐减小的图像，每个图像的分辨率是前一个级别的一半。

以下是 Mip Tree 中的几个概念：

1. **基本级别（Base Level）：** 最高分辨率的原始纹理图像。

2. **Mip Level 1, Mip Level 2, ...：** 逐渐减小的附加级别，每个级别的分辨率是前一个级别的一半。

Mip Tree 的结构允许在不同的渲染距离和观察角度下使用适当的纹理级别，以提高渲染效率。当物体在远处时，使用较低分辨率的 mipmap 级别可以减少纹理采样时的性能开销，而在物体靠近时，使用较高分辨率的 mipmap 级别可以提高图像质量。

在给定的场景中，渲染引擎可以根据物体到相机的距离选择适当的 mipmap 级别，这通常称为 "mipmapping"。这种技术在避免纹理锯齿效应、提高渲染效果和性能上都有很好的效果。

Mipmap（多级纹理映射）的实现通常涉及以下步骤：

1. **生成 Mipmap：** 针对原始纹理图像，生成一系列分辨率逐渐减小的图像。通常，每个级别的图像都是前一个级别图像的一半大小。生成 Mipmap 的方法可以是简单的平均采样、高质量的过滤算法（如三角滤波、高斯滤波等）。

2. **纹理采样时选择级别：** 在渲染时，根据物体到相机的距离选择适当的 Mipmap 级别。这可以通过计算物体的屏幕空间大小、视锥体投影等方法来确定。距离较远的物体使用较低级别的 Mipmap，距离较近的物体使用较高级别的 Mipmap。

3. **纹理采样时使用 Mipmap：** 在进行纹理采样时，根据选择的 Mipmap 级别来获取纹素。这可以通过在纹理采样过程中使用纹理坐标的不同级别来实现。这通常由图形硬件自动处理，但在某些情况下，可以手动指定纹理坐标的不同级别。

以下是一个简单的示例代码，演示了使用 OpenGL 进行 Mipmap 的生成和纹理采样的过程：

```cpp
#include <GL/glut.h>

GLuint textureID;

void init() {
    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // 设置纹理参数和加载原始纹理图像

    // 生成 Mipmap
    glGenerateMipmap(GL_TEXTURE_2D);

    // 设置 Mipmap 纹理过滤方式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // 设置 Mipmap 纹理包装方式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    // 在绘制时使用纹理
    glBindTexture(GL_TEXTURE_2D, textureID);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 0.0); glVertex2f(-1.0, -1.0);
    glTexCoord2f(1.0, 0.0); glVertex2f(1.0, -1.0);
    glTexCoord2f(1.0, 1.0); glVertex2f(1.0, 1.0);
    glTexCoord2f(0.0, 1.0); glVertex2f(-1.0, 1.0);
    glEnd();

    glutSwapBuffers();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutCreateWindow("Mipmap Example");
    glutDisplayFunc(display);

    init();

    glutMainLoop();
    return 0;
}
```

这个示例中，通过使用 `glGenerateMipmap` 生成 Mipmap，然后通过设置 `GL_TEXTURE_MIN_FILTER` 和 `GL_TEXTURE_MAG_FILTER` 来指定 Mipmap 的纹理过滤方式。在绘制时，通过 `glBindTexture` 绑定纹理，并使用 `glTexCoord2f` 指定纹理坐标。OpenGL 的纹理系统会根据 Mipmap 级别进行纹理采样。这只是一个简单的例子，实际应用中需要根据具体需求进行更详细的配置和调整。


## 平均采样

平均采样是一种简单的 Mipmap 级别间过渡的方法。在平均采样中，每个 Mipmap 级别的图像都是通过前一个级别的四个相邻像素的平均值来生成的。这种方法的优点在于实现简单，但它可能导致一些图像细节的丢失，特别是在图像中包含高频细节（例如锋利的边缘）时。

下面是一个简化的示例，演示如何进行平均采样。假设我们有一个原始纹理图像，它是一个数组 `originalImage`：

```cpp
// 假设 originalImage 是一个包含原始纹理图像的数组
uint8_t originalImage[width][height];
```

现在，我们要生成 Mipmap。对于每个 Mipmap 级别，我们可以使用平均采样方法：

```cpp
// 假设我们有多个 Mipmap 级别，每个级别都存储在 mipmaps 数组中
uint8_t mipmaps[numLevels][newWidth][newHeight];

// 原始图像的大小
int width = /* 原始图像宽度 */;
int height = /* 原始图像高度 */;

// 生成 Mipmap
for (int level = 1; level < numLevels; ++level) {
    int newWidth = width / 2;
    int newHeight = height / 2;

    for (int i = 0; i < newWidth; ++i) {
        for (int j = 0; j < newHeight; ++j) {
            // 平均采样：取前一个级别的四个相邻像素的平均值
            uint8_t averageValue = (
                originalImage[2 * i][2 * j] +
                originalImage[2 * i + 1][2 * j] +
                originalImage[2 * i][2 * j + 1] +
                originalImage[2 * i + 1][2 * j + 1]
            ) / 4;

            mipmaps[level][i][j] = averageValue;
        }
    }

    width = newWidth;
    height = newHeight;
}
```

这个例子中，`mipmaps` 数组存储了每个 Mipmap 级别的图像。在每个级别中，通过取前一个级别的四个相邻像素的平均值，生成新的图像。这个过程在每个级别都会将图像的宽度和高度减半，直到达到最低级别。这样就得到了包含多个 Mipmap 级别的数据结构。最终的 Mipmap 结构可以在纹理采样时使用，以根据距离和角度选择适当的 Mipmap 级别。



## 


/**
 * Called via ctx->Driver.GenerateMipmap().
 */
void
st_generate_mipmap(struct gl_context *ctx, GLenum target,
                   struct gl_texture_object *texObj)
{
   struct st_context *st = st_context(ctx);
   struct st_texture_object *stObj = st_texture_object(texObj);
   struct pipe_resource *pt = st_get_texobj_resource(texObj);
   const uint baseLevel = texObj->BaseLevel;
   enum pipe_format format;
   uint lastLevel, first_layer, last_layer;

   /* not sure if this ultimately actually should work,
      but we're not supporting multisampled textures yet. */
   assert(pt->nr_samples < 2);

   /* find expected last mipmap level to generate*/
   lastLevel = _mesa_compute_num_levels(ctx, texObj, target) - 1;

   if (lastLevel == 0)
      return;

   st_flush_bitmap_cache(st);
   st_invalidate_readpix_cache(st);

   /* The texture isn't in a "complete" state yet so set the expected
    * lastLevel here, since it won't get done in st_finalize_texture().
    */
   stObj->lastLevel = lastLevel;

   if (!texObj->Immutable) {
      const GLboolean genSave = texObj->GenerateMipmap;

      /* Temporarily set GenerateMipmap to true so that allocate_full_mipmap()
       * makes the right decision about full mipmap allocation.
       */
      texObj->GenerateMipmap = GL_TRUE;

      _mesa_prepare_mipmap_levels(ctx, texObj, baseLevel, lastLevel);

      texObj->GenerateMipmap = genSave;

      /* At this point, memory for all the texture levels has been
       * allocated.  However, the base level image may be in one resource
       * while the subsequent/smaller levels may be in another resource.
       * Finalizing the texture will copy the base images from the former
       * resource to the latter.
       *
       * After this, we'll have all mipmap levels in one resource.
       */
      st_finalize_texture(ctx, st->pipe, texObj, 0);
   }

   pt = stObj->pt;
   if (!pt) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "mipmap generation");
      return;
   }

   assert(pt->last_level >= lastLevel);

   if (pt->target == PIPE_TEXTURE_CUBE) {
      first_layer = last_layer = _mesa_tex_target_to_face(target);
   }
   else {
      first_layer = 0;
      last_layer = util_max_layer(pt, baseLevel);
   }

   if (stObj->surface_based)
      format = stObj->surface_format;
   else
      format = pt->format;

   /* First see if the driver supports hardware mipmap generation,
    * if not then generate the mipmap by rendering/texturing.
    * If that fails, use the software fallback.
    */
   if (!st->pipe->screen->get_param(st->pipe->screen,
                                    PIPE_CAP_GENERATE_MIPMAP) ||
       !st->pipe->generate_mipmap(st->pipe, pt, format, baseLevel,
                                  lastLevel, first_layer, last_layer)) {

      if (!util_gen_mipmap(st->pipe, pt, format, baseLevel, lastLevel,
                           first_layer, last_layer, PIPE_TEX_FILTER_LINEAR)) {
         _mesa_generate_mipmap(ctx, target, texObj);
      }
   }
}

