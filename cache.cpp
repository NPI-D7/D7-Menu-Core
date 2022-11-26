#include "cache.hpp"
#include "TitleManager.hpp"
#include "utils.hpp"
#include <cstdint>
#include <iostream>
#include <rd7.hpp>
#include <renderd7/ini.hpp>
#include <renderd7/log.hpp>
#include <sstream>

Log cachelog;

static const size_t ENTRYSIZE = 5341;

static void putPixel565(u8 *dst, u8 x, u8 y, u16 v) {
  dst[(x + (47 - y) * 48) * 3 + 0] = (v & 0x1F) << 3;
  dst[(x + (47 - y) * 48) * 3 + 1] = ((v >> 5) & 0x3F) << 2;
  dst[(x + (47 - y) * 48) * 3 + 2] = ((v >> 11) & 0x1F) << 3;
}

static void convertrgb565(unsigned char *pixels, u16 *icon_buffer) {
  // Convert RGB565 icon to a RGB24 one
  int tile_size = 16;
  int tile_number = 1;
  int extra_x = 0;
  int extra_y = 0;
  int i = 0;
  int tile_x[16] = {0, 1, 0, 1, 2, 3, 2, 3, 0, 1, 0, 1, 2, 3, 2, 3};
  int tile_y[16] = {0, 0, 1, 1, 0, 0, 1, 1, 2, 2, 3, 3, 2, 2, 3, 3};
  while (tile_number < 37) {
    while (i < (tile_size)) {
      putPixel565(pixels, tile_x[i - ((tile_number - 1) << 6)] + extra_x,
                  tile_y[i - ((tile_number - 1) << 6)] + extra_y,
                  icon_buffer[i]);
      putPixel565(pixels, 4 + tile_x[i - ((tile_number - 1) << 6)] + extra_x,
                  tile_y[i - ((tile_number - 1) << 6)] + extra_y,
                  icon_buffer[i + 16]);
      putPixel565(pixels, tile_x[i - ((tile_number - 1) << 6)] + extra_x,
                  4 + tile_y[i - ((tile_number - 1) << 6)] + extra_y,
                  icon_buffer[i + 32]);
      putPixel565(pixels, 4 + tile_x[i - ((tile_number - 1) << 6)] + extra_x,
                  4 + tile_y[i - ((tile_number - 1) << 6)] + extra_y,
                  icon_buffer[i + 48]);
      i++;
    }
    if (tile_number % 6 == 0) {
      extra_x = 0;
      extra_y = extra_y + 8;
    } else
      extra_x = extra_x + 8;
    tile_number++;
    tile_size = tile_size + 64;
    i = i + 48;
  }
}

void Cache::Create(std::vector<std::shared_ptr<Title>> t,
                   const std::string &path, int countall) {
  std::string path2 = path + ".count";
  std::string path3 = path + ".buf";
  std::string path4 = path + ".bmp";
  std::string path5 = path;
  remove(path.c_str());
  remove(path2.c_str());
  INI::INIFile cache(path);
  INI::INIFile cachecount(path2);
  INI::INIStructure cachedata;
  INI::INIStructure cachedatacount;
  RenderD7::BitmapPrinter tmap(1024, 1024);
  std::vector<BMP> maps;
  int zrowv = 0;
  int zrowh = 0;

  cachedatacount["count"]["cnt"] = std::to_string(countall);
  for (unsigned i = 0; i < t.size(); i++) {
    RenderD7::Msg::DisplayWithProgress(
        "D7-Menu-Core", "Writing Cache: " + t[i]->name(), i, (int)t.size(),
        RenderD7::Color::Hex("#00DD11"));
    cachedata[std::to_string(t[i]->ID())]["name"] = t[i]->name();
    cachedata[std::to_string(t[i]->ID())]["author"] = t[i]->author();
    cachedata[std::to_string(t[i]->ID())]["id"] = std::to_string(t[i]->ID());
    BMP bitmap(48, 48, false);
    smdh_s *smdh = loadSMDH(t[i]->lowid(), t[i]->highid(), t[i]->mediatype());
    if (smdh == NULL) {
      return;
    }
    convertrgb565(&bitmap.data[0], smdh->bigIconData);

    //bitmap.write(std::string(path5 + t[i]->name() + "z.bmp").c_str());
    maps.push_back(bitmap);
    delete smdh;
  }
  for (size_t z = 0; z < maps.size(); z++) {
    if (zrowv * 48 + 48 >= 1024) {
      zrowh++;
      zrowv = 0;
    }
    tmap.DrawBitmap(zrowv * 48, zrowh * 48, maps[z]);
    zrowv++;
  }
  RenderD7::Msg::Display("D7-Menu-Core", "Writing Bitmap...", Top);
  tmap.SaveBmp(path4);
  cache.write(cachedata);
  cachecount.write(cachedatacount);
}

