//lobocobra modifications in this tab
/*
  xsns_98_shutter.ino - Ooperate your shutter

  Add-on by lobocobra

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/



// #ifdef shutter

/*********************************************************************************************\
   Translations
  \*********************************************************************************************/
// Translation of the new commands
// Commands Shutter
#define D_CMND_shutPosDSec        "ShutPosDSec"
#define D_CMND_shutStartDelayDSec "ShutStartDelayDSec"
#define D_CMND_shutLagUpwardsDSec "ShutLagUpwardsDSec"
#define D_CMND_ShutMinDSec        "ShutMinDSec"
#define D_CMND_shutterMax_Sec     "ShutMaxDSec"
#define D_CMND_shutGoPercent      "ShutGoPercent"
#define D_CMND_ShutReset          "ShutReset"
#define D_CMND_shutHelp           "shutHelp"
#define D_SHUT_PERCENT1           "Position Shutter1 in %"
#define D_SHUT_PERCENT2           "Position Shutter2 in %"     
#define D_SHUTTER1                "SHUTTER1"                    // used for MQTT
#define D_SHUTTER2                "SHUTTER2"                    // used for MQTT


/*********************************************************************************************\
   Variables
  \*********************************************************************************************/

enum ShutterCommands { CMND_shutPosDSec, CMND_shutStartDelayDSec, CMND_shutLagUpwardsDSec, CMND_ShutMinDSec, CMND_shutMaxDSec, CMND_shutGoPercent, CMND_ShutReset, CMND_shutHelp };
const char kShutterCommands[] PROGMEM = D_CMND_shutPosDSec "|" D_CMND_shutStartDelayDSec "|" D_CMND_shutLagUpwardsDSec "|" D_CMND_ShutMinDSec "|" D_CMND_shutterMax_Sec "|" D_CMND_shutGoPercent "|" D_CMND_ShutReset "|" D_CMND_shutHelp;

const char JSON_SHUTTER_PERCENT[] PROGMEM = "%s,\"%s\":%d";

// Global variables
int shutDeciSecMem[2] = {0L};           // counts up the time all 1/10 seconds 0=endless timer, 1=save for device1 2=save for device2
int shutterPosRequested[2] = {-1 ,-1};  // stop shutter at this position
unsigned long shutDeciSecCounter = 0L;         // a counter to calculate how long the shutter did run, might be replaced by UTC
boolean MS100 = false;





/*********************************************************************************************\
    Procedures / Functions
  \*********************************************************************************************/
void ShutterInit() {
  for (byte i = 0; i < 2; i++)  {
    //eliminate impossible values
    if ( Settings.shutterPosMaxDeciSec[i] <= 0 || Settings.shutterPosMaxDeciSec[i] >= 3000) Settings.shutterPosMaxDeciSec[i] = 150;
    if (Settings.shutterPosMinDeciSec[i] >= round((float)Settings.shutterPosMaxDeciSec[i] / 2)  ) Settings.shutterPosMinDeciSec[i] = 0; // min should not be bigger than half of max
    if (Settings.shutterPosCurrentDeciSec[i] >= Settings.shutterPosMaxDeciSec[i] + 100)   Settings.shutterPosCurrentDeciSec[i] = Settings.shutterPosMaxDeciSec[i];
    if (Settings.shutterPosCurrentDeciSec[i] < 0 )                                        Settings.shutterPosCurrentDeciSec[i] = 0;
    if (Settings.shutterStartDelayDeciSec[i] >= round((float)Settings.shutterPosMaxDeciSec[i] / 4)  ) Settings.shutterStartDelayDeciSec[i] = 0;
    if (Settings.shutterLagUpwardsDeciSec[i] >= round((float)Settings.shutterPosMaxDeciSec[i] / 4)  ) Settings.shutterLagUpwardsDeciSec[i] = 0;
    if (Settings.shutterStdPulseTDsec[i] <=0 || Settings.shutterStdPulseTDsec[i] > Settings.shutterPosMaxDeciSec[i]) Settings.shutterStdPulseTDsec[i] = Settings.shutterPosMaxDeciSec[i];
  }
}

