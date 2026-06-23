import fs from "node:fs/promises";
import { Presentation, PresentationFile } from "@oai/artifact-tool";

const OUT = "../final_presentation.pptx";
const PREVIEW = "./preview";

async function writeBlob(path, blob) {
  await fs.writeFile(path, Buffer.from(await blob.arrayBuffer()));
}

const deck = Presentation.create({
  slideSize: { width: 1280, height: 720 },
});

const colors = {
  ink: "#172033",
  muted: "#5A6475",
  blue: "#1E5B9A",
  cyan: "#2AA8C8",
  green: "#2E8B57",
  amber: "#C98218",
  red: "#B23B3B",
  bg: "#F6F8FB",
  panel: "#FFFFFF",
  line: "#D7DFEA",
};

function addText(slide, text, x, y, w, h, style = {}) {
  const shape = slide.shapes.add({
    geometry: "textbox",
    position: { left: x, top: y, width: w, height: h },
    fill: "none",
    line: { style: "solid", fill: "none", width: 0 },
  });
  shape.text = text;
  shape.text.style = {
    fontSize: style.size ?? 24,
    bold: style.bold ?? false,
    color: style.color ?? colors.ink,
    alignment: style.align ?? "left",
  };
  return shape;
}

function addTitle(slide, title, subtitle = "") {
  slide.background.fill = colors.bg;
  addText(slide, title, 70, 44, 900, 58, { size: 36, bold: true, color: colors.ink });
  if (subtitle) addText(slide, subtitle, 72, 96, 900, 32, { size: 18, color: colors.muted });
  slide.shapes.add({
    geometry: "rect",
    position: { left: 70, top: 132, width: 1140, height: 2 },
    fill: colors.blue,
    line: { style: "solid", fill: colors.blue, width: 0 },
  });
}

function addPanel(slide, x, y, w, h, title, body, accent = colors.blue) {
  slide.shapes.add({
    geometry: "roundRect",
    position: { left: x, top: y, width: w, height: h },
    fill: colors.panel,
    line: { style: "solid", fill: colors.line, width: 1 },
    borderRadius: "rounded-xl",
  });
  slide.shapes.add({
    geometry: "rect",
    position: { left: x, top: y, width: 8, height: h },
    fill: accent,
    line: { style: "solid", fill: accent, width: 0 },
  });
  addText(slide, title, x + 24, y + 18, w - 42, 30, { size: 22, bold: true, color: colors.ink });
  const lines = Array.isArray(body) ? body : String(body).split("\n");
  let lineY = y + 62;
  for (const raw of lines) {
    const line = String(raw).replace(/^•\s*/, "");
    slide.shapes.add({
      geometry: "ellipse",
      position: { left: x + 25, top: lineY + 8, width: 6, height: 6 },
      fill: accent,
      line: { style: "solid", fill: accent, width: 0 },
    });
    addText(slide, line, x + 40, lineY, w - 62, 40, { size: 16, color: colors.muted });
    lineY += 45;
  }
}

function bulletText(items) {
  return items;
}

function addChip(slide, text, x, y, w, color) {
  slide.shapes.add({
    geometry: "roundRect",
    position: { left: x, top: y, width: w, height: 42 },
    fill: color,
    line: { style: "solid", fill: color, width: 0 },
    borderRadius: "rounded-lg",
  });
  addText(slide, text, x + 14, y + 10, w - 28, 24, { size: 16, bold: true, color: "#FFFFFF", align: "center" });
}

function addArrow(slide, x1, y1, x2, y2, color = colors.blue) {
  slide.shapes.add({
    geometry: "line",
    position: { left: x1, top: y1, width: x2 - x1, height: y2 - y1 },
    line: { style: "solid", fill: color, width: 3, beginArrowType: "none", endArrowType: "triangle" },
    fill: "none",
  });
}

// 1 Title
{
  const slide = deck.slides.add();
  slide.background.fill = "#EDF4FA";
  addText(slide, "基于 STM32F103C8T6 的\n多传感器自平衡小车", 80, 92, 760, 150, { size: 50, bold: true, color: colors.ink });
  addText(slide, "《传感与测量技术》Z1期末项目简报", 84, 270, 650, 34, { size: 24, color: colors.blue, bold: true });
  addText(slide, "姿态测量 · 速度反馈 · 温湿度采集 · 压力/重量估算 · OLED现场显示", 84, 322, 760, 34, { size: 19, color: colors.muted });
  slide.shapes.add({
    geometry: "roundRect",
    position: { left: 850, top: 96, width: 320, height: 420 },
    fill: "#FFFFFF",
    line: { style: "solid", fill: colors.line, width: 1 },
    borderRadius: "rounded-2xl",
  });
  addText(slide, "项目核心", 890, 140, 240, 36, { size: 28, bold: true, color: colors.ink, align: "center" });
  addChip(slide, "MPU6050 姿态采样", 890, 210, 240, colors.blue);
  addChip(slide, "双编码器测速", 890, 270, 240, colors.green);
  addChip(slide, "DHT11 + FSR402", 890, 330, 240, colors.amber);
  addChip(slide, "SSD1306 OLED显示", 890, 390, 240, colors.cyan);
  addText(slide, "第XX组", 84, 596, 300, 32, { size: 20, color: colors.muted });
}

