//************************************************************
// this is a simple example that uses the painlessMesh library to
// setup a node that logs to a central logging node
// The logServer example shows how to configure the central logging nodes
//************************************************************
extern "C"
{
  #include <user_interface.h>
}

#include <painlessMesh.h>
#include <cstdint>
#include "structures.h"
// #ifndef SNIFFING_H
// #define SNIFFING_H
#include "sniffing.h"
// #endif

//set target here

//FC:2D:5E:9B:3F:8F
// uint8_t _target[6] = { 0xFC, 0x2D, 0x5E, 0x9B, 0x3F, 0x8F };
std::vector<std::array<uint8_t, 6> > _targets =
        {
          { 0xFC, 0x2D, 0x5E, 0x9B, 0x3F, 0x8F }
        };


#define SENSOR_ID             0x00
#define DISABLE 0
#define ENABLE  1
#define TIME_PRECISION_MS = 10

//Wifi Configuration
#define   MESH_PREFIX     "dc719"
#define   MESH_PASSWORD   "whatdoesthefoxsay?"
#define   MESH_PORT       5566

//Scheduling Intervals
// #define RX_PER_CHANNEL_HOP 3
#define RX_COMM_INTERVAL 10000


// struct clientinfo
// {
//   uint8_t bssid[ETH_MAC_LEN];
//   uint8_t station[ETH_MAC_LEN];
//   uint8_t ap[ETH_MAC_LEN];
//   int channel;
//   int err;
//   signed rssi;
//   uint16_t seq_n;
// /* rpw additions */
//   uint8_t header;
//   uint32_t last_heard;
//   uint8_t reported;
// };

// struct clientinfo {
//   uint8_t beacon[ETH_MAC_LEN];
//   uint8_t station[ETH_MAC_LEN];
//   uint8_t rssi;
//   uint16_t seq;
//   bool err;
// };
//

//mesh members...NO_OBJECTS == NO_HOPE
bool botInitialization();
bool initializeSniffer();
void channelHop();
void resync();
void meshDisabled();
void snifferDisabled();
static void showMetaData();
uint8_t MostSignificantByte(uint32_t bytes);
void receivedCallback( uint32_t from, String &msg );
uint32_t CalculateSynchronizationDelay();
void onTimeAdjusted(int32_t offset);
void CalculateSyncAndLaunchTasks();
void sendAlert();
void sendFinAck();
void initializeAlertMode();
void openMeshComm(bool restartSnifferDelayed = false);
void LaunchTasks();
void getMAC(char *addr, uint8_t* data);
struct clientinfo parse_probe(uint8_t *frame, uint16_t framelen, signed rssi, unsigned channel);
int register_probe(clientinfo probe);
void print_probe(clientinfo ci);

void getMAC(char *addr, uint8_t* data) {
  sprintf(addr, "%02x:%02x:%02x:%02x:%02x:%02x", data[0], data[1], data[2], data[3], data[4], data[5]);
}



// Prototype
uint16_t channel = 6;
// uint16_t _channelCount = 1;
// long logServerId = 0;
uint32_t meshCommInterval = 20000; //ms
uint32_t sniffInterval = 9000; //ms
uint32_t resyncInterval = 900000; //ms
unsigned long _initDelay = 15000000;
bool syncd = false;
uint8_t channelHopInterval = 400;
bool _alertMode = false;
signed lastFoundRSSI = 0;
int lastFoundChannel = 0;
// uint8_t lastFoundMac[6] = {};
char lastFoundMac[] = "00:00:00:00:00:00";
bool fromSync = false;

// void receivedCallback( uint31_t from, String &msg );
Scheduler     userScheduler; // to control your personal task
painlessMesh  mesh;
Task _botInitializationTask(meshCommInterval, TASK_ONCE, &botInitialization, &userScheduler, false, NULL, &meshDisabled);
Task channelHopTask(channelHopInterval, TASK_FOREVER, &channelHop, &userScheduler, false, NULL, NULL);
Task resyncTask(resyncInterval, TASK_ONCE, &resync, &userScheduler, false, NULL, NULL);
Task snifferInitializationTask(sniffInterval, TASK_ONCE, &initializeSniffer, &userScheduler, false, NULL, &snifferDisabled);
//Task::Task(long unsigned int, int, void (*)(), Scheduler*, bool, void (*)(), void (*)())'
Task _sendAlertTask(TASK_SECOND * 2, 60, &sendAlert, &userScheduler, false, NULL, &CalculateSyncAndLaunchTasks);
uint32_t roundUp(uint32_t numToRound, uint32_t multiple);

