#include "cache.hpp"
#include "renderd7.hpp"
#include "log.hpp"
#include "ini.hpp"
#include <iostream>
#include <sstream>
#include <cstdint>
#include "utils.hpp"
#include "TitleManager.hpp"

Log cachelog;

static const size_t ENTRYSIZE = 5341;

static C2D_Image loadIconTex(u16* icndata){
    C3D_Tex* tex                          = new C3D_Tex;
	static const Tex3DS_SubTexture subt3x = {48, 48, 0.0f, 48 / 64.0f, 48 / 64.0f, 0.0f};
	C3D_TexInit(tex, 64, 64, GPU_RGB565);
	
	u16* dest = (u16*)tex->data + (64 - 48) * 64;
	u16* src  = (u16*)icndata;
	for (int j = 0; j < 48; j += 8)
	{
		std::copy(src, src + 48 * 8, dest);
		src += 48 * 8;
		dest += 64 * 8;
	}
	
	return C2D_Image{tex, &subt3x};
}

void Cache::Create(std::vector<std::shared_ptr<Title>> t, const std::string& path)
{
    remove(path.c_str());
    INI::INIFile cache(path);
    INI::INIStructure cachedata;
    for(unsigned i = 0; i < t.size(); i++)
    {
        RenderD7::Msg::DisplayWithProgress("D7-Menu-Core", "Writing Cache: " + t[i]->name(), i, (int)t.size(), RenderD7::Color::Hex("#00DD11"));
        cachedata[t[i]->name()]["name"] = t[i]->name();
        cachedata[t[i]->name()]["author"] = t[i]->author();
        cachedata[t[i]->name()]["prod"] = t[i]->prodcode();
        cachedata[t[i]->name()]["id"] = std::to_string(t[i]->ID());
        
    }
    cache.write(cachedata);
}

bool Cache::Read(std::vector<std::shared_ptr<Title>> t, const std::string& path, bool nand)
{
    int zz = 0;
    std::fstream binary_file("sdmc:/data", std::ios::in|std::ios::binary);
    RenderD7::Msg::Display("D7-Menu-Core", "Look for exisring cache...", Top);
    if (!RenderD7::FS::FileExist(path))
    {
        return false;
    }
    std::vector<std::string> secs;
    RenderD7::Msg::Display("D7-Menu-Core",  "Loading Titles from cache...", Top);
    INI::INIFile cache(path);
    INI::INIStructure cachedata;
    cache.read(cachedata);
    for (auto const& it : cachedata)
    {
	    auto const& section = it.first;
        secs.push_back(section);
        RenderD7::Msg::DisplayWithProgress("D7-Menu-Core",  "Loading Data: " + section, zz, cachedata.size(), RenderD7::Color::Hex("#00DD11"));
        zz++;
    }
    //Check for Changes
    amInit();
    Result res = 0;
    u32 count__ = 0;
    int titlecount__ = 0;
    res = AM_GetTitleCount(nand ? MEDIATYPE_NAND : MEDIATYPE_SD, &count__);
	if (R_FAILED(res))
	{
		return false;
	}
	titlecount__ = (int)count__;

    if ((int)secs.size() != titlecount__)
    {
        return false;
    }
    
    for(unsigned i = 0; i < secs.size(); i++)
    {
        auto newData = std::make_shared<Title>();

        
        
        std::string title = cachedata[secs[i]]["name"];

        std::string prodCode = cachedata[secs[i]]["prod"];
        
        std::string __author__ = cachedata[secs[i]]["author"];
        uint64_t newID = 0;
        std::istringstream iss(cachedata[secs[i]]["id"]);
        iss >> newID;
        RenderD7::Msg::DisplayWithProgress("D7-Menu-Core",  "Loading Titles from cache: ", i, secs.size(), RenderD7::Color::Hex("#00DD11"));
        newData->LoadFromCache(newID, title, __author__, prodCode, nand ? MEDIATYPE_NAND : MEDIATYPE_SD);
        t.push_back(newData);
    }
    for (size_t f = 0; f < t.size(); f++)
    {
        TitleManager::sdtitles.push_back(t[f]);
    }
    return true;
    
    
    cachelog.Write("return");
}
