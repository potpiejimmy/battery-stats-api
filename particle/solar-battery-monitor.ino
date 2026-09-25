// This #include statement was automatically added by the Particle IDE.
#include <TM1637Display.h>

#include "Particle.h"

SYSTEM_THREAD(DISABLED);

const size_t SCAN_RESULT_MAX = 10;
const size_t RESPONSE_MESSAGE_SIZE = 34;

BleScanResult scanResults[SCAN_RESULT_MAX];

BleCharacteristic readBattery;
BleCharacteristic writeBattery;
BleCharacteristic solar002;
BleCharacteristic solar003;
BleCharacteristic solar004;

char printbuf[100];
uint8_t dataReceived[RESPONSE_MESSAGE_SIZE];
int dataReceivedLen = 0;

short watts;
short permill;

int writeNextInNSeconds = 0;
int publishStatsToCloud = 0;

// --- DISPLAY

#define D1CLK D8
#define D1DIO D7
#define D2CLK D6
#define D2DIO D5

TM1637Display displaySoC(D1CLK, D1DIO);
TM1637Display displayAmp(D2CLK, D2DIO);

void setup() {
    
	BLE.on();

#if SYSTEM_VERSION == SYSTEM_VERSION_v310
	// This is required with 3.1.0 only
	BLE.setScanPhy(BlePhy::BLE_PHYS_AUTO);
#endif

    readBattery.onDataReceived(onDataReceivedBms, NULL);
    solar002.onDataReceived(onDataReceivedSolar, NULL);
    solar003.onDataReceived(onDataReceivedSolar, NULL);
    solar004.onDataReceived(onDataReceivedSolar, NULL);
    
    // -- DISPLAY --
    displaySoC.setBrightness(0x0a);
    displayAmp.setBrightness(0x0a);
    
    // --- Cloud function
    Particle.function("stats", getBatteryStats);
}