void channelHop()
{
  // hoping channels 1-14
  uint8 new_channel = wifi_get_channel() + 1;
  if (new_channel > 13)
    new_channel = 1;
// Serial.printf("***HOP***\n");
  wifi_set_channel(new_channel);
  // _channelCount+=1;
}

void resync()
{
    syncd = false;
    resyncTask.restartDelayed();
}
//*************** DEMARC SNIFFER ***************/
struct beaconinfo parse_beacon(uint8_t *frame, uint16_t framelen, signed rssi)
{
  // takes 112 byte beacon frame
  struct beaconinfo bi;
  bi.ssid_len = 0;
  bi.channel = 0;
  bi.err = 0;
  bi.rssi = rssi;
  bi.last_heard=millis()/1000;
  bi.reported=0;
  bi.header=frame[0];
  int pos = 36;
  uint8_t frame_type= (frame[0] & 0x0C)>>2;
  uint8_t frame_subtype= (frame[0] & 0xF0)>>4;
  if (frame[pos] == 0x00) {
    while (pos < framelen) {
      switch (frame[pos]) {
        case 0x00: //SSID
          bi.ssid_len = (int) frame[pos + 1];
          if (bi.ssid_len == 0) {
            memset(bi.ssid, '\x00', 33);
            break;
          }
          if (bi.ssid_len < 0) {
            bi.err = -1;
            break;
          }
          if (bi.ssid_len > 32) {
            bi.err = -2;
            break;
          }
          memset(bi.ssid, '\x00', 33);
          memcpy(bi.ssid, frame + pos + 2, bi.ssid_len);
          bi.err = 0;  // before was error??
          break;
        case 0x03: //Channel
          bi.channel = (int) frame[pos + 2];
          pos = -1;
          break;
        default:
          break;
      }
      if (pos < 0) break;
      pos += (int) frame[pos + 1] + 2;
    }
  } else {
    bi.err = -3;
  }

  bi.capa[0] = frame[34];
  bi.capa[1] = frame[35];
  memcpy(bi.bssid, frame + 10, ETH_MAC_LEN);
  return bi;
};

int register_beacon(beaconinfo beacon)
{
  // // add beacon to list if not already included
  int known = 0;   // Clear known flag

    for (const auto& target : _targets)
    {
      if (! memcmp(target.data(), beacon.bssid, 6)) {
        known = 1;
      }
    }
  // }
  // if (! known)  // AP is NEW, copy MAC to array and return it
  // {
  //   memcpy(&aps_known[aps_known_count], &beacon, sizeof(beacon));
  //   aps_known_count++;
  //
  //   if ((unsigned int) aps_known_count >=
  //       sizeof (aps_known) / sizeof (aps_known[0]) ) {
  //     Serial.printf("exceeded max aps_known\n");
  //     aps_known_count = 0;
  //   }
  // }
  return known;
}
int register_probe(clientinfo probe)
{
  // // add beacon to list if not already included
  int known = 0;   // Clear known flag

    for (const auto& target : _targets)
    {
      if (! memcmp(target.data(), probe.station, 6)) {
        known = 1;
      }
    }
  // }
  // if (! known)  // AP is NEW, copy MAC to array and return it
  // {
  //   memcpy(&aps_known[aps_known_count], &beacon, sizeof(beacon));
  //   aps_known_count++;
  //
  //   if ((unsigned int) aps_known_count >=
  //       sizeof (aps_known) / sizeof (aps_known[0]) ) {
  //     Serial.printf("exceeded max aps_known\n");
  //     aps_known_count = 0;
  //   }
  // }
  return known;
}

