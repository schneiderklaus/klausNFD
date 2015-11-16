/* -*- Mode:C++; c-file-style:"gnu";
 * indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Klaus Schneider, University of Bamberg, Germany
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Klaus Schneider <klaus.schneider@uni-bamberg.de>
 */
#ifndef NFD_DAEMON_FW_LOWEST_COST_STRATEGY_HPP
#define NFD_DAEMON_FW_LOWEST_COST_STRATEGY_HPP

#include "strategy.hpp"
#include "strategy-helper.hpp"
#include "../../common.hpp"
#include "../face/face.hpp"
#include "../table/fib-entry.hpp"
#include "../table/pit-entry.hpp"
#include "../table/strategy-choice.hpp"
#include "forwarder.hpp"
#include "strategy-requirements.hpp"
#include "interface-estimation.hpp"

namespace nfd {
namespace fw {

/** \brief Lowest Cost Strategy version 1
 *
 * Sends out interest packets on the face that satisfies all requirements.
 *
 * Current parameters:
 * \param maxloss double of loss percentage (between 0 and 1)
 * \param maxdelay double maximal round trip delay in milliseconds
 * \parm  minbw  minimal bandwidth in Kbps
 */
class LowestCostStrategy : public Strategy
{
public:

  LowestCostStrategy(Forwarder& forwarder, const Name& name = STRATEGY_NAME);

  virtual void
  afterReceiveInterest(const Face& inFace, const Interest& interest,
      shared_ptr<fib::Entry> fibEntry, shared_ptr<pit::Entry> pitEntry)
  DECL_OVERRIDE;

  virtual void
  beforeSatisfyInterest(shared_ptr<pit::Entry> pitEntry, const Face& inFace, const Data& data)
  DECL_OVERRIDE;

public:

  static const Name STRATEGY_NAME;

private:

  /**
   * Sends out probed interest packets with a new nonce.
   * Currently all paths except the working path are probed whenever probingDue() returns true.
   */
  void probeInterests(const shared_ptr<Face> outFace, const Interest& interest,
      const fib::NextHopList& nexthops, shared_ptr<pit::Entry> pitEntry);

  /**
   * Returns the face with the lowest cost that satisfies all requirements.
   * Runs getLowestTypeFace() with type = priorityType if no face satisfies all requirements.
   */
  shared_ptr<Face> getOutputFace(const fib::NextHopList& nexthops, shared_ptr<pit::Entry> pitEntry,
      StrategyRequirements &paramPtr, FaceId currentWorkingFace);

  /**
   * Returns the face with the lowest cost that satisfies exactly one requirement.
   *
   * Returns the face with the best value (e.g., lowest delay if type == DELAY) if no face satisfies the requirement.
   */
  shared_ptr<Face> getLowestTypeFace(const fib::NextHopList& nexthops,
      shared_ptr<pit::Entry> pitEntry, RequirementType type, StrategyRequirements& requirements,
      FaceId currentWorkingFace, bool isUpwardAttribute = false);

private:

  // 5% Hysteresis
  const double HYSTERESIS_PERCENTAGE = 0.05;

  StrategyHelper helper;
  std::unordered_map<FaceId, InterfaceEstimation> faceInfoTable;
  StrategyChoice& ownStrategyChoice;

  // The type to use when not all requirements can be met.
  // Defaults to "DELAY"
  RequirementType priorityType;

};

}  // namespace fw
}  // namespace nfd

#endif
