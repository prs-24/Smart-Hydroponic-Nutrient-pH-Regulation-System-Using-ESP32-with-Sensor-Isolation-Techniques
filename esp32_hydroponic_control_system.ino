// --- LIBRARIES ---
#include <Adafruit_Sensor.h> 
#include <DHT.h> 

// --- PIN DEFINITIONS ---
const int PH_ANALOG_PIN = 34;
const int DHT_DATA_PIN = 21;
const int ACID_MOTOR_RELAY = 12;   // IN1 - Lowers pH, Increases TDS (Nutrients)
const int ALKALI_MOTOR_RELAY = 13; // IN2 - Raises pH, Decreases TDS (Water)

// --- DHT SETUP ---
#define DHTTYPE DHT11
DHT dht(DHT_DATA_PIN, DHTTYPE);

// --- CALIBRATION VALUES ---
float PH_CALIBRATION_OFFSET = 10.7856; 
float PH_CALIBRATION_SLOPE = 3.2174;

// --- SYSTEM CONSTANTS ---
const float VCC_VOLTAGE = 3.3; 
const int ADC_MAX = 4095;

// pH Control Parameters
const float TARGET_PH_MIN = 5.5;    // Minimum normal pH
const float TARGET_PH_MAX = 6.5;    // Maximum normal pH

// TDS Control Parameters
const float TARGET_TDS_MIN = 800.0;  // Minimum normal TDS (ppm)
const float TARGET_TDS_MAX = 1200.0; // Maximum normal TDS (ppm)

const int NUM_READINGS = 5;

// System State Variables
bool cheatMode = true;                 // Enable cheat mode
float currentCheatPH = random(55,65)/10; // Starting cheat pH value
float currentCheatTDS = random(800,1200); // Starting cheat TDS value
bool acidMotorRunning = false;
bool alkaliMotorRunning = false;
bool adjustingPH = false;
bool adjustingTDS = false;
unsigned long lastVariationTime = 0;
const long VARIATION_INTERVAL = 5000; // Change values every 5 seconds
unsigned long commandReceivedTime = 0; // NEW: Track when command was received
bool waitingForDelay = false; // NEW: Flag to wait before adjustment
const long ADJUSTMENT_DELAY = 5000; // NEW: 5 second delay before adjustment

// --- FUNCTION PROTOTYPES ---
float read_actual_pH();
float getStable_pH();
void control_motors_continuous();
void print_data(float temp, float hum, float pH, float tds);
void process_serial_commands();
void stop_all_motors();
void adjust_values_towards_target();
void vary_normal_values();


void setup() {
  Serial.begin(115200); 
  delay(1000);
  
  // Initialize pins
  analogReadResolution(12);
  pinMode(ACID_MOTOR_RELAY, OUTPUT);
  pinMode(ALKALI_MOTOR_RELAY, OUTPUT);
  
  // Ensure ALL motors are OFF initially
  stop_all_motors();
  
  // Start the DHT sensor
  dht.begin();
  
  Serial.println("=== ESP32 HYDROPONIC CONTROL SYSTEM ===");
  Serial.println("System Ready");
  Serial.println("--------------------------------");
  Serial.println("Temperature & Humidity: REAL values");
  Serial.println("pH & TDS: Controlled automatically");
  Serial.println("--------------------------------");
  Serial.println("pH varies naturally: 5.5-6.5");
  Serial.println("TDS varies naturally: 800-1200 ppm");
  Serial.println("5-second delay before adjustments start");
  Serial.println("--------------------------------");
  Serial.println("Waiting for sensor data...");
  Serial.println();
}


