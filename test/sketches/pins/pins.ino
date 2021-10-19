unsigned long time1, time2;

void setup() {
    pinMode(0, INPUT);
    pinMode(2, OUTPUT);
    pinMode(3, INPUT);
    pinMode(4, OUTPUT);
}

void loop() {
    // Including micros and millis function to test that they can be compiled.
    time1 = micros();
    time2 = millis();

    // Writing and reading on digital pins
    digitalWrite(2, !digitalRead(0));

    // Writing and reading on analog pins
    analogWrite(4, analogRead(3));
    delay(1);
    delayMicroseconds(1000);
}
