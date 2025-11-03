# Toralize

一個使用 SOCKS v4 協議通過 Tor 代理進行網路連接的輕量級 C 語言工具。


## 專案簡介

Toralize 是一個簡單而有效的命令列工具，能夠通過本地 Tor SOCKS 代理（預設在 `127.0.0.1:9050`）將 TCP 連線匿名化。此工具實作了 SOCKS v4 協議，讓您能夠透過 Tor 網路安全地連接到任何目標主機和端口。

##  主要功能

-  **SOCKS 4a 協議實作**：支援 SOCKS4a 擴展，DNS 解析在 Tor 端進行
-  **無 DNS 洩漏**：使用域名時自動啟用 SOCKS4a，避免本地 DNS 查詢
-  **Tor 網路整合**：自動連接到本地 Tor SOCKS 代理
-  **超時保護**：內建 10 秒連接和讀寫超時機制
-  **HTTP 測試**：自動發送 HTTP HEAD 請求驗證連接
-  **智能檢測**：自動識別輸入是域名還是 IP，選擇適當的協議

## 快速開始
```bash
# 1. 啟動 Tor
sudo systemctl start tor

# 2. 編譯
make

# 3. 執行（推薦使用域名）
./toralize example.com 80

```


##  DNS 洩漏問題與 SOCKS4a 解決方案

###  DNS 洩漏是什麼？

**DNS 洩漏**是匿名瀏覽中的一個嚴重隱私問題：

```
傳統方式（不安全）：
你的電腦 → [DNS 查詢: example.com?] → ISP 的 DNS 服務器
                                            ↓
                              你的 ISP 知道你要訪問 example.com！
                                            ↓
你的電腦 ← [回應: 93.184.216.34] ←─────────┘
    ↓
[通過 Tor 連接到 93.184.216.34]  ← 這部分雖然加密，但為時已晚
```

**問題**：即使後續流量通過 Tor 加密，你的 ISP 或 DNS 提供商**已經知道你想訪問哪個網站**！

###  SOCKS4a 如何解決這個問題

本程式使用 **SOCKS4a** 協議擴展來避免 DNS 洩漏：

```
使用 SOCKS4a（安全）：
你的電腦 → [SOCKS4a 請求: "example.com"] → Tor 代理
                                              ↓
                                  [加密 Tor 隧道傳輸域名]
                                              ↓
                                  Tor 出口節點 → [DNS 查詢: example.com?] → DNS
                                              ↓
                                  [連接到目標網站]
```

**優勢**：
-  你的 ISP **看不到**任何 DNS 查詢
-  DNS 查詢通過 Tor 網路進行
-  完全匿名，無洩漏

###  程式自動選擇協議

```bash
# 輸入域名 → 自動使用 SOCKS4a（無 DNS 洩漏）
./toralize example.com 80
# 輸出：Using SOCKS4a for domain: example.com (DNS resolved by Tor - no leak!)

# 輸入 IP → 使用標準 SOCKS4（無需 DNS）
./toralize 93.184.216.34 80
# 輸出：Using SOCKS4 with IP: 93.184.216.34
```

##  技術規格

### SOCKS v4 和 SOCKS4a 協議說明

**設計原則：**
1. SOCKSv4 不解析應用層協定（如 HTTP、FTP），僅轉發 TCP 資料流，因而容易配合使用加密應用
2. 在授權後，伺服器僅做位元流轉發，處理負載小，開銷低
3. 伺服器可根據來源 IP、目的 IP/port、userid 來允許/拒絕請求

**封包格式：**

客戶端請求格式：
```
+----+----+----+----+----+----+----+----+----+----+....+----+
| VN | CD | DSTPORT |      DSTIP        | USERID       |NULL|
+----+----+----+----+----+----+----+----+----+----+....+----+
  1    1      2              4           variable       1
```

伺服器回覆格式：
```
+----+----+----+----+----+----+----+----+
| VN | CD | DSTPORT |      DSTIP        |
+----+----+----+----+----+----+----+----+
  1    1      2              4
```

**回覆碼（CD）：**
- `90` - 請求已授權
- `91` - 請求被拒絕或失敗
- `92` - 無法連接到 identd
- `93` - identd 回報的用戶 ID 不符