// 2 Motivation
{
  const slide = deck.slides.add();
  addTitle(slide, "制作动机与项目目标", "从单一传感器实验走向完整的测量与控制系统");
  addPanel(slide, 80, 170, 340, 380, "为什么选择自平衡小车", bulletText([
    "两轮结构天然不稳定，能体现实时测量的重要性",
    "需要姿态、速度、压力、温湿度等多源数据",
    "覆盖传感器选型、采样、滤波、校准和显示"
  ]), colors.blue);
  addPanel(slide, 470, 170, 340, 380, "项目实现目标", bulletText([
    "完成自平衡控制基础框架",
    "通过OLED显示温湿度和重量估算",
    "用Ozone实时观察关键变量并在线调参"
  ]), colors.green);
  addPanel(slide, 860, 170, 340, 380, "课程知识对应", bulletText([
    "惯性测量：角度与角速度",
    "力学测量：压力与重量估算",
    "数字/模拟接口：I2C、GPIO、ADC、TIM"
  ]), colors.amber);
}

// 3 Hardware
{
  const slide = deck.slides.add();
  addTitle(slide, "硬件组成与接口分配", "各模块分工明确，避免关键总线冲突");
  const rows = [
    ["STM32F103C8T6", "主控与控制算法运行平台"],
    ["MPU6050 / PB8 PB9", "I2C1 姿态采样，地址 0x68"],
    ["OLED / PB10 PB11", "I2C2 显示温湿度、重量和ADC"],
    ["DHT11 / PC14", "单总线温湿度采集"],
    ["FSR402+RFP602 / PA2", "ADC1_IN2 采集压力模拟电压"],
    ["TB6612 + 编码器", "电机驱动与左右轮速度反馈"],
  ];
  let y = 160;
  for (const [name, desc] of rows) {
    addChip(slide, name, 90, y, 320, colors.blue);
    addText(slide, desc, 450, y + 8, 680, 28, { size: 22, color: colors.ink });
    y += 70;
  }
  addText(slide, "设计要点：OLED 改用 I2C2，避免与 MPU6050 的 I2C1 PB8/PB9 引脚冲突。", 92, 610, 1000, 32, { size: 19, color: colors.muted });
}

// 4 Software
{
  const slide = deck.slides.add();
  addTitle(slide, "软件结构设计", "按驱动层、控制层、显示层分离，便于调试与维护");
  addPanel(slide, 80, 170, 330, 360, "底层驱动", bulletText([
    "mpu6050_hal：读取加速度与陀螺仪",
    "encoder_hal：TIM1/TIM2 编码器测速",
    "motor_tb6612：方向与PWM输出",
    "dht11 / fsr_adc / oled_ssd1306"
  ]), colors.blue);
  addPanel(slide, 475, 170, 330, 360, "控制核心", bulletText([
    "balance_control：10ms角度环",
    "50ms速度环与转向环",
    "PID带积分限幅、微分先行、死区补偿",
    "跌倒保护与停机逻辑"
  ]), colors.green);
  addPanel(slide, 870, 170, 330, 360, "调试与显示", bulletText([
    "g_balance_debug：启停、目标和校准",
    "g_balance_state：角度、速度、PWM",
    "g_sensor_state：温湿度、ADC、OLED状态",
    "display_ui：OLED分页刷新"
  ]), colors.cyan);
}

// 5 Control
{
  const slide = deck.slides.add();
  addTitle(slide, "平衡控制与测量链路", "互补滤波 + 串级PID控制");
  const top = 190;
  const boxes = [
    ["MPU6050", "ax/az 与 gy", 100, top, colors.blue],
    ["互补滤波", "angle_acc + angle_gyro", 350, top, colors.cyan],
    ["角度环PID", "输出平均PWM", 620, top, colors.green],
    ["TB6612电机", "驱动左右轮", 890, top, colors.amber],
  ];
  for (const [t, b, x, y, c] of boxes) {
    addPanel(slide, x, y, 210, 120, t, b, c);
  }
  addArrow(slide, 310, top + 60, 350, top + 60);
  addArrow(slide, 560, top + 60, 620, top + 60);
  addArrow(slide, 830, top + 60, 890, top + 60);
  addPanel(slide, 220, 410, 360, 130, "速度环", "编码器读取 left_speed/right_speed，平均速度反馈到目标倾角", colors.green);
  addPanel(slide, 700, 410, 360, 130, "转向环", "左右轮速度差 dif_speed 产生差分PWM，实现可控转向", colors.cyan);
  addText(slide, "TIM4每1ms产生节拍；角度环10ms执行，速度/转向环50ms执行。", 150, 600, 980, 30, { size: 22, color: colors.ink, align: "center" });
}

