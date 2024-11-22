#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/error-model.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpExample");

class MyApp : public Application {
public:
  MyApp();
  virtual ~MyApp();
  void Setup(Ptr<Socket> socket, Address address, uint32_t packetSize,
             uint32_t nPackets, DataRate dataRate);

private:
  virtual void StartApplication() override;
  virtual void StopApplication() override;
  void ScheduleTx();
  void SendPacket();
  Ptr<Socket> m_socket;
  Address m_peer;
  uint32_t m_packetSize;
  uint32_t m_nPackets;
  DataRate m_dataRate;
  EventId m_sendEvent;
  uint32_t m_packetsSent;
};

MyApp::MyApp()
    : m_socket(0), m_peer(), m_packetSize(0), m_nPackets(0), m_dataRate(0),
      m_sendEvent(), m_packetsSent(0) {}

MyApp::~MyApp() { m_socket = 0; }

void MyApp::Setup(Ptr<Socket> socket, Address address, uint32_t packetSize,
                  uint32_t nPackets, DataRate dataRate) {
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

void MyApp::StartApplication() {
  m_packetsSent = 0;
  m_socket->Bind();
  m_socket->Connect(m_peer);
  SendPacket();
}

void MyApp::StopApplication() {
  if (m_sendEvent.IsRunning()) {
    Simulator::Cancel(m_sendEvent);
  }
  if (m_socket) {
    m_socket->Close();
  }
}

void MyApp::SendPacket() {
  Ptr<Packet> packet = Create<Packet>(m_packetSize);
  m_socket->Send(packet);
  if (++m_packetsSent < m_nPackets) {
    ScheduleTx();
  }
}

void MyApp::ScheduleTx() {
  if (m_sendEvent.IsRunning()) {
    return;
  }
  Time tNext(Seconds(m_packetSize * 8 / static_cast<double>(m_dataRate.GetBitRate())));
  m_sendEvent = Simulator::Schedule(tNext, &MyApp::SendPacket, this);
}

static void
CwndChange(Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
  *stream->GetStream() << Simulator::Now().GetSeconds() << " " << newCwnd << std::endl;
}

int main(int argc, char *argv[]) {
  
  std::string tcpVariant = "TcpNewReno";
  uint32_t maxPackets = 50;  
  double error_rate = 0.000001;
  uint32_t payloadSize = 1460;
  Time simulationTime = Seconds(10.0);

  
  Config::SetDefault("ns3::TcpL4Protocol::SocketType", 
                    StringValue("ns3::" + tcpVariant));

  
  NodeContainer nodes;
  nodes.Create(3);

  
  PointToPointHelper p2p1, p2p2;
  
  
  p2p1.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
  p2p1.SetChannelAttribute("Delay", StringValue("100ms"));
  p2p1.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("50p"));

  
  p2p2.SetDeviceAttribute("DataRate", StringValue("10Mbps")); 
  p2p2.SetChannelAttribute("Delay", StringValue("100ms")); 
  p2p2.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("50p"));

  
  NetDeviceContainer devices1 = p2p1.Install(nodes.Get(0), nodes.Get(1));
  NetDeviceContainer devices2 = p2p2.Install(nodes.Get(1), nodes.Get(2));

  
  Ptr<RateErrorModel> em1 = CreateObject<RateErrorModel>();
  em1->SetAttribute("ErrorRate", DoubleValue(error_rate));
  devices1.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(em1));

  Ptr<RateErrorModel> em2 = CreateObject<RateErrorModel>();
  em2->SetAttribute("ErrorRate", DoubleValue(error_rate));
  devices2.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(em2));

  
  InternetStackHelper stack;
  stack.Install(nodes);

  
  Ipv4AddressHelper address1, address2;
  address1.SetBase("10.1.1.0", "255.255.255.0");
  address2.SetBase("10.1.2.0", "255.255.255.0");

  Ipv4InterfaceContainer interfaces1 = address1.Assign(devices1);
  Ipv4InterfaceContainer interfaces2 = address2.Assign(devices2);

  
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  
  uint16_t sinkPort = 8080;
  Address sinkAddress(InetSocketAddress(interfaces2.GetAddress(1), sinkPort));
  PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory",
                                   InetSocketAddress(Ipv4Address::GetAny(), sinkPort));
  ApplicationContainer sinkApps = packetSinkHelper.Install(nodes.Get(2));
  sinkApps.Start(Seconds(0.0));
  sinkApps.Stop(simulationTime);

  
  Ptr<Socket> tcpSocket = Socket::CreateSocket(nodes.Get(0), TcpSocketFactory::GetTypeId());
  Ptr<MyApp> app = CreateObject<MyApp>();
  app->Setup(tcpSocket, sinkAddress, payloadSize, maxPackets * 100, DataRate("10Mbps"));
  nodes.Get(0)->AddApplication(app);
  app->SetStartTime(Seconds(1.0));
  app->SetStopTime(simulationTime);

  
  AsciiTraceHelper asciiTraceHelper;
  Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream("tcp-example.cwnd");
  tcpSocket->TraceConnectWithoutContext("CongestionWindow", 
                                       MakeBoundCallback(&CwndChange, stream));

  
  p2p1.EnablePcap("tcp-example", devices1.Get(0), true);
  p2p1.EnablePcap("tcp-example", devices1.Get(1), true);
  p2p2.EnablePcap("tcp-example", devices2.Get(1), true);

  
  AsciiTraceHelper ascii;
  p2p1.EnableAsciiAll(ascii.CreateFileStream("tcp-example.tr"));

  Simulator::Stop(simulationTime);
  Simulator::Run();
  Simulator::Destroy();

  return 0;
}
