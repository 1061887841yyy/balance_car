import fs from "node:fs/promises";
import path from "node:path";
import { Presentation, PresentationFile } from "@oai/artifact-tool";

const ROOT = path.resolve("../..");
const OUT = path.join(ROOT, "outputs", "《传感与测量技术》Z1期末简报_第XX组_暗黑科技风.pptx");
const PREVIEW = path.resolve("preview");
const ASSET_DIR = path.resolve("assets");
const BG_SRC = "C:\\Users\\future\\.codex\\generated_images\\019eed5a-51ee-7920-b30e-afc227dd67f8\\ig_022fa744c187e279016a39475ff39881988afcda3fe56d89b4.png";
const IMG_DIR = path.join(ROOT, "outputs", "图片材料");

async function writeBlob(filePath, blob) {
  await fs.writeFile(filePath, Buffer.from(await blob.arrayBuffer()));
}

await fs.mkdir(ASSET_DIR, { recursive: true });
const bgBytes = await fs.readFile(BG_SRC);
const img = {
  robot: await fs.readFile(path.join(IMG_DIR, "平衡小车废案.jpg")),
  stm32: await fs.readFile(path.join(IMG_DIR, "主控stm32板.jpg")),
  dht: await fs.readFile(path.join(IMG_DIR, "温湿度传感器.jpg")),
  fsr: await fs.readFile(path.join(IMG_DIR, "柔性传感器.jpg")),
  oled: await fs.readFile(path.join(IMG_DIR, "oled.jpg")),
  bt: await fs.readFile(path.join(IMG_DIR, "蓝牙模块.jpg")),
  app: await fs.readFile(path.join(IMG_DIR, "蓝牙控制手机app.jpg")),
  board: await fs.readFile(path.join(IMG_DIR, "自研扩展版.png")),
  scheme: await fs.readFile(path.join(IMG_DIR, "扩展版原理图1.png")),
};

const deck = Presentation.create({ slideSize: { width: 1280, height: 720 } });

const C = {
  white: "#F5F7FA",
  dim: "#AEB6C2",
  red: "#E1162D",
  line: "#E9EDF2",
  black: "#060708",
};

function text(slide, value, x, y, w, h, style = {}) {
  const shape = slide.shapes.add({
    geometry: "textbox",
    position: { left: x, top: y, width: w, height: h },
    fill: "none",
    line: { style: "solid", fill: "none", width: 0 },
  });
  shape.text = value;
  shape.text.style = {
    fontSize: style.size ?? 22,
    bold: style.bold ?? false,
    color: style.color ?? C.white,
    alignment: style.align ?? "left",
    fontFace: style.font ?? "Microsoft YaHei",
  };
  return shape;
}

function rect(slide, x, y, w, h, fill, lineFill = "none", lineWidth = 0) {
  slide.shapes.add({
    geometry: "rect",
    position: { left: x, top: y, width: w, height: h },
    fill,
    line: { style: "solid", fill: lineFill, width: lineWidth },
  });
}

function line(slide, x1, y1, x2, y2, color = C.line, width = 1) {
  slide.shapes.add({
    geometry: "line",
    position: { left: x1, top: y1, width: x2 - x1, height: y2 - y1 },
    fill: "none",
    line: { style: "solid", fill: color, width },
  });
}

function node(slide, x, y, r = 5, color = C.white) {
  slide.shapes.add({
    geometry: "ellipse",
    position: { left: x - r, top: y - r, width: r * 2, height: r * 2 },
    fill: color,
    line: { style: "solid", fill: color, width: 0 },
  });
}

function bg(slide, idx, title, subtitle = "") {
  slide.background.fill = C.black;
  slide.images.add({
    blob: bgBytes,
    contentType: "image/png",
    alt: "dark polygon network background",
    fit: "cover",
    position: { left: 0, top: 0, width: 1280, height: 720 },
    crop: idx % 2 === 0 ? { left: 0, top: 0, right: 0, bottom: 0 } : { left: 0.08, top: 0, right: 0, bottom: 0 },
  });
  rect(slide, 0, 0, 1280, 720, "rgba(0,0,0,0.28)");
  rect(slide, 0, 0, 1280, 720, "rgba(0,0,0,0.18)");
  text(slide, String(idx).padStart(2, "0"), 46, 34, 104, 74, { size: 54, bold: true, color: idx === 1 ? C.white : C.red, font: "Arial" });
  line(slide, 160, 72, 352, 72, C.red, 4);
  line(slide, 360, 72, 1180, 72, "rgba(245,247,250,0.42)", 1);
  if (title) text(slide, title, 64, 95, 760, 52, { size: 34, bold: true });
  if (subtitle) text(slide, subtitle, 66, 144, 850, 28, { size: 17, color: C.dim });
  node(slide, 1170, 70, 6, C.red);
  node(slide, 1200, 102, 3, C.white);
}

function tag(slide, value, x, y, w = 132) {
  rect(slide, x, y, w, 30, C.red, C.red, 0);
  text(slide, value, x + 10, y + 6, w - 20, 18, { size: 13, bold: true, color: C.white, align: "center" });
}

