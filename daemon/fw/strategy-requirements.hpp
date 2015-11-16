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
#ifndef NFD_DAEMON_FW_STRATEGY_REQUIREMENTS_HPP
#define NFD_DAEMON_FW_STRATEGY_REQUIREMENTS_HPP

#include <cstdbool>
#include <map>
#include <utility>
#include <set>
#include <string>

namespace nfd {
namespace fw {

enum class RequirementType
{
  BANDWIDTH, COST, DELAY, LOSS
};

/**
 * A class that represents the requirements (parameters) that a strategy currently holds.
 */
class StrategyRequirements
{
public:

  /**
   * Constructor.
   *
   * \param supportedRequirements A set that contains all the requirement types
   * that the strategy supports.
   *
   */
  StrategyRequirements(std::set<RequirementType> supportedRequirements = {
      RequirementType::BANDWIDTH, RequirementType::COST, RequirementType::DELAY,
      RequirementType::LOSS });

  /**
   * Returns if the given requirement is an upward or downward attribute.
   *
   * Upward attributes are BANDWIDTH (a higher value is preferable)
   *
   * Downward attributes: LOSS, DELAY, COST (a lower value is preferable)
   */
  static bool isUpwardAttribute(RequirementType type);

  /**
   * Takes a string of parameters and adds the corresponding requirement attributes and values.
   *
   * \param parameterString has the syntax "p1=v1,...pn=vn" or "p1=vl1-vl2,...".
   * Valid names for pi are "maxloss", "mindelay", "minbw" and "mincost".
   *
   * \returns true if at least one parameter was valid (supported and contained in parameterString).
   * Returns false otherwise.
   */
  bool parseParameters(std::string parameterString);

  /**
   * Returns the limits for one specific requirement type.
   *
   * \returns std::pair<lower limit, upper limit>
   * \returns the same value for both if there is only one limit.
   * \returns std::pair<-1,-1> if the type is not instantiated.
   */
  std::pair<double, double> getLimits(RequirementType type);

  /**
   * Returns the limit for one specific requirement type.
   *
   * Returns The lower limit if they differ and logs a warning message.
   *
   */
  double getLimit(RequirementType type);

  /**
   * Returns true if the requirement type is supported and has an assigned value.
   */
  bool contains(RequirementType type);

  /**
   * Returns a set with all supported and instantiated requirement types.
   */
  std::set<RequirementType> getOwnTypes() const;

private:

  std::map<RequirementType, std::pair<double, double>> requirementMap;
  std::set<RequirementType> supportedRequirements;
  std::set<RequirementType> initializedRequirements;

};

}  // namespace fw
}  // namespace nfd

#endif
