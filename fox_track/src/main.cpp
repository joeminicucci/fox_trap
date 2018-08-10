#include <painlessMesh.h>

#define   MESH_PREFIX     "dc719"
#define   MESH_PASSWORD   "whatdoesthefoxsay?"
#define   MESH_PORT       5566

// Prototype
Scheduler     _userScheduler; // to control your personal task
painlessMesh  _mesh;
uint16_t _channel = 6;
int32_t _maxRttDelay = 0;
unsigned long _initDelay = 15000000;
uint32_t _acknowledgementInterval = 900000; //ms
// size_t _foundTargetNodeId = 0;
std::vector<size_t> _foundTargetNodeIds;
const char* _mac;


void receivedCallback( uint32_t from, String &msg );
void rootInitialization();
void acknowledgeTargets();
void disableAck();
//Async functions
Task _rootInitializationTask(TASK_IMMEDIATE, TASK_ONCE, &rootInitialization, &_userScheduler);
Task _acknowledgementTask(TASK_SECOND * 3, 20, &acknowledgeTargets, &_userScheduler, false, NULL, &disableAck);


void acknowledgeTargets(){

    if (!_foundTargetNodeIds.empty()){


        DynamicJsonBuffer jsonBuffer;
        JsonObject& msg = jsonBuffer.createObject();
        for (auto nodeId : _foundTargetNodeIds) // access by reference to avoid copying
        {
            msg["found_ack"] = nodeId;
            String str;
            msg.printTo(str);
            msg.printTo(Serial);
            Serial.printf("\n");
            _mesh.sendBroadcast(str);
        }
    }

}

void disableAck()
{
    _foundTargetNodeIds.clear();
}

void sendInitializationSignal()
{
    DynamicJsonBuffer jsonBuffer;
    JsonObject& msg = jsonBuffer.createObject();
    // msg["topic"] = "initialize";
    //we're hoping to god the signed _maxRTTDelay is not negative for some reason
    msg["initialize"] = (_maxRttDelay*2) + _mesh.getNodeTime();

    String str;
    msg.printTo(str);
    _mesh.sendBroadcast(str);

    // log to serial
    Serial.printf("c2 initialization sent: ");
    msg.printTo(Serial);
    Serial.printf("\n");
}
//PainlessMesh function callbacks
void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("New Connection %u\n", nodeId);
    //calls to onNodeDelayReceived
    // _mesh.startDelayMeas(nodeId);

}

void onNodeDelayReceivedCallback(uint32_t nodeId, int32_t delay)
{
    Serial.printf("ROOT:onNodeDelayReceivedCallback: %u, %i\n", nodeId, delay);
    _maxRttDelay = (delay > _maxRttDelay)
        ? delay
        : _maxRttDelay;
}

void rootInitialization(){
    Serial.printf("ROOT:rootInitialization");
    //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE | DEBUG ); // all types on
    //mesh.setDebugMsgTypes( ERROR | CONNECTION | SYNC | S_TIME );  // set before init() so that you can see startup messages

    // _mesh.setDebugMsgTypes(SYNC | CONNECTION | MESH_STATUS | COMMUNICATION);  // set before init() so that you can see startup messages

    _mesh.init( MESH_PREFIX, MESH_PASSWORD, &_userScheduler, MESH_PORT, WIFI_AP, _channel);
    _mesh.onReceive(&receivedCallback);
    //keep the topology from fucking itself
    _mesh.setRoot(true);
    // This and all other mesh should ideally now the mesh contains a root
    _mesh.setContainsRoot(true);

    // _mesh.onNodeDelayReceived(&onNodeDelayReceivedCallback);
    _mesh.onNewConnection(&newConnectionCallback);

  //rid of inlines and push to prototype object oriented structure later..be a lazy piece of shit for now..
    _mesh.onDroppedConnection([](size_t nodeId) {
        Serial.printf("Dropped Connection %u\n", nodeId);

    });

    //Wait for a specified amount of time to gather all RTTs from bots..not a brillaint design decision
    // _rootInitializationTask.delay(_initDelay);

    // sendInitializationSignal();
    Serial.printf("ROOT:rootInitialization_END");
}



//Task(unsigned long aInterval, long aIterations, void (*aCallback)(), Scheduler* aScheduler, bool aEnable, bool (*aOnEnable)(), void (*aOnDisable)())

void setup() {
  Serial.begin(115200);
  // _userScheduler.addTask(logServerTask);
  _userScheduler.addTask(_rootInitializationTask);
  // _userScheduler.addTask(logServerTask);
  _userScheduler.addTask(_acknowledgementTask);
    _rootInitializationTask.enable();
    // logServerTask.enable();
}

void loop() {
  _userScheduler.execute(); // it will run _mesh scheduler as well
  _mesh.update();
}

void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("logServer: Received from %u msg=%s\n", from, msg.c_str());
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(msg);
  if (root.containsKey("found")) {
      // if (String("logServer").equals(root["topic"].as<String>())) {
          // check for on: true or false
          // _foundTargetNodeId = root["found"];
          size_t foundTargetNodeId = root["found"];
          int channel = root["chan"];
          signed rssi = root["rssi"];
          _mac = root["mac"];
          Serial.printf("[FOUND] %u | %s | %i | %d \n", foundTargetNodeId, _mac, channel, rssi);

        if(std::find(_foundTargetNodeIds.begin(), _foundTargetNodeIds.end(), foundTargetNodeId) == _foundTargetNodeIds.end()) {
            _foundTargetNodeIds.push_back(foundTargetNodeId);
          // signed rssi = root["rssi"];
          Serial.printf("[ADDED]NODE %u ADDED TARGET\n", foundTargetNodeId);
        }

        //checks if it is diabled and re-enables it. if it is out of iterations AND disabled, restart
        if( !_acknowledgementTask.enableIfNot()){
            _acknowledgementTask.restart();
        }
  }
  if (root.containsKey("fin_ack")){
      _foundTargetNodeIds.erase(std::remove(_foundTargetNodeIds.begin(), _foundTargetNodeIds.end(), root["fin_ack"]), _foundTargetNodeIds.end());
  }
}
