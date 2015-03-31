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
#include "ns3/basic-energy-source.h"
#include "ns3/simple-device-energy-model.h"

#include <string>



using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("WirelessAnimationExample");

int 
main (int argc, char *argv[])
{
  uint32_t nWifi = 10;
  //double mWatts = 1;
  CommandLine cmd;
  cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
  

  cmd.Parse (argc,argv);
  NodeContainer allNodes;
  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifi);
  allNodes.Add (wifiStaNodes);
  //NodeContainer wifiApNode ;
  //wifiApNode.Create (1);
  //allNodes.Add (wifiApNode);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  
  phy.Set("TxPowerStart", DoubleValue(100));
  phy.Set("TxPowerEnd", DoubleValue(100));
  
  phy.SetChannel (channel.Create ());

  WifiHelper wifi = WifiHelper::Default ();
  // wifi.SetRemoteStationManager ("ns3::AarfWifiManager");
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue ("OfdmRate54Mbps"));

  NqosWifiMacHelper mac = NqosWifiMacHelper::Default ();

  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);
  //mac.SetType ("ns3::ApWifiMac",
               //"Ssid", SsidValue (ssid));

  //NetDeviceContainer apDevices;
  //apDevices = wifi.Install (phy, mac, wifiApNode);


//   NodeContainer p2pNodes;
//   p2pNodes.Add (wifiApNode);
//   p2pNodes.Create (1);
//   allNodes.Add (p2pNodes.Get (1));

//   PointToPointHelper pointToPoint;
//   pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
//   pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

//   NetDeviceContainer p2pDevices;
//   p2pDevices = pointToPoint.Install (p2pNodes);

//   NodeContainer csmaNodes;
//   csmaNodes.Add (p2pNodes.Get (1));
//   csmaNodes.Create (1);
//   allNodes.Add (csmaNodes.Get (1));

//   CsmaHelper csma;
//   csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
//   csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

//   NetDeviceContainer csmaDevices;
//   csmaDevices = csma.Install (csmaNodes);

  // Mobility

  MobilityHelper mobility;
//   mobility.SetPositionAllocator ("ns3::RandomPositionAllocator",
//                                  "MinX", DoubleValue (0.0),
//                                  "MinY", DoubleValue (0.0),
//                                  "DeltaX", DoubleValue (5.0),
//                                  "DeltaY", DoubleValue (2.0),
//                                  "GridWidth", UintegerValue (10),
//                                  "LayoutType", StringValue ("RowFirst"));
  mobility.SetPositionAllocator ("ns3::RandomRectanglePositionAllocator",
                                 "X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=50.0]"),
                                 "Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=50.0]"));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiStaNodes);
  //mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  //mobility.Install (wifiApNode);
  //AnimationInterface::SetConstantPosition (p2pNodes.Get (1), 10, 500); 
  //AnimationInterface::SetConstantPosition (csmaNodes.Get (1), 10, 500); 

  Ptr<BasicEnergySource> energySource = CreateObject<BasicEnergySource>();
  Ptr<SimpleDeviceEnergyModel> energyModel = CreateObject<SimpleDeviceEnergyModel>();

  energySource->SetInitialEnergy (300);
  energyModel->SetEnergySource (energySource);
  energySource->AppendDeviceEnergyModel (energyModel);
  energyModel->SetCurrentA (20);

  // aggregate energy source to node
  //wifiApNode.Get (0)->AggregateObject (energySource);

  // Install internet stack

  InternetStackHelper stack;
  stack.Install (allNodes);

  // Install Ipv4 addresses

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
//   Ipv4InterfaceContainer p2pInterfaces;
//   p2pInterfaces = address.Assign (p2pDevices);
//   address.SetBase ("10.1.2.0", "255.255.255.0");
//   Ipv4InterfaceContainer csmaInterfaces;
//   csmaInterfaces = address.Assign (csmaDevices);
//   address.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer staInterfaces;
  staInterfaces = address.Assign (staDevices);
//   Ipv4InterfaceContainer apInterface;
//   apInterface = address.Assign (apDevices);

  // Install applications

  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (wifiStaNodes.Get (1));
   serverApps.Start (Seconds (1.0));
   serverApps.Stop (Seconds (15.0));
   UdpEchoClientHelper echoClient (wifiStaNodes.Get(1)->GetDevice(0)->GetAddress(), 9);
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
  /*for (uint32_t i = 0; i < wifiApNode.GetN (); ++i)
    {
      anim.UpdateNodeDescription (wifiApNode.Get (i), "AP"); // Optional
      anim.UpdateNodeColor (wifiApNode.Get (i), 0, 255, 0); // Optional
    }*/
  /*for (uint32_t i = 0; i < csmaNodes.GetN (); ++i)
    {
      anim.UpdateNodeDescription (csmaNodes.Get (i), "CSMA"); // Optional
      anim.UpdateNodeColor (csmaNodes.Get (i), 0, 0, 255); // Optional 
    }*/

  //anim.EnablePacketMetadata (); // Optional
  anim.EnableIpv4RouteTracking ("routingtable-wireless.xml", Seconds (0), Seconds (5), Seconds (0.25)); //Optional
  anim.EnableWifiMacCounters (Seconds (0), Seconds (10)); //Optional
  anim.EnableWifiPhyCounters (Seconds (0), Seconds (10)); //Optional
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
