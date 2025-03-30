#ifndef PINOUT_H
#define PINOUT_H

// --- DEFINICE PINŮ ---------------------------------------
// ASIAR DHT11
#define DHTPIN 2          // digitální pin DHT11
#define DHTTYPE DHT21     // druh senzoru DHT

// BMP280
#define BMP_SDA A4           // SDA pro BMP280
#define BMP_SCL A5           // SCL pro BMP280

// PZEM-004T
#define PZEM_RX 1         // RX pro UART s PZEM-004T
#define PZEM_TX 0         // TX pro UART s PZEM-004T

// PH-450C
#define PH_SENSOR A0      // analogový pin pro pH senzor

// TDS
#define TDS_SENSOR A1     // analogový pin pro TDS

// DS18B20
#define DS18B20 3         // digitální pin pro DS18B20

// RELÉ
#define RELAY_1 13
#define RELAY_2 12
#define RELAY_3 11
#define RELAY_4 10

// H-můstek
#define HBRIDGE_IN1 9
#define HBRIDGE_IN2 8
#define HBRIDGE_IN3 7
#define HBRIDGE_IN4 6

// --- DEFINICE FUNKCÍ PINŮ ---------------------------------------

#define RELAY_LED RELAY_1
#define RELAY_OUTFAN RELAY_3
#define RELAY_INFAN RELAY_4
#define RELAY_PUMP RELAY_2

#define H_BRIDGE_PH_PLUS HBRIDGE_IN1
#define H_BRIDGE_PH_MINUS HBRIDGE_IN2

#define H_BRIDGE_TDS_A HBRIDGE_IN3
#define H_BRIDGE_TDS_B HBRIDGE_IN4

#endif