function panel(slide, x, y, w, h, title, items, accent = C.red) {
  rect(slide, x, y, w, h, "rgba(9,12,16,0.70)", "rgba(245,247,250,0.45)", 1);
  line(slide, x, y, x + 64, y, accent, 4);
  text(slide, title, x + 22, y + 18, w - 44, 28, { size: 22, bold: true });
  let yy = y + 62;
  for (const item of items) {
    node(slide, x + 28, yy + 10, 4, accent);
    text(slide, item, x + 44, yy, w - 62, 34, { size: 16, color: C.dim });
    yy += 43;
  }
}

function photo(slide, bytes, alt, x, y, w, h, caption, accent = C.red) {
  rect(slide, x - 3, y - 3, w + 6, h + 34, "rgba(8,10,14,0.72)", "rgba(245,247,250,0.45)", 1);
  line(slide, x - 3, y - 3, x + 62, y - 3, accent, 4);
  slide.images.add({
    blob: bytes,
    contentType: alt.endsWith(".png") ? "image/png" : "image/jpeg",
    alt,
    fit: "cover",
    position: { left: x, top: y, width: w, height: h },
  });
  text(slide, caption, x, y + h + 7, w, 20, { size: 13, color: C.dim, align: "center" });
}

function flowBox(slide, label, x, y, w, color = C.red) {
  rect(slide, x, y, w, 58, "rgba(12,16,22,0.76)", color, 1.5);
  text(slide, label, x + 10, y + 12, w - 20, 36, { size: 14, bold: true, align: "center" });
}

function connector(slide, x1, y1, x2, y2) {
  line(slide, x1, y1, x2, y2, "rgba(245,247,250,0.72)", 2);
  node(slide, x2, y2, 4, C.red);
}

// 1 Cover
{
  const slide = deck.slides.add();
  bg(slide, 1, "", "");
  text(slide, "FOOD SUPPLY\nROBOT", 80, 135, 610, 150, { size: 58, bold: true, font: "Arial" });
  line(slide, 84, 304, 540, 304, C.white, 2);
  line(slide, 84, 314, 430, 314, C.white, 1);
  text(slide, "食品供应链双轮运输机器人", 84, 345, 720, 44, { size: 30, bold: true });
  text(slide, "《传感与测量技术》Z1期末项目简报", 86, 400, 560, 30, { size: 20, color: C.dim });
  tag(slide, "第05组", 86, 462, 118);
  text(slide, "双轮底盘 · 食材称重 · 温湿度环境 · 蓝牙APP控制", 86, 514, 720, 30, { size: 18, color: C.dim });
}

// 2 Scenario
{
  const slide = deck.slides.add();
  bg(slide, 2, "场景定位与项目目标", "面向食品供应链末端搬运，关注足称、环境与远程调度");
  panel(slide, 80, 210, 330, 308, "为什么是双轮底盘", [
    "减少宽度和转弯空间",
    "适合厨房、仓储、通道等狭窄场景",
    "左右轮差速实现灵活转向",
  ]);
  panel(slide, 474, 210, 330, 308, "为什么要测量", [
    "重量用于检查食材是否足称",
    "温湿度用于判断环境是否适合放置",
    "OLED现场反馈关键数据",
  ], C.white);
  panel(slide, 868, 210, 330, 308, "当前可实现", [
    "蓝牙APP远程控制运动",
    "温湿度与重量估算显示",
    "保留RFID/导航/视觉识别扩展方向",
  ]);
}

// 3 Hardware
{
  const slide = deck.slides.add();
  bg(slide, 3, "硬件平台", "双轮移动底盘 + STM32主控 + 食品质控测量模块");
  photo(slide, img.robot, "robot.jpg", 82, 200, 250, 190, "双轮运输底盘");
  photo(slide, img.stm32, "stm32.jpg", 372, 200, 250, 190, "STM32主控连接");
  photo(slide, img.board, "board.png", 662, 200, 250, 190, "自研扩展板");
  photo(slide, img.scheme, "scheme.png", 952, 200, 230, 190, "扩展板原理图");
  panel(slide, 120, 455, 1040, 120, "硬件分工", [
    "底盘负责食品搬运，主控负责传感器采集与电机控制",
    "扩展板与支撑结构用于固定模块、整理接口、提升可靠性",
  ]);
}

// 4 Sensors
{
  const slide = deck.slides.add();
  bg(slide, 4, "质量与保鲜环境测量", "重量看是否足称，温湿度看是否适合长时间放置");
  photo(slide, img.dht, "dht11.jpg", 94, 210, 300, 210, "DHT11温湿度模块");
  photo(slide, img.fsr, "fsr.jpg", 490, 210, 300, 210, "柔性压力/称重传感器");
  photo(slide, img.oled, "oled.jpg", 886, 210, 300, 210, "OLED现场显示");
  panel(slide, 120, 485, 1040, 116, "测量意义", [
    "重量估算用于食材验收与足称检查；温湿度用于判断当前环境是否适合保鲜暂存",
    "当前版本显示实时数据，食材分类阈值与报警策略放入未来扩展",
  ], C.white);
}