// 6 Sensor Display
{
  const slide = deck.slides.add();
  addTitle(slide, "温湿度与压力显示模块", "把传感器测量结果变成可观察、可校准的数据");
  addPanel(slide, 90, 170, 320, 330, "DHT11", bulletText([
    "PC14 单总线通信",
    "约2秒采样一次",
    "输出 temperature_c 与 humidity_percent"
  ]), colors.green);
  addPanel(slide, 480, 170, 320, 330, "FSR402 + RFP602", bulletText([
    "PA2 / ADC1_IN2 采样",
    "输出 fsr_adc_raw 与 fsr_voltage_mv",
    "通过零点与系数估算 weight_g"
  ]), colors.amber);
  addPanel(slide, 870, 170, 320, 330, "OLED显示", bulletText([
    "SSD1306 I2C地址 0x3C",
    "PB10/PB11 使用 I2C2",
    "显示 Temp、Humi、Weight、ADC"
  ]), colors.cyan);
  addText(slide, "重量估算公式：weight_g = max(0, (fsr_adc_raw - fsr_zero_adc) × fsr_g_per_count)", 120, 585, 1040, 32, { size: 21, color: colors.ink, align: "center" });
}

// 7 Debug
{
  const slide = deck.slides.add();
  addTitle(slide, "调试验证与当前结果", "Ozone变量让每条测量链路可观察");
  addPanel(slide, 90, 165, 350, 370, "传感器状态", bulletText([
    "temperature_c 约 30°C",
    "humidity_percent 约 31%~32%",
    "fsr_voltage_mv 接近 3.3V时说明压力链路有效",
    "dht_valid / fsr_valid 为 1 表示读取成功"
  ]), colors.green);
  addPanel(slide, 465, 165, 350, 370, "OLED排查", bulletText([
    "oled_ready = 1 表示初始化成功",
    "oled_addr_7bit = 0x3C 表示地址探测成功",
    "oled_fail_step = 0 表示无失败步骤",
    "sensor_fault_flags = 0 表示显示与传感器无故障"
  ]), colors.cyan);
  addPanel(slide, 840, 165, 350, 370, "平衡安全", bulletText([
    "run_enable 默认 0，避免上电误动作",
    "fall_angle_limit 超限自动停机",
    "PWM输出限幅 -100~100",
    "调参先架空，再落地测试"
  ]), colors.red);
}

// 8 Summary
{
  const slide = deck.slides.add();
  addTitle(slide, "总结与展望", "完成多传感器测量、闭环控制和现场显示的综合实践");
  addPanel(slide, 90, 170, 500, 340, "项目完成度", bulletText([
    "实现姿态、速度、温湿度、压力等多类数据采集",
    "完成自平衡车控制框架与PID调试入口",
    "完成OLED现场显示和Ozone变量观测",
    "形成可扩展的模块化嵌入式工程"
  ]), colors.blue);
  addPanel(slide, 690, 170, 500, 340, "后续优化", bulletText([
    "继续整定角度环、速度环和转向环参数",
    "对FSR402进行多点标定或更换HX711称重方案",
    "加入蓝牙/串口上传与电池电压检测",
    "扩展循迹功能，实现平衡状态下路线跟随"
  ]), colors.green);
  addText(slide, "谢谢观看", 460, 590, 360, 48, { size: 40, bold: true, color: colors.ink, align: "center" });
}

await fs.mkdir(PREVIEW, { recursive: true });
for (const [index, slide] of deck.slides.items.entries()) {
  const stem = `slide-${String(index + 1).padStart(2, "0")}`;
  await writeBlob(`${PREVIEW}/${stem}.png`, await deck.export({ slide, format: "png", scale: 1 }));
  await fs.writeFile(`${PREVIEW}/${stem}.layout.json`, await (await slide.export({ format: "layout" })).text());
}
await writeBlob(`${PREVIEW}/montage.webp`, await deck.export({ format: "webp", montage: true, scale: 1 }));
const pptx = await PresentationFile.exportPptx(deck);
await pptx.save(OUT);
console.log(OUT);
