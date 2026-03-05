# B37VB-Open Loop Control (Motor Characterisation)

## Overview 
#### The main goal was to program a robot to go in a straight line using a given code. Initially the robot steared to the left side, so the Pulse Width Modulation (PWM) had to be changed on the wheels. Three different speeds were programmed for the robot with each wheel having a different PWM to ensure it goes in a straight line. The translational velocities of each speed can be found and compared to the tangential velocity of each wheel, the rpm of each wheel can be converted to angular velocity and then into tangential velocity. Tangential velocity of each wheel should be equal to the translational velocity of the robot.

## Method
#### The RPM was found first by taking a video of the robot driving with its wheels in the air. The RPM was converted into angular velocity using the formula ω=θ/t where θ=2π*RPM to convert RPM to radians and t=60 to change it to per second. Once the angular velocity was found it was converted to tangential velocity. This was repeated for three different speeds for the two wheels for a total of six measurements. To find the translational velocity of the robot it was timed travelling a distance of 1 metre and the equation for velocity v=d/t was used to find the velocity where d=1m and t=time to pass the translational and tangential velocities were then compared 

## Data



table      pwm L   pwm R    translational v    
slow
medium 
fast



|Modes     | Velocity L|Right Wheel RPM |Left  Wheel RPM |    L v R v   
|----------|----------|----------------|----------------|
|Slow      |          |15.04           |16.42           |
|Medium    |          |63.83           |63.22           |
|Fast      |          |92.31           |93.31           |

## Conclusion

#### The same pwm values did not produce the same velocity for each wheel which caused the buggy to turn in different directions instead of going staright. After chnaging the pwm values the robot would go in a straight line. The pwm values needed changing to produce the same tangential velocities in the wheels due to wear of the motors causing them to be less effective. After the rpm was found and converted into tangential velocity it was found that there were slight differneces between the wheels but the robot still went in a straight line and did not turn due to differences in tangential velocities in the wheels. Slight differences in RPM between wheels can be explained by the centre of mass is may not be centre so more weight is beig applied to one wheel than the other causing it to have more grip with the ground so it needs less torque to drive the buggy forward compared to the other wheel. This means that the PWM values can be different and yet still have the buggy go forward. There may also be large uncertainties in the readings so repeating the results and finding an average should reduce any uncertainties.
