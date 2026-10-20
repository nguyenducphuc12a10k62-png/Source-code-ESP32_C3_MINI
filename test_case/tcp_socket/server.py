import socketserver
import socket

class MyTCPHandler(socketserver.BaseRequestHandler):
    def handle(self):
        client_ip = self.client_address[0]
        print(f"\n[+] ESP32 đã kết nối từ: {client_ip}")
        
        # Timeout vô hạn để chờ ESP32
        self.request.settimeout(None)

        while True:
            try:
                # 1. Nhận dữ liệu
                data = self.request.recv(1024)
                
                if not data:
                    print(f"[-] ESP32 ({client_ip}) đã ngắt kết nối.")
                    break

                # 2. Xử lý hiển thị & Tách dòng nếu bị dính
                try:
                    text = data.decode('utf-8')
                    # Tách chữ nếu bị dính chùm (VD: Button..Button..)
                    formatted_text = text.replace("Button", "\nButton")
                    print(formatted_text.strip())
                except UnicodeDecodeError:
                    print(f"<Binary Data: {len(data)} bytes>")

                # 3. QUAN TRỌNG: KHÔNG GỬI ACK NỮA!
                # self.request.sendall(b"ACK\n")  <-- Đã bỏ dòng này
                # Lý do: ESP32 của bạn không đọc về, gửi đi sẽ làm tắc mạng.

            except (TimeoutError, ConnectionResetError, OSError) as e:
                # Bắt lỗi 10060 nhưng không in ra màu đỏ đáng sợ nữa
                # Đây chỉ là thông báo phiên cũ đã hết hạn
                print(f"[*] Phiên kết nối cũ đã đóng ({e}). Đang chờ kết nối mới...")
                break
            
            except Exception as e:
                print(f"[!] Lỗi: {e}")
                break

class ThreadedTCPServer(socketserver.ThreadingMixIn, socketserver.TCPServer):
    allow_reuse_address = True
    daemon_threads = True 

if __name__ == "__main__":
    HOST, PORT = "0.0.0.0", 3333
    
    server = ThreadedTCPServer((HOST, PORT), MyTCPHandler)
    print(f"==========================================")
    print(f"[*] Server đang chạy tại {HOST}:{PORT}")
    print(f"[*] Sẵn sàng nhận nút nhấn. (Đã tắt phản hồi ACK)")
    print(f"==========================================")
    
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n[!] Đã dừng Server.")