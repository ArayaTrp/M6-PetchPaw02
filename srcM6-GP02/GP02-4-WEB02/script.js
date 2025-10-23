// อัปเดตวันที่และเวลาเรียลไทม์
function updateDateTime() {
  const now = new Date();
  const formatted = now.toLocaleString("th-TH", {
    year: "numeric",
    month: "2-digit",
    day: "2-digit",
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit"
  });
  const dtSpan = document.getElementById("currentDateTime");
  if (dtSpan) dtSpan.innerText = formatted;
}

// เรียกทุก 1 วินาที
setInterval(updateDateTime, 1000);
updateDateTime(); // เรียกทันทีตอนโหลด

const maxRows = 10; 
let lastSqueezeCount = 0;
let isTableFull = false;

// สร้างตาราง 10 แถวตั้งแต่แรก
const table = document.getElementById("pressureTableBody");
for (let i = 0; i < maxRows; i++) {
  const row = table.insertRow(); // สร้างแถวใหม่ใน <tbody>
  // สร้างเซลล์ในแถวตาม index (ค่าเริ่มต้นเป็น -)
  row.insertCell(0).innerText = i + 1;  // ลำดับ
  row.insertCell(1).innerText = "-";    // Current/Peak
  row.insertCell(2).innerText = "-";    // Time
  row.insertCell(3).innerText = "-";    // Status
}

// เพิ่ม flag กันไม่ให้เติมข้อมูลย้อนหลังตอนโหลดครั้งแรก
let initialized = false

setInterval(() => { 
  fetch("http://192.168.135.83/read") // fetch(url) – ดึงข้อมูล JSON จาก ESP32
    .then(res => res.json()) // แปลงข้อมูลที่ได้เป็น JSON
    .then(data => {
      document.getElementById("currentPressure").innerText = data.pressure_kPa.toFixed(2);
      document.getElementById("avrPressure").innerText = data.avrPressure_kPa.toFixed(2);
      document.getElementById("maxPressure").innerText = 
        (data.squeezeCount === 0) ? "-" : data.maxPressure_kPa.toFixed(2);

      document.getElementById("minPressure").innerText = 
        (data.squeezeCount === 0) ? "-" : data.minPressure_kPa.toFixed(2);

      document.getElementById("squeezeCount").innerText = data.squeezeCount;
      document.getElementById("motivationText").innerText = data.status;

      // อัปเดตแถบกราฟ
      const pressurePercent = Math.min((data.pressure_kPa / 10) * 100, 100);
      document.getElementById('pressureFill').style.width = pressurePercent + "%";

      if (!initialized) {
        lastSqueezeCount = data.squeezeCount; // sync ค่าล่าสุด
        initialized = true;
        return; // ข้ามรอบแรก ไม่อัปเดตตารางย้อนหลัง
      }
      // อัปเดตเฉพาะเมื่อ squeezeCount เพิ่มขึ้น
      if (data.squeezeCount > lastSqueezeCount) {
        const rowIndex = (data.squeezeCount - 1) % maxRows;  // % maxRows เพื่อวนรอบ 0–9 
        const row = table.rows[rowIndex];

        // ถ้าตารางรอบเก่ายังเต็มและผู้ใช้เริ่มรอบใหม่ ให้รีเซ็ต
        if (isTableFull) {
          for (let i = 0; i < maxRows; i++) {
            const r = table.rows[i];
            r.cells[1].innerText = "-";
            r.cells[2].innerText = "-";
            r.cells[3].innerText = "-";
          }
          isTableFull = false; // รีเซ็ต flag
        }

        // อัปเดต peak pressure, เวลา, สถานะ ในตาราง
        // row.cells[1].innerText = data.peakPressure_kPa.toFixed(2);
        // row.cells[2].innerText = new Date().toLocaleTimeString();
        // row.cells[3].innerText = data.statuslight;
        for (let count = lastSqueezeCount + 1; count <= data.squeezeCount; count++) {
          const rowIndex = (count - 1) % maxRows;
          const row = table.rows[rowIndex];
          row.cells[1].innerText = data.peakPressure_kPa.toFixed(2);
          row.cells[2].innerText = new Date().toLocaleTimeString();
          row.cells[3].innerText = data.statuslight;
        }

        lastSqueezeCount = data.squeezeCount; // อัปเดต lastSqueezeCount เพื่อรอบถัดไป

        // รีเซ็ต **รอบถัดไป** หลังจากแสดงแถวที่ 10 แล้ว
        // if (data.squeezeCount % maxRows === 0) {
        //   setTimeout(() => {   // เลื่อนการรีเซ็ตสักเล็กน้อยเพื่อให้ผู้ใช้เห็นข้อมูลครั้งที่ 10
        //     for (let i = 0; i < maxRows; i++) {
        //       const r = table.rows[i];
        //       r.cells[1].innerText = "-";
        //       r.cells[2].innerText = "-";
        //       r.cells[3].innerText = "-";
        //     }
        //     lastSqueezeCount = 0;
        //   }, 50);
        // }

        if (data.squeezeCount % maxRows === 0) {
          isTableFull = true; // mark ว่ารอบนี้เต็มแล้ว
          // แสดง popup
          const popup = document.createElement("div");
          popup.innerText = "ยินดีด้วย! ครบ 10 ครั้งแล้ว 🎉";
          popup.style.position = "fixed";
          popup.style.top = "20px";
          popup.style.left = "50%";
          popup.style.transform = "translateX(-50%)";
          popup.style.padding = "10px 20px";
          popup.style.backgroundColor = "#4caf50";
          popup.style.color = "#fff";
          popup.style.borderRadius = "8px";
          popup.style.boxShadow = "0 2px 8px rgba(0,0,0,0.2)";
          document.body.appendChild(popup);

          setTimeout(() => {
            popup.remove();  
 
            // for (let i = 0; i < maxRows; i++) {
            //   const r = table.rows[i];
            //   r.cells[1].innerText = "-";
            //   r.cells[2].innerText = "-";
            //   r.cells[3].innerText = "-";
            // } 
            // lastSqueezeCount = 0;
          }, 1000); // ให้หายหลัง 1 วินาที

          // const lastRow = table.rows[maxRows - 1];
          // lastRow.cells[1].innerText = "-";
          // lastRow.cells[2].innerText = "-";
          // lastRow.cells[3].innerText = "-";

          
        }


      }

    })
    .catch(err => console.error("Fetch error:", err));
}, 200); // เรียกฟังก์ชันทุก 0.2 วินาที
