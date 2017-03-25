# mqttlog
the central repo

Log file format rules:
----------------------
1. if line starts with '#' it is a comment
    - only fill line comments.
    - no '#' elsewhere will be interpreted
1. if line starts with a 'T' it is a time sync point
    - 'T'
    - no space
    - Time in YYYY-MM-DD hh:mm:ss:mmm
1. if line starts with 'N', it is a Name line
    - 'N'
    - no space
    - ID of sensor
    - space
    - sensor name [string]
1. If line starts with a number, it is a data point with the format:
    - ID of sensor
    - time [ms] after last sync point
    - value [string]

Example:

    #header bla
    T2016-03-20 0:12:23:340
    N 1 Sensor.blubb
    1 150 23.5
    1 1520 49.5