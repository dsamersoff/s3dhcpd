-- This SQL file provided for documentation purpose
-- Minimal required static database structure
-- Notice, mac address have to be in uppercase
DROP table IF EXISTS dhcp_asmt;
CREATE TABLE "dhcp_asmt" (
    "mac" varchar(20),
    "ip" varchar(20)
);


DROP index IF EXISTS dhcp_asmt_mac_idx;
CREATE INDEX dhcp_asmt_mac_idx on dhcp_asmt(mac);

--- TEST DATA --
INSERT INTO dhcp_asmt (mac, ip) VALUES ('18:E7:28:F0:2A:55', '10.20.0.77');

-- Structure of lease database
-- Daemon creates and populates the database for you automatically
DROP table IF EXISTS dhcp_leases;
CREATE TABLE "dhcp_leases" (
    "mac" varchar(20),
    "ip" varchar(20),
    "ts" integer,
    "expire" integer
);

DROP index IF EXISTS dhcp_leases_mac_idx;
CREATE INDEX dhcp_leases_mac_idx on dhcp_leases(mac,ts);

DROP index IF EXISTS dhcp_leases_ts_idx;
CREATE INDEX dhcp_leases_ts_idx on dhcp_leases(ts);

DROP index IF EXISTS dhcp_leases_ip_idx;
CREATE UNIQUE INDEX dhcp_leases_ip_idx on dhcp_leases(ip);
