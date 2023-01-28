#ifndef _RADIUS_CONSTANTS_H_
#define _RADIUS_CONSTANTS_H_


/* RADIUS server constants */
#define RADIUS_AUTH_PORT                         1812
#define RADIUS_ACCT_PORT                         1813

#define RADIUS_MAX_MTU                           4096

/* Custom */
#define RAD_SESSION_DHCP_START                  11
#define RAD_SESSION_DHCP_END                    12

/* Message codes */
#define RAD_ACCESS_REQUEST 1
#define RAD_ACCESS_ACCEPT  2
#define RAD_ACCESS_REJECT  3
#define RAD_ACCT_REQUEST  4
#define RAD_ACCT_RESPONSE 5
#define RAD_ACCESS_CHALLENGE 11
#define RAD_STATUS_SERVER 12
#define RAD_STATUS_CLIENT 13

/* Attribute Types */
#define RAD_ATTR_USER_NAME                    1
#define RAD_ATTR_USER_PASSWORD                2
#define RAD_ATTR_CHAP_PASSWORD                3
#define RAD_ATTR_NAS_IP_ADDRESS               4
#define RAD_ATTR_NAS_PORT                     5
#define RAD_ATTR_SERVICE_TYPE                 6
#define RAD_ATTR_FRAMED_PROTOCOL              7
#define RAD_ATTR_FRAMED_IP_ADDRESS            8
#define RAD_ATTR_FRAMED_IP_NETMASK            9
#define RAD_ATTR_FRAMED_ROUTING               10
#define RAD_ATTR_FILTER_ID                    11
#define RAD_ATTR_FRAMED_MTU                   12
#define RAD_ATTR_FRAMED_COMPRESSION           13
#define RAD_ATTR_LOGIN_IP_HOST                14
#define RAD_ATTR_LOGIN_SERVICE                15
#define RAD_ATTR_LOGIN_TCP_PORT               16
#define RAD_ATTR_OLD_PASSWORD                 17
#define RAD_ATTR_REPLY_MESSAGE                18
#define RAD_ATTR_CALLBACK_NUMBER              19
#define RAD_ATTR_CALLBACK_ID                  20
#define RAD_ATTR_EXPIRATION                   21
#define RAD_ATTR_FRAMED_ROUTE                 22
#define RAD_ATTR_FRAMED_IPXNET                23
#define RAD_ATTR_STATE                        24
#define RAD_ATTR_CLASS                        25
#define RAD_ATTR_VENDOR_SPECIFIC              26
#define RAD_ATTR_SESSION_TIMEOUT              27
#define RAD_ATTR_IDLE_TIMEOUT                 28
#define RAD_ATTR_CALLED_STATION_ID            30
#define RAD_ATTR_CALLING_STATION_ID           31
#define RAD_ATTR_NAS_IDENTIFIER               32
#define RAD_ATTR_PROXY_STATE                  33
//
#define RAD_ATTR_ACCT_STATUS_TYPE             40
#define RAD_ATTR_ACCT_DELAY_TIME              41
#define RAD_ATTR_ACCT_INPUT_OCTETS            42
#define RAD_ATTR_ACCT_OUTPUT_OCTETS           43
#define RAD_ATTR_ACCT_SESSION_ID              44
#define RAD_ATTR_ACCT_AUTHENTIC               45
#define RAD_ATTR_ACCT_SESSION_TIME            46
#define RAD_ATTR_ACCT_INPUT_PACKETS           47
#define RAD_ATTR_ACCT_OUTPUT_PACKETS          48
#define RAD_ATTR_ACCT_TERMINATE_CAUSE         49
//
#define RAD_ATTR_EVENT_TIMESTAMP              55
//
#define RAD_ATTR_CHAP_CHALLENGE               60
#define RAD_ATTR_NAS_PORT_TYPE                61
#define RAD_ATTR_PORT_LIMIT                   62
//
#define RAD_ATTR_ARAP_PASSWORD                70
#define RAD_ATTR_ARAP_FEATURES                71
#define RAD_ATTR_ARAP_ZONE_ACCESS             72
#define RAD_ATTR_ARAP_SECURITY                73
#define RAD_ATTR_ARAP_SECURITY_DATA           74
#define RAD_ATTR_PASSWORD_RETRY               75
#define RAD_ATTR_PROMPT                       76
#define RAD_ATTR_CONNECT_INFO                 77
#define RAD_ATTR_CONFIGURATION_TOKEN          78
#define RAD_ATTR_EAP_MESSAGE                  79
#define RAD_ATTR_MESSAGE_AUTHENTICATOR        80
//
#define RAD_ATTR_ARAP_CHALLENGE_RESPONSE      84
#define RAD_ATTR_NAS_PORT_ID_STRING           87
#define RAD_ATTR_FRAMED_POOL                  88
#define RAD_ATTR_CHARGEABLE_USER_IDENTITY     89
#define RAD_ATTR_NAS_IPV6_ADDRESS             95

#endif
