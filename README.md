# mqttlog
Logging MQTT data and plotting it

Log file format rules:
----------------------
1. if line starts with '#' it is a comment
    - only full line comments.
    - no '#' except line start will be interpreted
1. if line starts with a 'T' it is a time sync point. All subsequent data are based on this time.
    - 'T'
    - no space
    - Time in YYYY-MM-DD hh:mm:ss:mmm (with space between date & time)
1. if line starts with 'N', it is a Name line
    - 'N'
    - no space
    - ID of sensor [INT string]
    - space
    - sensor name [string] until end of line
1. If line starts with a number, it is a data point with the format:
    - ID of sensor [INT string]
    - space
    - time [ms] relative to the last sync point
    - value [string] until end of line
1. Non conformal lines throw an error.
1. time [ms] should be kept short (<6 digits) 
    * SW must be able to handle 32 bit 2^32 (49 days)
    * some value to be transmitted is expected in 49 days.


stuff to think about:

- use cs or ds (100th or 10th of a second or full seconds) as logging timebase
- how to avoid excessive time syncs
- is timestamp somewhat compressible using its monotonic character?

Example:

    #header bla
    T2016-03-20 0:12:23:340
    N1 Sensor.blubb
    1 150 23.5
    1 1520 49.5
