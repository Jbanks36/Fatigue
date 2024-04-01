#include <LiquidCrystal.h>
#include <Keypad.h>

const int ROW_NUM = 4;
const int COLUMN_NUM = 4;

char keys[ROW_NUM][COLUMN_NUM] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte pin_rows[ROW_NUM] = {6, 7, 8, 9};
byte pin_column[COLUMN_NUM] = {2, 3, 4, 5};

Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM);

const int rs = 23, en = 25, d4 = 22, d5 = 24, d6 = 26, d7 = 28;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
int lcdstate = 0;

int RPWM_Output = 10;
int LPWM_Output = 11;
int r_en = 12;
int l_en = 13;

int Speed; // PWM for actuator speed
int MaxTargetVal; // Initialize with appropriate value
int MinTargetVal;  // Initialize with appropriate value
int TravelDistance; // in mm
int CycleNum;
bool initialMaxSet = false;
const int MinCyclesNum = 1;
const unsigned long MaxCyclesNum = 100000;
const int MaxTravelLimit = 152;
const int MaxSpeedLimit = 67;
int cyclecount = 0;
int Speedmms;
bool speedconfirmation = false;
bool cycleconfirmation = false;

float pos;
float avgpos;
bool retracting = true; // state condition for start

float conNum = 0.180997624703087885; // conversion from ADC to mm (Update when fully assembled) (high = 880, low =38)

int numReadings = 500; // Initial number of readings to average
int postnumReadings = 27;
float *readings ;
int index = 0; // Index for the current reading

int menuint = 1;
int confirmint = 1;
bool allsetup = false;

String enteredText = "";

byte UpArrow[8] = {
  B00100,
  B01110,
  B11111,
  B00100,
  B00100,
  B00100,
  B00100,
  B00000
};
byte DownArrow[8] = {
  B00000,
  B00100,
  B00100,
  B00100,
  B11111,
  B01110,
  B00100,
  B00000
};

unsigned long delayStartTime = 0;
const unsigned long delayInterval = 3000;





void setup() {
  pinMode(A11, INPUT);
  pinMode(RPWM_Output, OUTPUT);
  pinMode(LPWM_Output, OUTPUT);
  pinMode(r_en, OUTPUT);
  pinMode(l_en, OUTPUT);
  lcd.begin(16, 2); // initialize the lcd
  readings = new float[numReadings];
  lcd.createChar(0, UpArrow);
  lcd.createChar(1, DownArrow);
}

void loop() {
  readPotentiometer();

  if (menuint == 1) {
    SetZeroPosition();
  }

  if (menuint == 2) {
    EnterTravelDistance();
  }

  if (menuint == 3) {
    if (speedconfirmation == false) {
    SetSpeed();
    }
    else {
      menuint = 4;
    }
  }

  if (menuint == 4) {
    if (cycleconfirmation == false) {
    SetCycleNum();
    }
    else {
      menuint = 5;
    }
  }

  if (menuint == 5) {
    ConfirmSettings();
  }

  if (menuint == 6) {
    readPotentiometer();
    delay(750);
    menuint = 7;
  }

  if (menuint == 7) {
    SetPotZero();
    delay(200);
    allsetup = true;
    menuint = 8;
  }

  if (menuint == 8) {
    if (lcdstate == 0) {
    lcd.clear();  // Clear the entire LCD screen
    lcd.setCursor(0, 0);
    lcd.print("Test Running...");
    lcd.setCursor(0, 1);
    lcd.print("#cycles=");
    delay(200);
    lcdstate = 1;
  }
  if (lcdstate == 1) {
    menuint = 9;
  }
  }

  if (menuint == 9 && allsetup == true) {
    if (cyclecount < CycleNum){
       readPotentiometer();
       CycleTest();
       lcd.setCursor(8, 1);
      lcd.print(cyclecount);
    }
    else {
      RetractZeroFine();
      stopActuator();
      delay(1000);
      lcdstate=0;
      menuint = 10;
    }
  }
  if (menuint == 10) {
    if (lcdstate == 0) {
    lcd.clear();  // Clear the entire LCD screen
    lcd.setCursor(0, 0);
    lcd.print("Test Completed!");
    delay(200);
    lcdstate = 1;
  }
  if (lcdstate == 1) {
    lcd.setCursor(0, 0);
    lcd.print("Test Completed!");
    lcd.setCursor(0, 1);
    lcd.print("#cycles=");
    lcd.setCursor(9, 1);
    lcd.print(cyclecount);
    delay(4000);
    lcdstate = 0;
    menuint = 11;
    
  }
  }
  if (menuint == 11) {
    if (lcdstate == 0) {
    lcd.clear();  // Clear the entire LCD screen
    lcd.setCursor(2, 0);
    lcd.print("Hit Reset To");
    lcd.setCursor(1, 1);
    lcd.print("Start New Test");
    delay(200);
    lcdstate = 1;

  }
    lcd.setCursor(2, 0);
    lcd.print("Hit Reset To");
    lcd.setCursor(1, 1);
    lcd.print("Start New Test");
    delay(4000);
    lcdstate = 0;
    menuint = 10; 
    
  } 
  
}

