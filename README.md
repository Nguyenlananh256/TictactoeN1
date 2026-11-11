# Caro — Giao diện web

Giao diện tiếng Việt cho bài tập AI 2026–2027, viết bằng HTML/CSS/JavaScript thuần, không cần cài thư viện. Phạm vi hiện tại là frontend và luật bàn cờ; **chưa cài đặt thuật toán AI hoặc thực nghiệm**.

Thiết kế Caro Arena theo phong cách giấy ô ly: nét mực xanh, O đỏ, thẻ ghi chú vàng nhạt và đường viền vẽ tay; tham khảo ảnh giao diện do người dùng cung cấp. Giữ bàn 9×9 và các thao tác phục vụ đề bài.

## Chạy

Mở `index.html` bằng Edge, Chrome hoặc Firefox phiên bản mới. Có thể dùng Live Server trong VS Code khi nhóm tích hợp backend.

Font Google Fonts cần mạng; khi offline giao diện dùng font dự phòng trên máy. Toàn bộ thao tác bàn cờ hoạt động offline.

## Chức năng

- Bàn 9×9, X đi trước; thắng khi có ít nhất 5 quân ngang/dọc/chéo, không xét chặn hai đầu; báo hòa khi đầy bàn.
- Hai người để kiểm tra UI; thiết lập người đấu máy hoặc máy đấu máy khi có kết nối AI.
- Chọn Random, MiniMax + Alpha-Beta, MCTS + UCT riêng cho từng máy.
- Cấu hình chung: độ sâu 2–10, giới hạn ứng viên 1–81, c ∈ {0.5, 1.0, 1.4, 2.0}, iteration ∈ {100, 300, 500}. Cấu hình áp dụng ở ván mới.
- Đánh dấu nước gần nhất, tô chuỗi thắng, hoàn tác, chơi lại có xác nhận, tạm dừng máy đấu máy, tải lịch sử JSON.
- Hiển thị thời gian ván, thời gian AI thực tế, độ sâu và số nút do AI trả về. Dữ liệu chưa có hiển thị “—”. Đồng hồ ván đo thời gian thực kể cả khi tạm dừng.
- Bố cục thích ứng màn hình nhỏ; ô trống có thể điều hướng bằng phím mũi tên, đặt quân bằng Enter hoặc Space.

## Các file

| File | Phụ trách |
| --- | --- |
| `index.html` | Cấu trúc, biểu mẫu và nội dung |
| `styles.css` | Màu sắc, bố cục, responsive |
| `game.js` | Khởi tạo bàn và kiểm tra chuỗi thắng |
| `app.js` | Trạng thái giao diện, thao tác người dùng, kết nối AI |

## Hợp đồng kết nối AI

Nhóm thêm file `ai-adapter.js`, rồi thêm `<script src="ai-adapter.js" defer></script>` **sau** `app.js` trong HTML. Đăng ký từng thuật toán theo tên `random`, `minimax`, `mcts`:

```js
CaroUI.registerAI('minimax', async ({ board, player, algorithm, options, signal }) => {
  // Chuyển board cho thuật toán của nhóm hoặc gọi API backend.
  // Ví dụ backend dưới đây là hợp đồng đề xuất, chưa được triển khai trong repo.
  const response = await fetch('http://localhost:8000/move', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ board, player, algorithm, options }),
    signal
  });
  if (!response.ok) throw new Error(`HTTP ${response.status}`);
  return response.json();
});
```

Đầu vào:

- `board`: mảng 9×9 chứa `null`, `'X'`, `'O'`; bản sao, không phải trạng thái nội bộ.
- `player`: quân đến lượt, `'X'` hoặc `'O'`.
- `algorithm`: tên đã đăng ký.
- `options`: `{ depth, candidateLimit, c, iterations }`.
- `signal`: AbortSignal để hủy yêu cầu khi hoàn tác, chơi lại hoặc tạm dừng.

Kết quả trả về hoặc Promise resolve:

```js
{
  row: 4, // chỉ số hàng từ 0 đến 8
  col: 4, // chỉ số cột từ 0 đến 8; (4,4) là E5
  metrics: {
    depth: 2, // chỉ cung cấp khi thuật toán có số liệu
    nodes: 1250
  }
}
```

Giao diện tự đo `timeMs` từ lúc gọi adapter đến khi nhận kết quả (bao gồm độ trễ API). Thuật toán phải tuân thủ luật bàn cờ và trả về ô trống. Kết quả muộn của lượt đã hủy sẽ bị bỏ qua. Lỗi kết nối hoặc nước đi sai được hiển thị, không tự sinh nước AI thay thế. Khi lỗi, có thể bắt đầu lại, hoàn tác hoặc tạm dừng/tiếp tục ở chế độ máy đấu máy để thử lại.

Backend khác cổng cần cấu hình CORS cho origin của frontend. Nếu thuật toán viết bằng JavaScript và tính toán nặng, chạy trong Web Worker để không khóa giao diện; không đặt vòng tìm kiếm đồng bộ nặng trên main thread.

`CaroUI.getState()` trả bản sao bàn cờ, lượt, lịch sử và kết quả để phục vụ tích hợp/debug. Chưa có provider thì UI thông báo rõ và chờ ở lượt máy.

## Phạm vi tiếp theo của nhóm

Thành viên phụ trách AI cài Random, MiniMax/Alpha-Beta, MCTS/UCT và trả số liệu thật qua adapter. Phần chạy thực nghiệm nhiều ván, biểu đồ so sánh và dữ liệu báo cáo chưa nằm trong bản giao diện này. Không dùng nhật ký chơi thủ công thay cho bộ thực nghiệm theo đề.

Giao diện được xây dựng với sự hỗ trợ của Codex; nhóm cần ghi nhận công cụ và hiểu phần mã mình sử dụng theo yêu cầu đề bài.
