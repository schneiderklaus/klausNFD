/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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
#ifndef NFD_DAEMON_FW_MADM_STRATEGY_HPP
#define NFD_DAEMON_FW_MADM_STRATEGY_HPP

#include "strategy.hpp"
#include "strategy-helper.hpp"
#include "cost-estimator.hpp"
#include "forwarder.hpp"

namespace nfd {
namespace fw {

/** \brief A strategy based on Multiple Attribute Decision Making (MADM).
 *
 * Calculates a simple additive weight over all normalized requirement values.
 *
 * Current requirements (upper and lower limit):
 * \param minbw=[vl-vu] minimal acceptable bandwidth
 * \param maxcost=[vl-vu] maximal acceptable cost. Should be in the range of [0-1000]
 * \param maxdelay=[vl-vu] maximal acceptable delay in milliseconds
 * \param maxloss=[vl-vu] maximal acceptable packet loss percentage in the range of [0-1]
 *
 */
class MadmStrategy : public Strategy
{
public:
  MadmStrategy(Forwarder& forwarder, const Name& name = STRATEGY_NAME);

  virtual void
  afterReceiveInterest(const Face& inFace, const Interest& interest,
      shared_ptr<fib::Entry> fibEntry, shared_ptr<pit::Entry> pitEntry)
  DECL_OVERRIDE;

  virtual void beforeSatisfyInterest(shared_ptr<pit::Entry> pitEntry, const Face& inFace,
      const Data& data)
  DECL_OVERRIDE;

public:

  static const Name STRATEGY_NAME;

private:

  double calculateValue(double currentValue, double minLimit, double maxLimit,
      bool upwardAttribute = false);

  void probeInterests(const shared_ptr<Face> outFace, const Interest& interest,
      StrategyRequirements &requirements, const fib::NextHopList& nexthops,
      shared_ptr<pit::Entry> pitEntry);

  shared_ptr<Face> getOutputFace(const fib::NextHopList& nexthops, shared_ptr<pit::Entry> pitEntry);

private:

  // 5% Hysteresis
  const double HYSTERESIS_PERCENTAGE = 0.05;

  StrategyHelper helper = StrategyHelper();
  StrategyChoice& ownStrategyChoice;
  std::unordered_map<FaceId, InterfaceEstimation> faceInfoTable;
  std::unordered_map<FaceId, CostEstimator> costMap;

  bool initialized = false;

};

}  // namespace fw
}  // namespace nfd

#endif
