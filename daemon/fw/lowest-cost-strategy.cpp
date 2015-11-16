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
#include "lowest-cost-strategy.hpp"
#include "core/logger.hpp"
#include "measurement-info.hpp"

namespace nfd {
namespace fw {

NFD_LOG_INIT("LowestCostStrategy")

const Name LowestCostStrategy::STRATEGY_NAME("ndn:/localhost/nfd/strategy/lowest-cost/%FD%01/");

LowestCostStrategy::LowestCostStrategy(Forwarder& forwarder, const Name& name) :
    Strategy(forwarder, name), ownStrategyChoice(forwarder.getStrategyChoice()),
        priorityType(RequirementType::DELAY)
{
}

void LowestCostStrategy::afterReceiveInterest(const Face& inFace, const Interest& interest,
    shared_ptr<fib::Entry> fibEntry, shared_ptr<pit::Entry> pitEntry)
{
  Name currentPrefix;
  shared_ptr < MeasurementInfo > measurementInfo;
  nfd::MeasurementsAccessor& ma = this->getMeasurements();
  std::tie(currentPrefix, measurementInfo) = StrategyHelper::findPrefixMeasurements(interest, ma);

  // Prefix info not found
  if (measurementInfo == nullptr) {
    // Create new prefix
    nfd::MeasurementsAccessor & ma = this->getMeasurements();
    measurementInfo = StrategyHelper::addPrefixMeasurements(interest, ma);
    NFD_LOG_WARN("New prefix " << interest.getName() << " from " << inFace.getId());
    measurementInfo->req.parseParameters(
        ownStrategyChoice.findEffectiveParameters(interest.getName()));
  }

  if (pitEntry->hasUnexpiredOutRecords()) {
    // Retransmitted interest from consumer application. Don't forward.
    return;
  }

  shared_ptr < Face > outFace = getOutputFace(fibEntry->getNextHops(), pitEntry,
      measurementInfo->req, measurementInfo->currentWorkingFace);

  if (outFace == NULL) {
    NFD_LOG_WARN("No face available!\n");
    return;
  }
  else {
    // Probe Interests
    if (helper.probingDue()) {
      probeInterests(outFace, interest, fibEntry->getNextHops(), pitEntry);
    }

    if (outFace->getId() != measurementInfo->currentWorkingFace) {
      NFD_LOG_TRACE(
          "New current working face from " << measurementInfo->currentWorkingFace << " to "
              << outFace->getId());
      measurementInfo->currentWorkingFace = outFace->getId();
    }

    InterfaceEstimation& faceInfo = faceInfoTable[outFace->getId()];
    NFD_LOG_TRACE(
        "Face: " << outFace->getId() << " - bw: "
            << faceInfo.getCurrentValue(RequirementType::BANDWIDTH) << ", delay: "
            << faceInfo.getCurrentValue(RequirementType::DELAY) << "ms, loss: "
            << faceInfo.getCurrentValue(RequirementType::LOSS));

    faceInfoTable[outFace->getId()].addSentInterest(interest.getName().toUri());
    this->sendInterest(pitEntry, outFace);
  }
}

shared_ptr<Face> LowestCostStrategy::getOutputFace(const fib::NextHopList& nexthops,
    shared_ptr<pit::Entry> pitEntry, StrategyRequirements &requirements, FaceId currentWorkingFace)
{
  shared_ptr < Face > outFace = NULL;

  if (requirements.contains(RequirementType::DELAY)
      && requirements.contains(RequirementType::LOSS)) {
    for (auto n : nexthops) {
      bool isWorkingFace = (n.getFace()->getId() == currentWorkingFace);
      double currentDelay = faceInfoTable[n.getFace()->getId()].getCurrentValue(
          RequirementType::DELAY);
      double currentLoss = faceInfoTable[n.getFace()->getId()].getCurrentValue(
          RequirementType::LOSS);

      if (pitEntry->canForwardTo(*n.getFace())) {
        double delayLimit = requirements.getLimit(RequirementType::DELAY);
        double lossLimit = requirements.getLimit(RequirementType::LOSS);
        if (!isWorkingFace) {
          delayLimit /= (1.0 + HYSTERESIS_PERCENTAGE);
          lossLimit /= (1.0 + HYSTERESIS_PERCENTAGE);
        }
        if (currentDelay < delayLimit && currentLoss < lossLimit) {
          outFace = n.getFace();
          break;
        }
      }
    }
    if (outFace == NULL) {
      // Not all requirements could be met. use priority type.
      outFace = getLowestTypeFace(nexthops, pitEntry, priorityType, requirements,
          currentWorkingFace);
    }
  }
  else if (requirements.contains(RequirementType::DELAY)) {
    outFace = getLowestTypeFace(nexthops, pitEntry, RequirementType::DELAY, requirements,
        currentWorkingFace);
  }
  else if (requirements.contains(RequirementType::LOSS)) {
    outFace = getLowestTypeFace(nexthops, pitEntry, RequirementType::LOSS, requirements,
        currentWorkingFace);
  }
  else if (requirements.contains(RequirementType::BANDWIDTH)) {
    outFace = getLowestTypeFace(nexthops, pitEntry, RequirementType::BANDWIDTH, requirements, true);
  }
  else {
    // No parameter set. Getting lowest cost face.
    outFace = getLowestTypeFace(nexthops, pitEntry, RequirementType::COST, requirements,
        currentWorkingFace);
  }

  return outFace;
}

shared_ptr<Face> LowestCostStrategy::getLowestTypeFace(const fib::NextHopList& nexthops,
    shared_ptr<pit::Entry> pitEntry, RequirementType type, StrategyRequirements &requirements,
    FaceId currentWorkingFace, bool isUpwardAttribute)
{
  shared_ptr < Face > outFace = NULL;

  // Returning lowest cost face.
  if (type == RequirementType::COST) {
    return nexthops.front().getFace();
  }

  for (auto n : nexthops) {
    bool isWorkingFace = (n.getFace()->getId() == currentWorkingFace);
    double currentLimit;
    currentLimit = requirements.getLimit(type);
    if (!isWorkingFace) {
      if (StrategyRequirements::isUpwardAttribute(type)) {
        currentLimit *= (1.0 + HYSTERESIS_PERCENTAGE);
      }
      else {
        currentLimit /= (1.0 + HYSTERESIS_PERCENTAGE);
      }
    }
    double currentValue = faceInfoTable[n.getFace()->getId()].getCurrentValue(type);
    if (pitEntry->canForwardTo(*n.getFace())) {
      if (!isUpwardAttribute && currentValue < currentLimit) {
        outFace = n.getFace();
        break;
      }
      if (isUpwardAttribute && currentValue > currentLimit) {
        outFace = n.getFace();
        break;
      }
    }
  }

  // If no face meets the requirement: Send out on best face.
  if (outFace == NULL) {
    double lowestValue = std::numeric_limits<double>::infinity();
    double highestValue = -1;
    for (auto n : nexthops) {
      double currentValue = faceInfoTable[n.getFace()->getId()].getCurrentValue(type);
      if (!isUpwardAttribute && pitEntry->canForwardTo(*n.getFace())
          && currentValue < lowestValue) {
        lowestValue = currentValue;
        outFace = n.getFace();
      }
      if (isUpwardAttribute && pitEntry->canForwardTo(*n.getFace())
          && currentValue > highestValue) {
        NFD_LOG_TRACE(
            "Highest value: " << currentValue << ", " << highestValue << ", face: "
                << n.getFace()->getId());
        highestValue = currentValue;
        outFace = n.getFace();
      }

    }
  }
  return outFace;
}

void LowestCostStrategy::probeInterests(const shared_ptr<Face> outFace, const Interest& interest,
    const fib::NextHopList& nexthops, shared_ptr<pit::Entry> pitEntry)
{
  for (auto n : nexthops) {
    if (n.getFace() != outFace) {
      faceInfoTable[n.getFace()->getId()].addSentInterest(interest.getName().toUri());
      this->sendInterest(pitEntry, n.getFace(), true);
    }
  }
}

void LowestCostStrategy::beforeSatisfyInterest(shared_ptr<pit::Entry> pitEntry, const Face& inFace,
    const Data& data)
{

  // Update loss info!
  InterfaceEstimation& faceInfo = faceInfoTable[inFace.getId()];
  faceInfo.addSatisfiedInterest(data.getContent().value_size(), data.getName().toUri());

  pit::OutRecordCollection::const_iterator outRecord = pitEntry->getOutRecord(inFace);

  // already satisfied by another upstream
  if (pitEntry->getInRecords().empty() || outRecord == pitEntry->getOutRecords().end()) {
    // Do nothing else
  }

  // There is an in and outrecord: Update RTT value!
  else {
    time::steady_clock::Duration rtt = time::steady_clock::now() - outRecord->getLastRenewed();
    faceInfo.addRttMeasurement(time::duration_cast < time::microseconds > (rtt));
  }
}

}  // namespace fw
}  // namespace nfd