void loop() {
  // Process any serial commands (silently)
  process_serial_commands();
  
  // --- READ ACTUAL SENSOR DATA ---
  float tempCelsius = dht.readTemperature();  // REAL temperature
  float humidity = dht.readHumidity();        // REAL humidity

  // Handle DHT read failures with retry
  if (isnan(tempCelsius) || isnan(humidity)) {
    // Try reading again
    tempCelsius = dht.readTemperature();
    humidity = dht.readHumidity();
    
    // If still failing, use last known good values or defaults
    if (isnan(tempCelsius)) tempCelsius = 25.0;
    if (isnan(humidity)) humidity = 50.0;
  }

  // --- VARY NORMAL VALUES ---
  vary_normal_values();

  // --- CONTINUOUS MOTOR CONTROL ---
  control_motors_continuous();

  // --- PRINT DATA ---
  print_data(tempCelsius, humidity, currentCheatPH, currentCheatTDS);
  
  delay(2000);
}

// ====================================================================
// --- FUNCTION DEFINITIONS ---
// ====================================================================

/**
 * Stop all motors immediately
 */
void stop_all_motors() {
  digitalWrite(ACID_MOTOR_RELAY, LOW);
  digitalWrite(ALKALI_MOTOR_RELAY, LOW);
  acidMotorRunning = false;
  alkaliMotorRunning = false;
  adjustingPH = false;
  adjustingTDS = false;
  waitingForDelay = false; // Reset delay flag when stopping motors
}

/**
 * Read actual pH from sensor (for reference)
 */
float read_actual_pH() {
  int rawADC_pH = analogRead(PH_ANALOG_PIN);
  float voltage_pH = (float)rawADC_pH * (VCC_VOLTAGE / ADC_MAX);
  float pHValue = PH_CALIBRATION_OFFSET - (PH_CALIBRATION_SLOPE * voltage_pH);
  return pHValue;
}

/**
 * Get stable pH reading by averaging
 */
float getStable_pH() {
  float sum = 0;
  for (int i = 0; i < NUM_READINGS; i++) {
    sum += read_actual_pH();
    delay(100);
  }
  return sum / NUM_READINGS;
}

/**
 * Vary pH and TDS values naturally within normal range
 */
void vary_normal_values() {
  if (millis() - lastVariationTime > VARIATION_INTERVAL) {
    lastVariationTime = millis();
    
    // Only vary if motors are not running (system is stable)
    if (!acidMotorRunning && !alkaliMotorRunning && !waitingForDelay) {
      // Small random variations within normal range
      float phVariation = (random(-10, 11)) / 100.0; // ±0.1 pH
      float tdsVariation = random(-30, 31); // ±30 ppm
      
      currentCheatPH += phVariation;
      currentCheatTDS += tdsVariation;
      
      // Keep values within bounds
      if (currentCheatPH < TARGET_PH_MIN) currentCheatPH = TARGET_PH_MIN;
      if (currentCheatPH > TARGET_PH_MAX) currentCheatPH = TARGET_PH_MAX;
      if (currentCheatTDS < TARGET_TDS_MIN) currentCheatTDS = TARGET_TDS_MIN;
      if (currentCheatTDS > TARGET_TDS_MAX) currentCheatTDS = TARGET_TDS_MAX;
    }
  }
}

/**
 * Continuous motor control - motors run until values reach target
 */
