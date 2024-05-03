#include <d7mc_config.hpp>
#include <smdh.hpp>
#include <title.hpp>
#include <utils.hpp>

namespace D7MC {

Title::Title(void) {
  // No need to do anything
}

Title::~Title(void) {
  if (d7mc_load_icons) {
    if (m_icon.tex != nullptr) {
      C3D_TexDelete(m_icon.tex);
    }
    delete_icon_buffer();
  }
}

bool Title::load_from_cache(const uint64_t &tid, const std::string &title,
                            const std::string &author, const uint8_t &mt) {
  m_tid = tid;
  m_mediatype = (FS_MediaType)mt;
  m_name = title;
  m_author = author;
  return true;
}

bool Title::load(const u64 &id, const FS_MediaType &media) {
  bool titleload = false;

  m_tid = id;
  m_mediatype = media;
  smdh_s *smdh = loadSMDH(lowid(), highid(), m_mediatype);
  if (smdh == NULL) {
    return false;
  }
  m_name = UTF16toUTF8((char16_t *)smdh->applicationTitles[1].shortDescription);
  if (!IsValidName(m_name)) {
    delete smdh;
    return false;
  }

  m_author = UTF16toUTF8((char16_t *)smdh->applicationTitles[1].publisher);
  titleload = true;
  if (d7mc_load_icons) {
    icon_buffer = new u16[0x1600];
    memset(icon_buffer, 0, 0x1600);
    memcpy(icon_buffer, smdh->bigIconData, 0x1600);
    m_icon = Title::load_icon_buffer(icon_buffer);
  }
  delete smdh;
  return titleload;
}

void Title::set_icon(const C2D_Image &icn) { m_icon = icn; }

u32 Title::highid(void) { return (u32)(m_tid >> 32); }

u32 Title::lowid(void) { return (u32)m_tid; }

u64 Title::id(void) { return m_tid; }

FS_MediaType Title::mediatype(void) { return m_mediatype; }

C2D_Image Title::icon(void) { return m_icon; }

// get the name of the Title from the SMDH.
std::string Title::name(void) { return m_name; }

std::string Title::author(void) { return m_author; }

std::string Title::str_mediatype(void) {
  switch (this->m_mediatype) {
    case MEDIATYPE_SD:
      return "SD";
      break;
    case MEDIATYPE_NAND:
      return "NAND";
      break;
    case MEDIATYPE_GAME_CARD:
      return "Game Card";
      break;
    default:
      return "Unknown";
      break;
  }
  return "Unknown";
}

u16 *Title::get_iconbuffer() { return icon_buffer; }

void Title::delete_icon_buffer() {
  if (icon_buffer != nullptr) delete[] icon_buffer;
}

C2D_Image Title::load_icon_buffer(u16 *buf) {
  C3D_Tex *tex = new C3D_Tex;
  static const Tex3DS_SubTexture subt3x = {48,         48,         0.0f,
                                           48 / 64.0f, 48 / 64.0f, 0.0f};
  C3D_TexInit(tex, 64, 64, GPU_RGB565);
  memset(tex->data, 0, tex->size);

  u16 *dest = (u16 *)tex->data + (64 - 48) * 64;
  for (int j = 0; j < 48; j += 8) {
    std::copy(buf, buf + 48 * 8, dest);
    buf += 48 * 8;
    dest += 64 * 8;
  }

  return C2D_Image{tex, &subt3x};
}

bool Title::IsValidName(const std::string &name) {
  if (name == "???") {
    return false;
  }

  else if (name == "") {
    return false;
  }

  return true;
}
}  // namespace D7MC