boolean ExecutePowerUpdateShutterPos(byte device ) { // when a status is changing update the shuter pos, return false, if we must stop the activation of the relay
// -------------------------
// react directly after butten is pressed // save the variable to calculate run time
// -------------------------
  int shutRuntime = 0;
  boolean result = true;
  // update current position of the shutter after it is stopped, it is triggered by sonoff.ino when a power command is executed
  for (byte i = 1; i <= devices_present; i++) {
    byte mask = 0x01 << (i - 1); //set the mask to the right device
    if ((device == i) && ((power & mask) != mask ) ) {

      // check if the shutter would go out boundaries and prevent start in such a case
      if ( (Settings.shutterPosCurrentDeciSec[(i > 2)]>= Settings.shutterPosMaxDeciSec[(i > 2)] &&  !(device & 0x01)) || (Settings.shutterPosCurrentDeciSec[(i > 2)]<= Settings.shutterPosMinDeciSec[(i > 2)] &&  device & 0x01)) 
        result = false; // check we we are outside or at boundaries and go into wrong direction, if yes ,stop movement
      else      
        shutDeciSecMem[(i > 2)] = shutDeciSecCounter;  //Power was OFF => we are about to start, so I save start time
    }
    // now check if the current device is the right one and already ON and will go off => then calculate position
    // odd=up
    if (device & 0x01)  {
      //odd=up
      if ((device == i) && (power & mask)) { //Power was ON => turned off, calculate new position and apply corrections
        // not needed currently, part of the old code, but might be usefull in future?
      }
    }
    else {
      //even=down
      if ((device == i) && (power & mask)) { //Power is ON => turn it off, calculate new position
        // not needed currently, part of the old code, but might be usefull in future? 
      }
    }
  }
   
  ShowShutterPos(round((float)device/2 ));
  return result;
}

void Shutter_everySek() {
  /*  if (power = 0){  // we reset the counter always if no power is on, so we do not have an overflow problem
      shutDeciSecCounter = 0L;
      shutDeciSecMem[0] = 0L;
      shutDeciSecMem[1] = 0L;
    }
    Serial.print("Power status:");Serial.print(power);Serial.print(" Mem is:");Serial.println(shutDeciSecMem[0]);          */
}