void print_pkt_header(uint8_t *buf, uint16_t len, char *pkt_type)
{
  char ssid_name[33];
  memset(ssid_name, '\x00', 33);
  Serial.printf("%s", pkt_type);
  uint8_t frame_control_pkt = buf[12]; // just after the RxControl Structure
  uint8_t protocol_version = frame_control_pkt & 0x03; // should always be 0x00
  uint8_t frame_type = (frame_control_pkt & 0x0C) >> 2;
  uint8_t frame_subtype = (frame_control_pkt & 0xF0) >> 4;
  uint8_t ds= buf[13] & 0x3;

  // print the 3 MAC-address fields
  Serial.printf("%02x %02x %02x ", frame_type, frame_subtype, ds);
  if (len<35) {
    Serial.printf("Short Packet!! %d\r\n",len);
    return;
  }
  for (int n = 16; n < 22; n++) {
    Serial.printf("%02x", buf[n]);
  };
  Serial.print(" ");
  for (int n = 22; n < 28; n++) {
    Serial.printf("%02x", buf[n]);
  };
  Serial.print(" ");
  for (int n = 28; n < 34; n++) {
    Serial.printf("%02x", buf[n]);
  };
  //ACTION frames are 112 long but contain nothing useful
  if (len>=112 && !( frame_type==0 && frame_subtype==13)) {

    int pos=49;
    if (frame_type==0 && frame_subtype==4) {
      pos=37; // probe request frames
    }

    int bssid_len=(int) buf[pos];
    if(bssid_len<0 || bssid_len>32) {
      Serial.printf(" Bad ssid len %d!!\r\n",bssid_len);
      return;
    };
    if (bssid_len==0) {
      Serial.print(" <open>");
    } else {
      memcpy(ssid_name,&(buf[pos+1]),bssid_len);
      Serial.printf(" %s",ssid_name);
    };
    Serial.printf(":%d %d %d ",(int)buf[pos+bssid_len+1],(int)buf[pos+bssid_len+2],(int)buf[pos+bssid_len+3]);

  };
  Serial.print("\r\n");
}