void readPotentiometer() {
  pos = conNum * (analogRead(A11) - 33);

  readings[index] = pos;
  index = (index + 1) % numReadings;

  float total = 0;
  for (int i = 0; i < numReadings; i++) {
    total += readings[i];
  }
  avgpos = total / numReadings;

int TravelDistanceMap;
  if(TravelDistance <= 50) {
    TravelDistanceMap = map(TravelDistance, 0, 50, 30, 5);
  postnumReadings = TravelDistanceMap;
  }

  if(TravelDistance > 50) {
  postnumReadings = 3;

  }
}


void SetPotZero() {
  if (initialMaxSet == false) {
    unsigned long startTime = millis();
    while (millis() - startTime < 4000) {
      readPotentiometer();
      delay(500);
    }
    delay(1000);
    MaxTargetVal = avgpos;
    MinTargetVal = MaxTargetVal - TravelDistance;
    initialMaxSet = true;
    delay(100);  // Adjust the delay period as needed
    
    // Free the memory for the old array
    delete[] readings;

    // Update the array size and allocate a new array
    numReadings = postnumReadings;
    readings = new float[numReadings];
  }
}

void stopActuator() {
  digitalWrite(r_en, LOW);
  digitalWrite(l_en, LOW);
  analogWrite(RPWM_Output, 0);
  analogWrite(LPWM_Output, 0);
  readPotentiometer();
}

void Extend() {
  digitalWrite(r_en, HIGH);
  digitalWrite(l_en, HIGH);
  analogWrite(RPWM_Output, Speed);
  analogWrite(LPWM_Output, 0);
  readPotentiometer();
}

void ExtendZero() {
  digitalWrite(r_en, HIGH);
  digitalWrite(l_en, HIGH);
  analogWrite(RPWM_Output, 50);
  analogWrite(LPWM_Output, 0);
  delay(200);
  readPotentiometer();
  stopActuator();
}

void Retract() {
  digitalWrite(r_en, HIGH);
  digitalWrite(l_en, HIGH);
  analogWrite(RPWM_Output, 0);
  analogWrite(LPWM_Output, Speed);
  readPotentiometer();
}

void RetractZero() {
  digitalWrite(r_en, HIGH);
  digitalWrite(l_en, HIGH);
  analogWrite(RPWM_Output, 0);
  analogWrite(LPWM_Output, 50);
  delay(200);
  readPotentiometer();
  stopActuator();
}

void RetractZeroFine() {
  digitalWrite(r_en, HIGH);
  digitalWrite(l_en, HIGH);
  analogWrite(RPWM_Output, 0);
  analogWrite(LPWM_Output, 15);
  delay(100);
  readPotentiometer();
  stopActuator();
}

void ExtendZeroFine() {
  digitalWrite(r_en, HIGH);
  digitalWrite(l_en, HIGH);
  analogWrite(RPWM_Output, 15);
  analogWrite(LPWM_Output, 0);
  delay(100);
  readPotentiometer();
  stopActuator();
}

void drive(int direction) {
  switch (direction) {
    case 1:
      while (avgpos <= MaxTargetVal) {
        Extend();
      }
      break;
    case 0:
      break;
    case -1:
      while (avgpos >= MinTargetVal) {
        Retract();
      }
      break;
  }
}

void CycleTest() {
  readPotentiometer();  // Update the moving average of position readings

  if (avgpos > MinTargetVal && retracting == true && initialMaxSet == true) {
    drive(-1);
    delay(10);
    retracting = false;

  } else if (avgpos < MaxTargetVal && retracting == false && initialMaxSet == true) {
    drive(1);
    delay(10);
    retracting = true;
    cyclecount++;

  } else {
    stopActuator();
    delay(2000);
  }
}

