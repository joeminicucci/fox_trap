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
#include <structures.h>
// #ifndef SNIFFING_H
// #define SNIFFING_H
#include "sniffing.h"
// #endif

//set target here
uint8_t _target[6] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
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


// Prototype
uint16_t _channel = 6;
// uint16_t _channelCount = 1;
size_t logServerId = 0;
int32_t _startTime = 0;
uint32_t _meshCommInterval = 15000; //ms
uint32_t _sniffInterval = 9000; //ms
uint32_t _resyncInterval = 900000; //ms
unsigned long _initDelay = 15000000;
bool _syncd = false;
uint8_t _channelHopInterval = 400;
bool _alertMode = false;
signed _lastFoundRSSI = 0;

// void receivedCallback( uint31_t from, String &msg );
Scheduler     _userScheduler; // to control your personal task
painlessMesh  _mesh;
Task _botInitializationTask(_meshCommInterval, TASK_ONCE, &botInitialization, &_userScheduler, false, NULL, &meshDisabled);
Task _channelHopTask(_channelHopInterval, TASK_FOREVER, &channelHop, &_userScheduler, false, NULL, NULL);
Task _resyncTask(_resyncInterval, TASK_ONCE, &resync, &_userScheduler, false, NULL, NULL);
Task _snifferInitializationTask(_sniffInterval, TASK_ONCE, &initializeSniffer, &_userScheduler, false, NULL, &snifferDisabled);
//Task::Task(long unsigned int, int, void (*)(), Scheduler*, bool, void (*)(), void (*)())'
Task _sendAlertTask(TASK_SECOND, 60, &sendAlert, &_userScheduler, false, NULL, &CalculateSyncAndLaunchTasks);
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
    _syncd = false;
    _resyncTask.restartDelayed();
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
  // for (int u = 0; u < aps_known_count; u++)
  // {
    if (! memcmp(_target, beacon.bssid, ETH_MAC_LEN)) {
      known = 1;
      // aps_known[u].last_heard = beacon.last_heard;
      // break;
    }   // AP known => Set known flag
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

            Serial.printf("TARGET ");
          print_beacon(beacon);
          if (!_sendAlertTask.isEnabled())
          {
              //We can thank the wonderful compiler for this hack, i.e. calling this function instead of properly setting the onEnable callback..
              initializeAlertMode();
          }
          _lastFoundRSSI = beacon.rssi;
          // sendAlert(beacon);
          // nothing_new = 0;
        };
      }
      // else if (frame_type ==0 && frame_subtype==4)
      // {
      //   struct probeinfo probe = parse_probe(sniffer->buf, 112, sniffer->rx_ctrl.rssi);
      //   if (register_probe(probe) == 0)
      //   {
      //     print_probe(probe);
      //     nothing_new = 0;
      //   };
      // }
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


void sendAlert()
{
    DynamicJsonBuffer jsonBuffer;
    JsonObject& msg = jsonBuffer.createObject();
    msg["found"] = _mesh.getNodeId();
    msg["rssi"] = _lastFoundRSSI;

    String str;
    msg.printTo(str);
    _mesh.sendBroadcast(str);

    // log to serial
    msg.printTo(Serial);
    Serial.printf("\n");

}

void initializeAlertMode()
{
        Serial.printf("SETTING ALERT MODE\n");
        _snifferInitializationTask.disable();
        _channelHopTask.disable();
        _botInitializationTask.disable();
        _resyncTask.disable();

        openMeshComm(false);
        _sendAlertTask.enable();

        //for post diable to resync to mesh so as to not get
        _syncd  = true;
}

bool botInitialization()
{
    openMeshComm(true);
    // Serial.printf("BOT:botInitialization_END\n");
    return true;
}

void openMeshComm(bool restartSnifferDelayed){
    wifi_set_opmode(STATIONAP_MODE);
    wifi_promiscuous_enable(DISABLE);

    if (restartSnifferDelayed)
    {
        _snifferInitializationTask.restartDelayed(_meshCommInterval);
        _channelHopTask.restartDelayed(_meshCommInterval);
    }

    _mesh.init( MESH_PREFIX, MESH_PASSWORD, &_userScheduler, MESH_PORT, WIFI_AP_STA, _channel);
}

bool initializeSniffer(){
    _botInitializationTask.restartDelayed(_sniffInterval);
    // _mesh.stop();
    wifi_set_opmode(STATION_MODE);
    wifi_promiscuous_enable(ENABLE);
    Serial.printf("promiscuos enable\n");
    return true;
}

void meshInitialization(){
    //keep the topology from fucking itself
    _mesh.setRoot(false);
    // This and all other mesh should ideally now the mesh contains a root
    _mesh.setContainsRoot(true);
    _mesh.onReceive(&receivedCallback);
    _mesh.onNodeTimeAdjusted(&onTimeAdjusted);
    // _mesh.setDebugMsgTypes(SYNC | CONNECTION);  // set before init() so that you can see startup messages
    //neeed to connect to the mesh once to get NTP sync

    _mesh.init( MESH_PREFIX, MESH_PASSWORD, &_userScheduler, MESH_PORT, WIFI_AP_STA, _channel);
}

void setup() {
  Serial.begin(115200);
    wifi_set_promiscuous_rx_cb(promisc_cb);
    // Serial.printf("promiscuos callback set\n");

    meshInitialization();

  Serial.printf("BOT:SETUP\n");
}

void loop() {
    _userScheduler.execute(); // it will run _mesh scheduler as well
    _mesh.update();
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
    if (_sendAlertTask.isEnabled() && root.containsKey("found_ack") ){
        if (root["found_ack"] == _mesh.getNodeId())
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
    msg["fin_ack"] = _mesh.getNodeId();
    String str;
    msg.printTo(str);
    _mesh.sendBroadcast(str);
    msg.printTo(Serial);
    Serial.printf("\n");
}
uint32_t CalculateSynchronizationDelay(){
  uint32_t current = _mesh.getNodeTime();

  //offset current time from the target interval
  //pulling by 1ms
  Serial.printf("current time is: %i microS\n",current);
  current = current / 1000;

uint32_t precision  = (_meshCommInterval + _sniffInterval);
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
    Serial.printf("SNTP GOOD WITH CURRENT_TIME=: %u\t DELTA: %u\n", _mesh.getNodeTime(), offset);
    if(!_syncd){
        _syncd = true;
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
        _userScheduler.addTask(_botInitializationTask);
        _userScheduler.addTask(_snifferInitializationTask);
        _userScheduler.addTask(_channelHopTask);
        _userScheduler.addTask(_resyncTask);
        _userScheduler.addTask(_sendAlertTask);
    }

    _botInitializationTask.restartDelayed(syncDelay);
    _snifferInitializationTask.restartDelayed(syncDelay + _meshCommInterval);
    _channelHopTask.restartDelayed(syncDelay + _meshCommInterval);
    _resyncTask.restartDelayed();
}