bool Cache::Read(std::vector<std::shared_ptr<Title>> t, const std::string &path,
                 bool nand) {
  std::string path2 = path + ".count";
  std::string path4 = path + ".bmp";
  int zz = 0;
  RenderD7::Msg::Display("D7-Menu-Core", "Look for exisring cache...", Top);
  if (!RenderD7::FS::FileExist(path)) {
    return false;
  }
  std::vector<std::string> secs;
  RenderD7::Msg::Display("D7-Menu-Core", "Loading Titles from cache...", Top);
  INI::INIFile cache(path);
  INI::INIFile cachecount(path2);
  INI::INIStructure cachedata;
  INI::INIStructure cachedatacount;
  cache.read(cachedata);
  cachecount.read(cachedatacount);

  BMP map_(path4.c_str());
  int zrowv = 0;
  int zrowh = 0;

  // Check for Changes
  amInit();
  Result res = 0;
  u32 count__ = 0;
  int titlecount__ = 0;
  res = AM_GetTitleCount(nand ? MEDIATYPE_NAND : MEDIATYPE_SD, &count__);
  if (R_FAILED(res)) {
    return false;
  }
  titlecount__ = (int)count__;

  if (std::atoi(cachedatacount["count"]["cnt"].c_str()) != titlecount__) {
    return false;
  }

  for (auto const &it : cachedata) {
    auto const &section = it.first;
    secs.push_back(section);
    RenderD7::Msg::DisplayWithProgress(
        "D7-Menu-Core", "Loading Data: " + section, zz, cachedata.size(),
        RenderD7::Color::Hex("#00DD11"));

    auto newData = std::make_shared<Title>();

    std::string title = cachedata[section]["name"];

    std::string __author__ = cachedata[section]["author"];
    uint64_t newID = 0;
    std::istringstream iss(cachedata[section]["id"]);
    iss >> newID;
    // RenderD7::Msg::DisplayWithProgress("D7-Menu-Core",  "Loading Titles from
    // cache: ", i, secs.size(), RenderD7::Color::Hex("#00DD11"));
    newData->LoadFromCache(newID, title, __author__,
                           nand ? MEDIATYPE_NAND : MEDIATYPE_SD);

    BMP temp_(48, 48);

    if (zrowv * 48 + 48 >= 1024) {
      zrowh++;
      zrowv = 0;
    }
    for (int i = 0; i < temp_.bmp_info_header.width+1; i++) {
      for (int j = 0; j < temp_.bmp_info_header.height+1; j++) {
        temp_.set_pixel(i, 48-j,
                        UNPACK_BGRA(map_.get_pixel((zrowv * 48)+i, 1024-j-(zrowh*48))));
      }
    }
    //temp_.write(std::string("sdmc:/D7-Menu/" + title + ".bmp").c_str());
    zrowv++;
    C2D_Image img;
    NImage_to_C3D(&img, temp_.DATA());
    newData->SetIcon(img);
    t.push_back(newData);
    zz++;
  }

  /*for(unsigned i = 0; i < secs.size(); i++)
  {
      auto newData = std::make_shared<Title>();



      std::string title = cachedata[secs[i]]["name"];

      std::string __author__ = cachedata[secs[i]]["author"];
      uint64_t newID = 0;
      std::istringstream iss(cachedata[secs[i]]["id"]);
      iss >> newID;
      RenderD7::Msg::DisplayWithProgress("D7-Menu-Core",  "Loading Titles from
  cache: ", i, secs.size(), RenderD7::Color::Hex("#00DD11"));
      newData->LoadFromCache(newID, title, __author__, nand ? MEDIATYPE_NAND :
  MEDIATYPE_SD); t.push_back(newData);
  }*/
  for (size_t f = 0; f < t.size(); f++) {
    TitleManager::sdtitles.push_back(t[f]);
  }
  return true;

  cachelog.Write("return");
}
