#include "Util.h"

#include "TrException.h"
//#include "libnetlink/libnetlink.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
//#include <asm/types.h>
#include <sys/ioctl.h>
//#include <linux/netlink.h>
//#include <linux/rtnetlink.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

/**
 * Return the source address (in network endianess) to use to contact the
 * destination <i>addr</i>.
 *
 * @param addr The destination address
 * @return The source address
 */
char*
Util::getRoute (const char* dest) {
  FILE * fd;
  char buff[20];
	int sock;
	struct sockaddr_in addr, name;
	int len;
	char *dst_addr;
  // Ouvre un tube nomm�
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		perror("socket");
		return NULL;
	}

	bzero(&addr, sizeof(struct sockaddr_in));

	addr.sin_port = htons(1234);
	/* XXX */
	if (dest == NULL)
		dst_addr = "216.239.51.100";
	else
		dst_addr = (char *)dest;
	
	addr.sin_addr.s_addr = inet_addr(dst_addr);
	addr.sin_family = AF_INET;
	
	if (connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr)) == -1)
	{
		perror("connect");
		return NULL;
	}
	
	len = sizeof(struct sockaddr);
	if (getsockname(sock, (struct sockaddr *)&name, (socklen_t *)&len) == -1)
	{
		perror("getsockname");
		return NULL;
	}
	
	//printf("%s\n", inet_ntoa(name.sin_addr));
	
	close(sock);

	return strdup(inet_ntoa(name.sin_addr));

#ifdef __APPLE__
  fd = popen(" a=`/usr/sbin/netstat -rn | grep default`; /sbin/ifconfig `echo $a | cut -d ' ' -f 6` | grep \"inet \" | cut -d ' ' -f 2", "r");
#endif

#ifdef __FreeBSD__
	fd = popen(" a=`netstat -rn | grep default`; /sbin/ifconfig `echo $a | cut -d ' ' -f 6` | grep \"inet \" | cut -d ' ' -f 2", "r");

#endif

#ifdef __NetBSD__
  
  fd = popen(" a=`/usr/bin/netstat -rn | grep default`; /sbin/ifconfig `echo $a | cut -d ' ' -f 7` | grep \"inet \" | cut -d ' ' -f 2", "r");
#endif
	
#ifdef __linux__
  fd = popen(" a=`/sbin/route -n | grep default`; /sbin/ifconfig `echo $a | cut -d ' ' -f 8` | grep \"inet \" | cut -d ':' -f 2 | cut -d ' ' -f 1", "r");
#endif

  fscanf(fd, "%s", buff);
  pclose(fd);

  log(INFO, "Source address = %s\n", buff);

  return strdup(buff);
}

/**
 * This function return the IP address of the interface through which
 * a packet destined for "addr" will go. This function uses libnetlink
 * to consult the kernel routing tables. Stolen from the guts of
 * iproute_get() and print_route() from the iproute package.
 */
/*char*
Util::getRoute (const char* dest) {
  in_addr addr;
  int res = inet_aton(dest, &addr);

  struct rtnl_handle rth;
  struct {
    struct nlmsghdr         n;
    struct rtmsg            r;
    char                    buf[1024];
  } req;
  int len;
  struct rtattr * tb[RTA_MAX+1];

  memset(&req, 0, sizeof(req));

  req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
  req.n.nlmsg_flags = NLM_F_REQUEST;
  req.n.nlmsg_type = RTM_GETROUTE;
  req.r.rtm_family = AF_INET;

  addattr_l(&req.n, sizeof(req), RTA_DST, &addr, sizeof(addr));
  req.r.rtm_dst_len = sizeof(addr) << 3;

  if (rtnl_open(&rth, 0) < 0) exit(1);

  if (rtnl_talk(&rth, &req.n, 0, 0, &req.n, NULL, NULL) < 0) exit(2);

  len = req.n.nlmsg_len - NLMSG_LENGTH(sizeof(struct rtmsg));
  memset(tb, 0, sizeof(tb));
  parse_rtattr(tb, RTA_MAX, RTM_RTA(NLMSG_DATA(&req.n)), len);

  if (tb[RTA_PREFSRC]) {
    struct in_addr ret_addr;
    ret_addr.s_addr = *((in_addr_t *) RTA_DATA(tb[RTA_PREFSRC]));
    return strdup(inet_ntoa(ret_addr));
  } else {
    return NULL;
  }
}*/

/**
 * Compute Internet Checksum.
 *
 * @param datagram The datagram
 * @param length The length of the datagram
 *
 * Cfr. RFC 1071
 */
uint16
Util::computeChecksum (const uint16* datagram, int length) {
  uint32 sum = 0;

  while (length > 1) {
    sum += *datagram++;
    length -= 2;
  }

  if (length > 0) sum += *(uint8*)datagram;

  while (sum >> 16) sum = (sum & 0xffff) + (sum >> 16);

  return ~sum;
}

