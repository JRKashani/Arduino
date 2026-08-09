void info_input()
{
    // timestamp
    currentData.timestamp = millis();

    // Ultrasonic sensors
    currentData.US60  = US60 .ranging(CM);
    currentData.US120 = US120.ranging(CM);
    currentData.US240 = US240.ranging(CM);
    currentData.US300 = US300.ranging(CM);

}