void print_beacon(beaconinfo beacon)
{
  uint64_t now = millis()/1000;
  if (beacon.err != 0) {
    Serial.printf("BEACON ERR: (%d)  \r\n", beacon.err);
  } else {
    Serial.printf("BEACON: <=============== [%32s]  ", beacon.ssid);
    for (int i = 0; i < 6; i++) Serial.printf("%02x", beacon.bssid[i]);
    //Serial.printf(" %02x", beacon.header);
    Serial.printf(" %3d", beacon.channel);
    Serial.printf("   %d", (now - beacon.last_heard));
    Serial.printf("   %d", (beacon.reported));
    Serial.printf("   %4d\r\n", beacon.rssi);

  }
}
void promisc_cb(uint8_t *buf, uint16_t len)
{
  int i = 0;
  uint16_t seq_n_new = 0;
#ifdef PRINT_RAW_HEADER
  if (len >= 35) {
    uint8_t frame_control_pkt = buf[12]; // just after the RxControl Structure
    uint8_t protocol_version = frame_control_pkt & 0x03; // should always be 0x00
    uint8_t frame_type = (frame_control_pkt & 0x0C) >> 2;
    uint8_t frame_subtype = (frame_control_pkt & 0xF0) >> 4;

    struct control_frame *cf=(struct control_frame *)&buf[12];

         switch (cf->type) {
           case 0:
            switch (cf->subtype) {
              case 0:
               print_pkt_header(buf,len,"ASREQ:");
               break;
              case 1:
               print_pkt_header(buf,len,"ASRSP:");
               break;
              case 2:
               print_pkt_header(buf,len,"RSRSP:");
               break;
              case 3:
               print_pkt_header(buf,len,"RSRSP:");
               break;
              case 4:
               print_pkt_header(buf,len,"PRREQ:");
               break;
              case 5:
               print_pkt_header(buf,len,"PRRSP:");
               break;
              case 8:
               print_pkt_header(buf,len,"BECON:");
               break;
              case 9:
               print_pkt_header(buf,len,"ATIM :");
               break;
              case 10:
               print_pkt_header(buf,len,"DASSO:");
               break;
              case 11:
               print_pkt_header(buf,len,"AUTH :");
               break;
              case 12:
               print_pkt_header(buf,len,"DAUTH:");
               break;
              case 13:
               print_pkt_header(buf,len,"ACTON:");
               break;
              default:
               print_pkt_header(buf,len,"MGMT?:");
               break;
            };
            break;
           case 1:
             print_pkt_header(buf,len,"CONTR:");
             break;
           case 2:
             print_pkt_header(buf,len,"DATA :");
             break;
           default:
             print_pkt_header(buf,len,"UNKNOW:");
         }

  }
  #endif

  if (len == 12) {
    struct RxControl *sniffer = (struct RxControl*) buf;
  } else if (len == 128) {
    uint8_t frame_control_pkt = buf[12]; // just after the RxControl Structure
    uint8_t frame_type = (frame_control_pkt & 0x0C) >> 2;
    uint8_t frame_subtype = (frame_control_pkt & 0xF0) >> 4;
    struct sniffer_buf2 *sniffer = (struct sniffer_buf2*) buf;
    if (frame_type == 0 && (frame_subtype == 8 || frame_subtype == 5))
      {
        struct beaconinfo beacon = parse_beacon(sniffer->buf, 112, sniffer->rx_ctrl.rssi);
        if (register_beacon(beacon) == 1)
        {

            Serial.printf("TARGET BEACON");
          print_beacon(beacon);
          if (!_sendAlertTask.isEnabled())
          {
              //We can thank the wonderful compiler for this hack, i.e. calling this function instead of properly setting the onEnable callback..
              initializeAlertMode();
          }
          lastFoundRSSI = beacon.rssi;
          lastFoundChannel = beacon.channel;
          // memcpy(lastFoundMac, beacon.bssid,ETH_MAC_LEN);
            getMAC(lastFoundMac, beacon.bssid);


          // sendAlert(beacon);
          // nothing_new = 0;
        };
      }
      else
      {
        struct sniffer_buf *sniffer = (struct sniffer_buf*) buf;
        struct clientinfo probe = parse_probe(sniffer->buf, 36, sniffer->rx_ctrl.rssi, sniffer->rx_ctrl.channel);
            // getMAC(lastFoundMac, probe.station);
          // Serial.printf("PROBE FROM : %s", lastFoundMac);

        if (register_probe(probe) == 1)
        {
          Serial.printf("TARGET PROBE");
          print_probe(probe);


        if (!_sendAlertTask.isEnabled())
        {
            //We can thank the wonderful compiler for this hack, i.e. calling this function instead of properly setting the onEnable callback..
            initializeAlertMode();
        }
        lastFoundRSSI = probe.rssi;
        lastFoundChannel = probe.channel;
        // memcpy(lastFoundMac, beacon.bssid,ETH_MAC_LEN);
        getMAC(lastFoundMac, probe.station);
      }
    }
      // else
      // {
      //   print_pkt_header(buf,112,"UKNOWN:");
      // };
  }
  // else
  // {
    // struct sniffer_buf *sniffer = (struct sniffer_buf*) buf;
    // struct clientinfo ci = parse_data(sniffer->buf, 36, sniffer->rx_ctrl.rssi, sniffer->rx_ctrl.channel);
    // if (register_client(ci) == 0) {
    //   print_client(ci);
    //   nothing_new = 0;
    // }
  // }
}


/*
 * Function that parses client information from packet
 */
struct clientinfo parse_probe(uint8_t *frame, uint16_t framelen, signed rssi, unsigned channel)
{
  struct clientinfo ci;
  ci.channel = channel;
  ci.err = 0;
  ci.rssi = rssi;
  int pos = 36;
  uint8_t *bssid;
  uint8_t *station;
  uint8_t *ap;
  uint8_t ds;

