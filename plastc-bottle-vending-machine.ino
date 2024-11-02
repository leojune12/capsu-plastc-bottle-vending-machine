#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <HCSR04.h>
#include <EasyButton.h>
#include <TaskScheduler.h>

LiquidCrystal_I2C lcd(0x27,16,2);
Scheduler runner;

Servo dispense_bottle_servo;
Servo dispense_item1_servo;
Servo dispense_item2_servo;

#define PLASTIC_SENSOR_PIN 2
#define METAL_SENSOR_PIN 3
#define ITEM1_BUTTON_PIN 4
#define ITEM2_BUTTON_PIN 5
#define IR1_PIN 6
#define IR2_PIN 7
#define DISPENSE_BOTTLE_SERVO_PIN 8
#define DISPENSE_ITEM1_SERVO_PIN 9
#define DISPENSE_ITEM2_SERVO_PIN 10

// Ultrasonic Sensor Pins
#define TRIG_PIN 11
#define ECHO_PIN 12

HCSR04 hc(TRIG_PIN, ECHO_PIN);

EasyButton item1_button(ITEM1_BUTTON_PIN);
EasyButton item2_button(ITEM2_BUTTON_PIN);

int bottle_count_address = 0;
int bottle_count = 0;

int item1_price = 3;
int item2_price = 3;

const long interval = 10000;

void homeScreen();

Task taskHomeScreen(100, TASK_FOREVER, &homeScreen);

void setup() {
    Serial.begin(9600);
    
    pinMode(PLASTIC_SENSOR_PIN, INPUT);
    pinMode(METAL_SENSOR_PIN, INPUT);
    pinMode(IR1_PIN, INPUT_PULLUP);
    pinMode(IR2_PIN, INPUT_PULLUP);

    dispense_bottle_servo.attach(DISPENSE_BOTTLE_SERVO_PIN);
    dispense_bottle_servo.write(90);
    delay(1000);
    dispense_bottle_servo.detach();

    lcd.init();
    lcd.backlight();

    // EEPROM.put(bottle_count_address, 0);
    EEPROM.get(bottle_count_address, bottle_count);

    item1_button.begin();
    item2_button.begin();
    item1_button.onPressed(dispenseItem1);
    item2_button.onPressed(dispenseItem2);

    runner.addTask(taskHomeScreen);

    taskHomeScreen.enable();
}

void loop() {
  runner.execute();
  item1_button.read();
  item2_button.read();
}

void homeScreen() {
  lcd.setCursor(0,1);
  lcd.print("Bottle count: ");
  lcd.setCursor(14,1);
  lcd.print(String(bottle_count) + "  ");
  
  check_cylinder_for_bottle();
}

void check_cylinder_for_bottle() {
    
    float cylinder_ultrasonic_distance = hc.dist();
    
    int plastic_sensor_reading = digitalRead(PLASTIC_SENSOR_PIN);
    int metal_sensor_reading = digitalRead(METAL_SENSOR_PIN);
    Serial.println(plastic_sensor_reading);
    Serial.println(metal_sensor_reading);
    
    if (cylinder_ultrasonic_distance < 10) {

        lcd.setCursor(0,0);
        lcd.print("Analyzing bottle");

        delay(2000);
    
        // 0 = Detected, 1 = Non Detected
        if (plastic_sensor_reading == 0 & metal_sensor_reading == 1) {
            // Plastic reading
            lcd.setCursor(0,0);
            lcd.print("Plastic found   ");
            lcd.setCursor(0,1);
            lcd.print("Disposing...       ");

            bottle_count = bottle_count + 1;
            EEPROM.put(bottle_count_address, bottle_count);
            
            do {
                turn_servo_to_plastic();
                delay(1000);
                cylinder_ultrasonic_distance = hc.dist();
            } while(cylinder_ultrasonic_distance < 10);
        } else {
            // Non-plastic reading        
            lcd.setCursor(0,0);
            lcd.print("Nonplastic found");
            lcd.setCursor(0,1);
            lcd.print("Disposing...       ");
            
            do {
                turn_servo_to_non_plastic();
                delay(1000);
                cylinder_ultrasonic_distance = hc.dist();
            } while(cylinder_ultrasonic_distance < 10);
        }
    } else {
        if (bottle_count > 2) {
            lcd.setCursor(0,0);
            lcd.print("Select a snack  "); 
        } else {
            lcd.setCursor(0,0);
            lcd.print("Insert a bottle ");
        }
    }
}

