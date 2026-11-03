// // put function declarations here:
// int myFunction(int, int);

// void setup() {
//   // put your setup code here, to run once:
//   Serial.begin(9600);
//   int result = myFunction(2, 3);
// }

// void loop() {
//   Serial.println("Hello world!");
//   delay(1000);
//   // put your main code here, to run repeatedly:
// }

// // put function definitions here:
// int myFunction(int x, int y) {
//   return x + y;
// }


// #include <WiFi.h>
// #include <cstring>

// // ---------- Configuration ----------

// const char* WIFI_SSID = "Marvel";
// const char* WIFI_PASSWORD = "Ironman$3000";

// const char* VM_HOST = "172.30.60.17";
// const uint16_t VM_PORT = 23;

// const char* VM_USER = "karthik";
// const char* VM_PASSWORD = "123";

// // These must match the prompts displayed by your VM.
// const char* LOGIN_PROMPT = "login:";
// const char* PASSWORD_PROMPT = "Password:";
// const char* SHELL_PROMPT = "$ ";

// // Command executed on the VM after login.
// const char* COMMAND = "pwd";

// WiFiClient client;

// // ---------- Telnet helpers ----------

// // Read one byte within an overall timeout.
// // Returns -1 if the connection closes or the timeout expires.
// int readByte(uint32_t start, uint32_t timeoutMs) {
//   while (millis() - start < timeoutMs) {
//     if (client.available()) {
//       return client.read();
//     }

//     if (!client.connected()) {
//       return -1;
//     }

//     delay(1);
//   }

//   return -1;
// }

// // Process Telnet control sequences.
// // Returns:
// //   0..255 : received text byte
// //   -2     : handled a Telnet control sequence
// //   -1     : connection closed or timeout
// int readTelnetByte(uint32_t start, uint32_t timeoutMs) {
//   int value = readByte(start, timeoutMs);

//   if (value < 0 || value != 255) {
//     return value;
//   }

//   // 255 is the Telnet IAC control prefix.
//   int command = readByte(start, timeoutMs);

//   if (command < 0) {
//     return -1;
//   }

//   if (command == 255) {
//     return 255;  // Escaped data byte.
//   }

//   // WILL=251, WONT=252, DO=253, DONT=254.
//   if (command >= 251 && command <= 254) {
//     int option = readByte(start, timeoutMs);

//     if (option < 0) {
//       return -1;
//     }

//     // Decline optional features:
//     // DO -> WONT
//     // WILL -> DONT
//     if (command == 253 || command == 251) {
//       uint8_t reply[] = {
//         255,
//         static_cast<uint8_t>(command == 253 ? 252 : 254),
//         static_cast<uint8_t>(option)
//       };

//       if (client.write(reply, sizeof(reply)) != sizeof(reply)) {
//         return -1;
//       }
//     }
//   } else if (command == 250) {
//     // Skip subnegotiation until IAC SE.
//     while (true) {
//       int next = readByte(start, timeoutMs);

//       if (next < 0) {
//         return -1;
//       }

//       if (next == 255) {
//         int after = readByte(start, timeoutMs);

//         if (after < 0) {
//           return -1;
//         }

//         if (after == 240) {
//           break;
//         }
//       }
//     }
//   }

//   return -2;
// }

// // Wait for specific text, optionally printing received output.
// bool waitFor(
//     const char* expected,
//     uint32_t timeoutMs,
//     bool showOutput
// ) {
//   String tail;
//   const size_t expectedLength = strlen(expected);
//   const uint32_t start = millis();

//   while (millis() - start < timeoutMs) {
//     int value = readTelnetByte(start, timeoutMs);

//     if (value == -1) {
//       break;
//     }

//     if (value == -2 || value == 0) {
//       continue;
//     }

//     if (showOutput) {
//       Serial.write(static_cast<uint8_t>(value));
//     }

//     // Keep only enough text to detect the expected prompt.
//     tail += static_cast<char>(value);

//     if (tail.length() > expectedLength) {
//       tail.remove(0, tail.length() - expectedLength);
//     }

//     if (tail == expected) {
//       return true;
//     }
//   }

//   Serial.print("\nConnection closed or timeout waiting for: ");
//   Serial.println(expected);

//   return false;
// }

// // Send text followed by Enter.
// bool sendLine(const char* text) {
//   if (client.print(text) != strlen(text) ||
//       client.print("\r\n") != 2) {
//     Serial.println("Send failed");
//     return false;
//   }

//   return true;
// }

// // ---------- Login and execute ----------

// bool runSession() {
//   Serial.println("Waiting for login prompt...");

//   if (!waitFor(LOGIN_PROMPT, 15000, false)) {
//     return false;
//   }

//   if (!sendLine(VM_USER)) {
//     return false;
//   }