  ds = frame[1] & 3;    //Set first 6 bits to 0
  switch (ds) {
    // p[1] - xxxx xx00 => NoDS   p[4]-DST p[10]-SRC p[16]-BSS
    case 0:
      bssid = frame + 16;
      station = frame + 10;
      ap = frame + 4;
      break;
    // p[1] - xxxx xx01 => ToDS   p[4]-BSS p[10]-SRC p[16]-DST
    case 1:
      bssid = frame + 4;
      station = frame + 10;
      ap = frame + 16;
      break;
    // p[1] - xxxx xx10 => FromDS p[4]-DST p[10]-BSS p[16]-SRC
    case 2:
      bssid = frame + 10;
      // hack - don't know why it works like this...
      if (memcmp(frame + 4, broadcast1, 3) || memcmp(frame + 4, broadcast2, 3) || memcmp(frame + 4, broadcast3, 3)) {
        station = frame + 16;
        ap = frame + 4;
      } else {
        station = frame + 4;
        ap = frame + 16;
      }
      break;
    // p[1] - xxxx xx11 => WDS    p[4]-RCV p[10]-TRM p[16]-DST p[26]-SRC
    case 3:
      bssid = frame + 10;
      station = frame + 4;
      ap = frame + 4;
      break;
  }

  memcpy(ci.station, station, ETH_MAC_LEN);
  memcpy(ci.bssid, bssid, ETH_MAC_LEN);
  memcpy(ci.ap, ap, ETH_MAC_LEN);

  ci.seq_n = frame[23] * 0xFF + (frame[22] & 0xF0);
  return ci;
}



void print_probe(clientinfo ci) {
  Serial.print("\"");
  for (int i = 0; i < ETH_MAC_LEN; i++) {
    if (i > 0) Serial.print(":");
    Serial.printf("%02x", ci.station[i]);
  }
  Serial.print("\":{\"beacon\":\"");
  for (int i = 0; i < ETH_MAC_LEN; i++) {
    if (i > 0) Serial.print(":");
    Serial.printf("%02x", ci.bssid[i]);
  }
  Serial.printf("\",\"rssi\":-%d}", ci.rssi);
}

void sendAlert()
{
    DynamicJsonBuffer jsonBuffer;
    JsonObject& msg = jsonBuffer.createObject();
    msg["found"] = ESP.getChipId();
    msg["rssi"] = lastFoundRSSI;
    msg["chan"] = lastFoundChannel;
    msg["mac"] = lastFoundMac;

    String str;
    msg.printTo(str);
    mesh.sendBroadcast(str);

    // log to serial
    msg.printTo(Serial);
    Serial.printf("\n");

}

void initializeAlertMode()
{
        resyncTask.disable();
        snifferInitializationTask.disable();
        channelHopTask.disable();
        _botInitializationTask.disable();

        Serial.printf("SETTING ALERT MODE\n");
        openMeshComm(false);
        _sendAlertTask.restart();
}

bool botInitialization()
{
    if (fromSync)
    {
      openMeshComm(false);
      fromSync = false;
    }
    else
      openMeshComm(true);
    // Serial.printf("BOT:botInitialization_END\n");
    return true;
}

void openMeshComm(bool restartSnifferDelayed){
    wifi_set_opmode(STATIONAP_MODE);
    wifi_promiscuous_enable(DISABLE);

    if (restartSnifferDelayed)
    {
        snifferInitializationTask.restartDelayed(meshCommInterval);
        channelHopTask.restartDelayed(meshCommInterval);
        // mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, _channel);
    }
}

bool initializeSniffer(){
    _botInitializationTask.restartDelayed(sniffInterval);
    // mesh.stop();
    wifi_set_opmode(STATION_MODE);
    wifi_promiscuous_enable(ENABLE);

    return true;
}



void meshInitialization(){
    //keep the topology from fucking itself
    mesh.setRoot(false);
    // This and all other mesh should ideally now the mesh contains a root
    mesh.setContainsRoot(true);
    mesh.onReceive(&receivedCallback);
    mesh.onNodeTimeAdjusted(&onTimeAdjusted);
    // mesh.setDebugMsgTypes(SYNC | CONNECTION);  // set before init() so that you can see startup messages
    //neeed to connect to the mesh once to get NTP sync

    mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, channel);
}

void setup() {
  Serial.begin(115200);
  //set mac here
  //  uint8_t mac[] = {0x77, 0x01, 0x02, 0x03, 0x04, 0x05};
  //wifi_set_macaddr(STATION_IF, &mac[0]);
    wifi_set_promiscuous_rx_cb(promisc_cb);
    // Serial.printf("promiscuos callback set\n");

    meshInitialization();

  Serial.printf("BOT:SETUP\n");
}

