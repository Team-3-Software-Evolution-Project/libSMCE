void setup() {
    pinMode(0, OUTPUT);
    pinMode(2, OUTPUT);
}

void loop() {
    //digitalWrite(1, isAlpha('A'));
   // bool var = isAlpha('A') && isAlphaNumeric('2') && isAscii('!') && isPrintable('!') && isControl('\n');

    digitalWrite(2, isAlpha('A') && isAlphaNumeric('2') && isAscii('!') && isPrintable('!') && isControl('\n')
                        && isDigit('2') && isGraph('|') && isHexadecimalDigit('F') && isLowerCase('z')
                        && isPunct('.') && isSpace(' ') && isUpperCase('Z') && isWhitespace('\t'));

    digitalWrite(0, !(isAlpha('1') && isAlphaNumeric('a')  && isControl('1')
                        && isDigit('a') && isGraph(' ') && isHexadecimalDigit('a') && isLowerCase('Z')
                        && isPunct(',') && isSpace('a') && isUpperCase('z') && isWhitespace(' ')));
    delay(1);
}
