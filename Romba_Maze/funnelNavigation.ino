void funnelNavigation(const SensorReading &sensors)
{
    int delta;
    while()
    {
        laserRightDistance = sensors.laserRight_mm;
        laserLeftDistance = sensors.laserLeft_mm;
        laserFrontDistance = sensors.laserFront_mm;

        delta = laserRightDistance - laserLeftDistance;

        car.straight(fast);
        car.turn(delta * K);

        if (abs(delta) > )



        
}