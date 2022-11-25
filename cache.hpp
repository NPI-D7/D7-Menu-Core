#include "TitleManager.hpp"
#include <memory>

unsigned NImage_to_C3D(C2D_Image *img, const std::vector<unsigned char> &bmpc);

namespace Cache {
void Create(std::vector<std::shared_ptr<Title>> t, const std::string &path,
            int countall);
bool Read(std::vector<std::shared_ptr<Title>> t, const std::string &path,
          bool nand);
} // namespace Cache