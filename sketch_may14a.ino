#include <HardwareSerial.h>

HardwareSerial RDM6300(2); // UART2: RX = GPIO16, TX = GPIO17

String currentTag = "";
String lastTag = "";
bool tagDetected = false;

unsigned long lastReadTime = 0;
unsigned long lastScanTime = 0;
const unsigned long scanCooldown = 600;  // minimal 1 detik antar pembacaan

void setup() {
  Serial.begin(115200);
  RDM6300.begin(9600, SERIAL_8N1, 16, 17); // RX, TX
  Serial.println("RDM6300 RFID Reader Ready...");
}

void loop() {
  static bool reading = false;
  static String buffer = "";

  while (RDM6300.available()) {
    char ch = RDM6300.read();

    lastReadTime = millis(); // Update waktu terakhir ada data masuk

    if (ch == 0x02) { // Start of text
      reading = true;
      buffer = "";
    } else if (ch == 0x03) { // End of text
      reading = false;
      if (buffer.length() >= 12) {
        String tag = buffer.substring(0, 10);
        String checksumStr = buffer.substring(10, 12);

        if (verifyChecksum(tag, checksumStr)) {
          currentTag = tag;

          // Cek jika tag baru dan sudah cukup lama sejak scan terakhir
          if (currentTag != lastTag && millis() - lastScanTime > scanCooldown) {
            lastScanTime = millis();
            Serial.print("Tag baru: ");
            Serial.println(currentTag);
            lastTag = currentTag;
            tagDetected = true;
          }
        } else {
          Serial.println("Checksum gagal");
        }
      }
    } else if (reading) {
      buffer += ch;
    }
  }

  // Jika sudah tidak ada data masuk selama 500ms, anggap tag dilepas
  if (tagDetected && millis() - lastReadTime > 500) {
    currentTag = "";
    lastTag = "";  // Reset agar tag yang sama bisa dibaca lagi nanti
    tagDetected = false;
    Serial.println("Tag dilepas");
  }
}

bool verifyChecksum(String tag, String checksumStr) {
  byte calcChecksum = 0;
  for (int i = 0; i < 10; i += 2) {
    byte value = strtoul(tag.substring(i, i + 2).c_str(), NULL, 16);
    calcChecksum ^= value;
  }
  byte receivedChecksum = strtoul(checksumStr.c_str(), NULL, 16);
  return calcChecksum == receivedChecksum;
}
