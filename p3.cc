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

#include <string>
 #include <math.h> 



using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("WirelessAnimationExample");

int 
main (int argc, char *argv[])
{
  uint32_t nWifi = 50;
  double TxPower = 1.0;
  double trafficIntesity = 0.1;
  std::string route_protocol("AODV");
  
  CommandLine cmd;
  cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
  cmd.AddValue("TrafficIntesity", "Demand to Bandwidth Ratio", trafficIntesity);
  cmd.AddValue("Routing_protocol", "AODV or OLSR", route_protocol);
  cmd.AddValue("Transmission Power", "Power in mWatts", TxPower);    

  cmd.Parse (argc,argv);
  NodeContainer allNodes;
  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifi);
  allNodes.Add (wifiStaNodes);
  

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  
  phy.Set("TxPowerStart", DoubleValue(10.0*log10(TxPower)));
  phy.Set("TxPowerEnd", DoubleValue(10.0*log10(TxPower)));
  
  phy.SetChannel (channel.Create ());

  WifiHelper wifi = WifiHelper::Default ();
  wifi.SetStandard (WIFI_PHY_STANDARD_80211a);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue ("OfdmRate54Mbps"));

  NqosWifiMacHelper mac = NqosWifiMacHelper::Default ();

  //Ssid ssid = Ssid ("ns-3-ssid"); //True source of error Found here
  //mac.SetType ("ns3::StaWifiMac",
    //           "Ssid", SsidValue (ssid),
     //          "ActiveProbing", BooleanValue (false));

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
  }
  else
  {
    OlsrHelper olsr;
    stack.SetRoutingHelper(olsr);
  }


  
  stack.Install (allNodes);

  

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");

  Ipv4InterfaceContainer staInterfaces;
  staInterfaces = address.Assign (staDevices);


  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (wifiStaNodes.Get (1));
   serverApps.Start (Seconds (1.0));
   serverApps.Stop (Seconds (15.0));
   UdpEchoClientHelper echoClient (staInterfaces.GetAddress(0), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (10));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
  
  ApplicationContainer clientApps = echoClient.Install (wifiStaNodes);
  
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (15.0));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  Simulator::Stop (Seconds (15.0));

  AnimationInterface anim ("wireless-animation2.xml"); // Mandatory
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
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
