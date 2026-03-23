DROP TABLE IF EXISTS `bms`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `bms` (
  `date` timestamp NOT NULL DEFAULT current_timestamp() ON UPDATE current_timestamp(),
  `voltage` double DEFAULT NULL,
  `current` double DEFAULT NULL,
  `soc` double DEFAULT NULL,
  `cycles` double DEFAULT NULL,
  `power_temp` double DEFAULT NULL,
  `battery_temp` double DEFAULT NULL,
  `cell0_voltage` double DEFAULT NULL,
  `cell1_voltage` double DEFAULT NULL,
  `cell2_voltage` double DEFAULT NULL,
  `cell3_voltage` double DEFAULT NULL,
  `cell_voltage_delta` double DEFAULT NULL,
  `charging_enabled` bool DEFAULT NULL,
  `discharging_enabled` bool DEFAULT NULL,
  `ischarging` bool DEFAULT NULL,
  `isdischarging` bool DEFAULT NULL,
  PRIMARY KEY (`date`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;
