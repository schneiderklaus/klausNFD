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
#include "cost-estimator.hpp"
#include "core/logger.hpp"

namespace nfd {
namespace fw {

NFD_LOG_INIT("CostEstimator");

CostEstimator::CostEstimator(double initialCost, double maxCost) :
    cost(initialCost), trafficLimitInMB(-1), consumedTrafficInMB(0), isLimited(false)
{
}

void CostEstimator::setCost(double cost)
{
  this->cost = cost;
}

//
double CostEstimator::getCost() const
{
  return cost;
}

void CostEstimator::addToTraffic(double sizeInByte)
{
  consumedTrafficInMB += sizeInByte / (1024.0 * 1024.0);
  adjustCost();
}

void CostEstimator::setTrafficLimit(double trafficLimitInMB)
{
  this->trafficLimitInMB = trafficLimitInMB;
  this->isLimited = true;
  adjustCost();
}

void CostEstimator::adjustCost()
{

  if (isLimited) {
    double percOfLimit = consumedTrafficInMB / trafficLimitInMB;
    double newCost;
    NFD_LOG_DEBUG("Running adjustcost perc: " << percOfLimit << "\n");

    if (percOfLimit > 1) {
      newCost = MAX_COST + 1;
    }
    else {
      newCost = percOfLimit * 1000;
    }

    // Setting new cost if higher
    if (newCost > cost) {
      setCost(newCost);
    }
  }
  else {
    NFD_LOG_INFO("Face is not limited in traffic. Not adjusting cost");
  }
}

}  //namespace fw
}  //namespace nfd
