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
#ifndef NFD_DAEMON_FW_LOSS_ESTIMATOR_TIME_WINDOW_HPP
#define NFD_DAEMON_FW_LOSS_ESTIMATOR_TIME_WINDOW_HPP

#include "common.hpp"
#include "loss-estimator.hpp"

namespace nfd {
namespace fw {

/**
 * \brief Implements the loss estimation with a sliding window over the last x time units.
 *
 * The loss percentage is calculated with all packets of status LOST or SATISFIED during the
 * sliding window.
 */
class LossEstimatorTimeWindow : public LossEstimator
{
public:

  /**
   * Constructs the Loss Estimator with the given sliding window time and interest lifetime.
   *
   *
   * \param interestLifetime The time after which unanswered interersts are considered lost
   * \param lossWindow the windows size for the loss calculation.
   * Must be larger than the interest lifetime.
   *
   * \throws runtime-exception if the interest lifetime is larger than the loss window!
   *
   */
  LossEstimatorTimeWindow(time::steady_clock::duration interestLifetime,
      time::steady_clock::duration lossWindow);

  /*
   * Adds an interest to the unknownMap.
   *
   * \throws runtime-exception if the name already exists!
   */
  void addSentInterest(const std::string& name)
  DECL_OVERRIDE;

  /*
   * Adds a satisfied interest packet.
   */
  void addSatisfiedInterest(const std::string& name)
  DECL_OVERRIDE;

  /*
   * Returns the loss percentage.
   *
   * \return 0 if the lossMap is empty or contains only FUTURESATISFIED packets.
   */
  double getLossPercentage()
  DECL_OVERRIDE;

private:

  /**
   * Packet types:
   * - SATISFIED: A data packet has returned for that interest.
   * - LOST: No data packet has returned and the interest lifetime is exceeded
   * - FUTURESATISFIED: No data packet has returned, but may return in the future(the interest lifetime is not exceeded yet)
   */
  enum class PacketType
  {
    SATISFIED, LOST, FUTURESATISFIED
  };

private:

  /**
   * The interest lifetime which acts as timeout before packets are either marked as LOST or SATISFIED
   */
  const time::steady_clock::duration m_interestLifetime;

  /**
   * The windowsize over which the final loss value is calculated
   */
  const time::steady_clock::duration m_windowSize;

  /**
   * The map for interests inside the interest lifetime.
   * Their status is undecided depending on wheter a data packet will return.
   */
  std::map<const std::string, const time::steady_clock::TimePoint> unknownMap;

  /**
   * The map for the final loss calculation.
   */
  std::map<const time::steady_clock::TimePoint, const PacketType> lossMap;

};

}  // namespace fw
}  // namespace nfd

#endif // NFD_DAEMON_FW_LOSS_ESTIMATOR_HPP