void SetZeroPosition() {
   if (lcdstate == 0) {
    lcd.clear();
    lcd.setCursor(4, 0);
    lcd.print("Set Zero");  
    lcd.setCursor(4, 1);
    lcd.print("Position");   
    delay(200);
    lcdstate = 1;
  }

  if (lcdstate == 1) {
    lcd.setCursor(4, 0);
    lcd.print("Set Zero");  
    lcd.setCursor(4, 1);
    lcd.print("Position");

    // Check if the delayInterval has passed
    if (millis() - delayStartTime >= delayInterval) {
      lcdstate = 2;
      delayStartTime = millis();  // Update the last time the delay was updated
    }
  }

  if (lcdstate == 2) {
    lcd.clear();
    lcdstate = 3;
  }

  if (lcdstate == 3) {
    lcd.setCursor(2, 0);
    lcd.print("A = Fine");  
    lcd.setCursor(12, 0);
    lcd.write(byte(0));
    lcd.setCursor(1, 1);
    lcd.print("C = Coarse");
    lcd.setCursor(12, 1);
    lcd.write(byte(0));

    // Check if the delayInterval has passed
    if (millis() - delayStartTime >= delayInterval) {
      lcdstate = 4;
      delayStartTime = millis();  // Update the last time the delay was updated
    }
  }

  if (lcdstate == 4) {
    lcd.clear();
    lcdstate = 5;
  }

  if (lcdstate == 5) {
    lcd.setCursor(2, 0);
    lcd.print("B = Fine");  
    lcd.setCursor(12, 0);
    lcd.write(byte(1));
    lcd.setCursor(1, 1);
    lcd.print("D = Coarse ");
    lcd.setCursor(12, 1);
    lcd.write(byte(1));

    // Check if the delayInterval has passed
    if (millis() - delayStartTime >= delayInterval) {
      lcdstate = 0;
      delayStartTime = millis();  // Update the last time the delay was updated
    }
  }


  char key = keypad.getKey();

  if (key) {
    if (key == 'A') {
      ExtendZeroFine();
    }
    if (key == 'B') {
      RetractZeroFine();
    }
    if (key == 'C') {
      ExtendZero();
    }
    if (key == 'D') {
      RetractZero();
    }
    if (key == '#') {
      lcdstate = 0;
      menuint = 2;
    }
  }
}

void EnterTravelDistance() {
  if (lcdstate == 0) {
    enteredText = "";
    lcd.clear();  // Clear the entire LCD screen
    lcd.setCursor(0, 0);
    lcd.print("Set Stroke Length");
    lcd.setCursor(0, 1);
    lcd.print("                  ");
    lcd.print("mm = " + enteredText);
    delay(200);
    lcdstate = 1;
  }
  if (lcdstate == 1) {
    lcd.setCursor(0, 0);
    lcd.print("Set Stroke Length");
    lcd.setCursor(0, 1);
    lcd.print("mm = " + enteredText);
  }

  char key = keypad.getKey();
  static bool numericInputReceived = false;  // Flag to track numeric input

  if (key) {
    if (isdigit(key)) {
      // Add the pressed key to the entered text
      enteredText += key;
      numericInputReceived = true;  // Set the flag when a numeric key is pressed
    } else if (key == '#' && numericInputReceived) {
      // Convert entered text to integer
      int enteredValue = enteredText.toInt();
      // Check if entered value is within the allowed range
      if (enteredValue <= MaxTravelLimit) {
        TravelDistance = enteredValue;
        lcdstate = 0;
        menuint = 3;
      } else {
        // Display an error message
        lcd.setCursor(0, 1);
        lcd.print("Exceeds max!");
        delay(2000);
        lcd.clear();
        lcdstate = 0;
      }
      enteredText = "";  // Clear the entered text
      numericInputReceived = false;  // Reset the flag
    } else {
      // Display an error message for non-numeric keys
      lcd.setCursor(0, 1);
      lcd.print("Invalid input!");
      delay(2000);
      lcd.clear();
      lcdstate = 0;
      enteredText = "";  // Clear the entered text
      numericInputReceived = false;  // Reset the flag
    }
  }
}

void SetSpeed() {
  if (lcdstate == 0) {
    enteredText = "";
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Set Speed (0-67)");
    lcd.setCursor(0, 1);
    lcd.print(enteredText + "(mm/s)");
    delay(200);
    lcdstate = 1;
  }

  if (lcdstate == 1) {
    lcd.setCursor(0, 0);
    lcd.print("Set Speed (0-67)");
    lcd.setCursor(0, 1);
    lcd.print(enteredText + " (mm/s)");
  }
  char key = keypad.getKey();
  static bool numericInputReceived = false;  // Flag to track numeric input

  if (key) {
    if (isdigit(key)) {
      // Add the pressed key to the entered text
      enteredText += key;
      numericInputReceived = true;  // Set the flag when a numeric key is pressed
    } else if (key == '#' && numericInputReceived) {
      // Convert entered text to integer
      int enteredValue = enteredText.toInt();
      int enteredValuemap;
      // Check if entered value is within the allowed range
      if (enteredValue <= MaxSpeedLimit) {
        enteredValuemap = map(enteredValue, 0, 67, 0, 255);
        Speed = enteredValuemap;
        Speedmms = enteredValue;
        lcdstate = 0;
        speedconfirmation = true;
        menuint = 4;
      } else {
        // Display an error message
        lcd.setCursor(0, 1);
        lcd.print("Exceeds max!");
        delay(2000);
        lcd.clear();
        lcdstate = 0;
      }
      enteredText = "";  // Clear the entered text
      numericInputReceived = false;  // Reset the flag
    } else {
      // Display an error message for non-numeric keys
      lcd.setCursor(0, 1);
      lcd.print("Invalid input!");
      delay(2000);
      lcd.clear();
      lcdstate = 0;
      enteredText = "";  // Clear the entered text
      numericInputReceived = false;  // Reset the flag
    }
  }
}

