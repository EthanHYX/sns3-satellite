/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Magister Solutions Ltd
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
 * Author: Jani Puttonen <jani.puttonen@magister.fi>
 */

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/mac48-address.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/uinteger.h"
#include "ns3/nstime.h"
#include "ns3/pointer.h"
#include "ns3/enum.h"
#include "ns3/boolean.h"

#include "satellite-mac-tag.h"
#include "satellite-net-device.h"
#include "satellite-signal-parameters.h"
#include "satellite-gw-mac.h"
#include "satellite-scheduling-object.h"


NS_LOG_COMPONENT_DEFINE ("SatGwMac");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (SatGwMac);

TypeId
SatGwMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SatGwMac")
    .SetParent<SatMac> ()
    .AddConstructor<SatGwMac> ()
    .AddAttribute ("Interval",
                   "The time to wait between packet (frame) transmissions",
                    TimeValue (Seconds (0.002)),
                    MakeTimeAccessor (&SatGwMac::m_tInterval),
                    MakeTimeChecker ())
    .AddAttribute ("DummyFrameSendingOn",
                   "Threshold time of total transmissions in BB Frame container to trigger a scheduling round.",
                    BooleanValue (false),
                    MakeBooleanAccessor (&SatGwMac::m_dummyFrameSendingOn),
                    MakeBooleanChecker ())
    .AddAttribute ("BBFrameUsageMode",
                   "Mode for selecting used BBFrames.",
                    EnumValue (SatGwMac::NORMAL_FRAMES),
                    MakeEnumAccessor (&SatGwMac::m_bbFrameUsageMode),
                    MakeEnumChecker (SatGwMac::SHORT_FRAMES, "Only short frames used.",
                                     SatGwMac::NORMAL_FRAMES, "Only normal frames used",
                                     SatGwMac::SHORT_AND_NORMAL_FRAMES, "Both short and normal frames used."))
    .AddAttribute ("SchedulingStartThresholdTime",
                   "Threshold time of total transmissions in BB Frame container to trigger a scheduling round.",
                    TimeValue (Seconds (0.005)),
                    MakeTimeAccessor (&SatGwMac::m_schedulingStartThresholdTime),
                    MakeTimeChecker ())
    .AddAttribute ("SchedulingStopThresholdTime",
                   "Threshold time of total transmissions in BB Frame container to stop a scheduling round.",
                    TimeValue (Seconds (0.015)),
                    MakeTimeAccessor (&SatGwMac::m_schedulingStopThresholdTime),
                    MakeTimeChecker ())
    .AddAttribute ("SchedulingSortCriteria",
                   "Sorting criteria fort scheduling objects from LLC.",
                    EnumValue (SatGwMac::NO_SORT),
                    MakeEnumAccessor (&SatGwMac::m_schedulingSortCriteria),
                    MakeEnumChecker (SatGwMac::NO_SORT, "No sorting",
                                     SatGwMac::BUFFERING_DELAY_SORT, "Sorting by delay in buffer",
                                     SatGwMac::BUFFERING_LOAD_SORT, "Sorting by load in buffer",
                                     SatGwMac::RANDOM_SORT, "Random sorting ",
                                     SatGwMac::PRIORITY_SORT, "Sorting by priority"))

  ;
  return tid;
}

SatGwMac::SatGwMac ()
{
  NS_LOG_FUNCTION (this);

  // Random variable used in scheduling
  m_random = CreateObject<UniformRandomVariable> ();
}

SatGwMac::~SatGwMac ()
{
  NS_LOG_FUNCTION (this);
}

void
SatGwMac::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_schedContextCallback.Nullify ();
  SatMac::DoDispose ();
}

void
SatGwMac::StartScheduling()
{
  NS_ASSERT (m_tInterval.GetDouble() > 0.0);

  // Note, carrierId currently set by default to 0
  ScheduleNextTransmissionTime (m_tInterval, 0);
}

