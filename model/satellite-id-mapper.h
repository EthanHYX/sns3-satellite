/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Magister Solutions Ltd.
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
 * Author: Frans Laakso <frans.laakso@magister.fi>
 */
#ifndef SATELLITE_ID_MAPPER_H
#define SATELLITE_ID_MAPPER_H

#include "satellite-enums.h"
#include "ns3/mac48-address.h"

namespace ns3 {

/**
 * \ingroup satellite
 *
 * \brief Class for ID -mapper
 */
class SatIdMapper : public Object
{
public:

  /**
   * \brief Constructor
   */
  SatIdMapper ();

  /**
   * \brief Destructor
   */
  ~SatIdMapper ();

  /**
   * \brief NS-3 type id function
   * \return type id
   */
  static TypeId GetTypeId (void);

  /**
   *  \brief Do needed dispose actions.
   */
  void DoDispose ();

  /** ATTACH TO MAPS */

  /**
   * \brief Attach MAC address to the Trace ID maps and give it a running trace ID
   * \param mac MAC address
   */
  void AttachMacToTraceId (Address mac);

  /**
   * \brief Attach MAC address to the UT ID maps and give it a running UT ID
   * \param mac MAC address
   */
  void AttachMacToUtId (Address mac);

  /**
   * \brief Attach MAC address to the beam ID maps
   * \param mac MAC address
   * \param beamID beam ID
   */
  void AttachMacToBeamId (Address mac, uint32_t beamId);

  /**
   * \brief Attach MAC address to the GW ID maps
   * \param mac MAC address
   * \param gwID GW ID
   */
  void AttachMacToGwId (Address mac, uint32_t gwId);

  /** ID GETTERS */

  /**
   * \brief Function for getting the trace ID with MAC
   * \param mac MAC address
   * \return Trace ID
   */
  uint32_t GetTraceIdWithMac (Address mac);

  /**
   * \brief Function for getting the UT ID with MAC
   * \param mac MAC address
   * \return UT ID
   */
  uint32_t GetUtIdWithMac (Address mac);

  /**
   * \brief Function for getting the beam ID with MAC
   * \param mac MAC address
   * \return beam ID
   */
  uint32_t GetBeamIdWithMac (Address mac);

  /**
   * \brief Function for getting the GW ID with MAC
   * \param mac MAC address
   * \return GW ID
   */
  uint32_t GetGwIdWithMac (Address mac);

  /** MAC GETTERS */

  /**
   * \brief Function for getting the MAC with trace ID
   * \param traceId Trace ID
   * \return MAC address
   */
  Address GetMacWithTraceId (uint32_t traceId);

  /**
   * \brief Function for getting the MAC with UT ID
   * \param utId UT ID
   * \return MAC address
   */
  Address GetMacWithUtId (uint32_t utId);

  /**
   * \brief Function for getting the MAC with beam ID
   * \param beamId beam ID
   * \return MAC address
   */
  Address GetMacWithBeamId (uint32_t beamId);

  /**
   * \brief Function for getting the MAC with GW ID
   * \param gwId GW ID
   * \return MAC address
   */
  Address GetMacWithGwId (uint32_t gwId);

  /**
   * \brief Function for printing out the maps
   */
  void PrintMaps ();

  /**
   * \brief Function for getting the IDs related to a MAC address in an info string
   * \param mac MAC address
   * \return info string
   */
  std::string GetMacInfo (Address mac);

private:

  /**
   * \brief Function for resetting the variables
   */
  void Reset ();

  /**
   * \brief Running trace index number
   */
  uint32_t m_traceIdIndex;

  /**
   * \brief Running UT index number
   */
  uint32_t m_utIdIndex;

  /**
   * \brief Map for MAC to trace ID conversion
   */
  std::map <Address, uint32_t> m_macToTraceIdMap;

  /**
   * \brief Map for trace ID to MAC conversion
   */
  std::map <uint32_t, Address> m_traceIdToMacMap;

  /**
   * \brief Map for MAC to UT ID conversion
   */
  std::map <Address, uint32_t> m_macToUtIdMap;

  /**
   * \brief Map for MAC to beam ID conversion
   */
  std::map <Address, uint32_t> m_macToBeamIdMap;

  /**
   * \brief Map for MAC to GW ID conversion
   */
  std::map <Address, uint32_t> m_macToGwIdMap;
};

} // namespace ns3

#endif /* SATELLITE_ID_MAPPER_H */