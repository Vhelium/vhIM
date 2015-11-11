-- MySQL dump 10.15  Distrib 10.0.22-MariaDB, for Linux (x86_64)
--
-- Host: localhost    Database: vhIM
-- ------------------------------------------------------
-- Server version	10.0.22-MariaDB-log

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `friend_requests`
--

DROP TABLE IF EXISTS `friend_requests`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `friend_requests` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `friend_id1` mediumint(9) NOT NULL,
  `friend_id2` mediumint(9) NOT NULL,
  PRIMARY KEY (`id`),
  KEY `friend_id2` (`friend_id2`),
  KEY `friend_id1` (`friend_id1`),
  CONSTRAINT `friend_requests_ibfk_1` FOREIGN KEY (`friend_id2`) REFERENCES `users` (`id`),
  CONSTRAINT `friend_requests_ibfk_2` FOREIGN KEY (`friend_id1`) REFERENCES `users` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `friend_requests`
--

LOCK TABLES `friend_requests` WRITE;
/*!40000 ALTER TABLE `friend_requests` DISABLE KEYS */;
/*!40000 ALTER TABLE `friend_requests` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `friends`
--

DROP TABLE IF EXISTS `friends`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `friends` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `friend_id1` mediumint(9) NOT NULL,
  `friend_id2` mediumint(9) NOT NULL,
  PRIMARY KEY (`id`),
  KEY `friend_id1` (`friend_id1`),
  KEY `friend_id2` (`friend_id2`),
  CONSTRAINT `friends_ibfk_1` FOREIGN KEY (`friend_id1`) REFERENCES `users` (`id`),
  CONSTRAINT `friends_ibfk_2` FOREIGN KEY (`friend_id2`) REFERENCES `users` (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=17 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `friends`
--

LOCK TABLES `friends` WRITE;
/*!40000 ALTER TABLE `friends` DISABLE KEYS */;
INSERT INTO `friends` VALUES (7,6,5),(8,5,6),(9,6,12),(10,12,6),(15,13,5),(16,5,13);
/*!40000 ALTER TABLE `friends` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `groups`
--

DROP TABLE IF EXISTS `groups`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `groups` (
  `id` mediumint(9) NOT NULL AUTO_INCREMENT,
  `name` text,
  `owner_id` mediumint(9) NOT NULL,
  PRIMARY KEY (`id`),
  KEY `owner_id` (`owner_id`),
  CONSTRAINT `groups_ibfk_1` FOREIGN KEY (`owner_id`) REFERENCES `users` (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=6 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `groups`
--

LOCK TABLES `groups` WRITE;
/*!40000 ALTER TABLE `groups` DISABLE KEYS */;
INSERT INTO `groups` VALUES (1,'testgroup',5),(2,'somegrp',5),(3,'anothergroup',13),(4,'testgrp',5),(5,'testgrp2',5);
/*!40000 ALTER TABLE `groups` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `users`
--

DROP TABLE IF EXISTS `users`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `users` (
  `id` mediumint(9) NOT NULL AUTO_INCREMENT,
  `user` text,
  `password` char(64) DEFAULT NULL,
  `salt` tinytext,
  `privilege` tinyint(3) unsigned NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=14 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `users`
--

LOCK TABLES `users` WRITE;
/*!40000 ALTER TABLE `users` DISABLE KEYS */;
INSERT INTO `users` VALUES (1,'tester','588BFE00F47B3754D1ADA7D72426B6D8929CCC86FB63FF147289EB301143DBE4','00638457EDE129C1D5296C3A734F93CEB27805292A1051A436DE707F872282BD',0),(3,'vh','26B30277255FA887D7D6B9D2863F4AE0491D2B0D3B99577745D37CCEFD06E8BC','12345678123456781234567812345678',0),(4,'test123','C1305DAF08079A98D721284898A8899A5841792680D05CCF9D717060971C2A49','12345678123456781234567812345678',0),(5,'vhel','C1305DAF08079A98D721284898A8899A5841792680D05CCF9D717060971C2A49','12345678123456781234567812345678',9),(6,'testuser','ECB735690B0E6F72B7758C6EF70A406B1459DD4A72DA9F6572C4C1AEFE6F7070','12345678123456781234567812345678',0),(9,'vhelium','68CA1F910C37FDC28D2824CDDE5A3155CE7423CFA4ED7226A71220A1A42881B2','12D394E70CC8539C072324755B9D5E39FD53414C5D02411287680F6A77072025',0),(12,'abcdefg','B67D48CBD6476D9360C62215A1D49DC7737077FF579379B099BA24A7871E9C35','F6671347BB9A43BB24A4C76CD1F040134F8A2651D105EC7C2FF7519C8F4A77ED',0),(13,'asdf','4E556A81F126BABDCF3565B54A478D39FE376E8039354A06B8BBAFB0E77099BF','88CCFE9BE68E50E244EDE6244F84933FDD4720A3CD1DDE09FCFFB867958EF25E',0);
/*!40000 ALTER TABLE `users` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `users_to_group`
--

DROP TABLE IF EXISTS `users_to_group`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `users_to_group` (
  `id` mediumint(9) NOT NULL AUTO_INCREMENT,
  `user_id` mediumint(9) NOT NULL,
  `group_id` mediumint(9) NOT NULL,
  PRIMARY KEY (`id`),
  KEY `users_to_group_ibfk_1` (`user_id`),
  KEY `users_to_group_ibfk_2` (`group_id`),
  CONSTRAINT `users_to_group_ibfk_1` FOREIGN KEY (`user_id`) REFERENCES `users` (`id`) ON DELETE CASCADE,
  CONSTRAINT `users_to_group_ibfk_2` FOREIGN KEY (`group_id`) REFERENCES `groups` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB AUTO_INCREMENT=10 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `users_to_group`
--

LOCK TABLES `users_to_group` WRITE;
/*!40000 ALTER TABLE `users_to_group` DISABLE KEYS */;
INSERT INTO `users_to_group` VALUES (1,13,1),(4,5,1),(5,5,3),(6,5,4),(7,5,5),(8,13,5),(9,6,5);
/*!40000 ALTER TABLE `users_to_group` ENABLE KEYS */;
UNLOCK TABLES;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2015-11-11 16:40:44
