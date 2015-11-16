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
#ifndef NFD_DAEMON_FW_COST_ESTIMATOR_HPP
#define NFD_DAEMON_FW_COST_ESTIMATOR_HPP

#include "common.hpp"

namespace nfd {
namespace fw {

/**
 * An estimator for interface costs.
 */
class CostEstimator
{
public:

  const static int DEFAULT_COST = 100;
  const static int MAX_COST = 1000;

  CostEstimator(double initialCost = DEFAULT_COST, double maxCost = MAX_COST);

  void
  setCost(double cost);

  /**
   * Adds one packet to the total consumed traffic and adjusts the interface cost.
   */
  void addToTraffic(double sizeInByte);

  /**
   * Sets a traffic limit for this interface.
   */
  void setTrafficLimit(double trafficLimitInMB);

  double
  getCost() const;

private:

  /**
   * Adjusts the current cost upwards according to the current traffic.
   * The new cost will be MAX_COST * consumedTraffic/trafficLimit.
   * If consumedTraffic > trafficlimit the new cost will be MAX_COST + 1.
   *
   */
  void adjustCost();

  double cost;
  double trafficLimitInMB;
  double consumedTrafficInMB;
  bool isLimited;

};

}  // namespace fw
}  // namespace nfd

#endif // NFD_DAEMON_FW_COST_ESTIMATOR_HPP
