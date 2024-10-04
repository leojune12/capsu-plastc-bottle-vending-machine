#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

LiquidCrystal_I2C lcd(0x27,16,2);

Servo myservo;

int servo_position = 0;

#define SERVO_PIN 9
#define PLASTIC_SENSOR_PIN 7
#define METAL_SENSOR_PIN 8

// Ultrasonic Sensor Pins
#define TRIG_PIN_1 11
#define ECHO_PIN_1 12

float temp_In_C = 20.0;     // Can enter actual air temp here for maximum accuracy
float speed_Of_Sound;       // Calculated speed of sound based on air temp
float distance_Per_uSec;    // Distance sound travels in one microsecond
float max_distance = 150;   // In centimeter

int bottle_count = 0;

void setup() {
    Serial.begin(9600);
    
    pinMode(PLASTIC_SENSOR_PIN, INPUT);
    pinMode(METAL_SENSOR_PIN, INPUT);
    pinMode(TRIG_PIN_1, OUTPUT);
    pinMode(ECHO_PIN_1, INPUT);

    myservo.attach(SERVO_PIN);
    myservo.write(90);

    // Formula to calculate speed of sound in meters/sec based on temp
    speed_Of_Sound = 331.1 +(0.606 * temp_In_C);  
    // Calculate the distance that sound travels in one microsecond in centimeters
    distance_Per_uSec = speed_Of_Sound / 10000.0;

    lcd.init();
    lcd.backlight();

    // EEPROM.put(0, 0);
    EEPROM.get(0, bottle_count);
}

void loop() {
  // Display bottle count 
  lcd.setCursor(0,1);
  lcd.print("Bottle count: ");
  lcd.setCursor(14,1);
  lcd.print(String(bottle_count) + "  ");
  
  check_cylinder_for_bottle();
}

void check_cylinder_for_bottle() {
    
    float cylinder_ultrasonic_distance = read_sensor("in cylinder", TRIG_PIN_1, ECHO_PIN_1);

    int plastic_sensor_reading = digitalRead(PLASTIC_SENSOR_PIN);
    int metal_sensor_reading = digitalRead(METAL_SENSOR_PIN);
    Serial.println(plastic_sensor_reading);
    Serial.println(metal_sensor_reading);
    
    if (cylinder_ultrasonic_distance < 10) {

        lcd.setCursor(0,0);
        lcd.print("Analyzing object");

        delay(1000);
    
        // 0 = Detected, 1 = No Detected
        if (plastic_sensor_reading == 0 & metal_sensor_reading == 1) {
            // Plastic reading
            lcd.setCursor(0,0);
            lcd.print("Plastic found   ");
            lcd.setCursor(0,1);
            lcd.print("Disposing       ");
            
            do {
                turn_servo_to_plastic();
                delay(1000);
                cylinder_ultrasonic_distance = read_sensor("in cylinder", TRIG_PIN_1, ECHO_PIN_1);
            } while(cylinder_ultrasonic_distance < 10);

            bottle_count = bottle_count + 1;
            EEPROM.put(0, bottle_count);
        } else {
            // Non-plastic reading        
            lcd.setCursor(0,0);
            lcd.print("Nonplastic found");
            lcd.setCursor(0,1);
            lcd.print("Disposing       ");
            
            do {
                turn_servo_to_non_plastic();
                delay(1000);
                cylinder_ultrasonic_distance = read_sensor("in cylinder", TRIG_PIN_1, ECHO_PIN_1);
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

float read_sensor(String sensor_name, int TRIG_PIN, int ECHO_PIN) {
    float duration, distanceCm, final_distance=0;
  
    for (int i=0; i<2; i++) {
        digitalWrite(TRIG_PIN, HIGH);       // Set trigger pin HIGH 
        delayMicroseconds(50);              // Hold pin HIGH for 20 uSec
        digitalWrite(TRIG_PIN, LOW);        // Return trigger pin back to LOW again.
        duration = pulseIn(ECHO_PIN, HIGH);  // Measure time in uSec for echo to come back.
       
        // convert the time data into a distance in centimeters, inches and feet
        duration = duration / 2.0;  // Divide echo time by 2 to get time in one direction
        distanceCm = duration * distance_Per_uSec;
    
        if (distanceCm > final_distance) {
            if (distanceCm > max_distance) {
                final_distance = max_distance;
            } else {
                final_distance = distanceCm;
            }
        }
    
        delay(10);
    }

    if (sensor_name != "") {
        if (final_distance <= 0){
            Serial.println("Sensor " + sensor_name +  " out of range");
        } else {
            Serial.print("Sensor " + sensor_name + ": ");
            Serial.print(distanceCm, 0);
            Serial.print("cm");
            Serial.println();
        }
    }
  
    return distanceCm;
}

void turn_servo_to_plastic() {
    for (servo_position = 90; servo_position <= 180; servo_position += 1) {
        myservo.write(servo_position);
        delay(10);
    }

    delay(1000);

    for (servo_position = 180; servo_position >= 90; servo_position -= 1) {
        myservo.write(servo_position);
        delay(10);
    }
}

void turn_servo_to_non_plastic() {
    for (servo_position = 90; servo_position >= 0; servo_position -= 1) {
        myservo.write(servo_position);
        delay(10);
    }

    delay(1000);

    for (servo_position = 0; servo_position <= 90; servo_position += 1) {
        myservo.write(servo_position);
        delay(10);
    }
}
