/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 * Author: Vikas Pushkar (Adapted from third.cc)
 */


#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/csma-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/aodv-helper.h"
#include "ns3/olsr-helper.h"
#include "ns3/basic-energy-source.h"
#include "ns3/simple-device-energy-model.h"
 #include "ns3/traced-value.h"
#include "ns3/trace-source-accessor.h"

#include <string>
 #include <math.h> 
 #include <iostream>



using namespace ns3;

uint64_t count = 0;

void AddupBytes (const Ptr< const Packet > packet)
{
  count+=packet->GetSize();
}





int 
main (int argc, char *argv[])
{
  RngSeedManager::SetSeed (11223344);
  Ptr<UniformRandomVariable> U = CreateObject<UniformRandomVariable> ();
  U->SetAttribute ("Stream", IntegerValue (6110));
  U->SetAttribute ("Min", DoubleValue (0.0));
  U->SetAttribute ("Max", DoubleValue (0.1));

  uint32_t nWifi = 100;
  double TxPower = 1.0;
  double trafficIntesity = 0.1;
  uint64_t dataRate = 0.0;
  //char udpDataRate[16];
  std::string route_protocol("AODV");
  
  CommandLine cmd;
  cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
  cmd.AddValue("TrafficIntensity", "Demand to Bandwidth Ratio", trafficIntesity);
  cmd.AddValue("Rprot", "AODV or OLSR", route_protocol);
  cmd.AddValue("TxPower", "Power in mWatts", TxPower);    

  cmd.Parse (argc,argv);

  dataRate = (int)((trafficIntesity*20.0*54.0)/((double)nWifi)*1000000.0);
  
  Config::SetDefault ("ns3::OnOffApplication::DataRate", 
                      DataRateValue (DataRate (dataRate)));

   //Config::SetDefault("ns3::OnOffApplication::MaxBytes", UintegerValue(1024));

  NodeContainer allNodes;
  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifi);
  allNodes.Add (wifiStaNodes);
  

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();

  //phy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
  phy.Set("TxPowerStart", DoubleValue(10.0*log10(TxPower)));
  phy.Set("TxPowerEnd", DoubleValue(10.0*log10(TxPower)));
  
  //channel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  //channel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel",
    //                                  "Exponent", DoubleValue (3.0));
  phy.SetChannel (channel.Create ());

  WifiHelper wifi = WifiHelper::Default ();
  wifi.SetStandard (WIFI_PHY_STANDARD_80211a);
  //wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
  //                              "DataMode", StringValue ("OfdmRate54Mbps"),
   //                             "ControlMode", StringValue("OfdmRate54Mbps"));

  NqosWifiMacHelper mac = NqosWifiMacHelper::Default ();
  mac.SetType ("ns3::AdhocWifiMac");


  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);
  



  // Mobility

  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::RandomRectanglePositionAllocator",
                                 "X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1000.0]"),
                                 "Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1000.0]"));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiStaNodes);

  // Install internet stack

  InternetStackHelper stack;
  
  if(route_protocol == "AODV"){
    AodvHelper aodv;
    stack.SetRoutingHelper(aodv);
    //Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("p3.routes", std::ios::out);
    //aodv.PrintRoutingTableAllEvery (Seconds (2), routingStream);
  }
  else
  {
    OlsrHelper olsr;
    stack.SetRoutingHelper(olsr);
  }
  stack.Install (allNodes);

  uint32_t *senders = new uint32_t[nWifi];

  for (uint32_t i = 0; i < nWifi; ++i)
    senders[i] = i;

  for (uint32_t i = nWifi - 1; i > 0; --i) {
    uint32_t j = rand() % i;
    uint32_t tmp = senders[i];
    senders[i] = senders[j];
    senders[j] = tmp;
  }

  Ipv4AddressHelper address;
  address.SetBase ("10.1.0.0", "255.255.0.0");

  Ipv4InterfaceContainer staInterfaces;
  staInterfaces = address.Assign (staDevices);

  //for(uint32_t ii = 0; ii < nWifi; ii++)
  //{
   // staInterfaces.GetAddress(ii,0).Print(std::cout);
   // std::cout << std::endl;
  //}

  ApplicationContainer serverApps, clientApps;

  for(uint32_t ii = 0; ii < nWifi; ii++)
  {
    PacketSinkHelper udpSink ("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(),9) );
    serverApps.Add(udpSink.Install (wifiStaNodes.Get(ii)));


    OnOffHelper udpClient ("ns3::UdpSocketFactory", InetSocketAddress(staInterfaces.GetAddress(senders[ii]),9));
    udpClient.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0.2]"));
    udpClient.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=0.01]"));
    clientApps.Add(udpClient.Install (wifiStaNodes.Get(ii)));
    (clientApps.Get(ii))->SetStartTime(Seconds (U->GetValue()));
  }

  
  serverApps.Start (Seconds (0.0));
  serverApps.Stop (Seconds (15.0));
  //clientApps.Start(Seconds(2.0));
  clientApps.Stop (Seconds (15.0));


  for(uint32_t ii = 0; ii < nWifi; ii++)
  {
    Ptr<OnOffApplication> source1 = DynamicCast<OnOffApplication> (clientApps.Get (ii));
    source1->TraceConnectWithoutContext("Tx",MakeCallback(&AddupBytes));
  }
  
  

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  Simulator::Stop (Seconds (10.0));

  /*AnimationInterface anim ("wireless-animation2.xml"); // Mandatory
  for (uint32_t i = 0; i < wifiStaNodes.GetN (); ++i)
    {
      char buffer[255];
      sprintf(buffer, "%d", i);
      anim.UpdateNodeDescription (wifiStaNodes.Get (i), buffer); // Optional
      anim.UpdateNodeColor (wifiStaNodes.Get (i), 255, 0, 0); // Optional
      
    }
  
  anim.EnableIpv4RouteTracking ("routingtable-wireless.xml", Seconds (0), Seconds (5), Seconds (0.25)); //Optional
  anim.EnableWifiMacCounters (Seconds (0), Seconds (10)); //Optional
  anim.EnableWifiPhyCounters (Seconds (0), Seconds (10)); //Optional
  anim.EnablePacketMetadata (); */

  Simulator::Run ();
  Simulator::Destroy ();

  std::cout << count << ",";
  
  uint64_t sumRx = 0;
  for(uint32_t ii = 0; ii < nWifi; ii++)
  {
    Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (serverApps.Get (ii));
    //std::cout << ii << ":" << sink1->GetTotalRx() << std::endl;
    sumRx+=sink1->GetTotalRx();
  }

  std::cout << sumRx << ",";
  std::cout << (double)nWifi/(1000.0*1000.0) << "," << TxPower << "," 
  << route_protocol << "," << trafficIntesity  << "," << (double)sumRx/(double)count 
  << std::endl;
  return 0;
}
