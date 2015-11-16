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
#ifndef NFD_DAEMON_FW_INTERFACE_ESTIMATION_HPP
#define NFD_DAEMON_FW_INTERFACE_ESTIMATION_HPP

#include "bandwidth-estimator.hpp"
#include <boost/chrono/duration.hpp>
#include <ndn-cxx/util/time.hpp>
#include <string>
#include "loss-estimator-time-window.hpp"
#include "rtt-estimator2.hpp"
#include "strategy-requirements.hpp"
#include <cstddef>
#include "../../common.hpp"

namespace nfd {
namespace fw {

/**
 * A class that combines all interface estimators (loss, delay & bandwidth) for easier handling.
 */
class InterfaceEstimation
{
public:

  // Some constants for the interface estimators
  const static int DEFAULT_INTEREST_LIFETIME = 2000;
  const static int CALCULATION_WINDOW_IN_MS = 5000;

  InterfaceEstimation(
      time::milliseconds interestLifetime = time::milliseconds(DEFAULT_INTEREST_LIFETIME),
      time::milliseconds calculationWindow = time::milliseconds(CALCULATION_WINDOW_IN_MS));

public:

  /**
   * Adds a satisfied interest to the loss estimator
   */
  void addSentInterest(std::string name);

  /**
   * Adds a satisfied interest to both loss and bandwidth estimators
   */
  void addSatisfiedInterest(size_t sizeInByte, std::string name);

  /**
   * Adds an rtt measurement to the delay estimator
   */
  void addRttMeasurement(time::microseconds durationMicroSeconds);

  /**
   * Returns the current value for the type.
   * Returns -1 if the type is invalid.
   */
  double getCurrentValue(RequirementType type);

private:

  RttEstimator2 rtt;
  LossEstimatorTimeWindow loss;
  BandwidthEstimator bw;

};

}
}

#endif