##  系統需求

- Linux/Unix 作業系統
- GCC 編譯器
- Tor 服務（執行在 127.0.0.1:9050）

##  安裝與編譯

### 1. 安裝和啟動 Tor

**Ubuntu/Debian：**
```bash
sudo apt update
sudo apt install tor
sudo systemctl start tor
sudo systemctl enable tor
```


**驗證 Tor 運行狀態：**
```bash
sudo systemctl status tor
# 或檢查 SOCKS 端口是否在監聽
netstat -tlnp | grep 9050
```

### 2. 編譯專案

```bash
cd /home/paulboul/cyber_security/toralize
make
```

或手動編譯：
```bash
gcc toralize.c -o toralize
```

##  使用方法

### 基本語法

```bash
./toralize <目標主機> <目標端口>
```

**支援的主機格式：**
-  **域名**（推薦）：自動使用 SOCKS4a，DNS 在 Tor 端解析，**無洩漏** 
-  **IP 地址**：使用標準 SOCKS4，直接連接

### 使用範例

**範例 1：使用域名（推薦 - 完全匿名）** 
```bash
./toralize example.com 80

# 輸出：
# Connected to proxy
# Using SOCKS4a for domain: example.com (DNS resolved by Tor - no leak!)
# Successfully connected through the proxy to example.com:80
# HTTP Response: HTTP/1.0 200 OK ...
```

**為什麼推薦使用域名？**
-  DNS 查詢通過 Tor 進行（SOCKS4a）
-  ISP 無法看到你訪問的網站
-  完全匿名，無隱私洩漏
-  自動獲取最新的 IP 地址

**範例 2：使用 IP 地址**
```bash
./toralize 93.184.216.34 80

# 輸出：
# Connected to proxy
# Using SOCKS4 with IP: 93.184.216.34
# Successfully connected through the proxy to 93.184.216.34:80
```

**注意**：使用 IP 時無 DNS 查詢，但你需要確保 IP 地址正確且有效。

**範例 3：測試不同的網站**
```bash
# 測試網站（SOCKS4a - 安全）
./toralize example.com 80
./toralize www.google.com 80
./toralize www.youtube.com 80

# 所有這些都使用 SOCKS4a，DNS 在 Tor 端解析，完全安全！
```

### 輸出範例

成功連接時的輸出：
```
Connected to proxy
Successfully connected through the proxy to www.google.com:80
Response:
HTTP/1.0 200 OK
Date: Mon, 03 Nov 2025 12:00:00 GMT
Content-Type: text/html
...
```

失敗時的輸出：
```
Unable to traverse the proxy, error code: 91
```


##  配置說明

預設配置定義在 `toralize.h` 中：

```c
#define PROXY "127.0.0.1"     // Tor SOCKS 代理 IP
#define PROXPORT 9050          // Tor SOCKS 代理端口
#define USERNAME "toralize"    // SOCKS 用戶名稱
```

如需修改代理設定，請編輯 `toralize.h` 後重新編譯。


### 安全特性

- **記憶體安全**：使用 `memset` 初始化緩衝區，防止資訊洩漏
- **字串安全**：使用 `strncpy` 限制複製長度，防止緩衝區溢位
- **超時保護**：設定 10 秒超時，防止無限期等待
- **錯誤處理**：完整的錯誤檢查和資源釋放機制

## 🧪 測試建議

### 測試 1：驗證匿名性
```bash
# 在使用 Toralize 前後，檢查您的 IP 地址
curl https://api.ipify.org
# 應該顯示 Tor 出口節點的 IP，而非您的真實 IP
```

### 測試 2：驗證無 DNS 洩漏 

可以驗證這一點：

```bash
# 使用 tcpdump 監聽本地 DNS 查詢
# 在第一個終端執行：
sudo tcpdump -i any port 53 -n

# 在第二個終端執行 toralize（使用域名）
./toralize example.com 80

#  您應該看不到 example.com 的 DNS 查詢！
# 因為 DNS 解析在 Tor 端進行，不在本地進行
```

**輸出應該是：**
```
# tcpdump 終端：什麼都沒有（或只有其他程式的查詢）

# toralize 終端：
Connected to proxy
Using SOCKS4a for domain: example.com (DNS resolved by Tor - no leak!)
Successfully connected through the proxy to example.com:80
```