void Shutter_every100ms() {
  // --------------------------------------
  // all DeciSecond check if we are above or below limit and stop the motor if it is the case
  // --------------------------------------

  // we need a counter to have 1/10 of a second exact time
  shutDeciSecCounter++;

  // control current position and stop motor if we are outside min/max
  for (byte i = 0; i < devices_present; i++) {
    float upcoef;
    int shutX;
    int OneOrTwo;

    byte mask = 0x01 << (i); //set the mask to the right device
    if ( power & mask ) { // we found a relay that is on
      OneOrTwo = i * 2 > 2; // save time, we calculate once the device group
      // calculate how long we did run in DSEC and update the relayCounter if we run longer than the delay is
      shutX = shutDeciSecCounter - shutDeciSecMem[OneOrTwo]; 
      if (shutX > Settings.shutterStartDelayDeciSec[OneOrTwo]) { Settings.shutterRelayOnDsec[i]++; } // if a relay is longer on than the delay, we start toincrease the relaycounter

      // calculate coef depending we go up/down
      ((i & 1) == 1 ) ? upcoef = 1 : upcoef = (float)Settings.shutterPosMaxDeciSec[OneOrTwo] / (float)(Settings.shutterPosMaxDeciSec[OneOrTwo] +  Settings.shutterLagUpwardsDeciSec[OneOrTwo]);              // Coefficient between lag and down  
      // calculate current position
          Settings.shutterPosCurrentDeciSec[OneOrTwo] = Settings.shutterRelayOnDsec[OneOrTwo*2+1] - (int)((float)Settings.shutterRelayOnDsec[OneOrTwo*2 ] * upcoef)   ; // Calculate current position
      
      // check if we are all UP, if yes stop the motor
      if ((i & 1) == 0 )  { // we go up, a 0 or 2 (even) device was pushed if we do not use (int) then we have an overflow!!!
        // we go UP, now lets stop the motor if we are risking to go below minimum on the top
        if (Settings.shutterPosCurrentDeciSec[OneOrTwo] <= Settings.shutterPosMinDeciSec[OneOrTwo]) {
          ExecuteCommandPower(i + 1, 0, SRC_IGNORE);
          //set all help variables to 0 as we are all up, prevent overflow 
          Settings.shutterRelayOnDsec[(OneOrTwo)*2 ]   = 0;
          Settings.shutterRelayOnDsec[(OneOrTwo)*2+1]  = Settings.shutterPosMinDeciSec[OneOrTwo];
          shutterPosRequested[OneOrTwo] = -1 ;   // just in case
        } else if ( shutterPosRequested[OneOrTwo] > -1 &&  Settings.shutterPosCurrentDeciSec[OneOrTwo] <= shutterPosRequested[OneOrTwo] ) {
          ExecuteCommandPower(i + 1, 0, SRC_IGNORE);
          shutterPosRequested[OneOrTwo] = -1;
        }
      }
      else { //we go DOWN, and stop if we are over Max
        if (Settings.shutterPosCurrentDeciSec[OneOrTwo] >= Settings.shutterPosMaxDeciSec[OneOrTwo]) {
          //set all help variables to 0 as we are all up, prevent overflow 
          ExecuteCommandPower(i + 1, 0, SRC_IGNORE); 
          Settings.shutterRelayOnDsec[OneOrTwo*2+0 ] = 0;
          Settings.shutterRelayOnDsec[OneOrTwo*2+1]  = Settings.shutterPosMaxDeciSec[OneOrTwo]; 
          shutterPosRequested[OneOrTwo] = -1 ;    // just in case
        } else if ( shutterPosRequested[OneOrTwo] > -1 &&  Settings.shutterPosCurrentDeciSec[OneOrTwo] >= shutterPosRequested[OneOrTwo] ) {
          ExecuteCommandPower(i + 1, 0, SRC_IGNORE);
          shutterPosRequested[OneOrTwo] = -1;
        }
      }
      //!! The debug log delays the counting and we are off 2 seconds.... so
      AddLog_P2(LOG_LEVEL_DEBUG, PSTR("Power:%d, PosCur:%d, PosReq:%d, RelayOn0:%d, RelayOn1:%d Upcoef:%d, shutx:%d, count:%d, mem:%d, delay:%d"), i, Settings.shutterPosCurrentDeciSec[i * 2 > 2],shutterPosRequested[i*2>2],Settings.shutterRelayOnDsec[0], Settings.shutterRelayOnDsec[1],(int)(upcoef*100),shutX,shutDeciSecCounter,shutDeciSecMem[OneOrTwo],Settings.shutterStartDelayDeciSec[OneOrTwo] );
      //AddLog_P2(LOG_LEVEL_DEBUG, PSTR("Power:%d, PosCur:%d, PosReq:%d, RelayOn0:%d, RelayOn1:%d Upcoef:%d, lobo:%d, shutx:%d, count:%d, mem:%d, delay:%d"), i, Settings.shutterPosCurrentDeciSec[i * 2 > 2],shutterPosRequested[i*2>2],Settings.shutterRelayOnDsec[0], Settings.shutterRelayOnDsec[1],(int)(upcoef*100),lobotestpulsetime[1],shutX,shutDeciSecCounter,shutDeciSecMem[OneOrTwo],Settings.shutterStartDelayDeciSec[OneOrTwo] );
    }
  }
}

void Shutter_every50ms() {
  //execute every 2nd round 100ms
  //if ( MS100 ^= true )  Shutter_every100ms();
}