//   Serial.println("Waiting for password prompt...");

//   if (!waitFor(PASSWORD_PROMPT, 15000, false)) {
//     return false;
//   }

//   if (!sendLine(VM_PASSWORD)) {
//     return false;
//   }

//   Serial.println("Waiting for Ubuntu shell...");

//   if (!waitFor(SHELL_PROMPT, 15000, false)) {
//     return false;
//   }

//   Serial.println("Logged in successfully");
//   Serial.print("Executing: ");
//   Serial.println(COMMAND);

//   if (!sendLine(COMMAND)) {
//     return false;
//   }

//   Serial.println("--- VM output ---");

//   if (!waitFor(SHELL_PROMPT, 30000, true)) {
//     return false;
//   }

//   return sendLine("exit");
// }

// // ---------- Arduino entry points ----------

// void setup() {
//   Serial.begin(115200);

//   // Allow USB serial to become ready without waiting forever.
//   uint32_t start = millis();

//   while (!Serial && millis() - start < 3000) {
//     delay(10);
//   }

//   delay(500);

//   Serial.println("\nConnecting to Wi-Fi...");

//   WiFi.mode(WIFI_STA);
//   WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

//   start = millis();

//   while (WiFi.status() != WL_CONNECTED) {
//     if (millis() - start >= 20000) {
//       Serial.println(
//         "Wi-Fi timeout: check SSID, password and 2.4 GHz network"
//       );
//       return;
//     }

//     delay(200);
//   }

//   Serial.print("ESP32 IP: ");
//   Serial.println(WiFi.localIP());

//   Serial.print("Connecting to VM: ");
//   Serial.println(VM_HOST);

//   if (!client.connect(VM_HOST, VM_PORT, 5000)) {
//     Serial.println(
//       "TCP connection failed: check VM IP, port 23 and firewall"
//     );
//     return;
//   }

//   bool success = runSession();

//   client.stop();

//   Serial.println(
//     success ? "\nSession finished" : "\nSession failed"
//   );
// }

// void loop() {
//   // Runs once per reset.
//   delay(100);
// }
#include <Arduino.h>
#include <WiFi.h>
#include <cstring>

// ---------- Configuration ----------

// const char* WIFI_SSID = "YOUR_WIFI_NAME";
// const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// const char* VM_HOST = "172.30.60.17";
// const uint16_t VM_PORT = 23;

// const char* VM_USER = "karthik";
// const char* VM_PASSWORD = "YOUR_UBUNTU_PASSWORD";

// const char* LOGIN_PROMPT = "login:";
// const char* PASSWORD_PROMPT = "Password:";
// const char* SHELL_PROMPT = "$ ";

const char* WIFI_SSID = "connect";
const char* WIFI_PASSWORD = "karthik123";

const char* VM_HOST = "10.197.236.13";
const uint16_t VM_PORT = 23;

const char* VM_USER = "karthik";
const char* VM_PASSWORD = "123";

// These must match the prompts displayed by your VM.
const char* LOGIN_PROMPT = "login:";
const char* PASSWORD_PROMPT = "Password:";
const char* SHELL_PROMPT = "$ ";

// Bash executes this command on the Ubuntu VM.
// const char* COMMAND = "bash -c 'pwd'";

const char* COMMAND ="wsl.exe -d Ubuntu --exec bash -lc \"pwd; whoami\"";

WiFiClient client;

// ---------- Telnet helpers ----------

// Read one byte within the operation's overall timeout.
int readByte(uint32_t start, uint32_t timeoutMs) {
  while (millis() - start < timeoutMs) {
    if (client.available()) {
      return client.read();
    }

    if (!client.connected()) {
      return -1;
    }

    delay(1);
  }

  return -1;
}

// Returns:
//   0..255 : received text byte
//   -2     : handled Telnet control information
//   -1     : timeout or connection closed
int readTelnetByte(uint32_t start, uint32_t timeoutMs) {
  int value = readByte(start, timeoutMs);

  if (value < 0 || value != 255) {
    return value;
  }

  // Telnet IAC control prefix received.
  int command = readByte(start, timeoutMs);

  if (command < 0) {
    return -1;
  }

  if (command == 255) {
    return 255;  // Escaped data byte.
  }

  // WILL=251, WONT=252, DO=253, DONT=254.
  if (command >= 251 && command <= 254) {
    int option = readByte(start, timeoutMs);

    if (option < 0) {
      return -1;
    }

    // Decline optional features:
    // DO -> WONT
    // WILL -> DONT
    if (command == 253 || command == 251) {
      uint8_t reply[] = {
        255,
        static_cast<uint8_t>(command == 253 ? 252 : 254),
        static_cast<uint8_t>(option)
      };

      if (client.write(reply, sizeof(reply)) != sizeof(reply)) {
        return -1;
      }
    }
  } else if (command == 250) {
    // Skip subnegotiation until IAC SE.
    while (true) {
      int next = readByte(start, timeoutMs);

      if (next < 0) {
        return -1;
      }

      if (next == 255) {
        int after = readByte(start, timeoutMs);

        if (after < 0) {
          return -1;
        }

        if (after == 240) {
          break;
        }
      }
    }
  }

  return -2;
}

