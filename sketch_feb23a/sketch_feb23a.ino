#include <FlashStorage.h>
#include <Arduino.h>

#define MAX_POINTS 20
#define SENSOR_PIN A0

typedef struct {
  uint16_t sensorValue;
  float liters;
} CalibrationPoint;

typedef struct {
  uint16_t numPoints = 0;
  CalibrationPoint points[MAX_POINTS];
} CalibrationData;

FlashStorage(flashStorage, CalibrationData);
CalibrationData calibration;

bool simulationMode = false;
unsigned long simulationStartTime = 0;
bool increasingPhase = true;
int simulatedValue = 10;


void setup() {
  Serial.begin(9600);
  while (!Serial);
  
  // Загрузка сохраненных данных
  calibration = flashStorage.read();
  if (calibration.numPoints > MAX_POINTS) {
    calibration.numPoints = 0;
  }
}

void loop() {
  handleSerialCommands();
  updateSimulation();
  readSensor();
  delay(500);
}

void updateSimulation() {
  if (!simulationMode) return;
  
  unsigned long currentMillis = millis();
  unsigned long elapsed = currentMillis - simulationStartTime;

  if (increasingPhase) {
    if (elapsed <= 60000) { // 1 минута роста
      simulatedValue = (int)(10 + (1023-10) * (elapsed/60000.0));
    } else {
      increasingPhase = false;
      simulationStartTime = currentMillis;
    }
  } else {
    if (elapsed <= 180000) { // 3 минуты снижения
      simulatedValue = (int)(1023 - (1023-10) * (elapsed/180000.0));
    } else {
      increasingPhase = true;
      simulationStartTime = currentMillis;
    }
  }
  simulatedValue = constrain(simulatedValue, 10, 1023);
}

void readSensor() {
  int val = simulationMode ? simulatedValue : analogRead(SENSOR_PIN);
  
  // Вывод сырого значения и преобразованного
  Serial.print("ADC: ");
  Serial.print(val);
  
  if (val == 1024) {
    Serial.println(" -> Error: Sensor fault!");
    return;
  }
  
  if (calibration.numPoints == 0) {
    Serial.println(" -> Warning: No calibration data!");
    return;
  }

  Serial.print(" -> Volume: ");
  Serial.print(convertToLiters(val));
  Serial.println(" L");
}



float convertToLiters(uint16_t sensorVal) {
  for (int i = 1; i < calibration.numPoints; i++) {
    if (sensorVal <= calibration.points[i].sensorValue) {
      float x0 = calibration.points[i-1].sensorValue;
      float y0 = calibration.points[i-1].liters;
      float x1 = calibration.points[i].sensorValue;
      float y1 = calibration.points[i].liters;
      
      return y0 + (y1 - y0) * (sensorVal - x0) / (x1 - x0);
    }
  }
  return calibration.points[calibration.numPoints-1].liters;
}

void handleSerialCommands() {
  if (!Serial.available()) return;
  
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();

  if (cmd.equalsIgnoreCase("CALIBRATE")) {
    calibrate();
  } else if (cmd.equalsIgnoreCase("SAVE")) {
    flashStorage.write(calibration);
    Serial.println("Calibration saved!");
  } else if (cmd.equalsIgnoreCase("LOAD")) {
    calibration = flashStorage.read();
    Serial.println("Calibration loaded!");
  } else if (cmd.equalsIgnoreCase("SHOW")) {
    printCalibration();
  } else if (cmd.startsWith("CONVERT")) {
    processConvert(cmd);
  } else if (cmd.equalsIgnoreCase("DELETE")) {
    deleteCalibration();
  } else if (cmd.equalsIgnoreCase("SIM ON")) {
    simulationMode = true;
    simulationStartTime = millis();
    increasingPhase = true;
    simulatedValue = 10;
    Serial.println("Simulation STARTED");
  } else if (cmd.equalsIgnoreCase("SIM OFF")) {
    simulationMode = false;
    Serial.println("Simulation STOPPED");
  }
}

void deleteCalibration() {
  calibration.numPoints = 0;
  flashStorage.write(calibration);
  Serial.println("Calibration table DELETED from flash!");
  Serial.println("Don't forget to SAVE if you want to keep changes!");
}



void calibrate() {
  Serial.println("Enter calibration points [ADC] [liters] (type 'END' to finish):");
  calibration.numPoints = 0;
  
  while (true) {
    while (!Serial.available());
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    if (input.equalsIgnoreCase("END")) break;
    
    // Разбиваем строку на две части
    int spaceIndex = input.indexOf(' ');
    if (spaceIndex == -1) {
      Serial.println("Invalid format! Use: [ADC] [liters]");
      continue;
    }
    
    // Парсим значения
    uint16_t adc = input.substring(0, spaceIndex).toInt();
    float liters = input.substring(spaceIndex+1).toFloat();
    
    // Проверка корректности значений
    if (adc >= 1024) {
      Serial.println("Error: ADC value must be <1024");
      continue;
    }
    
    if (liters == 0 && input.substring(spaceIndex+1) != "0") {
      Serial.println("Error: Invalid liters value");
      continue;
    }
    
    if (calibration.numPoints < MAX_POINTS) {
      calibration.points[calibration.numPoints] = {adc, liters};
      calibration.numPoints++;
      Serial.print("Added: ");
      Serial.print(adc);
      Serial.print(" => ");
      Serial.print(liters);
      Serial.println(" L");
    } else {
      Serial.println("Error: Max points reached");
    }
  }
  
  sortCalibration();
  Serial.println("Calibration completed!");
}


void sortCalibration() {
  for (int i = 0; i < calibration.numPoints-1; i++) {
    for (int j = 0; j < calibration.numPoints-i-1; j++) {
      if (calibration.points[j].sensorValue > calibration.points[j+1].sensorValue) {
        CalibrationPoint temp = calibration.points[j];
        calibration.points[j] = calibration.points[j+1];
        calibration.points[j+1] = temp;
      }
    }
  }
}

void printCalibration() {
  Serial.println("Current calibration:");
  for (int i = 0; i < calibration.numPoints; i++) {
    Serial.print(i+1);
    Serial.print(". ADC: ");
    Serial.print(calibration.points[i].sensorValue);
    Serial.print(" => ");
    Serial.print(calibration.points[i].liters);
    Serial.println(" L");
  }
}

void processConvert(String cmd) {
  int spaceIdx = cmd.indexOf(' ');
  if (spaceIdx == -1) return;
  
  uint16_t val = cmd.substring(spaceIdx+1).toInt();
  if (val >= 1024) {
    Serial.println("Invalid ADC value!");
    return;
  }
  
  Serial.print("Converted value: ");
  Serial.print(convertToLiters(val));
  Serial.println(" L");
}