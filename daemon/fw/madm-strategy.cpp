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
#include "madm-strategy.hpp"
#include "core/logger.hpp"

namespace nfd {
namespace fw {

NFD_LOG_INIT("MADMStrategy");

const Name MadmStrategy::STRATEGY_NAME("ndn:/localhost/nfd/strategy/madm/%FD%01/");

MadmStrategy::MadmStrategy(Forwarder& forwarder, const Name& name) :
    Strategy(forwarder, name), ownStrategyChoice(forwarder.getStrategyChoice())
{
}

void MadmStrategy::afterReceiveInterest(const Face& inFace, const Interest& interest,
    shared_ptr<fib::Entry> fibEntry, shared_ptr<pit::Entry> pitEntry)
{

  if (!initialized) {
    // Set initial cost
    for (auto n : fibEntry->getNextHops()) {
      costMap[n.getFace()->getId()].setCost(n.getCost());
    }
    initialized = true;
  }

  Name currentPrefix;
  shared_ptr < MeasurementInfo > measurementInfo;
  std::tie(currentPrefix, measurementInfo) = StrategyHelper::findPrefixMeasurements(interest,
      this->getMeasurements());

  // Prefix info not found, create new prefix measurements
  if (measurementInfo == nullptr) {
    NFD_LOG_INFO("New prefix " << interest.getName() << " from " << inFace.getId());

    measurementInfo = StrategyHelper::addPrefixMeasurements(interest, this->getMeasurements());
    measurementInfo->req.parseParameters(
        ownStrategyChoice.findEffectiveParameters(interest.getName()));

    NFD_LOG_INFO(
        "Requirements: " << measurementInfo->req.getLimit(RequirementType::DELAY) << ", "
            << measurementInfo->req.getLimit(RequirementType::LOSS) << ", "
            << measurementInfo->req.getLimit(RequirementType::COST));
  }

  shared_ptr < Face > outFace = NULL;
  double maxtotalValue = -1;
  for (auto n : fibEntry->getNextHops()) {
    bool isWorkingFace = (n.getFace()->getId() == measurementInfo->currentWorkingFace);

    double totalValue = 0;
    for (auto currentType : measurementInfo->req.getOwnTypes()) {
      InterfaceEstimation &currentFaceInfo = faceInfoTable[n.getFace()->getId()];
      double currentReqValue;
      if (currentType == RequirementType::COST) {
        currentReqValue = costMap[n.getFace()->getId()].getCost();
      }
      else {
        currentReqValue = currentFaceInfo.getCurrentValue(currentType);
      }
      double lowerLimit = measurementInfo->req.getLimits(currentType).first;
      double upperLimit = measurementInfo->req.getLimits(currentType).second;

      double localValue;
      if (currentType == RequirementType::BANDWIDTH
          && n.getFace()->getId() != measurementInfo->currentWorkingFace) {
        localValue = 0.5;
      }
      else {
        localValue = calculateValue(currentReqValue, lowerLimit, upperLimit,
            StrategyRequirements::isUpwardAttribute(currentType));
      }

      if (localValue == 0) {
        totalValue = 0;
        break;
      }
      else {
        totalValue += localValue;
      }
    }

    if (isWorkingFace) {
      totalValue *= (1.0 + HYSTERESIS_PERCENTAGE);
    }

    if (totalValue >= maxtotalValue) {
      maxtotalValue = totalValue;
      outFace = n.getFace();
    }

  }
  NFD_LOG_TRACE("Face: " << outFace->getId() << " value: " << maxtotalValue);

  if (helper.probingDue()) {
    probeInterests(outFace, interest, measurementInfo->req, fibEntry->getNextHops(), pitEntry);
  }

  faceInfoTable[outFace->getId()].addSentInterest(interest.getName().toUri());

  if (outFace->getId() != measurementInfo->currentWorkingFace) {
    NFD_LOG_TRACE(
        "New current working face from " << measurementInfo->currentWorkingFace << " to "
            << outFace->getId());
    measurementInfo->currentWorkingFace = outFace->getId();
  }
  this->sendInterest(pitEntry, outFace);

}

double MadmStrategy::calculateValue(double currentValue, double lowerLimit, double upperLimit,
    bool upwardAttribute)
{
  double value;
  if (currentValue <= lowerLimit) {
    value = 1;
  }
  else if (currentValue >= upperLimit) {
    value = 0;
  }
  else {
    value = 1 - (currentValue - lowerLimit) / (upperLimit - lowerLimit);
  }
  if (upwardAttribute) {
    return (1 - value);
  }
  else {
    return value;
  }
}

void MadmStrategy::probeInterests(const shared_ptr<Face> outFace, const Interest& interest,
    StrategyRequirements &requirements, const fib::NextHopList& nexthops,
    shared_ptr<pit::Entry> pitEntry)
{

  for (auto n : nexthops) {
    const shared_ptr<Face>& thisFace = n.getFace();

    bool costTooHigh = false;
    if (requirements.getLimits(RequirementType::COST).second != -1) {
      costTooHigh = costMap[thisFace->getId()].getCost()
          > (requirements.getLimits(RequirementType::COST)).second;
      if (costTooHigh) {
        NFD_LOG_DEBUG(
            "Cost too high: " << costMap[thisFace->getId()].getCost() << " > "
                << (requirements.getLimits(RequirementType::COST)).second);
      }
    }
    if (thisFace != outFace && !costTooHigh) {
      NFD_LOG_TRACE("Probing face: " << thisFace->getId());
      faceInfoTable[thisFace->getId()].addSentInterest(interest.getName().toUri());
      this->sendInterest(pitEntry, thisFace, true);
    }
  }
}

void MadmStrategy::beforeSatisfyInterest(shared_ptr<pit::Entry> pitEntry, const Face& inFace,
    const Data& data)
{
  InterfaceEstimation& prefixInfo = faceInfoTable[inFace.getId()];

  prefixInfo.addSatisfiedInterest(data.getContent().value_size(), data.getName().toUri());
  costMap[inFace.getId()].addToTraffic(data.getContent().size());

  // Interest is already satisfied by another upstream
  pit::OutRecordCollection::const_iterator outRecord = pitEntry->getOutRecord(inFace);
  if (pitEntry->getInRecords().empty() || outRecord == pitEntry->getOutRecords().end()) {
    // Do nothing else
  }
  // There is an in and outrecord: Update RTT value!
  else {
    time::steady_clock::Duration rtt = time::steady_clock::now() - outRecord->getLastRenewed();
    prefixInfo.addRttMeasurement(time::duration_cast < time::microseconds > (rtt));
  }
}

}  // Namespace fw
}  // Namespace nfd