void loop() {
    
	if (BLE.connected()) {
	    if (!writeNextInNSeconds) {
    		// We're currently connected to a sensor
            //Particle.publish("already connected, now writing", String::format("received %d", dataReceived));
            dataReceivedLen = 0;
            
            const uint8_t requestBatteryInfo[] = { 0xDD,0xA5,0x03,0x00,0xFF,0xFD,0x77 };
            writeBattery.setValue(requestBatteryInfo, 7);
/*
            const uint8_t requestSolarInfo1[] = { 0xF9,0x41 };
            solar002.setValue(requestSolarInfo1, 2);
            const uint8_t requestSolarInfo2[] = { 0x03, 0x00 };
            solar003.setValue(requestSolarInfo2, 2);
*/            
            writeNextInNSeconds = 1; // fetch data again in 1 second
	    } else {
	        // count down the seconds
	        delay(1000);
	        writeNextInNSeconds--;
	    }
	} else {
	    
		// We are not connected to a sensor, scan for one
		int count = BLE.scan(scanResults, SCAN_RESULT_MAX);

        Particle.publish("scan results", String::format("Found %d devices.", count));
    
	    for (int ii = 0; ii < count; ii++) {
			uint8_t advertisingData[BLE_MAX_ADV_DATA_LEN+1];
			uint8_t scanResponse[BLE_MAX_ADV_DATA_LEN+1];
			advertisingData[BLE_MAX_ADV_DATA_LEN] = 0;
			scanResponse[BLE_MAX_ADV_DATA_LEN] = 0;

			memcpy(advertisingData, &scanResults[ii].advertisingData(), BLE_MAX_ADV_DATA_LEN);
			memcpy(scanResponse, &scanResults[ii].scanResponse(), BLE_MAX_ADV_DATA_LEN);
			
			char addrStr[18];
			strcpy(addrStr, String::format("%02X:%02X:%02X:%02X:%02X:%02X",
					scanResults[ii].address()[0], scanResults[ii].address()[1], scanResults[ii].address()[2],
					scanResults[ii].address()[3], scanResults[ii].address()[4], scanResults[ii].address()[5]).c_str());

//			Particle.publish("device found", String::format("rssi=%d advertisingData=%s scanResponse=%s address=%s",
//					scanResults[ii].rssi(),
//					advertisingData,
//					scanResponse,
//					addrStr));
					
			const char* PEER_ADDRESS_BMS   = "65:30:49:38:C1:A4";	// e018 (BMS)	 // found 8 characteristics
			//const char* PEER_ADDRESS_LED = "B8:00:00:C3:22:B0";	// QHM-00B8  // LED?
			const char* PEER_ADDRESS_SOLAR = "D8:60:F8:82:43:D8";	// SmartSolar HQ2046ANR4Q	
					
			if (!strcmp(addrStr, PEER_ADDRESS_BMS)) {
			    
                BlePeerDevice peer = BLE.connect(scanResults[ii].address());
                if (peer.connected()) {
                    Particle.publish("connected", String::format("Connected to %s", addrStr));
                    
                    // 0=00002A00-0000-1000-8000-00805F9B34FB
                    // 1=00002A01-0000-1000-8000-00805F9B34FB
                    // 2=00002A04-0000-1000-8000-00805F9B34FB
                    // 3=00002A05-0000-1000-8000-00805F9B34FB
                    // 4=00002A50-0000-1000-8000-00805F9B34FB
                    // 5=0000FF01-0000-1000-8000-00805F9B34FB  "DM SPP: Module->Pho" (read)
                    // 6=0000FF02-0000-1000-8000-00805F9B34FB  "DM SPP: Phone->Modu" (write)
                    // 7=00000000-0000-0000-0000-000000000012  "OTA"
                    
                    // Note: getCharacteristicByUUID does not seem to work, always returns false
                                                                              
                    //bool bResult = peer.getCharacteristicByUUID(readBattery, BleUuid("0000FF01-0000-1000-8000-00805F9B34FB"));
                    //Particle.publish("got characteristic", String::format("found=%02X, desc=%s", bResult, readBattery.description().c_str()));
                    //printCharacteristicInfo(99, readBattery);

                    bool bResult = peer.getCharacteristicByDescription(readBattery, "DM SPP: Module->Pho");
//                    Particle.publish("got characteristic", String::format("found=%02X, desc=%s", bResult, readBattery.description().c_str()));

                    bResult = peer.getCharacteristicByDescription(writeBattery, "DM SPP: Phone->Modu");
//                    Particle.publish("got characteristic", String::format("found=%02X, desc=%s", bResult, writeBattery.description().c_str()));

    			} else {
    				Log.info("connection failed");
    			}
/*    			
			} else if (!strcmp(addrStr, PEER_ADDRESS_SOLAR)) {
                BlePeerDevice peer = BLE.connect(scanResults[ii].address());
                if (peer.connected()) {
                    Particle.publish("connected", "Connected to SmartSolar");
                    
                    // 11=306B0002-B081-4037-83DC-E59FCC3CDFD0 ()
                    // 12=306B0003-B081-4037-83DC-E59FCC3CDFD0 ()
                    // 13=306B0004-B081-4037-83DC-E59FCC3CDFD0 ()

                    bool bResult = peer.getCharacteristicByUUID(solar002, BleUuid("306B0002-B081-4037-83DC-E59FCC3CDFD0"));
                    bResult = peer.getCharacteristicByUUID(solar003, BleUuid("306B0003-B081-4037-83DC-E59FCC3CDFD0"));
                    bResult = peer.getCharacteristicByUUID(solar004, BleUuid("306B0004-B081-4037-83DC-E59FCC3CDFD0"));
//                    Particle.publish("solar003", String::format("found=%02X, desc=%s", bResult, solar003.description().c_str()));
//                    printCharacteristicInfo(99, solar003);

                    // init sequence
                    const uint8_t solarInit1[] = { 0xfa, 0x80, 0xff };
                    solar002.setValue(solarInit1, 3);
                    const uint8_t solarInit2[] = { 0xf9, 0x80 };
                    solar002.setValue(solarInit2, 2);
                    const uint8_t solarInit3[] = { 0x01 };
                    solar003.setValue(solarInit3, 1);
                    const uint8_t solarInit4[] = { 0x03, 0x00 };
                    solar003.setValue(solarInit4, 2);
                    const uint8_t solarInit5[] = { 0x06, 0x00, 0x82, 0x18, 0x93, 0x42, 0x10, 0x27, 0x03, 0x01, 0x03, 0x03 };
                    solar003.setValue(solarInit5, 12);
                    const uint8_t solarInit6[] = { 0x05, 0x00, 0x81, 0x19, 0xec, 0x0f, 0x05, 0x00, 0x81, 0x19, 0xec, 0x0e, 0x05, 0x00, 0x81, 0x19, 0x01, 0x0c, 0x05, 0x00 };
                    solar004.setValue(solarInit6, 20);
                    const uint8_t solarInit7[] = { 0x81, 0x18, 0x90, 0x05, 0x00, 0x81, 0x19, 0xec, 0x3f, 0x05, 0x00, 0x81, 0x19, 0xec, 0x12 };
                    solar003.setValue(solarInit7, 15);
                    const uint8_t solarInit8[] = { 0x19, 0xec, 0xdc, 0x05, 0x03, 0x81, 0x19, 0xec, 0xeb, 0x05, 0x03, 0x81, 0x19, 0xec, 0xed };
                    solar003.setValue(solarInit8, 15);
                    // read all characteristics
//                    BleCharacteristic allChars[32];
                    
//                    ssize_t charCount = peer.discoverAllCharacteristics(allChars, 32);
                    
//                    for (int cc = 0; cc < charCount; cc++) {
//                        printCharacteristicInfo(cc, allChars[cc]);
//                        delay(300);
//                    }
                } */
            }
		}
    }
}

