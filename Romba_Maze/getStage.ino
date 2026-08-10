uint8_t getStage()
{
    //stage one - moving forward in open area

    //transfer between stage 1 and 2: both front US sensors read a distance of less than 500 mm.

    //stage two - funnel - walls closing in, the narrow part is around 200 mm. remain in the middle.

    //transfer between stage 2 and 3: at least one front US sensors read less than 200 mm, and the second front US sensor read more than 500 mm, or both read more than 500 mm simultansly (another limit needed).

    //stage three - wall following - the robot is in an open area near a wall on one side. need to navigate away from the wall to a distance of 300 mm and continue that distance while the wall whine.

    //transfer between stage 3 and 4: both front US sensors read more than 500 mm, same do the lasers, or the ADXL335 accelerometer detects a sudden drop in the Z axis.

    //stage four - wadi - the robot is in a wadi, need to navigate to the other side of the wadi without climbing the walls. the wadi is around 250 mm wide.

    //transfer between stage 4 and 5: LDRs detect a 
}