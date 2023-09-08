#include <3ds.h>

#include <card_slot.hpp>

std::string d7mc_icc_status = "NULL";
std::string d7mc_icc_type = "NULL";

namespace D7MC {
namespace CardSlot {
bool Check() {
  bool isinsert;
  FSUSER_CardSlotIsInserted(&isinsert);
  return isinsert;
}

void Update() {
  FS_CardType type;
  if (Check()) {
    d7mc_icc_status = "Inserted";
    FSUSER_GetCardType(&type);
    d7mc_icc_type = type ? "TWL" : "CTR";
  } else {
    d7mc_icc_status = "NotInserted";
    d7mc_icc_type = "None";
  }
}

std::string Satus() { return d7mc_icc_status; }

std::string Type() { return d7mc_icc_type; }
}  // namespace CardSlot
}  // namespace D7MC