boolean MqttShutterCommand()
{
  char command [CMDSZ];
  uint16_t i = 0;
  uint16_t index; // index
  boolean serviced = true;
  boolean valid_entry = false;
  uint16_t payload = -99;

  snprintf_P(log_data, sizeof(log_data), PSTR("Shutter: Command Code: '%s', Payload: %d"), XdrvMailbox.topic, XdrvMailbox.payload);
  AddLog(LOG_LEVEL_DEBUG);
  int command_code = GetCommandCode(command, sizeof(command), XdrvMailbox.topic, kShutterCommands);
  index = XdrvMailbox.index;
  payload = XdrvMailbox.payload;
  ShutterInit(); // ensure that we init the values once a while, which is the case when we have commands

  //Serial.print("command_index:") ;Serial.print(XdrvMailbox.index) ;Serial.print(" CMND_ShutMinDSec:") ;Serial.print(CMND_ShutMinDSec) ;Serial.print(" Mailbox:") ;Serial.print(XdrvMailbox.topic) ;Serial.print(" MailboxPayload:") ;Serial.print(XdrvMailbox.payload) ;Serial.print(" command:") ;Serial.println(command) ;
  if (-1 == command_code || index <= 0 || index > 2 || index*2 > devices_present  ) { // -1 means that no command is recorded with this value OR the INDEX is out of range or we have less devices than we address
    serviced = false;  // Unknown command
  }
  else if (CMND_ShutMinDSec == command_code) {
    if (payload >= 0 && payload <= round((float)Settings.shutterPosMaxDeciSec[index - 1] / 2)  ) Settings.shutterPosMinDeciSec[index - 1] = payload;
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_LVALUE, command, index, Settings.shutterPosMinDeciSec[index - 1] );
  }
  else if (CMND_shutMaxDSec == command_code) {
    if (payload >= 0 && payload <= 3000 ) Settings.shutterPosMaxDeciSec[index - 1] = payload;
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_LVALUE, command, index, Settings.shutterPosMaxDeciSec[index - 1] );
  }
  else if (CMND_shutPosDSec == command_code) {
    if (payload >= 0 && payload <= 3000 ) Settings.shutterPosCurrentDeciSec[index - 1] = payload;
    //AddLog_P2(LOG_LEVEL_DEBUG, PSTR(">>>>>PosCur:%d, PosReq:%d, RelayOn0:%d, RelayOn1:%d count:%d, mem:%d"), Settings.shutterPosCurrentDeciSec[i * 2 > 2],shutterPosRequested[i*2>2],Settings.shutterRelayOnDsec[0], Settings.shutterRelayOnDsec[1],shutDeciSecCounter,shutDeciSecMem[1] );
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_LVALUE, command, index, Settings.shutterPosCurrentDeciSec[index - 1] );

  }
  else if (CMND_shutGoPercent == command_code) {
    if (payload >= 0 && payload <= 100 ) {
      ExecuteCommandPower(index * 2 - 1, 0, SRC_IGNORE);                 // stop the relay to ensure we do not run over the postion.
      ExecuteCommandPower(index * 2 - 0, 0, SRC_IGNORE);                 // stop the relay to ensure we do not run over the postion.

      // calculate and set desired position
      shutterPosRequested[index-1] = round(payload * (float)Settings.shutterPosMaxDeciSec[index - 1] / 100);

      // check if we are already on the position, if not, start the motor into the right direction (all 100ms, we will check position if shuterPosRequested is not -1)
      if ( shutterPosRequested[index-1] != Settings.shutterPosCurrentDeciSec[index - 1] ) {
        if ( shutterPosRequested[index-1] > Settings.shutterPosCurrentDeciSec[index - 1]) {
          // we must go down
          ExecuteCommandPower(index * 2 - 0, 1, SRC_IGNORE);                 // activate relay
        } else {
          // we must go up
          ExecuteCommandPower(index * 2 - 1, 1, SRC_IGNORE);                 // activate relay
        }
      } else {
          snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_LVALUE, command, index, round((float)Settings.shutterPosCurrentDeciSec[index - 1] / Settings.shutterPosMaxDeciSec[index - 1] * 100) ); // we are already there, prevent error, so we give out position
      }
     } else { //we give the position in %
        snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_LVALUE, command, index, round((float)Settings.shutterPosCurrentDeciSec[index - 1] / Settings.shutterPosMaxDeciSec[index - 1] * 100) );
    }
  }
  else if (CMND_shutStartDelayDSec == command_code) {
    if (payload >= 0 && payload <= round((float)Settings.shutterPosMaxDeciSec[index - 1] / 4)  ) Settings.shutterStartDelayDeciSec[index - 1] = payload;
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_LVALUE, command, index, Settings.shutterStartDelayDeciSec[index - 1] );
  }
  else if (CMND_shutLagUpwardsDSec == command_code) {
    if (payload >= 0 && payload <= round((float)Settings.shutterPosMaxDeciSec[index - 1] / 4)  ) Settings.shutterLagUpwardsDeciSec[index - 1] = payload;
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_LVALUE, command, index, Settings.shutterLagUpwardsDeciSec[index - 1] );
  }
  else if (CMND_ShutReset == command_code) {
      // position set to all down (so we can move up if something is wrong)
      Settings.shutterRelayOnDsec[(index-1)*2]   = 0;
      Settings.shutterRelayOnDsec[(index-1)*2+1] = Settings.shutterPosMaxDeciSec[(i *2 > 2)];
      // counters
      shutDeciSecCounter = 0;
      shutDeciSecMem[0] = 0;
      shutDeciSecMem[1] = 0;
      shutterPosRequested[index] = -1; // stop GoPercent
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_LVALUE, command, index, 1 );      
  }
  else if (CMND_shutHelp == command_code) {   
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, command, "ShutPosDSec, ShutStartDelayDSec, ShutLagUpwardsDSec, ShutMinDSec, ShutMaxDSec, ShutGoPercent, ShutReset, shutHelp" );      
  }
  else { // no response for known command was defined, so we handle it as unknown
    serviced = false;  // Unknown command
  }
  return serviced;
}


