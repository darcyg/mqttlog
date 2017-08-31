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

Database structure

Example of a Datastructure on a mysql Db (sensor -- 1:n -- data)

stuff to think about:

- add category to sensor table e.g. weather outside, solar, power, etc.

CREATE TABLE `sensor` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `order` int(11) NOT NULL, 
  `description` varchar(50) CHARACTER SET utf8 COLLATE utf8_unicode_ci NOT NULL,
  `unit` varchar(10) CHARACTER SET utf8 COLLATE utf8_unicode_ci NOT NULL,
  `mqtt_topic` varchar(256) CHARACTER SET utf8 COLLATE utf8_unicode_ci DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE `data` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `sensor_id` int(11) NOT NULL,
  `datetime` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `value` float NOT NULL,
  PRIMARY KEY (`id`),
  KEY `sensor_id` (`sensor_id`),
  CONSTRAINT `data_ibfk_1` FOREIGN KEY (`sensor_id`) REFERENCES `sensor` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

Data retrievel for e.g. line chart:

Iterate over the sensors and retrieve for a sensor_id with a date range the data-value.
Put everything on a line chart together (e.g. multiple xy line chart, http://c3js.org/samples/simple_xy_multiple.html).