void turn_servo_to_plastic() {
    dispense_bottle_servo.attach(DISPENSE_BOTTLE_SERVO_PIN);
    for (int pos = 90; pos <= 180; pos += 1) {
        dispense_bottle_servo.write(pos);
        delay(10);
    }

    delay(1000);

    for (int pos = 180; pos >= 90; pos -= 1) {
        dispense_bottle_servo.write(pos);
        delay(10);
    }
    dispense_bottle_servo.detach();
}

void turn_servo_to_non_plastic() {
    dispense_bottle_servo.attach(DISPENSE_BOTTLE_SERVO_PIN);
    for (int pos = 90; pos >= 0; pos -= 1) {
        dispense_bottle_servo.write(pos);
        delay(10);
    }

    delay(1000);

    for (int pos = 0; pos <= 90; pos += 1) {
        dispense_bottle_servo.write(pos);
        delay(10);
    }
    dispense_bottle_servo.detach();
}

void dispenseItem1() {
    if ((bottle_count - item1_price) < 0) {
        lcd.setCursor(0,0);
        lcd.print("Need more bottle");
        delay(3000);
    } else {
        lcd.setCursor(0,0);
        lcd.print("Item #1 selected");
        lcd.setCursor(0,1);
        lcd.print("Please wait...  ");
        
        unsigned long startMillis = millis();
        bool timedOut = false;
    
        // Start servo
        dispense_item1_servo.attach(DISPENSE_ITEM1_SERVO_PIN);
        dispense_item1_servo.write(120);

        // Wait item to drop
        while (digitalRead(IR1_PIN) == 1) {
          if (millis() - startMillis >= interval) {
              timedOut = true;
              break;
          }
        }

        // Stop servo
        dispense_item1_servo.write(90);
        dispense_item1_servo.detach();
    
        if (timedOut == false) {
            bottle_count = bottle_count - item1_price;
            EEPROM.put(bottle_count_address, bottle_count);
    
            lcd.setCursor(0,0);
            lcd.print("Thank you!      ");
            lcd.setCursor(0,1);
            lcd.print("                ");
            delay(3000);
        }
    }
}

void dispenseItem2() {
    if ((bottle_count - item2_price) < 0) {
        lcd.setCursor(0,0);
        lcd.print("Need more bottle");
        delay(3000);
    } else {
        lcd.setCursor(0,0);
        lcd.print("Item #2 selected");
        lcd.setCursor(0,1);
        lcd.print("Please wait...  ");
        
        unsigned long startMillis = millis();
        bool timedOut = false;
    
        // Start servo
        dispense_item2_servo.attach(DISPENSE_ITEM2_SERVO_PIN);
        dispense_item2_servo.write(120);

        // Wait item to drop
        while (digitalRead(IR2_PIN) == 1) {
          if (millis() - startMillis >= interval) {
              timedOut = true;
              break;
          }
        }

        // Stop servo
        dispense_item2_servo.write(90);
        dispense_item2_servo.detach();
    
        if (timedOut == false) {
            bottle_count = bottle_count - item2_price;
            EEPROM.put(bottle_count_address, bottle_count);
    
            lcd.setCursor(0,0);
            lcd.print("Thank you!      ");
            lcd.setCursor(0,1);
            lcd.print("                ");
            delay(3000);
        }
    }
}
