#pragma once

#include <3ds.h>
#include <citro2d.h>

#include <algorithm>
#include <string>

namespace D7MC {
class Title {
 public:
  Title();
  ~Title();

  bool load_from_cache(const uint64_t& tid, const std::string& title,
                       const std::string& author, const uint8_t& mt);
  bool load(const u64& id, const FS_MediaType& mediatype);
  void set_icon(const C2D_Image& icn);
  u32 highid();
  u32 lowid();
  u64 id();
  FS_MediaType mediatype();
  std::string str_mediatype();
  std::string name();
  std::string author();
  C2D_Image icon();
  u16* get_iconbuffer();
  void delete_icon_buffer();
  static C2D_Image load_icon_buffer(u16* buf);
  static bool IsValidName(const std::string& name);

 private:
  u64 m_tid;
  FS_MediaType m_mediatype;
  FS_CardType m_cardtype;
  C2D_Image m_icon;
  u16* icon_buffer = nullptr;
  std::string m_name;
  std::string m_author;
};
}  // namespace D7MC