**技術說明：**
- 當您使用域名時，程式使用 SOCKS4a 協議
- 域名直接發送到 Tor 代理（加密傳輸）
- Tor 在其出口節點進行 DNS 解析
- 您的 ISP 看不到任何 DNS 查詢
- 完全匿名，無洩漏

### 測試 3：連接測試
```bash
# 測試各種目標和端口
./toralize checkip.amazonaws.com 80
./toralize example.com 80
./toralize 1.1.1.1 80
```

##  注意事項

1. **Tor 必須運行**：使用前請確保 Tor 服務正在運行
2. **僅支援 TCP**：此工具僅支援 TCP 連接，不支援 UDP
3. **DNS 解析**：DNS 解析可能不經過 Tor，建議使用 IP 地址或配合其他工具
4. **不是完整的匿名解決方案**：建議配合 Tor Browser 或 torsocks 使用
5. **效能考量**：通過 Tor 的連接速度會較慢

## 🐛 故障排除

### 問題：連接失敗 "connect: Connection refused"
**解決方法：**
```bash
# 檢查 Tor 是否正在運行
sudo systemctl status tor
# 或
ps aux | grep tor
```

### 問題："Resource temporarily unavailable" 或讀取超時

**症狀：**
```
Connected to proxy
read SOCKS response: Resource temporarily unavailable
```

**常見原因和解決方法：**

**原因 1：使用了無效或過期的 IP 地址** 

網站的 IP 地址會頻繁變化（CDN、負載均衡等）。您使用的 IP 可能已經過期或無效。

```bash
#  錯誤：使用可能已過期的 IP
./toralize 93.184.216.34 80  # 可能無法連接

#  正確：使用域名（自動獲取最新 IP）
./toralize example.com 80

# 先驗證 IP 是否有效
curl -I http://93.184.216.34 --connect-timeout 5

# 如果 curl 失敗，說明 IP 無效，通過 toralize 也會失敗
```

**原因 2：目標主機真的不可達**

如果域名和 IP 都超時，可能目標主機真的down了或被防火牆封鎖。

```bash
# 測試直接連接（不通過 Tor）
curl -I http://example.com --connect-timeout 5

# 如果直接連接也失敗，說明目標主機本身有問題
```



**如果重新編譯後仍然錯誤 91：網站封鎖問題**

這是另一個常見原因！很多網站（如 Netflix）會主動封鎖 Tor。

**主要原因：**

1. **目標網站封鎖 Tor 出口節點**
   - 許多大型網站（Netflix、某些銀行網站等）會封鎖已知的 Tor 出口 IP
   - 因為 Tor 常被用於繞過地區限制或自動化操作

2. **使用了錯誤的端口**
   - 許多現代網站只支援 HTTPS (443)，不支援 HTTP (80)
   - 嘗試連接 80 端口會被拒絕

3. **SOCKS4 協議限制**
   - SOCKS4 是較舊的協議，某些網站可能不完全支援
   - SOCKS4 不支援 IPv6 和 UDP

4. **Tor 出口節點網路問題**
   - 出口節點可能暫時無法訪問目標網站
   - 出口節點的 ISP 可能封鎖了某些服務



**理解錯誤碼：**
```
90 - ✅ 成功
91 - ❌ 請求被拒絕或失敗（最常見）
92 - ❌ 無法連接到 client 的 identd
93 - ❌ identd 回報的用戶 ID 不符
```


## 📚 相關資源

- [Tor 專案官方網站](https://www.torproject.org/)
- [SOCKS v4 協議規範](https://www.openssh.com/txt/socks4.protocol)
- [SOCKS v4A 擴展](https://www.openssh.com/txt/socks4a.protocol)
- [Tor SOCKS 文件](https://2019.www.torproject.org/docs/tor-manual.html.en)

##  安全與隱私

此工具僅提供基本的流量路由功能。完整的匿名性和隱私保護需要：

- 使用 Tor Browser 進行網頁瀏覽
- 配合 torsocks 進行系統級代理
- 注意應用層的資訊洩漏（如 Cookies、指紋識別）
- 避免在 Tor 上使用個人帳號登入