void
SatGwMac::Receive (SatPhy::PacketContainer_t packets, Ptr<SatSignalParameters> /*rxParams*/)
{
  NS_LOG_FUNCTION (this);

  for (SatPhy::PacketContainer_t::iterator i = packets.begin(); i != packets.end(); i++ )
    {
      // Hit the trace hooks.  All of these hooks are in the same place in this
      // device because it is so simple, but this is not usually the case in
      // more complicated devices.
      m_snifferTrace (*i);
      m_promiscSnifferTrace (*i);
      m_macRxTrace (*i);

      // Remove packet tag
      SatMacTag macTag;
      bool mSuccess = (*i)->PeekPacketTag (macTag);
      if (!mSuccess)
        {
          NS_FATAL_ERROR ("MAC tag was not found from the packet!");
        }

      NS_LOG_LOGIC("Packet from " << macTag.GetSourceAddress() << " to " << macTag.GetDestAddress());
      NS_LOG_LOGIC("Receiver " << m_macAddress );

      // If the packet is intended for this receiver
      Mac48Address destAddress = Mac48Address::ConvertFrom (macTag.GetDestAddress());
      if (destAddress == m_macAddress || destAddress.IsBroadcast())
        {
          // Pass the source address to LLC
          m_rxCallback (*i, Mac48Address::ConvertFrom(macTag.GetSourceAddress ()));
        }
      else
        {
          NS_LOG_LOGIC("Packet intended for others received by MAC: " << m_macAddress );
        }
    }
}


void
SatGwMac::ScheduleNextTransmissionTime (Time txTime, uint32_t carrierId)
{
  Simulator::Schedule (txTime, &SatGwMac::TransmitTime, this, 0);
}

void
SatGwMac::TransmitTime (uint32_t carrierId)
{
  NS_LOG_FUNCTION (this);

  /**
   * TODO: This is a first skeleton implementation of the FWD link scheduler.
   * It is sending only one packet from each UT at a time, which a predefined
   * interval. In reality, the FWD link scheduler should be building BBFrames,
   * with possible GSE packets to several UTs.
   * In forward link txBytes should be either
   * - Short BBFrame = 16200 bits = 2025 Bytes
   * - Long BBFrame = 64800 bits = 8100 Bytes
   * The usage of either short or long BBFrame should be decided on-the-fly
   * based on buffered bytes in LLC layer.
   *
  */

  ScheduleBbFrames ();
  Ptr<SatBbFrame> bbFrame;

  if ( m_bbFrameContainer.empty() )
    {
      if ( m_dummyFrameSendingOn )
        {
          bbFrame = CreateDummyFrame ();
        }
    }
  else
    {
      bbFrame = m_bbFrameContainer.front();
      m_bbFrameContainer.pop_front();
    }

  if ( bbFrame )
    {
        /* TODO: The carrierId should be acquired from somewhere. Now
         * we assume only one carrier in forward link, so it is safe to use 0.
         * The BBFrame duration should be calculated based on BBFrame length and
         * used MODCOD.
         */

        //Time DURATION (Seconds (0.001));
        SendPacket (bbFrame->GetTransmitData() , carrierId, bbFrame->GetDuration ());
    }

  // TODO: The next TransmitTime should be scheduled to be when just transmitted
  // packet transmission ends. This is dependent on the used BBFrame length and
  // used MODCOD.
  ScheduleNextTransmissionTime (m_tInterval, 0);
}

void
SatGwMac::SetSchedContextCallback (SatGwMac::SchedContextCallback cb)
{
  NS_LOG_FUNCTION (this << &cb);
  m_schedContextCallback = cb;
}

