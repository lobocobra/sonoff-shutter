//lobocobra modifications in this tab
/*
  xsns_91_relay.ino - Count the seconds your devices are ON

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



// #ifdef relay

/*********************************************************************************************\
   Translations
  \*********************************************************************************************/
// Translation of the new commands
// Commands Relay
#define D_CMND_relayReset         "relayReset"
#define D_CMND_relayOnDsec        "relayOnDsec"
#define D_CMND_relayDelayDsec     "relayDelayDsec"
#define D_CMND_relayStarts        "relayStarts"
#define D_CMND_relayDsecCons      "relayDsecCons"
#define D_CMND_relayCostFactor    "relayCostFactor"
#define D_CMND_relayUnitDivisor   "relayUnitDivisor"
#define D_CMND_relayUnitText      "relayUnitText"
#define D_CMND_relayCurrency      "relayCurrency"
#define D_CMND_relayConsumption   "relayConsumption"

#define D_Relay_Consumption       "Consumption"                    // used for MQTT


/*********************************************************************************************\
   Variables
\*********************************************************************************************/

enum RelayCommands { CMND_relayReset,CMND_relayOnDsec, CMND_relayDelayDsec, CMND_relayStarts, CMND_relayDsecCons, CMND_relayUnitDivisor, CMND_relayUnitText, CMND_relayConsumption, CMND_relayCostFactor, CMND_relayCurrency };
const char kRelayCommands[] PROGMEM = D_CMND_relayReset "|" D_CMND_relayOnDsec "|" D_CMND_relayDelayDsec "|" D_CMND_relayStarts "|" D_CMND_relayDsecCons "|" D_CMND_relayUnitDivisor "|" D_CMND_relayUnitText "|" D_CMND_relayConsumption "|" D_CMND_relayCostFactor "|" D_CMND_relayCurrency;

// Global variables
boolean MS100 = false;
unsigned long relayTimerMem[4]= {0UL,0UL,0UL,0UL};
unsigned long relayTimerDsec = 0UL;



/*********************************************************************************************\
    Procedures / Functions
  \*********************************************************************************************/
void RelayInit() {
  for (byte i = 0; i < devices_present; i++)  {
    //eliminate impossible values
    if (Settings.relayCostFactor[i]<=0)  Settings.relayCostFactor[i]= 1;
    if (Settings.relayUnitDivisor[i]<=0) Settings.relayUnitDivisor[i]= 1;
    if (Settings.relayDsecCons[i]<=0)  Settings.relayDsecCons[i]= 1;
    if(Settings.relayUnitText[i][0] == '\0')    strcpy(Settings.relayUnitText[i], "Dsec"); //check if empty
    if(Settings.relayCurrency[i][0] == '\0')strcpy(Settings.relayCurrency[i], "cost");
  }
}

boolean ExecutePowerUpdateRelayPos(byte device ) { // when a status is changing we count the ON seconds
  boolean result = true; //turn it false if you want to interrupt any start (over limit)
  // update current position of the relay after it is stopped, it is triggered by sonoff.ino when a power command is executed
  for (byte i = 1; i <= devices_present; i++) {
    byte mask = 0x01 << (i - 1); //set the mask to the right device
    if ((device == i) && ((power & mask) != mask ) ) {
        relayTimerMem[i-1] = relayTimerDsec;  //Power was OFF => we are about to start, so we save start time
    }
    if ((device == i) && (power & mask)) { //Power was ON => turned off, calculate new position and apply corrections
        // only increase starts and Dsec, if we were running longer than the device reaction time. Example, if it needs 9sec until the water is is coming out of your gardena and it stops after 8 sec, you do not have used wather.
        long RelayRuntime = relayTimerDsec - relayTimerMem[i-1];
        if (RelayRuntime > Settings.relayDelayDsec[i-1]) {
          Settings.relayStarts[i-1]++; // increase start counter as we were on longer than the delay is
          Settings.relayOnDsec[i-1] = Settings.relayOnDsec[i-1] + RelayRuntime - Settings.relayDelayDsec[i-1];  // add the time the relay was ON, but only if it did run longer than the delay
        } 
    ShowRelayPos(i,3); //shows also the status if the delay was bigger than the ON time
    MqttPublishPrefixTopic_P(STAT, PSTR(D_RSLT_STATE), Settings.flag.mqtt_sensor_retain); // we need to trigger the print! but show relay would double count the current running sec, so we need to avoid that by adapting the code. WHEN STAT, then no ADD-ON of running sec, see code in ShowRelay
    }
  }
  return result;  
}

void Relay_everySek() {
}

void Relay_every100ms() {
  // we need a counter to have 1/10 of a second exact time
  relayTimerDsec++;
}


void Relay_every50ms() {
  //execute every 2nd round 100ms
  if ( MS100 ^= true )  Relay_every100ms();
}


