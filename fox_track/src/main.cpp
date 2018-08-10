#include <painlessMesh.h>

#define   MESH_PREFIX     "dc719"
#define   MESH_PASSWORD   "whatdoesthefoxsay?"
#define   MESH_PORT       5566

// Prototype
Scheduler     userScheduler; // to control your personal task
painlessMesh  mesh;
uint16_t channel = 6;
int32_t maxRttDelay = 0;
unsigned long initDelay = 15000000;
uint32_t acknowledgementInterval = 900000; //ms
// size_t _foundTargetNodeId = 0;
std::vector<long> _foundTargetNodeIds;
const char* mac;


void receivedCallback( uint32_t from, String &msg );
void rootInitialization();
void acknowledgeTargets();
void disableAck();
//Async functions
Task _rootInitializationTask(TASK_IMMEDIATE, TASK_ONCE, &rootInitialization, &userScheduler);
Task acknowledgementTask(TASK_SECOND * 3, 20, &acknowledgeTargets, &userScheduler, false, NULL, &disableAck);


void acknowledgeTargets(){

    if (!_foundTargetNodeIds.empty()){


        DynamicJsonBuffer jsonBuffer;
        JsonObject& msg = jsonBuffer.createObject();
        for (auto nodeId : _foundTargetNodeIds) // access by reference to avoid copying
        {
            msg["foundack"] = nodeId;
            String str;
            msg.printTo(str);
            msg.printTo(Serial);
            Serial.printf("\n");
            mesh.sendBroadcast(str);
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
    //we're hoping to god the signed maxRttDelay is not negative for some reason
    msg["initialize"] = (maxRttDelay*2) + mesh.getNodeTime();

    String str;
    msg.printTo(str);
    mesh.sendBroadcast(str);

    // log to serial
    Serial.printf("c2 initialization sent: ");
    msg.printTo(Serial);
    Serial.printf("\n");
}
//PainlessMesh function callbacks
void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("New Connection %u\n", nodeId);
    //calls to onNodeDelayReceived
    // mesh.startDelayMeas(nodeId);

}

void onNodeDelayReceivedCallback(uint32_t nodeId, int32_t delay)
{
    Serial.printf("ROOT:onNodeDelayReceivedCallback: %u, %i\n", nodeId, delay);
    maxRttDelay = (delay > maxRttDelay)
        ? delay
        : maxRttDelay;
}

void rootInitialization(){
    Serial.printf("ROOT:rootInitialization");
    //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE | DEBUG ); // all types on
    //mesh.setDebugMsgTypes( ERROR | CONNECTION | SYNC | S_TIME );  // set before init() so that you can see startup messages

    // mesh.setDebugMsgTypes(SYNC | CONNECTION | MESH_STATUS | COMMUNICATION);  // set before init() so that you can see startup messages

    mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP, channel);
    mesh.onReceive(&receivedCallback);
    //keep the topology from fucking itself
    mesh.setRoot(true);
    // This and all other mesh should ideally now the mesh contains a root
    mesh.setContainsRoot(true);

    // mesh.onNodeDelayReceived(&onNodeDelayReceivedCallback);
    mesh.onNewConnection(&newConnectionCallback);

  //rid of inlines and push to prototype object oriented structure later..be a lazy piece of shit for now..
    mesh.onDroppedConnection([](size_t nodeId) {
        Serial.printf("Dropped Connection %u\n", nodeId);

    });

    //Wait for a specified amount of time to gather all RTTs from bots..not a brillaint design decision
    // _rootInitializationTask.delay(initDelay);

    // sendInitializationSignal();
    Serial.printf("ROOT:rootInitialization_END");
}



//Task(unsigned long aInterval, long aIterations, void (*aCallback)(), Scheduler* aScheduler, bool aEnable, bool (*aOnEnable)(), void (*aOnDisable)())

void setup() {
  Serial.begin(115200);
  // userScheduler.addTask(logServerTask);
  userScheduler.addTask(_rootInitializationTask);
  // userScheduler.addTask(logServerTask);
  userScheduler.addTask(acknowledgementTask);
    _rootInitializationTask.enable();
    // logServerTask.enable();
}

void loop() {
  userScheduler.execute(); // it will run mesh scheduler as well
  mesh.update();
}

void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("logServer: Received from %u msg=%s\n", from, msg.c_str());
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(msg);
  if (root.containsKey("found")) {
      // if (String("logServer").equals(root["topic"].as<String>())) {
          // check for on: true or false
          // _foundTargetNodeId = root["found"];
          long foundTargetNodeId = root["found"];
          int channel = root["chan"];
          signed rssi = root["rssi"];
          mac = root["mac"];
          Serial.printf("[FOUND] %d | %s | %i | %d \n", foundTargetNodeId, mac, channel, rssi);

        if(std::find(_foundTargetNodeIds.begin(), _foundTargetNodeIds.end(), foundTargetNodeId) == _foundTargetNodeIds.end()) {
            _foundTargetNodeIds.push_back(foundTargetNodeId);
          // signed rssi = root["rssi"];
          Serial.printf("[ADDED]NODE %u ADDED TARGET\n", foundTargetNodeId);
        }

        //checks if it is diabled and re-enables it. if it is out of iterations AND disabled, restart
        if( !acknowledgementTask.enableIfNot()){
            acknowledgementTask.restart();
        }
  }
  if (root.containsKey("fin_ack")){
      _foundTargetNodeIds.erase(std::remove(_foundTargetNodeIds.begin(), _foundTargetNodeIds.end(), root["fin_ack"]), _foundTargetNodeIds.end());
  }
}
