#include "toralize.h"

/*
  ./toralize  example.com 80
  ./toralize  1.2.3.5 80
  
  使用 SOCKS4a 協議避免 DNS 洩漏
*/

// 檢查字符串是否為 IP 地址
int is_ip_address(const char *str) {
    struct in_addr addr;
    return inet_pton(AF_INET, str, &addr) == 1;
}

// 為 SOCKS4a 創建請求（域名解析在 Tor 端進行）
char *request_socks4a(const char *domain, const int dstport, size_t *request_size) {
    size_t domain_len = strlen(domain);
    // SOCKS4a: 基本請求 + 域名 + NULL
    size_t total_size = reqsize + domain_len + 1;
    
    char *buf = malloc(total_size);
    if (!buf) return NULL;
    
    memset(buf, 0, total_size);
    
    Req *req = (Req *)buf;
    req->vn = 4;
    req->cd = 1;
    req->dstport = htons(dstport);
    req->dstip = htonl(0x00000001);  // 0.0.0.1 表示 SOCKS4a
    
    // 複製 username
    size_t len = strlen(USERNAME);
    if (len > 7) len = 7;
    memcpy(req->userid, USERNAME, len);
    req->userid[7] = 0;
    
    // 在基本請求之後附加域名
    memcpy(buf + reqsize, domain, domain_len);
    buf[reqsize + domain_len] = 0;  // NULL 終止
    
    *request_size = total_size;
    
    printf("Using SOCKS4a for domain: %s (DNS resolved by Tor - no leak!)\n", domain);
    
    return buf;
}

// 為 SOCKS4 創建請求（直接使用 IP）
Req *request_socks4(const char *ip, const int dstport) {
    Req *req = malloc(reqsize);
    if (!req) return NULL;
    
    memset(req, 0, reqsize);
    
    req->vn = 4;
    req->cd = 1;
    req->dstport = htons(dstport);
    req->dstip = inet_addr(ip);
    
    if (req->dstip == INADDR_NONE) {
        fprintf(stderr, "Invalid IP address: %s\n", ip);
        free(req);
        return NULL;
    }
    
    // 複製 username
    size_t len = strlen(USERNAME);
    if (len > 7) len = 7;
    memcpy(req->userid, USERNAME, len);
    req->userid[7] = 0;
    
    printf("Using SOCKS4 with IP: %s\n", ip);
    
    return req;
}

int main(int argc,char *argv[]){

    char *host;
    int port,s;
    struct sockaddr_in sock;
    Req *req;
    Res *res;
    char buf[ressize];
    int success;
    char tmp[512];

    /*predicate - property*/



    if (argc < 3){
        fprintf(stderr,"Usage: %s <host> <port>\n",argv[0]);
        return -1;
    }

    host=argv[1];
    port=atoi(argv[2]);


    s=socket(AF_INET,SOCK_STREAM,0);
    if (s<0) {
        perror("socket");

        return -1;
    }

    // 設置連接和讀寫超時 (10秒)
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    sock.sin_family=AF_INET;
    sock.sin_port=htons(PROXPORT);
    sock.sin_addr.s_addr=inet_addr(PROXY);

    if (connect(s,(struct sockaddr *)&sock,sizeof(sock))){
        perror("connect");

        return -1;
    }


    printf("Connected to proxy\n");
    
    // 根據輸入類型選擇 SOCKS4 或 SOCKS4a
    void *request_buf;
    size_t request_len;
    
    if (is_ip_address(host)) {
        // 使用 SOCKS4（直接 IP）
        req = request_socks4(host, port);
        if (req == NULL) {
            fprintf(stderr, "Failed to create SOCKS4 request\n");
            close(s);
            return -1;
        }
        request_buf = req;
        request_len = reqsize;
    } else {
        // 使用 SOCKS4a（域名，Tor 端解析 DNS）
        request_buf = request_socks4a(host, port, &request_len);
        if (request_buf == NULL) {
            fprintf(stderr, "Failed to create SOCKS4a request\n");
            close(s);
            return -1;
        }
    }
    
    ssize_t sent = write(s, request_buf, request_len);
    if (sent < 0) {
        perror("write");
        free(request_buf);
        close(s);
        return -1;
    }

    memset(buf,0,ressize);
    ssize_t received = read(s,buf,ressize);
    if (received < 1) {
        if (received == 0) {
            fprintf(stderr, "Connection closed by proxy (no SOCKS response)\n");
            fprintf(stderr, "Possible cause: Target host may be unreachable\n");
        } else {
            perror("read SOCKS response");
            fprintf(stderr, "\nTroubleshooting tips:\n");
            fprintf(stderr, "1. If using an IP address, verify it's current and valid\n");
            fprintf(stderr, "2. Try using the domain name instead: ./toralize example.com 80\n");
            fprintf(stderr, "3. Check if target is reachable: curl -I http://%s --connect-timeout 5\n", host);
        }
        free(request_buf);
        close(s);
        return -1;
    }

    res=(Res *)buf;
    success=(res->cd==90);
    if (!success){
        fprintf(stderr,"Unable to traverse the proxy, error code: %d\n",res->cd);
        
        close(s);
        free(request_buf);

        return -1;
    }

    printf("Successfully connected through the proxy to %s:%d\n",host,port);

    // 發送 HTTP HEAD 請求測試連接
    memset(tmp,0,512);
    snprintf(tmp,511,
        "HEAD / HTTP/1.0\r\n"
        "Host: %s\r\n"
        "User-Agent: toralize/1.0\r\n"
        "Connection: close\r\n"
        "\r\n", host
    );
    
    ssize_t written = write(s,tmp,strlen(tmp));
    if (written < 0) {
        perror("write HTTP request");
        close(s);
        free(request_buf);
        return -1;
    }
    
    memset(tmp,0,512);
    int bytes_read = read(s,tmp,511);
    if (bytes_read > 0) {
        printf("\nHTTP Response:\n%s\n",tmp);
    } else if (bytes_read == 0) {
        printf("\nNo HTTP response (connection closed by server)\n");
        printf("Note: This may happen when using IP addresses with Host header requirements\n");
    } else {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            printf("\nHTTP response timeout (this is often normal)\n");
            printf("Note: The SOCKS connection was successful!\n");
            printf("      Some servers don't respond to simple HTTP HEAD requests.\n");
        } else {
            perror("read HTTP response");
        }
    }

    
    close(s);
    free(request_buf);


    return 0;

    
}