void
SatGwMac::ScheduleBbFrames ()
{
  NS_LOG_FUNCTION (this);

  // TODO: logic of this function is needed to check
  // and implemented in own scheduler class
  uint32_t frameBytes = 0;
  Ptr<SatBbFrame> frame;

  // Get scheduling objects from LLC
  std::vector< Ptr<SatSchedulingObject> > sos = m_schedContextCallback ();

  // Schedule first all Control messages.
//  while ((sos.empty () == false) && sos[0]->IsControl() )
//    {
//      if ( frameBytes == 0)
//        {
//          frame = CreateFrame(3, SatBbFrame::SHORT_FRAME);
//          frameBytes = frame->GetBytesLeft();
//        }
//
//      Ptr<Packet> p = m_txOpportunityCallback (frameBytes, sos[0]->GetMacAddress ());
//
//      if ( p != NULL)
//        {
//          frame->AddTransmitData (p, true );
//          frameBytes -= 117;
//
//          if ( frameBytes == 0)
//            {
//              m_bbFrameContainer.push_back (frame);
//            }
//        }
//      else
//        {
//          m_bbFrameContainer.push_back (frame);
//          frameBytes = 0;
//        }
//
//      sos = m_schedContextCallback ();
//    }

  if ( sos.empty () == false )
    {
      uint32_t bytesToSent = 0;
      std::vector< Ptr<SatSchedulingObject> >::const_iterator currentObject;

      for ( currentObject = sos.begin (); currentObject != sos.end (); currentObject++ )
        {
          bytesToSent += (*currentObject)->GetBufferedBytes();
        }

      currentObject = sos.begin ();
      uint32_t currentObBytes = (*currentObject)->GetBufferedBytes();
      uint32_t currentObMinReqBytes = 5;

      if ( (*currentObject)->IsControl() )
        {
          currentObMinReqBytes = 500;
        }

      while ( bytesToSent )
        {
          if ( frameBytes == 0)
            {
              frame = CreateFrame (3, bytesToSent);
              frameBytes = frame->GetBytesLeft();
            }

          if ( frameBytes < currentObMinReqBytes )
            {
              m_bbFrameContainer.push_back (frame);
              frameBytes = 0;
            }
          else if ( currentObBytes > frameBytes )
            {
              uint32_t bytesLeft = 0;
              Ptr<Packet> p = m_txOpportunityCallback (frameBytes, (*currentObject)->GetMacAddress (), bytesLeft);

              if ( p )
                {
                  frameBytes = frame->AddTransmitData (p, (*currentObject)->IsControl() );
                }

              m_bbFrameContainer.push_back (frame);

              frameBytes = 0;
              bytesToSent -= currentObBytes;
              currentObBytes = bytesLeft;
            }
          else
            {
              // TODO: Something wrong here. We need to ask two extra bytes, otherwise fails.
              uint32_t bytesLeft = 0;
              Ptr<Packet> p = m_txOpportunityCallback (currentObBytes + 2, (*currentObject)->GetMacAddress (), bytesLeft);

              if ( p )
                {
                  frameBytes = frame->AddTransmitData (p, (*currentObject)->IsControl() );
                  bytesToSent -= currentObBytes;
                  currentObBytes = bytesLeft;
                }

              if ( bytesToSent )
                {
                  currentObject++;
                  currentObBytes = (*currentObject)->GetBufferedBytes();

                  currentObMinReqBytes = 5;

                  if ( (*currentObject)->IsControl() )
                    {
                      currentObMinReqBytes = 500;
                    }
                }
              else
                {
                  m_bbFrameContainer.push_back (frame);
                }
            }
        }
    }
//  else if (frameBytes > 0)
//    {
//      m_bbFrameContainer.push_back (frame);
//    }
}

Ptr<SatBbFrame> SatGwMac::CreateDummyFrame () const
{
  NS_LOG_FUNCTION (this);

  Ptr<SatBbFrame> dummyFrame = Create<SatBbFrame> ();
  Ptr<Packet> dummyPacket = Create<Packet> (SatBbFrame::m_shortBbFrameLengthInBytes);

  // Add MAC tag
  SatMacTag tag;
  tag.SetDestAddress (m_macAddress);
  tag.SetSourceAddress (m_macAddress);
  dummyPacket->AddPacketTag (tag);

  dummyFrame->AddTransmitData (dummyPacket, false);

  return dummyFrame;
}

Ptr<SatBbFrame>
SatGwMac::CreateFrame (uint32_t modCod, uint32_t byteCount) const
{
  Ptr<SatBbFrame> frame;

  switch (m_bbFrameUsageMode)
  {
    case SHORT_FRAMES:
      frame = Create<SatBbFrame> (modCod, SatBbFrame::SHORT_FRAME);
      break;

    case NORMAL_FRAMES:
      frame = Create<SatBbFrame> (modCod, SatBbFrame::NORMAL_FRAME);
      break;

    case SHORT_AND_NORMAL_FRAMES:
      if (byteCount > SatBbFrame::m_shortBbFrameLengthInBytes)
        {
          frame = Create<SatBbFrame> (modCod, SatBbFrame::NORMAL_FRAME);
        }
      else
        {
          frame = Create<SatBbFrame> (modCod, SatBbFrame::SHORT_FRAME);
        }
      break;

    default:
      NS_FATAL_ERROR ("Invalid BBFrame usage mode!!!");
      break;

  }

  return frame;
}


} // namespace ns3