void SetCycleNum() {
  if (lcdstate == 0) {
    enteredText = "";
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Set # of Cycles");
    lcd.setCursor(0, 1);
    lcd.print(enteredText);
    delay(200);
    lcdstate = 1;
  }

  if (lcdstate == 1) {
    lcd.setCursor(0, 0);
    lcd.print("Set # of Cycles");
    lcd.setCursor(0, 1);
    lcd.print(enteredText);
  }

  char key = keypad.getKey();
  static bool numericInputReceived = false;  // Flag to track numeric input

  if (key) {
    if (isdigit(key)) {
      // Add the pressed key to the entered text
      enteredText += key;
      numericInputReceived = true;  // Set the flag when a numeric key is pressed
    } else if (key == '#' && numericInputReceived) {
      // Convert entered text to integer
      int enteredValue = enteredText.toInt();
      // Check if entered value is within the allowed range
      if (enteredValue >= MinCyclesNum && enteredValue <= MaxCyclesNum) {
        CycleNum = enteredValue;
        lcdstate = 0;
        cycleconfirmation = true;
        menuint = 5;
      } else {
        // Display an error message
        lcd.setCursor(0, 1);
        lcd.print("Outside Limit");
        delay(2000);
        lcd.clear();
        lcdstate = 0;
      }
      enteredText = "";  // Clear the entered text
      numericInputReceived = false;  // Reset the flag
    } else {
      // Display an error message for non-numeric keys
      lcd.setCursor(0, 1);
      lcd.print("Invalid input!");
      delay(2000);
      lcd.clear();
      lcdstate = 0;
      enteredText = "";  // Clear the entered text
      numericInputReceived = false;  // Reset the flag
    }
  }
}

void ConfirmSettings() {
  if (confirmint == 1) {
    if (lcdstate == 0) {
      enteredText = "";
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Stroke= " + String(TravelDistance) + " mm");
      lcd.setCursor(0, 1);
      lcd.print("# = yes , * = no");
      delay(200);
      lcdstate = 1;
    }

    if (lcdstate == 1) {
      lcd.setCursor(0, 0);
      lcd.print("Stroke= " + String(TravelDistance) + " mm");
      lcd.setCursor(0, 1);
      lcd.print("# = yes , * = no");
    }

    char key = keypad.getKey();

    if (key == '#') {
      lcdstate = 0;
      confirmint = 2;
    }

    if (key == '*') {
      lcd.clear();
      menuint = 2;
    }
  }

  if (confirmint == 2) {
    if (lcdstate == 0) {
      enteredText = "";
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Speed= " + String(Speedmms) + " mm/s");
      lcd.setCursor(0, 1);
      lcd.print("# = yes , * = no");
      delay(200);
      lcdstate = 1;
    }

    if (lcdstate == 1) {
      lcd.setCursor(0, 0);
      lcd.print("Speed= " + String(Speedmms) + " mm/s");
      lcd.setCursor(0, 1);
      lcd.print("# = yes , * = no");
    }

    char key = keypad.getKey();

    if (key) {
      if (key == '#') {
        lcdstate = 0;
        confirmint = 3;
      }

      if (key == '*') {
        lcd.clear();
        lcdstate = 0;
        speedconfirmation = false;
        menuint = 3;
      }
    }
  }

  if (confirmint == 3) {
    if (lcdstate == 0) {
      enteredText = "";
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("#cycles= " + String(CycleNum));
      lcd.setCursor(0, 1);
      lcd.print("# = yes , * = no");
      delay(200);
      lcdstate = 1;
    }

    if (lcdstate == 1) {
      lcd.setCursor(0, 0);
      lcd.print("#cycles= " + String(CycleNum));
      lcd.setCursor(0, 1);
      lcd.print("# = yes , * = no");
    }

    char key = keypad.getKey();

    if (key) {
      if (key == '#') {
        lcdstate = 0;
        confirmint = 4;
      }

      if (key == '*') {
        lcd.clear();
        lcdstate = 0;
        cycleconfirmation = false;
        menuint = 4;
      }
    }
  }

  if (confirmint == 4) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Test Starting...");
    delay(1000);
    menuint = 6;
  }
}

