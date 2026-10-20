import socketserver

class MyUDPHandler(socketserver.DatagramRequestHandler):
    """
    Handler này xử lý các datagram UDP đến.
    """
    
    # --- [THAY ĐỔI QUAN TRỌNG] ---
    # Biến tĩnh (Class Variable) để lưu IP của người gửi gần nhất.
    # Nó tồn tại xuyên suốt các lần nhận gói tin khác nhau.
    last_client_ip = None 

    def handle(self):
        # Lấy dữ liệu
        data_bytes = self.request[0].strip()
        client_ip, client_port = self.client_address
        
        # --- [LOGIC HIỂN THỊ GOM NHÓM] ---
        # Nếu IP người gửi hiện tại KHÁC IP người gửi trước đó
        if client_ip != MyUDPHandler.last_client_ip:
            # Thì mới in dòng Header chứa IP
            print(f"[+] Received datagram from {client_ip}:{client_port}")
            # Cập nhật lại biến ghi nhớ
            MyUDPHandler.last_client_ip = client_ip
        
        # Nếu IP giống IP cũ, bỏ qua dòng Header, chỉ in nội dung bên dưới
        
        # Giải mã dữ liệu
        try:
            text = data_bytes.decode(errors='replace')
        except:
            text = repr(data_bytes)
            
        print(f"Received: {text}")
        
        # Phản hồi (Echo) - Tùy chọn, có thể bỏ nếu ESP32 không nhận
        self.wfile.write(b"ACK from server: " + data_bytes)

if __name__ == "__main__":
    HOST, PORT = "0.0.0.0", 3333
    
    # Cho phép tái sử dụng cổng (tránh lỗi Address already in use khi chạy lại nhanh)
    socketserver.UDPServer.allow_reuse_address = True

    with socketserver.UDPServer((HOST, PORT), MyUDPHandler) as server:
        print(f"[*] UDP Server listening on {HOST}:{PORT}...")
        print(f"[*] Chế độ hiển thị: Gom nhóm các tin nhắn cùng IP")
        try:
            server.serve_forever()
        except KeyboardInterrupt:
            print("\n[!] Server stopped by user")