void loop() {
    userScheduler.execute(); // it will run mesh scheduler as well
    mesh.update();
}

void meshDisabled(){

    Serial.printf("BOT INITIALIZED\n");
}
void snifferDisabled(){
    //STA_AP mode
    Serial.printf("SNIFFER INITIALIZED\n");
}


void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("logClient: Received from %u msg=%s\n", from, msg.c_str());

  // Saving logServer
  // DynamicJsonBuffer jsonBuffer;
  // JsonObject& root = jsonBuffer.parseObject(msg);
  // if (root.containsKey("initialize")) {
  //         _startTime = (root["initialize"] > _startTime)
  //           ? root["initialize"]
  //           : _startTime;
  //   }
  //
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(msg);
    if (_sendAlertTask.isEnabled() && root.containsKey("foundack") ){
        if (root["foundack"] == ESP.getChipId())
        {

            //try a one ditch effort to get the server to stfu. could make this a multi-ditch effort, but dont want to burn alot of cycles
            sendFinAck();
            //pull out of alerted state
            _sendAlertTask.disable();
        }
    }
      Serial.printf("logClient: Handled from %u msg=%s\n", from, msg.c_str());
}

void sendFinAck(){
    DynamicJsonBuffer jsonBuffer;
    JsonObject& msg = jsonBuffer.createObject();
    msg["fin_ack"] = ESP.getChipId();
    String str;
    msg.printTo(str);
    mesh.sendBroadcast(str);
    msg.printTo(Serial);
    Serial.printf("\n");
}
uint32_t CalculateSynchronizationDelay(){
  uint32_t current = mesh.getNodeTime();

  //offset current time from the target interval
  //pulling by 1ms
  Serial.printf("current time is: %i microS\n",current);
  current = current / 1000;

uint32_t precision  = (meshCommInterval + sniffInterval);
  //get the next closest interval to synchronize with the mesh
  uint32_t nextThreshold = roundUp(current, precision);
  Serial.printf("current time is: %i mS\n",current);
  Serial.printf("next threshold by rounded by %imS is: %imS\n", precision, nextThreshold);
  nextThreshold = nextThreshold - current;
  return nextThreshold;
}

//rounds from nearest interval of 10ms (time precision) multiplied by the defined time synchronization
uint32_t roundUp(uint32_t numToRound, uint32_t multiple)
{
    if (multiple == 0)
        return numToRound;

    int remainder = numToRound % multiple;
    if (remainder == 0)
        return numToRound;

    return numToRound + multiple - remainder;
}

void onTimeAdjusted(int32_t offset){
    Serial.printf("SNTP GOOD WITH CURRENT_TIME=: %u\t DELTA: %u\n", mesh.getNodeTime(), offset);
    if(!syncd){
        syncd = true;
        CalculateSyncAndLaunchTasks();
    }
}

bool addedTasks = false;
void CalculateSyncAndLaunchTasks()
{
    uint32_t syncDelay = CalculateSynchronizationDelay();
    Serial.printf("next sync is :%i \n",syncDelay);

    if (!addedTasks){
        addedTasks = true;
        userScheduler.addTask(_botInitializationTask);
        userScheduler.addTask(snifferInitializationTask);
        userScheduler.addTask(channelHopTask);
        userScheduler.addTask(resyncTask);
        userScheduler.addTask(_sendAlertTask);
    }

    //TODO get rid of this non-sense
    fromSync = true;
    _botInitializationTask.restartDelayed(syncDelay);
    // openMeshComm(false);
    snifferInitializationTask.restartDelayed(syncDelay + meshCommInterval);
    channelHopTask.restartDelayed(syncDelay + meshCommInterval);
    resyncTask.restartDelayed();
}
void LaunchTasks(){
    // _botInitializationTask.restartDelayed();
    snifferInitializationTask.restartDelayed(meshCommInterval);
    channelHopTask.restartDelayed(meshCommInterval);
    resyncTask.restartDelayed();
}