void onDataReceivedBms(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context) {

    memcpy(dataReceived + dataReceivedLen, data, len);
    dataReceivedLen += len;
    
    if (dataReceivedLen == RESPONSE_MESSAGE_SIZE) {
//        btox(printbuf, dataReceived, dataReceivedLen*2);
//        printbuf[dataReceivedLen*2] = 0;
//        Particle.publish("received data", String::format("Received %d bytes: %s", dataReceivedLen, printbuf));
        
        short vbat = (dataReceived[4] << 8 | dataReceived[5]); // 100th of V, cV
        short ibat = (dataReceived[6] << 8 | dataReceived[7]); // 100th of A, cA
        short ahrem = (dataReceived[8] << 8 | dataReceived[9]);
        short ahful = (dataReceived[10] << 8 | dataReceived[11]);
        
        watts = -round((((float) vbat) * ibat) / 1000);  // 10th of Watt, dW, to fit in signed short
        permill = round((((float) ahrem) * 1000) / ahful);
        
        displaySoC.showNumberDec(round(((float)permill)/10), false, 4, 0);
        
        if (watts < 0) { /* charging */
            const uint8_t minusSign[] = { 0b01000000 };
            displayAmp.setSegments(minusSign, 1, 0);
            displayAmp.showNumberDec(round(((float)-watts)/10), false, 3, 1);
        } else { /* draining */
            displayAmp.showNumberDec(round(((float)watts)/10), false, 4, 0);
        }
    
        if (!publishStatsToCloud) {
            Particle.publish("socbat", String::format("%.1f", ((float)permill)/10));
            Particle.publish("pbat", String::format("%.1f", ((float)watts)/10));
            Particle.publish("battery stats", String::format("SoC = %.1f %%, P = %.1f W, Ibat = %.2f A, Vbat = %hd mV", ((float)permill)/10, ((float)watts)/10, ((float)ibat)/100, vbat));
            publishStatsToCloud = 174; // publish only every 174 (minimum 1)     
        }
        publishStatsToCloud--;
    }
}


void onDataReceivedSolar(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context) {

    Particle.publish("received solar", String::format("Received %d bytes.", len));
//    btox(printbuf, data, len*2);
//    printbuf[len*2] = 0;
//    Particle.publish("received solar", String::format("Received %d bytes: %s", len, printbuf));
        
}

void printCharacteristicInfo(int index, BleCharacteristic& cx) {
    BleUuid uido = cx.UUID();
    uint8_t uuid[BLE_SIG_UUID_128BIT_LEN];
    uido.rawBytes(uuid);
    
    Particle.publish("UUID", String::format("%d=%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X (%s), Prop = %02X", index,
        uuid[15],uuid[14],uuid[13],uuid[12],uuid[11],uuid[10],uuid[9],uuid[8],
        uuid[7],uuid[6],uuid[5],uuid[4],uuid[3],uuid[2],uuid[1],uuid[0],
        cx.description().c_str(),
        cx.properties()));
}

void btox(char *xp, const unsigned char *bb, int n) 
{
    const char xx[]= "0123456789ABCDEF";
    while (--n >= 0) xp[n] = xx[(bb[n>>1] >> ((1 - (n&1)) << 2)) & 0xF];
}

int getBatteryStats(String command)
{
    return (permill << 16) | (watts & 0xffff);
}
