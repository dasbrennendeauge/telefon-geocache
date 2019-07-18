/**************************************************************
 *
 * TinyGSM Getting Started guide:
 *   https://tiny.cc/tinygsm-readme
 *
 * NOTE:
 * Some of the functions may be unavailable for your modem.
 * Just comment them out.
 *
 **************************************************************/

// Select your modem:
#define TINY_GSM_MODEM_SIM800

// Define the serial console for debug prints, if needed
#define TINY_GSM_DEBUG SerialMon

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial

// or Software Serial on Uno, Nano
#include <SoftwareSerial.h>
SoftwareSerial SerialAT(8,7); // RX, TX

/*
 * Test enabled
 */
#define TINY_GSM_TEST_GPRS true
#define TINY_GSM_TEST_CALL true
#define TINY_GSM_POWERDOWN true

// set GSM PIN, if any
#define GSM_PIN ""

// Set phone numbers, if you want to test SMS and Calls
#define CALL_TARGET "+49XXXXXXXXXXXX"

// Your GPRS credentials
// Leave empty, if missing user or pass
const char apn[]  = "sipgate";
const char gprsUser[] = "sipgate";
const char gprsPass[] = "sipgate";

#include <TinyGsmClient.h>

TinyGsm modem(SerialAT);

void setup() {
  // Set console baud rate
  SerialMon.begin(115200);
  delay(10);

  DBG("Wait...");

  SerialAT.begin(19200);
  delay(3000);
}

void loop() {

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  DBG("Initializing modem...");
  // if (!modem.restart()) {
  if (!modem.init()) {
    DBG("Failed to restart modem, delaying 10s and retrying");
    delay(3000);
    return;
  }

  String name = modem.getModemName();
  DBG("Modem Name:", name);

  String modemInfo = modem.getModemInfo();
  DBG("Modem Info:", modemInfo);

#if TINY_GSM_TEST_GPRS
  // Unlock your SIM card with a PIN if needed
  if ( GSM_PIN && modem.getSimStatus() != 3 ) {
    modem.simUnlock(GSM_PIN);
  }
#endif

#if TINY_GSM_TEST_WIFI
  DBG("Setting SSID/password...");
  if (!modem.networkConnect(wifiSSID, wifiPass)) {
    DBG(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" OK");
#endif

#if TINY_GSM_TEST_GPRS && defined TINY_GSM_MODEM_XBEE
  // The XBee must run the gprsConnect function BEFORE waiting for network!
  modem.gprsConnect(apn, gprsUser, gprsPass);
#endif

  DBG("Waiting for network...");
  if (!modem.waitForNetwork()) {
    delay(10000);
    return;
  }

  if (modem.isNetworkConnected()) {
    DBG("Network connected");
  }

#if TINY_GSM_TEST_GPRS
  DBG("Connecting to", apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    delay(10000);
    return;
  }

  bool res = modem.isGprsConnected();
  DBG("GPRS status:", res ? "connected" : "not connected");

  String ccid = modem.getSimCCID();
  DBG("CCID:", ccid);

  String imei = modem.getIMEI();
  DBG("IMEI:", imei);

  String cop = modem.getOperator();
  DBG("Operator:", cop);

  IPAddress local = modem.localIP();
  DBG("Local IP:", local);

  int csq = modem.getSignalQuality();
  DBG("Signal quality:", csq);

  // This is only supported on SIMxxx series
  String gsmLoc = modem.getGsmLocation();
  DBG("GSM location:", gsmLoc);

  // This is only supported on SIMxxx series
  String gsmTime = modem.getGSMDateTime(DATE_TIME);
  DBG("GSM Time:", gsmTime);
  String gsmDate = modem.getGSMDateTime(DATE_DATE);
  DBG("GSM Date:", gsmDate);
#endif

#if TINY_GSM_TEST_CALL && defined(CALL_TARGET)
  DBG("Calling:", CALL_TARGET);

  res = modem.callNumber(CALL_TARGET);
  DBG("Call:", res ? "OK" : "fail");

  int i = 0;
  while ((getCallStatus() == 4) || i == 10) {
    DBG("Rufaufbau");
    i++;
    delay(1000L);
  }

/*
  values of return:
 0 Ready (MT allows commands from TA/TE)
 2 Unknown (MT is not guaranteed to respond to tructions)
 3 Ringing (MT is ready for commands from TA/TE, but the ringer is active)
 4 Call in progress
*/
  
  if (res) {
    delay(10000L);

    res = modem.callHangup();
    DBG("Hang up:", res ? "OK" : "fail");
  }
#endif

#if TINY_GSM_TEST_BATTERY
  uint8_t chargeState = -99;
  int8_t percent = -99;
  uint16_t milliVolts = -9999;
  modem.getBattStats(chargeState, percent, milliVolts)
  DBG("Battery charge state:", chargeState);
  DBG("Battery charge 'percent':", percent);
  DBG("Battery voltage:", milliVolts / 1000.0F);

  float temp = modem.getTemperature();
  DBG("Chip temperature:", temp);
#endif

#if TINY_GSM_TEST_GPRS
  modem.gprsDisconnect();
  if (!modem.isGprsConnected()) {
    DBG("GPRS disconnected");
  } else {
    DBG("GPRS disconnect: Failed.");
  }
#endif

#if TINY_GSM_POWERDOWN
  // Try to power-off (modem may decide to restart automatically)
  // To turn off modem completely, please use Reset/Enable pins
  modem.poweroff();
  DBG("Poweroff.");
#endif

  // Do nothing forevermore
  while (true) {
    modem.maintain();
  }
}

int getCallStatus() {  
  modem.sendAT(GF("+CPAS"));
  if (modem.waitResponse(2000L, GF(GSM_NL "+CPAS: ")) != 1) {
        DBG("Nix bekommen");
        return -1;
  }
  int callStatus = SerialAT.readStringUntil('\n').toInt();
  modem.waitResponse();
  DBG(callStatus);  
  return callStatus;
}

