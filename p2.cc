#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/netanim-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("P2");

int main(int argc, char *argv[])
{
  Ptr<UniformRandomVariable> U = CreateObject<UniformRandomVariable> ();
  U->SetAttribute ("Stream", IntegerValue (6110));
  U->SetAttribute ("Min", DoubleValue (0.0));
  U->SetAttribute ("Max", DoubleValue (0.1));

  uint32_t left = 2;
  uint32_t right = 1;

  std::string queue_type = "DropTail";
  uint32_t maxbytes = 32000;
  double minth = 30;
  double maxth = 90;
  double wq = 1.0/128.0;
  double maxp = 20.0;
  uint32_t windowSize = 32000;
  std::string udp_traffic = "2Mbps";
  std::string tcp_traffic = "0.25Mbps";
  std::string delay = "10ms";


  CommandLine cmd;
  cmd.AddValue  ("qtype", "Red vs DropTail", queue_type);
  cmd.AddValue ("load", "udp load", udp_traffic);
  cmd.AddValue ("maxp", "max prob", maxp);
  cmd.AddValue ("minth", "min thresh", minth);
  cmd.AddValue ("maxth", "max thresh", maxth);
  cmd.AddValue ("wq", "wieght", wq);
  cmd.AddValue ("queueSize", "size of queue in bytes", maxbytes);
  cmd.AddValue("winSize", "window size", windowSize);
  cmd.Parse (argc,argv);

  Config::SetDefault ("ns3::DropTailQueue::Mode", EnumValue(DropTailQueue::QUEUE_MODE_BYTES));
  Config::SetDefault ("ns3::DropTailQueue::MaxBytes", UintegerValue(maxbytes));
  Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpTahoe::GetTypeId()));
  Config::SetDefault("ns3::TcpSocketBase::MaxWindowSize", UintegerValue(windowSize));
  Config::SetDefault("ns3::TcpSocketBase::WindowScaling", BooleanValue(false));

  Config::SetDefault ("ns3::RedQueue::Mode", EnumValue (RedQueue::QUEUE_MODE_BYTES));
  Config::SetDefault ("ns3::RedQueue::QueueLimit", UintegerValue (maxbytes));
  Config::SetDefault ("ns3::RedQueue::MinTh", DoubleValue(512* minth));
  Config::SetDefault ("ns3::RedQueue::MaxTh", DoubleValue(512* maxth));
  Config::SetDefault ("ns3::RedQueue::QW", DoubleValue (wq));
  Config::SetDefault ("ns3::RedQueue::LInterm", DoubleValue (maxp));
  Config::SetDefault ("ns3::RedQueue::Gentle", BooleanValue (false));

  PointToPointHelper pointToPoint1, pointToPoint2;

  pointToPoint2.SetDeviceAttribute("DataRate", StringValue("2Mbps"));
  pointToPoint2.SetChannelAttribute("Delay", StringValue(delay));
  
  pointToPoint1.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  pointToPoint1.SetChannelAttribute ("Delay", StringValue (delay));

  double timeStart[2];
  for(int ii = 0; ii < 2; ii++)
  {
    timeStart[ii] = U->GetValue();
    //std::cout << timeStart[ii] << std::endl;
  }
  
  if(queue_type == "RED")
  {
    pointToPoint2.SetQueue("ns3::RedQueue");
    pointToPoint1.SetQueue("ns3::RedQueue");
  }

  InternetStackHelper stack;
 

  PointToPointDumbbellHelper d (left, pointToPoint1, right, pointToPoint1, pointToPoint2);
  d.InstallStack(stack);
  d.AssignIpv4Addresses(Ipv4AddressHelper("10.1.1.0","255.255.255.0"), Ipv4AddressHelper("10.2.1.0","255.255.255.0"), Ipv4AddressHelper("10.3.1.0","255.255.255.0"));


  PointToPointStarHelper s(3, pointToPoint1);
  s.InstallStack(stack);
  s.AssignIpv4Addresses(Ipv4AddressHelper("10.4.1.0", "255.255.255.0"));

  NetDeviceContainer devices = pointToPoint2.Install(d.GetRight(), s.GetHub());
  Ipv4AddressHelper address;
  address.SetBase ("10.5.1.0", "255.255.255.0");

  Ipv4InterfaceContainer interfaces = address.Assign (devices);


  OnOffHelper client( "ns3::UdpSocketFactory", InetSocketAddress (s.GetSpokeIpv4Address(2), 9));
   Time interPacketInterval = Seconds (0.01);
  client.SetAttribute("DataRate", StringValue (udp_traffic));
  Ptr<ConstantRandomVariable> const_yt_off = CreateObject<ConstantRandomVariable>();
  const_yt_off->SetAttribute("Constant", DoubleValue(0.0));
  client.SetAttribute("OffTime", PointerValue(const_yt_off));
  ApplicationContainer clientApp = client.Install(d.GetRight(0));
  //clientApp.Add(client.Install(d.GetRight(0)));
  clientApp.Start(Seconds(0.0));

  PacketSinkHelper server("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9));
  ApplicationContainer serverApp = server.Install(s.GetSpokeNode(2));
  serverApp.Start(Seconds(0.0));

  ApplicationContainer sinkApp;
  ApplicationContainer sourceApp;
 for(int ii = 0; ii < 2; ii++)
 {
    OnOffHelper source("ns3::TcpSocketFactory", InetSocketAddress (s.GetSpokeIpv4Address (ii), 9));
    source.SetAttribute("DataRate", StringValue (tcp_traffic));
    Ptr<ConstantRandomVariable> const_yt_of = CreateObject<ConstantRandomVariable>();
    const_yt_of->SetAttribute("Constant", DoubleValue(0.0));
    source.SetAttribute("OffTime", PointerValue(const_yt_of));
     sourceApp.Add(source.Install(d.GetLeft(ii)));
    (sourceApp.Get(ii))->SetStartTime(Seconds (timeStart[ii]));

    PacketSinkHelper sink ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9));
     sinkApp.Add(sink.Install (s.GetSpokeNode(ii)));
    //sinkApp.Start(Seconds(0.0));
}

sinkApp.Start(Seconds(0.0));
  


  
  //d.BoundingBox(1,1,100,100);
  //s.BoundingBox(1,100,100,200);



  //AnimationInterface anim("test-p2.xml");
  //anim.EnablePacketMetadata (true);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Simulator::Stop(Time("10.0s"));
  Simulator::Run ();
  Simulator::Destroy ();
 


  Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (serverApp.Get (0));
  std::cout << "UDP goodput flow" <<": "<< sink1->GetTotalRx ()/(10.0) << std::endl;

  for(int ii = 0; ii < 2; ii++)
  {
    Ptr<PacketSink> sink2 = DynamicCast<PacketSink> (sinkApp.Get (ii));
    std::cout << "TCP goodput flow" << ii <<": "<< sink2->GetTotalRx ()/(10.0-timeStart[ii]) << std::endl;
  }

  return 0;

}

/*
UDP goodput flow: 232550
TCP goodput flow0: 1611.84
TCP goodput flow1: 808.167
UDP_traffic: 2 Mbps
TCP_traffic: 0.5 Mbps
Droptail: 32000 bytes
*/