// 5 Bluetooth
{
  const slide = deck.slides.add();
  bg(slide, 5, "蓝牙APP远程控制", "当前操控系统采用手机APP下发运动指令");
  photo(slide, img.bt, "bluetooth.jpg", 140, 205, 330, 260, "蓝牙通信模块");
  photo(slide, img.app, "bluetooth-app.jpg", 540, 205, 330, 260, "手机APP控制界面");
  panel(slide, 920, 205, 260, 260, "控制逻辑", [
    "APP发送前进/后退/转向",
    "蓝牙模块传给STM32",
    "主控驱动左右轮差速",
    "必要时人工接管",
  ]);
  text(slide, "蓝牙控制适合原型阶段展示，也符合食品供应链末端人工辅助调度场景。", 150, 560, 980, 30, { size: 20, color: C.dim, align: "center" });
}

// 6 Data Flow
{
  const slide = deck.slides.add();
  bg(slide, 6, "系统数据流", "运输动作、质量检测、环境显示同步工作");
  flowBox(slide, "手机APP\n控制指令", 95, 230, 160);
  flowBox(slide, "蓝牙模块\n串口通信", 300, 230, 160, C.white);
  flowBox(slide, "STM32主控\n调度处理", 505, 230, 180);
  flowBox(slide, "TB6612\n双轮底盘", 730, 230, 170, C.white);
  flowBox(slide, "OLED\n现场显示", 945, 230, 160);
  connector(slide, 255, 259, 300, 259);
  connector(slide, 460, 259, 505, 259);
  connector(slide, 685, 259, 730, 259);
  connector(slide, 900, 259, 945, 259);
  panel(slide, 150, 410, 980, 148, "传感器输入", [
    "DHT11输入当前温湿度；压力传感器输入食材重量估算值",
    "OLED显示环境和质量数据，调试阶段可继续通过Ozone观察原始变量",
  ]);
}

// 7 Current Result
{
  const slide = deck.slides.add();
  bg(slide, 7, "当前实现效果", "完成食品运输机器人原型的运动、测量、显示与遥控链路");
  panel(slide, 86, 208, 330, 300, "已完成", [
    "双轮底盘可由蓝牙APP控制",
    "OLED显示温湿度、重量和ADC",
    "压力传感器支持重量趋势检测",
    "温湿度传感器支持环境观察",
  ]);
  panel(slide, 476, 208, 330, 300, "适用展示", [
    "食材放置后读取重量估算",
    "观察当前环境温湿度",
    "远程控制机器人短距离搬运",
    "展示供应链末端辅助运输场景",
  ], C.white);
  panel(slide, 866, 208, 330, 300, "当前限制", [
    "称重精度仍需校准",
    "尚未建立食材保鲜阈值表",
    "导航与识别尚未实现",
    "仍以人工APP遥控为主",
  ]);
}

// 8 Future
{
  const slide = deck.slides.add();
  bg(slide, 8, "未来展望与废案方向", "从远程控制原型走向智能食品供应链运输节点");
  photo(slide, img.robot, "future-robot.jpg", 86, 210, 280, 360, "双轮导航运输构想");
  panel(slide, 430, 205, 350, 310, "食品保鲜判断", [
    "建立不同食材温湿度阈值",
    "环境不适合时报警",
    "必要时导航到合适存储点",
  ]);
  panel(slide, 840, 205, 350, 310, "智能识别与导航", [
    "RFID识别货架/目的地",
    "摄像头 + YOLO识别食材种类",
    "自动匹配保鲜规则和称重标准",
  ], C.white);
  text(slide, "这些功能目前作为未来扩展/废案，不写成已完成内容。", 430, 560, 760, 28, { size: 19, color: C.dim, align: "center" });
  text(slide, "THANKS", 520, 612, 260, 46, { size: 44, bold: true, align: "center", font: "Arial" });
  line(slide, 500, 666, 780, 666, C.red, 4);
}

await fs.mkdir(PREVIEW, { recursive: true });
for (const [index, slide] of deck.slides.items.entries()) {
  const stem = `slide-${String(index + 1).padStart(2, "0")}`;
  await writeBlob(path.join(PREVIEW, `${stem}.png`), await deck.export({ slide, format: "png", scale: 1 }));
  await fs.writeFile(path.join(PREVIEW, `${stem}.layout.json`), await (await slide.export({ format: "layout" })).text());
}
await writeBlob(path.join(PREVIEW, "montage.webp"), await deck.export({ format: "webp", montage: true, scale: 1 }));
await fs.writeFile(path.join(PREVIEW, "inspect.ndjson"), (await deck.inspect({ kind: "slide,textbox,shape,image", maxChars: 24000 })).ndjson);
const pptx = await PresentationFile.exportPptx(deck);
await pptx.save(OUT);
console.log(OUT);