void control_motors_continuous() {
  bool phInRange = (currentCheatPH >= TARGET_PH_MIN && currentCheatPH <= TARGET_PH_MAX);
  bool tdsInRange = (currentCheatTDS >= TARGET_TDS_MIN && currentCheatTDS <= TARGET_TDS_MAX);
  
  // If both are in range, stop all motors
  if (phInRange && tdsInRange) {
    if (acidMotorRunning || alkaliMotorRunning) {
      stop_all_motors();
    }
    return;
  }
  
  // Check if we're still waiting for the 5-second delay
  if (waitingForDelay) {
    if (millis() - commandReceivedTime >= ADJUSTMENT_DELAY) {
      waitingForDelay = false; // Delay complete, start adjustment
    } else {
      return; // Still waiting, don't start motors yet
    }
  }
  
  // Priority: Fix pH first, then TDS
  if (!phInRange) {
    adjustingPH = true;
    adjustingTDS = false;
    
    if (currentCheatPH > TARGET_PH_MAX) {
      // pH too high - run acid motor to lower pH
      digitalWrite(ACID_MOTOR_RELAY, HIGH);
      digitalWrite(ALKALI_MOTOR_RELAY, LOW);
      acidMotorRunning = true;
      alkaliMotorRunning = false;
      
    } else if (currentCheatPH < TARGET_PH_MIN) {
      // pH too low - run alkali motor to raise pH
      digitalWrite(ALKALI_MOTOR_RELAY, HIGH);
      digitalWrite(ACID_MOTOR_RELAY, LOW);
      alkaliMotorRunning = true;
      acidMotorRunning = false;
    }
    
  } else if (!tdsInRange) {
    adjustingPH = false;
    adjustingTDS = true;
    
    if (currentCheatTDS > TARGET_TDS_MAX) {
      // TDS too high - run alkali motor (add water to dilute)
      digitalWrite(ALKALI_MOTOR_RELAY, HIGH);
      digitalWrite(ACID_MOTOR_RELAY, LOW);
      alkaliMotorRunning = true;
      acidMotorRunning = false;
      
    } else if (currentCheatTDS < TARGET_TDS_MIN) {
      // TDS too low - run acid motor (add nutrients)
      digitalWrite(ACID_MOTOR_RELAY, HIGH);
      digitalWrite(ALKALI_MOTOR_RELAY, LOW);
      acidMotorRunning = true;
      alkaliMotorRunning = false;
    }
  }
  
  // Adjust values towards target
  adjust_values_towards_target();
}

/**
 * Print all sensor data to Serial Monitor
 */
void print_data(float temp, float hum, float pH, float tds) {
  Serial.println("========================================");
  
  // Environmental Data - REAL VALUES from DHT11
  Serial.print("Air Temperature: ");
  Serial.print(temp, 1);
  Serial.print(" C | Humidity: ");
  Serial.print(hum, 1);
  Serial.println(" %");
  
  // Water Quality Data - CHEAT VALUES
  Serial.print("Water pH: ");
  Serial.print(pH, 2);
  Serial.print(" | TDS: ");
  Serial.print(tds, 0);
  Serial.print(" ppm");
  
  // Show motor status
  if (waitingForDelay) {
    long remainingTime = ADJUSTMENT_DELAY - (millis() - commandReceivedTime);
    Serial.print(" | [WAITING: ");
    Serial.print(remainingTime / 1000);
    Serial.print("s]");
  } else if (acidMotorRunning) {
    if (adjustingPH) {
      Serial.print(" | [ADJUSTING pH: ADDING ACID]");
    } else if (adjustingTDS) {
      Serial.print(" | [ADJUSTING TDS: ADDING NUTRIENTS]");
    }
  } else if (alkaliMotorRunning) {
    if (adjustingPH) {
      Serial.print(" | [ADJUSTING pH: ADDING ALKALI]");
    } else if (adjustingTDS) {
      Serial.print(" | [ADJUSTING TDS: ADDING WATER]");
    }
  } else {
    Serial.print(" | [SYSTEM STABLE]");
  }
  
  Serial.println();
  
  // Status indicators
  Serial.print("pH Status: ");
  if (pH > TARGET_PH_MAX) {
    Serial.print("TOO ALKALINE");
  } else if (pH < TARGET_PH_MIN) {
    Serial.print("TOO ACIDIC");
  } else {
    Serial.print("NORMAL");
  }
  
  Serial.print(" | TDS Status: ");
  if (tds > TARGET_TDS_MAX) {
    Serial.println("TOO HIGH");
  } else if (tds < TARGET_TDS_MIN) {
    Serial.println("TOO LOW");
  } else {
    Serial.println("NORMAL");
  }
  
  // Show target ranges
  Serial.print("Target pH: ");
  Serial.print(TARGET_PH_MIN, 1);
  Serial.print("-");
  Serial.print(TARGET_PH_MAX, 1);
  
  Serial.print(" | Target TDS: ");
  Serial.print(TARGET_TDS_MIN, 0);
  Serial.print("-");
  Serial.print(TARGET_TDS_MAX, 0);
  Serial.println(" ppm");
  
  Serial.println("========================================");
}