/**
 * Extract a unsigned 16-bits integer from an array in network endianess
 * (big endian).
 */
uint16
Util::readbe16 (uint8* data, int ofs) {
  return ntohs(*((uint16*)(data+ofs)));
}

/**
 * Extract a unsigned 32-bits integer from an array in network endianess
 * (big endian).
 */
uint32
Util::readbe32 (uint8* data, int ofs) {
  return ntohl(*((uint32*)(data+ofs)));
}

/**
 * Extract a unsigned 16-bits integer from an array in current system endianess.
 */
uint16
Util::read16 (uint8* data, int ofs) {
  return *((uint16*)(data+ofs));
}

/**
 * Extract a unsigned 32-bits integer from an array in current system endianess.
 */
uint32
Util::read32 (uint8* data, int ofs) {
  return *((uint32*)(data+ofs));
}

/**
 * Insert an unsigned 16-bits integer into an array in network endianess
 * (big endian).
 */
void
Util::writebe16 (uint8* data, int ofs, uint16 value) {
  uint16* ptr = (uint16*)(data + ofs);
  *ptr = htons(value);
}

/**
 * Insert an unsigned 32-bits integer into an array in network endianess
 * (big endian).
 */
void
Util::writebe32 (uint8* data, int ofs, uint32 value) {
  uint32* ptr = (uint32*)(data + ofs);
  *ptr = htonl(value);
}

/**
 * Insert an unsigned 16-bits integer into an array in current system endianess.
 */
void
Util::write16 (uint8* data, int ofs, uint16 value) {
  uint16* ptr = (uint16*)(data + ofs);
  *ptr = value;
}

/**
 * Insert an unsigned 32-bits integer into an array in current system endianess.
 */
void
Util::write32 (uint8* data, int ofs, uint32 value) {
  uint32* ptr = (uint32*)(data + ofs);
  *ptr = value;
}

/**
 * Consult "/etc/protocol" to obtain the number associated to a protocol.
 */
int
Util::protocol2int (const char* protocol) {
  struct protoent* proto = getprotobyname(protocol);
  log(DUMP, "p_proto = %d", proto->p_proto);
  return proto->p_proto;
}

char*
Util::my_inet_ntoa(uint32 addr) {
	struct in_addr host_addr;
 	host_addr.s_addr         = addr;
  return inet_ntoa(host_addr);
}

uint32
Util::my_inet_aton(char *address) {
	struct in_addr buff;
  int res = inet_aton(address, &buff);
  if (res == 0)
    throw TrException(str_log(ERROR,
			"Invalid destination address : %s", address));
	
	return buff.s_addr;
}

char*
Util::my_gethostbyname(char* host) {
	// Parse destination address
 struct in_addr host_addr;
 int res = inet_aton(host, &host_addr);
 if (res != 0) {
   // numbers-and-dot notation
   return strdup(host);
 } else {
   // resolve hostname
   struct hostent* phost = gethostbyname(host);
   if (phost == NULL || phost->h_addrtype != AF_INET) {
     // Not a valid IP4 address
     log(ERROR, "Invalid address");
     return NULL;
   }
   struct in_addr *buff = (in_addr*)phost->h_addr_list[0];
   char* dst_addr = strdup(inet_ntoa(*buff));
   log(DUMP, "dst_addr = %s", dst_addr);
   return dst_addr;
 }
}

/**
 * Resolve and return the host name associated to an host address.
 * The result is a string which has to be freed by the caller.
 *
 * @param host_address The host address in numbers-and-dots notation.
 *
 * @throw TrException A error occured
 */
/*char*
Util::getHostName (const char* host_address) {
  struct in_addr addr;
  int res = inet_aton(host_address, &addr);
  if (res < 0) throw TrException(str_log(ERROR,
		    "Cannot convert %s to 'in_addr'", host_address));
  struct hostent* host = gethostbyaddr((char *)&addr, sizeof(in_addr), AF_INET);
  if (host == NULL) throw TrException(str_log(ERROR,
	"Error in gethostbyaddr(%s) : %s", host_address, strerror(h_errno)));
  return strdup(host->h_addr_list[0]);
}*/

/**
 * Resolve and return the host address associated to an host name.
 * The result is a string which has to be freed by the caller.
 *
 * @param host_address An host name or host address.
 *
 * @throw TrException A error occured
 */
/*char*
Util::getHostAddress (const char* host_name) {
  struct hostent* host = gethostbyname(host_name);
  if (host != NULL) throw TrException(str_log(ERROR,
	"Error in gethostbyname(%s) : %s", host_name, strerror(h_errno)));
  return strdup(host->h_name);
}*/

