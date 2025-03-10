# s3dhcpd
libpcap based **DHCP server** designed designed primarily for serving mobile clients. **Simple and stable**

It has been running for over 15 years in several large hotels and continues to operate in my home.  

### Features:  
- **Stability** – Uptime of several years under hotel-level load.  
- **Simplicity** – Easy configuration.  
- **Dynamic update of static addresses table** – No need to restart the server to modify the static table.  
- **BSD PF integration** – Can add/remove IP address from firewall table, therefor protect internal clients, such as printers, and provide support from the landing with authorization.  
- **Radius Server statistics** – Supports sending usage statistics to a Radius server.  

### Known Issues:  
1) **Legacy-style code** – Many things could be implemented more efficiently today.  
2) **Lack of documentation** – This will be addressed if there is sufficient interest.