void ShowShutterPos(byte shutter) {

 //Shutter0: Position:% -15000 Position Sec: 150
 // FIX IT, not working currently
 //snprintf_P(log_data, sizeof(log_data), PSTR("Shutter%d: Position%% %d Position Sec: %d "), shutter,round((float)Settings.shutterPosCurrentDeciSec[shutter-1]/ Settings.shutterPosMaxDeciSec[shutter-1]*100) , Settings.shutterPosCurrentDeciSec[shutter-1]);
 AddLog(LOG_LEVEL_INFO);

 
}

void WebserverUpdate(bool json){
//  char ShutterWebPosition[33];
//  dtostrfd(round((float)Settings.shutterPosCurrentDeciSec[0] / Settings.shutterPosMaxDeciSec[0] * 100) ,        Settings.flag2.voltage_resolution, ShutterWebPosition);
  // dtostrfd(round((float)Settings.shutterPosCurrentDeciSec[0] / Settings.shutterPosMaxDeciSec[0] * 100),        Settings.flag2.voltage_resolution, voltage);
   snprintf_P(mqtt_data, sizeof(mqtt_data), JSON_SHUTTER_PERCENT, mqtt_data, D_SHUT_PERCENT1, round((float)Settings.shutterPosCurrentDeciSec[0]/ Settings.shutterPosMaxDeciSec[0]*100) );
}


/*********************************************************************************************\
   Interface
\*********************************************************************************************/
#define XDRV_91 91

bool Xdrv91(byte function){
  bool result = false;
  
  if (Settings.flag3.shuttermode) {
    switch (function) {
      case FUNC_INIT:
        ShutterInit();
        break;
      case FUNC_JSON_APPEND:
      // {"Time":"2018-07-17T23:06:46","Position Shutter1 in %":49,"Position Shutter2 in %":30}
        snprintf_P(mqtt_data, sizeof(mqtt_data), JSON_SHUTTER_PERCENT, mqtt_data, D_SHUT_PERCENT1, round((float)Settings.shutterPosCurrentDeciSec[0]/ Settings.shutterPosMaxDeciSec[0]*100) ); // it does what it says (Json append), it adds it to mqtt
        if (devices_present>2 ) snprintf_P(mqtt_data, sizeof(mqtt_data), JSON_SHUTTER_PERCENT, mqtt_data, D_SHUT_PERCENT2, round((float)Settings.shutterPosCurrentDeciSec[1]/ Settings.shutterPosMaxDeciSec[1]*100) );
        //ShowShutterPos(0);
        break;
      case FUNC_COMMAND:
        result = MqttShutterCommand();
        break;
      case FUNC_EVERY_SECOND:
        //Shutter_everySek();
        break;
      case FUNC_EVERY_100_MSECOND:
        Shutter_every100ms();
        break;        
      case FUNC_EVERY_50_MSECOND:
        //Shutter_every50ms();
        break;        
      case FUNC_SAVE_BEFORE_RESTART:
        ;
//#ifdef USE_WEBSERVER
//      case FUNC_WEB_APPEND:
//         WebserverUpdate(1);
//        break;
//#endif  // USE_WEBSERVER
    }
  }
  return result;
}

//#endif  // shutter

