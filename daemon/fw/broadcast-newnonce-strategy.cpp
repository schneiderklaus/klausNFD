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
#include "broadcast-newnonce-strategy.hpp"
#include "strategy-helper.hpp"
#include <map>
#include "core/logger.hpp"

namespace nfd {
namespace fw {

NFD_LOG_INIT("BroadcastNewNonceStrategy");

const Name BroadcastNewNonceStrategy::STRATEGY_NAME(
    "ndn:/localhost/nfd/strategy/broadcast-newnonce/%FD%01");

BroadcastNewNonceStrategy::BroadcastNewNonceStrategy(Forwarder& forwarder, const Name& name) :
    Strategy(forwarder, name), ownStrategyChoice(forwarder.getStrategyChoice())
{
  NFD_LOG_INFO("BroadcastNewNonceStrategy initialized!");
}

BroadcastNewNonceStrategy::~BroadcastNewNonceStrategy()
{
}

void BroadcastNewNonceStrategy::afterReceiveInterest(const Face& inFace, const Interest& interest,
    shared_ptr<fib::Entry> fibEntry, shared_ptr<pit::Entry> pitEntry)
{

  bool useNonce = true;
  // Getting nonce value from parameters;
  std::string parameterString = ownStrategyChoice.findEffectiveParameters(
      interest.getName().toUri());
  std::map < std::string, std::string > parameterMap = StrategyHelper::getParameterMap(
      parameterString);

  std::map<std::string, std::string>::const_iterator it = parameterMap.find("nonce");
  if (it != parameterMap.end()) {
    if (it->second == "false") {
      useNonce = false;
    }
  }

  const fib::NextHopList& nexthops = fibEntry->getNextHops();

  for (fib::NextHopList::const_iterator it = nexthops.begin(); it != nexthops.end(); ++it) {
    shared_ptr < Face > outFace = it->getFace();
    if (pitEntry->canForwardTo(*outFace)) {
      NFD_LOG_TRACE("New Nonce? " << useNonce);
      // If nonce == true, change interest nonce for each new packet
      this->sendInterest(pitEntry, outFace, useNonce);
    }
  }

  if (!pitEntry->hasUnexpiredOutRecords()) {
    this->rejectPendingInterest(pitEntry);
  }
}

}  // namespace fw
}  // namespace nfd
