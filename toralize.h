#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>


#define PROXY "127.0.0.1"
#define PROXPORT 9050
#define USERNAME "toralize"
#define reqsize sizeof(struct proxy_request)
#define ressize sizeof(struct proxy_response)


typedef unsigned char int8;
typedef unsigned short int16;
typedef unsigned int int32;

/*
SOCKS v4 協定格式
簡介:描述如何在防火牆/中介主機上轉送（relay）TCP 連線的輕量協定規格
目的:SOCKS 的目的是讓內網或受限主機經由一個 SOCKS 伺服器，
透明地存取外部應用服務（telnet、ftp、WWW、finger、whois 等），同時在伺服器端可做存取控制與記錄。
*/

/*
設計原則
1. SOCKSv4 不解析應用層協定（例如 HTTP、FTP），僅轉發 TCP 資料流，因而容易配合使用加密應用。
2. 在授權後，伺服器僅做位元流轉發，處理負載小，開銷低
3. 伺服器可根據來源 IP、目的 IP/port、userid、或透過 IDENT（RFC 1413）查詢的帳號來允許/拒絕請求。
*/

/*
基本封包格式（byte-level）
VN: 版本號 (1 byte)
CD: 命令碼 (1 byte)
DSTPORT: 目的埠 (2 bytes)
DSTIP: 目的IP (4 bytes)
USERID: 用戶ID (variable bytes)
NULL: 結尾 (1 byte)
Client → SOCKS（請求）（CONNECT 或 BIND 共用格式）：
            +----+----+----+----+----+----+----+----+----+----+....+----+
            | VN | CD | DSTPORT |      DSTIP        | USERID       |NULL|
            +----+----+----+----+----+----+----+----+----+----+....+----+
# of bytes:	   1    1      2              4           variable       1
*/


/*
SOCKS4a 擴展：
- 支援在代理端進行 DNS 解析（避免 DNS 洩漏）
- 當 DSTIP = 0.0.0.x (x != 0) 時，表示 SOCKS4a 請求
- 格式：VN | CD | DSTPORT | 0.0.0.x | USERID | NULL | DOMAIN | NULL
*/

struct proxy_request{
    int8 vn;
    int8 cd;
    int16 dstport;
    int32 dstip;
    unsigned char userid[8];
};

typedef struct proxy_request Req;


/*
VN：回覆中的 VN 固定為 0（一個位元組，注意不是 4）。

CD：回覆碼（90/91/92/93，見下）。

DSTPORT, DSTIP：在成功的 BIND
SOCKS → Client（回覆）
                +----+----+----+----+----+----+----+----+
                | VN | CD | DSTPORT |      DSTIP        |
                +----+----+----+----+----+----+----+----+
 # of bytes:	   1    1      2              4
*/

struct proxy_response{
    int8 vn;
    int8 cd;
    int16 _;
    int32 __;
};

typedef struct proxy_response Res;

Req *request(const char *,const int);


int main(int,char**);