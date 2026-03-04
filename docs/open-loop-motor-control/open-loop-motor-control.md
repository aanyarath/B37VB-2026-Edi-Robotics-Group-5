# B37VB-Open Loop Control (Motor Characterisation)

## Overview 
### The main goal was to program a robot to go in a straight line using a given code. Initially the robot steared to the left side, so the Pulse Width Modulation (PWM) had to be changed on the wheels. Three different speeds were programmed for the robot with each wheel having a different PWM to ensure it goes in a straight line. The translational velocities of each speed can be found and compared to the tangential velocity of each wheel, the rpm of each wheel can be converted to angular velocity and then into tangential velocity. Tangential velocity of each wheel should be equal to the translational velocity of the robot.

## Method
### The RPM was found first by taking a video of the robot driving with its wheels in the air. The RPM was converted into angular velocity using the formula ω=θ/t where θ=2π*RPM to convert RPM to radians and t=60 to change it to per second. Once the angular velocity was found it was converted to tangential velocity. This was repeated for three different speeds for the two wheels for a total of six measurements. To find the translational velocity of the robot it was timed travelling a distance of 1 metre and the equation for velocity v=d/t was used to find the velocity where d=1m and t=time to pass the translational and tangential velocities were then compared 

## Data
|Modes     |Velocity  |Right Wheel RPM |Left  Wheel RPM |
|----------|----------|----------------|----------------|
|Slow      |          |15.04           |16.42           |
|Medium    |          |63.83           |63.22           |
|Fast      |          |92.31           |93.31           |

## Conclusion
#### The RPM for each wheel was extremely similar. Slight differences in RPM between wheels explains why we needed to adjust the speed of each wheel. The translational velocity of the robot was also calculated by timing how long it took to travel 1 metre.
