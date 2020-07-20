# GM_P2control

grafmarine Prototype V2 controler

Uses an ESP32 Lora module as the main processor to communicate with the test array and record / manange the atate of the array.

All recored data is pushed to the cloud via the GSM model or LoRa network interface

this controller uses a GPS GT-U7 module to lcate the contrller and a HMC5883 digital compass to detect the array heading. There is also a STEVAL-MKI136V1 gyroscope eval board to determine the orientation of the array when fitted to the test vessel.

The controller also incorporates a battary charger for a seconday battery supply.
