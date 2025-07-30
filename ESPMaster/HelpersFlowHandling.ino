String downloadDreamflowsCsv() {
  WiFiClientSecure client;
  client.setInsecure(); // WARNING: insecure connection

  if (!client.connect("www.dreamflows.com", 443)) {
    Serial.println("Connection to Dreamflows failed");
    return "";
  }

  client.print("GET /realtime.csv.php HTTP/1.1\r\n" \
               "Host: www.dreamflows.com\r\n" \
               "Connection: close\r\n\r\n");

  bool bodyStarted = false;
  String csv = "";

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      bodyStarted = true;
      continue;
    }
    if (bodyStarted) {
      csv += line + "\n";
    }
  }

  client.stop();
  return csv;
}

String getCombinedFlowsString(String csv, std::vector<int> riverIds) {
  String combinedFlows = "";
  Serial.println("Starting flow parsing for selected rivers...");

  for (int id : riverIds) {
    if (riverIdToName.find(id) == riverIdToName.end()) {
      Serial.print("Warning: River ID not found in map: ");
      Serial.println(id);
      continue;
    }

    String riverName = riverIdToName[id];
    Serial.print("Looking for flow data for: ");
    Serial.println(riverName);

    int lineStart = 0;
    bool found = false;

    while (lineStart >= 0) {
      int lineEnd = csv.indexOf('\n', lineStart);
      if (lineEnd < 0) break;

      String line = csv.substring(lineStart, lineEnd);
      if (line.indexOf(riverName) >= 0) {
        int secondComma = line.indexOf(',', line.indexOf(',') + 1);
        int thirdComma = line.indexOf(',', secondComma + 1);
        String flowCfs = line.substring(secondComma + 1, thirdComma);
        flowCfs.trim();

        Serial.print("  Raw flow: ");
        Serial.println(flowCfs);

        int flowValue = flowCfs.toInt();
        if (flowValue > 9999) {
          Serial.println("  Flow exceeds 9999, capping to 9999");
          flowValue = 9999;
        }

        char buffer[5];
        snprintf(buffer, sizeof(buffer), "%04d", flowValue);
        String paddedFlow = String(buffer);

        Serial.print("  Final padded flow: ");
        Serial.println(paddedFlow);

        combinedFlows += paddedFlow;
        found = true;
        break;
      }

      lineStart = lineEnd + 1;
    }

    if (!found) {
      Serial.print("  Error: No matching line found for river: ");
      Serial.println(riverName);
    }
  }

  Serial.print("Final combined flow string: ");
  Serial.println(combinedFlows);
  return combinedFlows;
}




String getRiverOptionsJson() {
  String csv = downloadDreamflowsCsv(); // reuse the CSV downloader
  String json = "[";

  int lineStart = 0;
  bool firstEntry = true;

  while (lineStart >= 0) {
    int lineEnd = csv.indexOf('\n', lineStart);
    if (lineEnd < 0) break;

    String line = csv.substring(lineStart, lineEnd);
    int firstComma = line.indexOf(',');
    int secondComma = line.indexOf(',', firstComma + 1);

    if (firstComma > 0 && secondComma > firstComma) {
      String id = line.substring(0, firstComma);
      String name = line.substring(firstComma + 1, secondComma);

      name.replace("\"", "\\\""); // escape quotes

      if (!firstEntry) json += ",";
      json += "{\"id\":" + id + ",\"name\":\"" + name + "\"}";
      firstEntry = false;
    }

    lineStart = lineEnd + 1;
  }

  json += "]";
  return json;
}

/* std::vector<int> parseSelectedFlows(String flowStr) {
  std::vector<int> result;
  int start = 0;
  while (start < flowStr.length()) {
    int commaIndex = flowStr.indexOf(',', start);
    if (commaIndex == -1) commaIndex = flowStr.length();
    String piece = flowStr.substring(start, commaIndex);
    int id = piece.toInt();
    if (id > 0) result.push_back(id);
    start = commaIndex + 1;
  }
  return result;
} */

#include <Arduino.h>
#include <vector>

std::vector<int> parseSelectedFlows(const String& flowsCsv) {
  std::vector<int> result;

  int start = 0;
  while (start < flowsCsv.length()) {
    int commaIndex = flowsCsv.indexOf(',', start);
    String idStr;
    if (commaIndex == -1) {
      idStr = flowsCsv.substring(start);
      start = flowsCsv.length();
    } else {
      idStr = flowsCsv.substring(start, commaIndex);
      start = commaIndex + 1;
    }

    int id = idStr.toInt();
    if (id > 0) result.push_back(id);
  }

  return result;
}