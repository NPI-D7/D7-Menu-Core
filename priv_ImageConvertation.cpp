#include <renderd7/Image.hpp>
#include <renderd7/Ovl.hpp>
#include <renderd7/Toast.hpp>

#include <cache.hpp>
#include <renderd7/external/stb_image.h>


extern bool usedbgmsg;

static u32 GetNextPowerOf2(u32 v) {
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v++;
  return (v >= 64 ? v : 64);
}

static bool C3DTexToC2DImage(C2D_Image *texture, u32 width, u32 height,
                             u8 *buf) {
  if (width >= 1024 || height >= 1024)
    return false;

  C3D_Tex *tex = new C3D_Tex[sizeof(C3D_Tex)];
  Tex3DS_SubTexture *subtex = new Tex3DS_SubTexture[sizeof(Tex3DS_SubTexture)];
  subtex->width = static_cast<u16>(width);
  subtex->height = static_cast<u16>(height);
  // RGBA -> ABGR
  for (u32 row = 0; row < subtex->width; row++) {
    for (u32 col = 0; col < subtex->height; col++) {
      u32 z = (row + col * subtex->width) * 4;

      u8 r = *(u8 *)(buf + z);
      u8 g = *(u8 *)(buf + z + 1);
      u8 b = *(u8 *)(buf + z + 2);
      u8 a = *(u8 *)(buf + z + 3);

      *(buf + z) = a;
      *(buf + z + 1) = b;
      *(buf + z + 2) = g;
      *(buf + z + 3) = r;
    }
  }

  u32 w_pow2 = GetNextPowerOf2(subtex->width);
  u32 h_pow2 = GetNextPowerOf2(subtex->height);
  subtex->left = 0.f;
  subtex->top = 1.f;
  subtex->right = (subtex->width / static_cast<float>(w_pow2));
  subtex->bottom = (1.0 - (subtex->height / static_cast<float>(h_pow2)));
  C3D_TexInit(tex, static_cast<u16>(w_pow2), static_cast<u16>(h_pow2),
              GPU_RGBA8);
  C3D_TexSetFilter(tex, GPU_NEAREST, GPU_NEAREST);

  std::memset(tex->data, 0, tex->size);

  for (u32 x = 0; x < subtex->width; x++) {
    for (u32 y = 0; y < subtex->height; y++) {
      u32 dst_pos = ((((y >> 3) * (w_pow2 >> 3) + (x >> 3)) << 6) +
                     ((x & 1) | ((y & 1) << 1) | ((x & 2) << 1) |
                      ((y & 2) << 2) | ((x & 4) << 2) | ((y & 4) << 3))) *
                    4;
      u32 src_pos = (y * subtex->width + x) * 4;
      std::memcpy(&(static_cast<u8 *>(tex->data))[dst_pos],
                  &(static_cast<u8 *>(buf))[src_pos], 4);
    }
  }

  C3D_TexFlush(tex);
  tex->border = RenderD7::Color::Hex("#000000", 0);
  C3D_TexSetWrap(tex, GPU_CLAMP_TO_BORDER, GPU_CLAMP_TO_BORDER);
  if (tex && subtex) {
    texture->tex = tex;
    texture->subtex = subtex;
    return true;
  }
  return false;
}

extern "C" {
#include <renderd7/external/libnsbmp/libnsbmp.h>
}
static const u32 BYTES_PER_PIXEL = 4;
#define MAX_IMAGE_BYTES (48 * 1024 * 1024)

namespace LIBBMP {
static void *bitmap_create(int width, int height,
                           [[maybe_unused]] unsigned int state) {
  /* ensure a stupidly large (>50Megs or so) bitmap is not created */
  if ((static_cast<long long>(width) * static_cast<long long>(height)) >
      (MAX_IMAGE_BYTES / BYTES_PER_PIXEL))
    return nullptr;

  return std::calloc(width * height, BYTES_PER_PIXEL);
}

static unsigned char *bitmap_get_buffer(void *bitmap) {
  assert(bitmap);
  return static_cast<unsigned char *>(bitmap);
}

static size_t bitmap_get_bpp([[maybe_unused]] void *bitmap) {
  return BYTES_PER_PIXEL;
}

static void bitmap_destroy(void *bitmap) {
  assert(bitmap);
  std::free(bitmap);
}
} // namespace LIBBMP

unsigned NImage_to_C3D(C2D_Image *img, const std::vector<unsigned char> &bmpc) {
  bmp_bitmap_callback_vt bitmap_callbacks = {
      LIBBMP::bitmap_create, LIBBMP::bitmap_destroy, LIBBMP::bitmap_get_buffer,
      LIBBMP::bitmap_get_bpp};

  bmp_result code = BMP_OK;
  bmp_image bmp;
  bmp_create(&bmp, &bitmap_callbacks);

  code = bmp_analyse(&bmp, bmpc.size(), (u8 *)bmpc.data());
  if (code != BMP_OK) {
    bmp_finalise(&bmp);
    return 1;
  }

  code = bmp_decode(&bmp);
  if (code != BMP_OK) {
    if ((code != BMP_INSUFFICIENT_DATA) && (code != BMP_DATA_ERROR)) {
      bmp_finalise(&bmp);
      return 2;
    }

    /* skip if the decoded image would be ridiculously large */
    if ((bmp.width * bmp.height) > 200000) {
      bmp_finalise(&bmp);
      return 3;
    }
  }
  bool ret = C3DTexToC2DImage(img, static_cast<u32>(bmp.width),
                              static_cast<u32>(bmp.height),
                              static_cast<u8 *>(bmp.bitmap));
  bmp_finalise(&bmp);
  if (!ret) {
    return 4;
  }
  return 0;
}