Here you will find some files from a project which uses Gimbals (x and y axes) and potentiometers.
It is based upon the adc_tools and Sensors "libraries" I have uploaded recently.

In this project, I am testing class implementation of gimbals and potentiometers, and explore some concepts like having an asynchronous
adc running in the background, while the adc_results are beeing processed by Gimbal's and Pot's pipelines.
```
-------------input------------------------------------------------------------------output---------------
Incoming data -> TransformPipeline(-> (DataFilter) -> (Deadzone) -> (DataHandler) ->) -> result
                                          |               |               |
                                        ACTIVE         BYPASSED         ACTIVE
```
In the above diagram, I'm trying to explain roughly what's happening when we invoke the `AnalogSensor.update_results()`/`Gimbal.update_sensors()` method.
The input data (raw adc_result) is sent into a TransformPipeline which handles conversions and data mapping.
Then, the TransformPipeline decides to pass this raw input to the datafilter (which removes noise) and catches the output of datafilter.
This result is then sent (or "connected") to the Deadzone element and so on until the TransformPipeline reaches the end of the pipeline.
Converted data is sent back to the sensor and this value will be the final output of the sensor that we may read afterward.

Note that we can bypass one or several elements in the pipeline. 
It could be usefull to deactivate some of the features of the pipeline and then lighten the computation process (data is directly sent to the next element).

I've tried to debug this piece of work as much as I could, however some tiny bugs may remain somewhere in this code.
I'll try to remove all of them, time and testing will help correcting those errors.

Have fun!
