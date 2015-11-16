#ifndef NFD_DAEMON_FW_PER_PREFIX_INFO_HPP
#define NFD_DAEMON_FW_PER_PREFIX_INFO_HPP

#include "strategy-info.hpp"
#include "strategy-requirements.hpp"
#include <unordered_map>
#include "../face/face.hpp"
#include "interface-estimation.hpp"

namespace nfd {
namespace fw {

/**
 * Measurement information that can be saved and retrieved per-name-prefix.
 */
class MeasurementInfo : public StrategyInfo
{
public:
  static constexpr int getTypeId()
  {
    return 1012;
  }

  MeasurementInfo() :
      currentWorkingFace(-1)
  {
  }

public:
  std::unordered_map<FaceId, InterfaceEstimation> faceInfoMap;
  StrategyRequirements req;
  int currentWorkingFace;
};

}  //fw
}  //nfd

#endif