// Wait for a prompt, optionally printing received output.
bool waitFor(
    const char* expected,
    uint32_t timeoutMs,
    bool showOutput
) {
  String tail;
  const size_t expectedLength = strlen(expected);
  const uint32_t start = millis();

  while (millis() - start < timeoutMs) {
    int value = readTelnetByte(start, timeoutMs);

    if (value == -1) {
      break;
    }

    if (value == -2 || value == 0) {
      continue;
    }

    if (showOutput) {
      Serial.write(static_cast<uint8_t>(value));
    }

    // Keep only enough text to detect the expected prompt.
    tail += static_cast<char>(value);

    if (tail.length() > expectedLength) {
      tail.remove(0, tail.length() - expectedLength);
    }

    if (tail == expected) {
      return true;
    }
  }

  Serial.print("\nConnection closed or timeout waiting for: ");
  Serial.println(expected);

  return false;
}

// Send text followed by Enter.
bool sendLine(const char* text) {
  if (client.print(text) != strlen(text) ||
      client.print("\r\n") != 2) {
    Serial.println("Send failed");
    return false;
  }

  return true;
}

// ---------- Login and execute ----------

bool runSession() {
  Serial.println("Waiting for login prompt...");

  if (!waitFor(LOGIN_PROMPT, 15000, false)) {
    return false;
  }

  if (!sendLine(VM_USER)) {
    return false;
  }

  Serial.println("Waiting for password prompt...");

  if (!waitFor(PASSWORD_PROMPT, 15000, false)) {
    return false;
  }

  if (!sendLine(VM_PASSWORD)) {
    return false;
  }

  Serial.println("Waiting for Ubuntu shell...");

  if (!waitFor(SHELL_PROMPT, 15000, false)) {
    return false;
  }

  Serial.println("Logged in successfully");
  Serial.print("Executing: ");
  Serial.println(COMMAND);

  if (!sendLine(COMMAND)) {
    return false;
  }

  Serial.println("--- VM output ---");

  if (!waitFor(SHELL_PROMPT, 30000, true)) {
    return false;
  }

  return sendLine("exit");
}

// ---------- Network diagnostics ----------

void printNetworkInfo() {
  Serial.println("\n--- Network diagnostics ---");

  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.localIP());

  Serial.print("Subnet mask: ");
  Serial.println(WiFi.subnetMask());

  Serial.print("Gateway: ");
  Serial.println(WiFi.gatewayIP());

  Serial.print("DNS: ");
  Serial.println(WiFi.dnsIP());

  Serial.print("Signal (dBm): ");
  Serial.println(WiFi.RSSI());

  Serial.print("Target VM: ");
  Serial.println(VM_HOST);

  Serial.print("Target port: ");
  Serial.println(VM_PORT);
}

bool connectToVM() {
  for (int attempt = 1; attempt <= 3; ++attempt) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Wi-Fi disconnected before TCP connection");
      return false;
    }

    client.stop();

    Serial.print("\nTCP attempt ");
    Serial.print(attempt);
    Serial.println("/3");

    if (client.connect(VM_HOST, VM_PORT, 5000)) {
      Serial.println("TCP connected");
      return true;
    }

    Serial.println("TCP connection failed");

    if (attempt < 3) {
      delay(1000);
    }
  }

  Serial.println(
    "VM not reached. Check VMware network mode and LAN access."
  );

  Serial.println(
    "Copy the network diagnostics above for troubleshooting."
  );

  return false;
}

// ---------- Arduino entry points ----------

void setup() {
  Serial.begin(115200);

  // Allow USB serial to become ready without waiting forever.
  uint32_t start = millis();

  while (!Serial && millis() - start < 3000) {
    delay(10);
  }

  delay(500);

  Serial.println("\nESP32-C3 Telnet diagnostic build");
  Serial.println("Connecting to 2.4 GHz Wi-Fi...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  start = millis();

  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start >= 30000) {
      Serial.println(
        "Wi-Fi timeout: check SSID, password and 2.4 GHz support"
      );
      return;
    }

    delay(200);
  }

  printNetworkInfo();

  if (!connectToVM()) {
    return;
  }

  const bool success = runSession();

  client.stop();

  Serial.println(
    success ? "\nSession finished" : "\nSession failed"
  );
}

void loop() {
  // Press Reset to run the demonstration again.
  delay(100);
}