boolean MqttRelayCommand() //react ont received MQTT commands
{
  char command [CMDSZ];
  byte i = 0;
  byte index; // index
  boolean serviced = true;
  long    payload = -99L;
  double  payload_double = -99.0;
  char    payload_char[XdrvMailbox.data_len+1];  //create the string variable (+1 needed for termination "0")
      
 // snprintf_P(log_data, sizeof(log_data), PSTR("Shutter: Command Code: '%s', Payload: %d"), XdrvMailbox.topic, XdrvMailbox.payload);
 //  AddLog(LOG_LEVEL_DEBUG);

  int command_code = GetCommandCode(command, sizeof(command), XdrvMailbox.topic, kRelayCommands);
  index = XdrvMailbox.index;
  if (XdrvMailbox.data_len > 0 && XdrvMailbox.data_len <= 12 ) { //get the MQTT message and as the original value is INT only, we read the data ourselves
    strlcpy(payload_char,  XdrvMailbox.data, XdrvMailbox.data_len+1);
    payload_double = CharToDouble(payload_char); //use internal function as atof(), takes too much space
    payload = round(payload_double);
  } else payload = XdrvMailbox.payload;

  RelayInit(); // ensure that we init the values once a while, which is the case when we have commands
  if (-1 == command_code || index <= 0 || index > devices_present ) { // -1 means that no command is recorded with this value OR the INDEX is out of range or we have less devices than we address
    serviced = false;  // Unknown command
  }
  else if (CMND_relayReset == command_code) {
    if (payload == 0 )  relayTimerDsec = 0UL; // reset timer if 0 is provided
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{Last reset: Hour: %d, Min: %d, Sec: %d}"), (relayTimerDsec/36000), (relayTimerDsec/600)%60, (relayTimerDsec/10)%60 ); // show the time
    for (byte i = 0; i < devices_present; i++) { //we reset all run-data
      relayTimerMem[i] = 0UL; // reset all memory to 0
      Settings.relayStarts[i] = 0L;
      Settings.relayOnDsec[i] = 0L;
    }
  }
  else if (CMND_relayOnDsec == command_code) {
    if (payload >= 0)  Settings.relayOnDsec[index - 1] = payload;
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_LVALUE, command, index, Settings.relayOnDsec[index - 1] );
  }
  else if (CMND_relayDelayDsec == command_code) {
    if (payload >= 0 && payload <= 65000   ) Settings.relayDelayDsec[index - 1] = payload;
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_LVALUE, command, index, Settings.relayDelayDsec[index - 1] );
  }  
  else if (CMND_relayStarts == command_code) {
    if (payload >= 0  ) Settings.relayStarts[index - 1] = payload;
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_LVALUE, command, index, Settings.relayStarts[index - 1] );
  }
  else if (CMND_relayDsecCons == command_code) {
    if (payload_double >= 0 ) Settings.relayDsecCons[index - 1] = (double)payload_double;
    char buffer[16]; // Arduino does not support %f, so we have to use dtostrf
    snprintf_P(mqtt_data, sizeof(mqtt_data),PSTR("{\"%s%d\":%s}"), command, index, dtostrf(Settings.relayDsecCons[index - 1],XdrvMailbox.data_len+1,4,buffer) ); 
  }
  else if (CMND_relayCostFactor == command_code) {
    if (payload_double >= 0 ) Settings.relayCostFactor[index - 1] = (double)payload_double;
    char buffer[16]; // Arduino does not support %f, so we have to use dtostrf
    snprintf_P(mqtt_data, sizeof(mqtt_data),PSTR("{\"%s%d\":%s}"), command, index, dtostrf(Settings.relayCostFactor[index - 1],XdrvMailbox.data_len+1,4,buffer) ); 
  }  
  else if (CMND_relayUnitDivisor == command_code) {
    if (payload > 0 ) Settings.relayUnitDivisor[index - 1] = payload; //(must be at least 1, to avoid div error)
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_LVALUE, command, index, Settings.relayUnitDivisor[index - 1] );
  }
  else if ((CMND_relayUnitText == command_code) && (index > 0) && (index <= devices_present)) {
      if (XdrvMailbox.data_len > 0 && XdrvMailbox.data_len <= 8 ) {
        strlcpy(Settings.relayUnitText[index -1],  XdrvMailbox.data, XdrvMailbox.data_len+1);
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_SVALUE, command, index, Settings.relayUnitText[index -1]);
    }
  else if ((CMND_relayCurrency == command_code) && (index > 0) && (index <= devices_present)) {
      if (XdrvMailbox.data_len > 0 && XdrvMailbox.data_len <= 6 ) {
        strlcpy(Settings.relayCurrency[index -1],  XdrvMailbox.data, XdrvMailbox.data_len+1);
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_SVALUE, command, index, Settings.relayCurrency[index -1]);
    }    
  else if (CMND_relayConsumption == command_code) {
   ShowRelayPos(index,2);
  }    
  else { // no response for known command was defined, so we handle it as unknown
    serviced = false;  // Unknown command
  }
  return serviced;
}

 
void ShowRelayPos(byte relay_i, uint8_t type_i) { 
  // INDEX99 show all devices
  // TYPE 2) STAT 3) SENSOR
  byte index;
  byte start_R = relay_i;
  byte end_R  =  relay_i;

  // check if we want to show all devices or only 1 specific
  if (relay_i == 99) { start_R = 1; end_R = devices_present;}

  if (type_i > 0 && type_i <= 3) {
  for (byte i = start_R; i <= end_R; i++) {
    index =i;

   //check if power is currently ON, if yes, we add the current time, BUT we do not update the variables, we just show the current staus. The reason is that we only know at the end of a cycle, if the delaySTART was needed
   long RelayRuntime = 0;
    if (power & 1<<(i-1)){
      RelayRuntime = relayTimerDsec - relayTimerMem[i-1]-Settings.relayDelayDsec[index-1];
    }
    double relayconsumption = (double)(((Settings.relayOnDsec[index - 1]+ RelayRuntime)- (Settings.relayDelayDsec[index-1]*Settings.relayStarts[index-1]))*Settings.relayDsecCons[index - 1]) / Settings.relayUnitDivisor[index - 1];
    double relayconsumptioncost = (double)relayconsumption * Settings.relayCostFactor[index-1];
    int Rlength[2];
    if (relayconsumption >0) Rlength[0] = (int) (log10(relayconsumption) + 1); else Rlength[0] =1;
    if (relayconsumption >0) Rlength[1] = (int) (log10(relayconsumption) + 1); else Rlength[1] =1;
    char Rbuffer1[Rlength[0]+1+4]; // Arduino does not support %f, so we have to use dtostrf
    char Rbuffer2[Rlength[1]+1+4]; // Arduino does not support %f, so we have to use dtostrf

    // prepare the data you want to send !!! if you have less variables than definedin PSTR, your sonoff will crash on mqtt display
    snprintf_P(mqtt_data, sizeof(mqtt_data),PSTR("%s{\"%s%d %s\":%s,\"%s%d %s\":%s,\"%s%d %s\":%d, \"%s%d %s\":%d}"), 
        (relay_i == 99 && index == 1 || relay_i != 99) ? "": mqtt_data,
        D_Relay_Consumption, index, Settings.relayUnitText[index -1],   dtostrf(relayconsumption,Rlength[0]+1+414,4,Rbuffer1),
        D_Relay_Consumption, index, Settings.relayCurrency[index-1],dtostrf(relayconsumptioncost,Rlength[1]+1+4,2,Rbuffer2),
        D_Relay_Consumption, index, D_CMND_relayOnDsec, Settings.relayOnDsec[index - 1] + (type_i==3 ? 0: RelayRuntime),    //type_i, when we tele, then we need to add current values, if STAT, then we are launched by OFF trigger, so its not needed
        D_Relay_Consumption, index, D_CMND_relayStarts, Settings.relayStarts[index - 1] + (type_i==3 ? 0: (RelayRuntime>0)) //type_i, when we tele, then we need to add current values, if STAT, then we are launched by OFF trigger, so its not needed
        );
  } // we ned the "(relay_i == 99 && index == 1 || relay_i != 99) ? "": mqtt_data" part in order to prevent, that we also display the button status
   // trigger the mqtt send mechano
   //type_i = (type_i = 3) ? type_i=TELE : type_i=STAT; // I had issues with variables, now I ensure we have a valid item
   //MqttPublishPrefixTopic_P(type_i, PSTR(D_RSLT_STATE), Settings.flag.mqtt_sensor_retain); // before I needed it to start the print, since I fixed the problem with 2 outputs by moving xdrvcall down in the code, it is not needed (it would dupplicate prints)
  }//only print if we have a valid input
}

/*********************************************************************************************\
   Interface
\*********************************************************************************************/
#define XDRV_91

boolean Xdrv91(byte function){
  boolean result = false;
  
  if (Settings.flag3.relaymode) {
    switch (function) {
      case FUNC_INIT:
        RelayInit();
        break;
      case FUNC_JSON_APPEND:
        ShowRelayPos(99,2); //99 = show all relay
        MqttPublishPrefixTopic_P(TELE, PSTR(D_RSLT_STATE), Settings.flag.mqtt_sensor_retain); // after data is loaded to mqtt_data, we force the send out of the message
        break;
      case FUNC_COMMAND:
        result = MqttRelayCommand();
        break;
      case FUNC_EVERY_SECOND:
        Relay_everySek();
        break;
      case FUNC_EVERY_50_MSECOND:
        Relay_every50ms();
        break;        
#ifdef USE_WEBSERVER
      case FUNC_WEB_APPEND:
        ;
        break;
#endif  // USE_WEBSERVER
      case FUNC_SAVE_BEFORE_RESTART:
        ;
    }
  }
  return result;
}

//#endif  // shutter

