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
#ifndef NFD_DAEMON_FW_OWNSTRATEGY_HPP
#define NFD_DAEMON_FW_OWNSTRATEGY_HPP

#include "strategy.hpp"

namespace nfd {
namespace fw {

/**
 *  \brief Forwards interests to all nexthops using a new nonce.
 *
 *  \param nonce denotes if a new nonce should be used (nonce=true) or not (nonce=false).
 *  Defaults to nonce=true.
 *   */
class BroadcastNewNonceStrategy : public Strategy
{
public:
  BroadcastNewNonceStrategy(Forwarder& forwarder, const Name& name = STRATEGY_NAME);

  virtual
  ~BroadcastNewNonceStrategy();

  virtual void
  afterReceiveInterest(const Face& inFace, const Interest& interest,
      shared_ptr<fib::Entry> fibEntry, shared_ptr<pit::Entry> pitEntry)
  DECL_OVERRIDE;

public:
  static const Name STRATEGY_NAME;

private:
  StrategyChoice& ownStrategyChoice;
};

}  // namespace fw
}  // namespace nfd

#endif
