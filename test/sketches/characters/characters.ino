void setup() {
    pinMode(0, INPUT);
    pinMode(2, OUTPUT);
}

void loop() {
    digitalWrite(2, isAlpha('A'));
    bool var = isAlpha('A');
    delay(1);
}
