#include <citro2d.h>

#include <cache.hpp>
#include <d7mc_config.hpp>
#include <fstream>

struct CacheHdr {
  uint32_t magic;  // 0x4548434E 'NCHE'
  uint32_t num_all;
  uint32_t num_entrys;
  bool has_icon;
};

struct CacheEntry {
  CacheEntry() {
    memset(name, 0, sizeof(name));
    memset(author, 0, sizeof(author));
    memset(icon, 0, sizeof(icon));
  }
  char name[64];
  char author[64];
  uint64_t tid;
  uint16_t icon[0x1600];
};

// Map for Iconless Cache
struct CacheEntryIL {
  char name[64];
  char author[64];
  uint64_t tid;
};

namespace D7MC {

void Cache::Create(std::vector<std::shared_ptr<Title>> t,
                   const std::string &path, int countall) {
  remove(path.c_str());
  std::vector<CacheEntry> entrys;
  entrys.reserve(t.size());

  for (unsigned i = 0; i < t.size(); i++) {
    CacheEntry tmp;
    memcpy(tmp.name, t[i]->name().c_str(), t[i]->name().length());
    memcpy(tmp.author, t[i]->author().c_str(), t[i]->author().length());
    if (d7mc_load_icons) {
      memcpy(tmp.icon, t[i]->get_iconbuffer(), sizeof(tmp.icon));
    }
    tmp.tid = t[i]->id();
    entrys.push_back(tmp);
  }
  CacheHdr hdr;
  hdr.magic = 0x4548434E;
  hdr.num_all = countall;
  hdr.num_entrys = entrys.size();
  hdr.has_icon = d7mc_load_icons;

  std::fstream fout(path, std::ios::out | std::ios::binary);
  fout.write(reinterpret_cast<const char *>(&hdr), sizeof(CacheHdr));
  for (const auto &it : entrys) {
    if (d7mc_load_icons)
      fout.write(reinterpret_cast<const char *>(&it), sizeof(CacheEntry));
    else
      fout.write(reinterpret_cast<const char *>(&it), sizeof(CacheEntryIL));
  }
  fout.close();
}

bool Cache::Read(std::vector<std::shared_ptr<Title>> &t,
                 const std::string &path, bool nand) {
  CacheHdr hdr;
  uint32_t titlecount = 0;
  std::fstream fin(path, std::ios::in | std::ios::binary);
  if (!fin.is_open()) {
    return false;
  }
  fin.read(reinterpret_cast<char *>(&hdr), sizeof(CacheHdr));
  if (hdr.magic != 0x4548434E) {
    fin.close();
    return false;
  }
  amInit();
  if (R_FAILED(AM_GetTitleCount((nand ? MEDIATYPE_NAND : MEDIATYPE_SD),
                                &titlecount))) {
    fin.close();
    return false;
  }
  amExit();
  if (titlecount != hdr.num_all) {
    fin.close();
    return false;
  }
  t.reserve(hdr.num_entrys);
  for (unsigned int i = 0; i < hdr.num_entrys; i++) {
    auto new_data = std::make_shared<Title>();
    CacheEntry tmp;
    if (hdr.has_icon)
      fin.read(reinterpret_cast<char *>(&tmp), sizeof(CacheEntry));
    else
      fin.read(reinterpret_cast<char *>(&tmp), sizeof(CacheEntryIL));
    new_data->load_from_cache(tmp.tid, tmp.name, tmp.author,
                              nand ? MEDIATYPE_NAND : MEDIATYPE_SD);
    new_data->set_icon(Title::load_icon_buffer(tmp.icon));
    t.push_back(new_data);
  }
  return true;
}
}  // namespace D7MC