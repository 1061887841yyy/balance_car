# STM32F103C8T6 Balance Car

这是一个基于 `STM32F103C8T6` 的两轮自平衡小车控制程序，工程由 STM32CubeMX 生成 Makefile 项目后继续开发，适合使用 GCC 工具链编译，并通过 Ozone + J-Link 进行变量观察和在线调参。

项目主要功能包括：

- MPU6050 姿态采样
- 互补滤波计算俯仰角
- 角度环 PID
- 速度环 PID
- 转向环 PID
- TB6612 电机驱动
- 双编码器测速
- PB6 按键启停和 PC13 运行指示灯
- SSD1306 OLED 状态显示
- DHT11 温湿度采集
- FSR402/RFP602 压力与重量趋势估算
- ESP8266 Wi-Fi 网页遥控
- Ozone + J-Link 调试变量入口

调试阶段可以通过 Ozone 直接观察和修改全局变量，完成姿态校准、电机方向确认、编码器方向确认和 PID 参数调节。

## 1. 硬件连接

主控为 `STM32F103C8T6`。

### MPU6050

| 功能 | STM32 引脚 | MPU6050 |
| --- | --- | --- |
| I2C1_SCL | PB8 | SCL |
| I2C1_SDA | PB9 | SDA |
| 3.3V | 3.3V | VCC |
| GND | GND | GND |

代码默认 MPU6050 地址为 `0x68`，即 AD0 接 GND 或悬空。

MPU6050 与 OLED 共用 I2C1 的 PB8/PB9，总线上两个模块地址不同，可以同时使用。

### TB6612 电机驱动

| 功能 | STM32 引脚 | TB6612 |
| --- | --- | --- |
| STBY | PA3 | STBY |
| AIN1 | PA4 | AIN1 |
| AIN2 | PA5 | AIN2 |
| PWMA | PA6 / TIM3_CH1 | PWMA |
| BIN1 | PB1 | BIN1 |
| BIN2 | PB0 | BIN2 |
| PWMB | PA7 / TIM3_CH2 | PWMB |
| GND | GND | GND |

注意：

- TB6612 的 `VM` 接电机电源，不要接 STM32 3.3V。
- TB6612 的逻辑电源 `VCC` 接 3.3V。
- STM32、TB6612、电机电源必须共地。
- 程序停机时会拉低 `STBY`，并把 PWM 清零。

### 编码器

| 功能 | STM32 引脚 | 定时器 |
| --- | --- | --- |
| 左编码器 A | PA8 | TIM1_CH1 |
| 左编码器 B | PA9 | TIM1_CH2 |
| 右编码器 A | PA0 | TIM2_CH1 |
| 右编码器 B | PA1 | TIM2_CH2 |

如果编码器方向反了，可以交换 A/B 相，也可以在代码中给对应 delta 加负号。

### 启停按键和运行指示灯

| 功能 | STM32 引脚 | 说明 |
| --- | --- | --- |
| 启停按键 | PB6 | 按键另一端接 GND，代码内部使用上拉输入 |
| 运行指示灯 | PC13 | 最小系统板常见板载 LED，运行允许时点亮 |

PB6 按键带 20ms 软件消抖。每次确认按下后，`g_balance_debug.run_enable` 会在 0 和 1 之间切换；PC13 指示灯跟随 `run_enable` 亮灭。

### OLED 显示屏

默认使用 0.96 寸 I2C SSD1306 128x64 OLED，地址 `0x3C`。

| 功能 | STM32 引脚 | OLED |
| --- | --- | --- |
| I2C1_SCL | PB8 | SCL |
| I2C1_SDA | PB9 | SDA |
| 3.3V | 3.3V | VCC |
| GND | GND | GND |

OLED 与 MPU6050 共用 I2C1。PB10/PB11 已作为 USART3 连接 ESP8266，不再接 OLED。

### ESP8266 Wi-Fi 遥控模块

ESP8266 使用 USART3 与 STM32 通信，网页端只负责发送命令，平衡控制仍由 STM32 完成。

| STM32F103C8T6 | ESP8266 | 说明 |
| --- | --- | --- |
| PB10 / USART3_TX | RX | STM32 发给 ESP8266 |
| PB11 / USART3_RX | TX | ESP8266 发给 STM32 |
| GND | GND | 必须与 STM32、电机电源共地 |
| 3.3V 稳压电源 | VCC / 3V3 | 建议电流 >= 500mA |

注意：

- STM32 和 ESP8266 都是 3.3V 串口电平，ESP8266 RX 不能接 5V。
- 不建议用 STM32 最小系统板的弱 3.3V 直接给 ESP8266 供电，供电不足会导致网页卡顿、热点消失或反复复位。
- ESP-01/ESP-01S 正常运行时：`EN/CH_PD` 接 3.3V，`GPIO0` 接 3.3V 或上拉，`GPIO2` 接 3.3V 或上拉，`RST` 上拉到 3.3V。
- ESP-01/ESP-01S 烧录固件时：`GPIO0` 拉低到 GND，再复位或重新上电。
- NodeMCU / Wemos D1 mini 可以直接 USB 烧录，接线时使用板子的 `TX/RX/GND/3V3`。

### DHT11 温湿度模块

| 功能 | STM32 引脚 | DHT11 |
| --- | --- | --- |
| DATA | PC14 | DATA |
| 3.3V | 3.3V | VCC |
| GND | GND | GND |

DHT11 数据脚需要上拉电阻；多数模块板已自带上拉。

### FSR402 + RFP602 压力/重量估算

| 功能 | STM32 引脚 | RFP602 |
| --- | --- | --- |
| 模拟输出 | PA2 / ADC1_IN2 | AO |
| 3.3V | 3.3V | VCC |
| GND | GND | GND |

注意：

- RFP602 输出必须限制在 `0~3.3V`，不能超过 STM32 ADC 输入范围。
- FSR402 不是精密称重传感器，OLED 上显示的重量是通过校准系数估算出来的克重。

## 2. 工程结构

核心代码在：

```text
Core/Inc/balance_car/
Core/Src/balance_car/
```

主要文件：

| 文件 | 作用 |
| --- | --- |
| `balance_control.c/.h` | 平衡车主控制逻辑、调试变量、控制节拍、PID 串级关系 |
| `pid.c/.h` | 位置式 PID，带积分限幅、微分先行、输出死区补偿 |
| `mpu6050_hal.c/.h` | HAL I2C 版 MPU6050 初始化和原始数据读取 |
| `motor_tb6612.c/.h` | TB6612 电机方向和 PWM 输出 |
| `encoder_hal.c/.h` | TIM1/TIM2 编码器模式读取左右轮速度 |
| `i2c_bus.c/.h` | I2C1/I2C2 总线初始化，MPU6050 和 OLED 共用 I2C1 |
| `oled_ssd1306.c/.h` | SSD1306 128x64 I2C OLED 分页刷新 |
| `remote_control.c/.h` | USART3 遥控命令接收、解析、限幅和超时保护 |
| `dht11.c/.h` | PC14 单总线读取 DHT11 温湿度 |
| `fsr_adc.c/.h` | PA2 / ADC1_IN2 读取 RFP602 模拟电平 |
| `app_sensors.c/.h` | 温湿度、ADC、电压、重量估算和 Ozone 状态变量 |
| `display_ui.c/.h` | OLED 四行数据显示和后台刷新 |

ESP8266 Arduino 固件在：

```text
esp8266_remote/esp8266_remote.ino
```

CubeMX 生成的主入口在：

```text
Core/Src/main.c
```

其中调用：

```c
(void)BalanceCar_Init();

while (1)
{
    BalanceCar_Background();
}
```

## 3. 编译方法

需要安装 `arm-none-eabi-gcc`，并确保 `make` 可以在终端中使用。

编译：

```bash
make clean
make
```

生成文件：

```text
build/balance_car.elf
build/balance_car.hex
build/balance_car.bin
```

Ozone 调试时建议加载 `build/balance_car.elf`，因为 ELF 中带有调试符号，可以直接看到全局变量名。

## 4. 控制逻辑解析

控制系统由姿态解算、角度环、速度环和转向环组成：

```text
MPU6050
  -> 加速度计角度 angle_acc
  -> 陀螺仪积分角度 angle_gyro
  -> 互补滤波 angle
  -> 角度环 PID
  -> 平均 PWM

编码器
  -> left_speed / right_speed
  -> ave_speed / dif_speed
  -> 速度环 PID 输出角度目标
  -> 转向环 PID 输出差分 PWM
```

### 4.1 10ms 角度环

TIM4 每 1ms 产生节拍，后台每 10ms 执行一次角度环：

```c
Mpu6050_ReadRaw(&raw);
angle_acc = -atan2f(raw.ax, raw.az) / PI * 180;
angle_gyro = s_angle + gy_calibrated / 32768 * 2000 * 0.01;
angle = 0.01 * angle_acc + 0.99 * angle_gyro;
PID_Update(&g_angle_pid);
```

当前代码中的最终控制输出为：

```c
float ave_pwm = g_angle_pid.Out;
```

如果你的车方向相反，可以根据后面的“方向反了怎么办”调整这里的正负号。

### 4.2 50ms 速度环和转向环

后台每 50ms 读取编码器：

```c
left_speed = left_delta / ENCODER_MAGNET_LINES / SPEED_LOOP_PERIOD_S / MOTOR_REDUCTION_RATIO;
right_speed = right_delta / ENCODER_MAGNET_LINES / SPEED_LOOP_PERIOD_S / MOTOR_REDUCTION_RATIO;
ave_speed = (left_speed + right_speed) / 2.0;
dif_speed = left_speed - right_speed;
```

当前代码参数为：

```c
ENCODER_MAGNET_LINES = 13.0f;
SPEED_LOOP_PERIOD_S = 0.05f;
MOTOR_REDUCTION_RATIO = 30.0f;
```

含义是：50ms 统计一次编码器增量，先除以电机编码器每圈线数，再除以 50ms 得到电机轴转速，最后除以减速比换算成输出轴速度。

速度环输出给角度环目标：

```c
g_speed_pid.Actual = ave_speed;
PID_Update(&g_speed_pid);
g_angle_pid.Target = g_speed_pid.Out;
```

转向环输出给差分 PWM：

```c
g_turn_pid.Actual = dif_speed;
PID_Update(&g_turn_pid);
s_dif_pwm = g_turn_pid.Out;
```

最终左右轮 PWM：

```c
left_pwm = ave_pwm + dif_pwm / 2;
right_pwm = ave_pwm - dif_pwm / 2;
```

## 5. Ozone 调试变量

主要看这几个全局变量：

```c
g_balance_debug
g_balance_state
g_sensor_state
g_remote_state
g_remote_debug
g_angle_pid
g_speed_pid
g_turn_pid
```

### 5.1 g_balance_debug

这是 Ozone 中最重要的调试入口，用来启动、停机、设定目标和校准传感器。

| 变量 | 含义 | 调试建议 |
| --- | --- | --- |
| `run_enable` | 0=停机，1=允许输出 PWM | 第一次必须保持 0，确认安全后再改 1 |
| `reset_pid_request` | 写 1 后清空 PID 历史量 | 每次重新调参前可以写 1 |
| `clear_fault_request` | 写 1 后清除故障位 | 硬件故障未解决会再次置位 |
| `speed_target` | 目标前后速度 | 当前代码初值为 1.0；重新调试速度环时建议先改成 0 |
| `turn_target` | 目标转向速度差 | 初期保持 0 |
| `gyro_y_offset` | 陀螺仪 Y 轴零漂 | 静止时观察 `g_balance_state.gy`，把静止平均值填进去 |
| `angle_offset` | 机械竖直角度偏移 | 扶正车后观察 `angle_acc`，按公式修正到接近 0 |
| `fall_angle_limit` | 倒车保护角度 | 当前代码初值为 12 度，超过后自动停机 |

`gyro_y_offset` 和 `angle_offset` 都不是通用固定值。不同 MPU6050 模块、不同安装角度、不同车架机械中心都会不一样，必须按自己的车实测后写回 `Core/Src/balance_car/balance_control.c`。

### 5.2 g_balance_state

这是 Ozone 中主要观察的状态变量。

| 变量 | 含义 |
| --- | --- |
| `control_ms` | 1ms 递增，确认控制节拍在运行 |
| `fault_flags` | 故障标志位 |
| `run_active` | 1=当前正在输出控制，0=停机 |
| `imu_ready` | 1=MPU6050 可用 |
| `mpu_id` | MPU6050 ID，正常应为 `0x68` |
| `button_raw` | PB6 按键原始状态，1=当前按下，0=当前松开 |
| `button_stable` | PB6 按键消抖后的稳定状态 |
| `button_toggle` | 每次按键事件产生后翻转一次，用来确认按键是否被程序识别 |
| `ax/ay/az` | 加速度计 X/Y/Z 原始值 |
| `gx/gy/gz` | 陀螺仪 X/Y/Z 原始值 |
| `angle_acc` | 只由加速度计算出的俯仰角 |
| `angle_gyro` | 由陀螺仪积分得到的角度 |
| `angle` | 互补滤波后的最终俯仰角，调平衡主要看它 |
| `left_speed/right_speed` | 左右轮速度 |
| `ave_speed` | 左右轮平均速度，速度环实际值 |
| `dif_speed` | 左右轮速度差，转向环实际值 |
| `left_pwm/right_pwm` | 左右电机最终 PWM，范围 -100 到 100 |
| `ave_pwm` | 平均 PWM，主要来自角度环 |
| `dif_pwm` | 差分 PWM，主要来自转向环 |

### 5.3 g_sensor_state

这是 OLED、DHT11 和 FSR402/RFP602 的观察与校准入口。

| 变量 | 含义 |
| --- | --- |
| `temperature_c` | DHT11 温度，单位摄氏度 |
| `humidity_percent` | DHT11 湿度，单位百分比 |
| `fsr_adc_raw` | PA2 / ADC1_IN2 原始 ADC 值，范围约 0~4095 |
| `fsr_voltage_mv` | RFP602 模拟输出电压，单位 mV |
| `weight_g` | 按校准系数估算出的重量，单位 g |
| `fsr_zero_adc` | 空载零点 ADC 值，可在 Ozone 中手动修正 |
| `fsr_g_per_count` | 每个 ADC count 对应多少克，可在 Ozone 中手动校准 |
| `sensor_fault_flags` | 传感器和 OLED 故障位 |
| `dht_valid` | 1=DHT11 最近一次读取成功 |
| `fsr_valid` | 1=FSR ADC 最近一次读取成功 |
| `oled_ready` | 1=OLED 初始化成功且正在刷新 |
| `oled_addr_7bit` | OLED 实际使用的 7 位 I2C 地址，正常通常是 `0x3C` 或 `0x3D` |
| `oled_probe_mask` | OLED 地址探测结果，bit0=`0x3C` 有应答，bit1=`0x3D` 有应答 |
| `oled_fail_step` | OLED 初始化失败步骤，1=I2C1 初始化失败，2=地址无应答，3=初始化命令失败，4=首次刷新失败 |

重量换算公式：

```text
weight_g = max(0, (fsr_adc_raw - fsr_zero_adc) * fsr_g_per_count)
```

容易混淆的一点：

```c
g_balance_state.ax
g_balance_state.ay
g_balance_state.az
```

它们不是俯仰角，而是加速度计三个轴的原始值。真正的俯仰角是：

```c
g_balance_state.angle
```

### 5.4 g_remote_state

这是 ESP8266 网页遥控的接收状态。调串口、调网页遥控时主要看它。

| 变量 | 含义 |
| --- | --- |
| `link_active` | 1=最近 `timeout_ms` 内收到过有效遥控命令 |
| `command_ready` | 1=收到完整命令行并等待后台解析，通常只会短暂出现 |
| `parser_error` | 1=最近一次命令格式错误 |
| `rx_count` | USART3 收到的字节数 |
| `valid_cmd_count` | 有效命令计数 |
| `invalid_cmd_count` | 无效命令计数 |
| `timeout_count` | 遥控超时次数 |
| `last_rx_ms` | 最近一次有效命令的系统毫秒时间 |
| `fault_flags` | 遥控模块故障位 |
| `speed_cmd` | 最近一次遥控速度目标，已经过限幅 |
| `turn_cmd` | 最近一次遥控转向目标，已经过限幅 |
| `last_command` | 最近一次完整命令字符串 |

常见现象：

- 点网页按钮或用串口助手发命令时，`rx_count` 应该增加。
- 命令格式正确时，`valid_cmd_count` 应该增加。
- 命令格式错误时，`invalid_cmd_count` 和 `parser_error` 会变化。
- 超过 500ms 没收到有效命令时，`link_active` 会变 0，`speed_target/turn_target` 会自动清零。

### 5.5 g_remote_debug

这是 ESP8266 遥控功能的调试配置，可以在 Ozone 中临时修改。

| 变量 | 含义 | 默认值 |
| --- | --- | --- |
| `enable` | 1=允许遥控命令改变目标，0=忽略遥控命令 | 1 |
| `allow_run_command` | 1=允许网页 `RUN 1/RUN 0` 控制启停 | 1 |
| `timeout_stop_enable` | 1=遥控超时后自动清零速度和转向目标 | 1 |
| `speed_limit` | 遥控速度目标绝对值限幅 | 1.0 |
| `turn_limit` | 遥控转向目标绝对值限幅 | 0.8 |
| `timeout_ms` | 遥控超时时间，单位 ms | 500 |

调试建议：

- 第一次联调时可以先保持 `run_enable = 0`，只看 `speed_target/turn_target` 是否会跟随网页按钮变化。
- 如果你只想用 PB6 按键启动，不想让网页启动小车，可以把 `allow_run_command = 0`。
- 如果网页松手后车还继续走，优先看 `timeout_stop_enable` 是否为 1，以及 `timeout_count` 是否会增加。

### 5.6 三个 PID

| PID | 作用 | 调试顺序 |
| --- | --- | --- |
| `g_angle_pid` | 角度环，让车直立 | 第一个调 |
| `g_speed_pid` | 速度环，让车不乱跑 | 第二个调 |
| `g_turn_pid` | 转向环，控制左右差速 | 最后调 |

PID 字段含义：

| 字段 | 含义 |
| --- | --- |
| `Target` | 目标值 |
| `Actual` | 实际值 |
| `Out` | PID 输出 |
| `Kp` | 比例系数 |
| `Ki` | 积分系数 |
| `Kd` | 微分系数 |
| `Error0` | 当前误差 |
| `ErrorInt` | 误差积分 |
| `OutMax/OutMin` | 输出限幅 |
| `OutOffset` | 输出死区补偿 |

## 6. 第一次调试顺序

不要一开始就让车落地运行。推荐严格按下面顺序来。

### 6.1 只接 STM32 和 MPU6050

先不要接电机电源 `VM`。

在 Ozone 中运行程序，看：

```c
g_balance_state.mpu_id
g_balance_state.imu_ready
g_balance_state.fault_flags
g_balance_state.angle
```

如果使用 Ozone Data Graph，建议先画：

```c
g_balance_state.angle
g_balance_state.angle_acc
g_balance_state.angle_gyro
g_balance_state.gy
```

调好标准：

- `mpu_id == 0x68`
- `imu_ready == 1`
- `fault_flags` 没有 `BALANCE_FAULT_MPU_INIT` 或 `BALANCE_FAULT_MPU_READ`
- 手动前后倾斜车架，`angle` 连续变化，不乱跳
- 车竖直时，`angle` 接近 0
- Data Graph 中 `angle` 曲线应该跟随车体前后倾斜平滑变化，不应突然跳变。
- 静止不动时，`angle_gyro` 不应快速单方向漂移；如果漂移明显，优先调 `gyro_y_offset`。

#### 6.1.1 校准陀螺仪零漂 `gyro_y_offset`

`gyro_y_offset` 调的是 MPU6050 陀螺仪 Y 轴的静态零漂。代码里俯仰角积分使用的是：

```c
gy_calibrated = raw.gy - g_balance_debug.gyro_y_offset;
```

所以校准目标是：车完全静止时，让 `raw.gy - gyro_y_offset` 尽量接近 0。

调试步骤：

1. 保持停机，不让电机输出。

```c
g_balance_debug.run_enable = 0
```

2. 把小车或 MPU6050 固定住，保持完全静止。此时不一定非要直立，关键是不要动、不要振动。

3. 在 Ozone 里观察：

```c
g_balance_state.gy
```

4. 看 `gy` 静止时大概稳定在多少。如果看到它在 `-13, -12, -12, -11, -12` 附近跳动，平均值大概就是 `-12`。

5. 把这个平均值填到：

```c
g_balance_debug.gyro_y_offset = -12.0f
```

也就是说：静止时 `g_balance_state.gy` 平均是多少，`g_balance_debug.gyro_y_offset` 就填多少。

例子：

```text
静止时 gy 大约是 -11.6，gyro_y_offset 就设为 -11.6
静止时 gy 大约是 25，gyro_y_offset 就设为 25
静止时 gy 大约是 -38，gyro_y_offset 就设为 -38
```

验证标准：

- `g_balance_state.gy` 是原始值，调 `gyro_y_offset` 后它不会变，这是正常的。
- `gyro_y_offset` 影响的是 `angle_gyro` 和 `angle` 的长期漂移，不会让角度立刻跳变。
- 静止不动时，看 Ozone Data Graph 里的 `g_balance_state.angle_gyro` 和 `g_balance_state.angle`，它们不应该几秒钟内快速单方向漂移很多度。
- 允许非常慢的小漂移；如果几秒钟就明显跑偏，继续微调 `gyro_y_offset`。

调好后，把最终值写回：

```c
.gyro_y_offset = 你的实测平均值,
```

位置在 `Core/Src/balance_car/balance_control.c` 的 `g_balance_debug` 初始化处。改完后重新编译并烧录，否则单独上电还是旧参数。

#### 6.1.2 校准机械零点 `angle_offset`

`angle_offset` 调的是小车的机械直立零点。目标是：小车真正直立时，程序算出来的俯仰角应该接近 0 度。

代码里加速度角度的计算是：

```c
float angle_acc = -atan2f((float)raw.ax, (float)raw.az) / PI_F * 180.0f;
angle_acc += g_balance_debug.angle_offset;
```

所以 `angle_offset` 的作用是把 `angle_acc` 整体平移。它最直接影响的是：

```c
g_balance_state.angle_acc
```

然后通过互补滤波慢慢影响：

```c
g_balance_state.angle
```

当前滤波系数是 `0.01`，所以改 `angle_offset` 后，`angle_acc` 会立刻变，最终的 `angle` 会慢慢跟过去。调这个参数时，优先看 `angle_acc`，不要只看 `angle`。

调试步骤：

1. 保持停机，不让电机输出。

```c
g_balance_debug.run_enable = 0
```

2. 用手把小车扶到你认为的机械直立点，也就是两个轮子着地、车身刚好能平衡的位置。

3. 在 Ozone 里观察：

```c
g_balance_state.angle_acc
```

4. 使用这个公式修正：

```text
新的 angle_offset = 当前 angle_offset - 当前直立时的 angle_acc
```

例子 1：

```text
当前 angle_offset = 0.5
直立时 angle_acc = +3.0
新的 angle_offset = 0.5 - 3.0 = -2.5
```

例子 2：

```text
当前 angle_offset = 0.5
直立时 angle_acc = -2.0
新的 angle_offset = 0.5 - (-2.0) = 2.5
```

5. 把新的值写到 Ozone 里的：

```c
g_balance_debug.angle_offset
```

6. 再扶正车，继续观察 `g_balance_state.angle_acc`。调好后，直立时它应该接近 0。

验证标准：

- 直立时 `g_balance_state.angle_acc` 最好在 `-0.5 ~ +0.5` 度附近。
- 直立时 `g_balance_state.angle` 应该慢慢靠近 0，通常在 `-1 ~ +1` 度附近比较理想。
- 如果车放倒，`angle` 在接近 `+90` 或 `-90` 度是正常现象。
- 如果车直立时 `angle` 仍然接近 `+90` 或 `-90` 度，说明 MPU6050 安装方向和代码里的角度轴不匹配，应该先检查 `ax/ay/az` 或调整角度计算公式，不建议单纯用 `angle_offset` 硬补 90 度。

调好后，把最终值写回：

```c
.angle_offset = 你的实测修正值,
```

位置同样在 `Core/Src/balance_car/balance_control.c` 的 `g_balance_debug` 初始化处。写回代码后要重新编译并烧录。

### 6.2 架空测试电机

接电机电源前必须把车架空，让轮子离地。

先在 Ozone 中保持：

```c
g_balance_debug.run_enable = 0
```

然后把 PID 临时调小：

```c
g_angle_pid.Kp = 1.0
g_angle_pid.Ki = 0.0
g_angle_pid.Kd = 0.0
g_angle_pid.OutOffset = 0.0
g_speed_pid.Kp = 0.0
g_speed_pid.Ki = 0.0
g_turn_pid.Kp = 0.0
g_turn_pid.Ki = 0.0
```

再把：

```c
g_balance_debug.run_enable = 1
```

调好标准：

- 车往前倒，轮子应该往前追
- 车往后倒，轮子应该往后追
- `left_pwm/right_pwm` 不应突然打满
- 如果方向反了，不要继续加 PID，先修方向

Data Graph 建议观察：

```c
g_balance_state.angle
g_angle_pid.Out
g_balance_state.left_pwm
g_balance_state.right_pwm
```

调好的曲线现象：

- 前后轻轻倾斜车身时，`left_pwm/right_pwm` 会跟着变化。
- PWM 不应该一启动就长期贴着 `+100` 或 `-100`。
- 如果车身倾斜很小但 PWM 立刻打满，先检查角度方向、电机方向或把 `Kp` 降低。

### 6.3 调角度环

只调角度环，速度环和转向环先关掉。

建议顺序：

1. `Ki = 0`
2. `Kd = 0`
3. 慢慢增加 `Kp`
4. 方向正确、有扶正力后增加 `Kd`
5. 最后再加少量 `Ki`
6. 电机小 PWM 不动时再加 `OutOffset`

调好标准：

- 架空时，前后倾斜轮子追车方向正确
- 落地短时间测试时，车能明显抵抗倾倒
- 不会一启动就打满 PWM
- 不会疯狂高频抖动

Data Graph 建议观察：

```c
g_balance_state.angle
g_angle_pid.Target
g_angle_pid.Out
g_balance_state.left_pwm
g_balance_state.right_pwm
```

调好的曲线现象：

- `angle` 能围绕 `g_angle_pid.Target` 附近小幅波动。
- 轻推车身后，`angle` 会偏离，然后能回到目标附近。
- `g_angle_pid.Out` 不长期顶到 `OutMax/OutMin`。
- `left_pwm/right_pwm` 不长期打满 `+100` 或 `-100`。
- 曲线不会越振越大。

异常曲线：

- `angle` 振荡越来越大：方向可能反了，或 `Kp/Kd` 不合适。
- `g_angle_pid.Out` 长期打满：角度环输出饱和，参数太大或方向错误。
- PWM 长期满输出：不要继续加 PID，先检查角度方向和电机方向。

### 6.4 调编码器

用手转轮子，观察：

```c
g_balance_state.left_speed
g_balance_state.right_speed
```

调好标准：

- 左轮往车前进方向转，`left_speed` 为正
- 右轮往车前进方向转，`right_speed` 为正
- 两边速度数值变化连续，不乱跳

如果某边反了，可以交换编码器 A/B 相，或者在代码中给该侧 delta 加负号。

### 6.5 调速度环

角度环能基本直立后，再打开速度环。

先保持：

```c
g_balance_debug.speed_target = 0
```

从小参数开始：

```c
g_speed_pid.Kp = 0.5
g_speed_pid.Ki = 0.0
g_speed_pid.Kd = 0.0
```

观察：

```c
g_balance_state.ave_speed
g_speed_pid.Out
g_angle_pid.Target
```

调好标准：

- `speed_target = 0` 时，车不会持续向一个方向越跑越快
- 轻推车后，速度环会通过改变 `g_angle_pid.Target` 把速度拉回来
- `g_speed_pid.Out` 不应长期打到 `OutMax/OutMin`

Data Graph 建议观察：

```c
g_speed_pid.Target
g_balance_state.ave_speed
g_speed_pid.Out
g_angle_pid.Target
g_balance_state.angle
```

如果图像窗口放得下，也可以加：

```c
g_balance_state.left_speed
g_balance_state.right_speed
```

速度环的数据关系是：

```text
g_speed_pid.Target        -> 目标速度
g_balance_state.ave_speed -> 实际平均速度
g_speed_pid.Out           -> 速度环输出
g_angle_pid.Target        -> 速度环给角度环的目标倾角
```

`speed_target = 0` 时，调好的曲线现象：

- `ave_speed` 围绕 0 上下波动，不会一直偏正或一直偏负。
- `g_speed_pid.Out` 不会一直单方向增大。
- `g_angle_pid.Target` 不会长期顶到速度环输出限幅。
- 车不会自己越跑越远。

给一个小速度目标，例如：

```c
g_balance_debug.speed_target = 0.3f;
```

或：

```c
g_balance_debug.speed_target = -0.3f;
```

调好的曲线现象：

- `ave_speed` 会朝 `g_speed_pid.Target` 的方向靠近。
- `ave_speed` 不一定完全等于目标值，但不能反方向跑。
- `g_speed_pid.Out` 会先变化，然后逐渐收敛。
- `g_angle_pid.Target` 是小幅变化，不应该突然变得很大。
- 车能平稳前进或后退。

异常曲线：

- 目标速度为正，但 `ave_speed` 长期往负方向走：编码器方向、速度环方向或电机方向可能反了。
- `ave_speed` 追目标时振荡越来越大：`Kp` 可能过大，或 `Ki` 加得太早。
- `g_speed_pid.Out` 长期顶到 `+20` 或 `-20`：速度环输出饱和，先减小参数或检查方向。
- `g_angle_pid.Target` 突然很大：速度环给角度环的目标太猛。

### 6.6 调转向环

最后调转向环。

先保持：

```c
g_balance_debug.turn_target = 0
```

从小参数开始：

```c
g_turn_pid.Kp = 1.0
g_turn_pid.Ki = 0.0
g_turn_pid.Kd = 0.0
```

观察：

```c
g_balance_state.dif_speed
g_turn_pid.Out
g_balance_state.dif_pwm
```

调好标准：

- `turn_target = 0` 时，左右轮不应长期有很大的差分输出
- 小幅给 `turn_target` 后，左右轮能产生可控差速
- 车不会因为转向环介入而破坏直立

Data Graph 建议观察：

```c
g_turn_pid.Target
g_balance_state.dif_speed
g_turn_pid.Out
g_balance_state.dif_pwm
g_balance_state.left_speed
g_balance_state.right_speed
g_balance_state.angle
```

转向环的数据关系是：

```text
g_turn_pid.Target         -> 目标左右速度差
g_balance_state.dif_speed -> 实际左右速度差
g_turn_pid.Out            -> 转向环输出
g_balance_state.dif_pwm   -> 最终差分PWM
```

`turn_target = 0` 时，调好的曲线现象：

- `dif_speed` 围绕 0 附近波动。
- `left_speed/right_speed` 不会明显一正一负互相打架。
- `g_turn_pid.Out` 不会长期顶到 `+50` 或 `-50`。

给一个小转向目标，例如：

```c
g_balance_debug.turn_target = 0.3f;
```

调好的曲线现象：

- `dif_speed` 会朝 `g_turn_pid.Target` 的方向变化。
- `left_speed` 和 `right_speed` 会拉开差值。
- 车会产生可控转向。
- 转向时 `angle` 不会明显失控。

异常曲线：

- `turn_target` 为正，但 `dif_speed` 长期往负方向走：转向方向可能反了。
- `g_turn_pid.Out` 长期打满 `+50` 或 `-50`：转向环参数太大或方向错误。
- 转向一介入，`angle` 大幅震荡：转向环太猛，先减小 `Kp/Ki`。

### 6.7 循迹接口预留调试

当前工程未启用循迹控制。后续接入循迹前，先确认模块类型：

- 数字输出循迹模块：输出只有高低电平，可接普通 GPIO。
- 模拟输出循迹模块：输出是连续电压，必须接 ADC 引脚或外部 ADC。

加入循迹前，建议先让角度环、速度环和转向环稳定。循迹代码只负责把循迹偏差转换成：

```c
g_balance_debug.turn_target
```

调试时应观察：

```c
g_balance_debug.turn_target
g_balance_state.dif_speed
g_turn_pid.Out
g_balance_state.angle
g_balance_state.ave_speed
```

### 6.8 OLED、温湿度和重量显示调试

先不要接电机电源，只接 STM32、MPU6050、OLED、DHT11 和 RFP602。

在 Ozone 中观察：

```c
g_sensor_state.oled_ready
g_sensor_state.temperature_c
g_sensor_state.humidity_percent
g_sensor_state.fsr_adc_raw
g_sensor_state.fsr_voltage_mv
g_sensor_state.weight_g
g_sensor_state.sensor_fault_flags
g_sensor_state.oled_addr_7bit
g_sensor_state.oled_probe_mask
g_sensor_state.oled_fail_step
```

调好标准：

- OLED 每隔约 500ms 更新一次显示。
- OLED 正常显示温度、湿度、重量估算值和 ADC 原始值。
- DHT11 温湿度每隔约 2s 更新一次。
- 按压 FSR402 时，`fsr_adc_raw` 和 `fsr_voltage_mv` 连续变化。
- 空载时 `weight_g` 接近 0。

FSR402 重量校准步骤：

1. 空载时观察 `g_sensor_state.fsr_adc_raw`，把稳定值写入：

```c
g_sensor_state.fsr_zero_adc
```

2. 放一个已知重量的物体，观察新的 ADC 值。

3. 按下面公式计算：

```text
fsr_g_per_count = 已知重量g / (当前fsr_adc_raw - fsr_zero_adc)
```

4. 把结果写入：

```c
g_sensor_state.fsr_g_per_count
```

FSR402 受受力面积、安装结构和材料回弹影响很大，建议只把它当作估算重量或压力趋势显示。

### 6.9 ESP8266 网页遥控调试

ESP8266 固件在：

```text
esp8266_remote/esp8266_remote.ino
```

同时提供 PlatformIO 工程：

```text
esp8266_remote/platformio.ini
esp8266_remote/src/main.cpp
```

网页遥控的工作链路是：

```text
手机浏览器
  -> ESP8266热点网页
  -> ESP8266串口TX
  -> STM32 PB11 / USART3_RX
  -> g_balance_debug.speed_target / turn_target / run_enable
```

#### 6.9.1 烧录 ESP8266 固件

推荐方式是 VS Code + PlatformIO。它可以把依赖下载到英文路径的 PlatformIO 目录里，通常比 Arduino IDE 更不容易受中文 Windows 用户名影响。

PlatformIO 配置步骤：

1. 安装 VS Code。
2. 打开 VS Code 扩展商店，搜索并安装 `PlatformIO IDE`。
3. 安装完成后重启 VS Code。
4. 在 VS Code 中选择 `File -> Open Folder`，打开：

```text
F:\STM32CubeMX\project\balance_car\balance_car\esp8266_remote
```

5. 左侧会出现 PlatformIO 图标，等待它自动安装 PlatformIO Core 和 ESP8266 平台。
6. 根据你的模块选择环境：

| 模块 | PlatformIO 环境 |
| --- | --- |
| NodeMCU | `nodemcuv2` |
| Wemos D1 mini | `d1_mini` |
| ESP-01/ESP-01S | `esp01_1m` |

7. 点击 PlatformIO 的 `Build` 先编译。
8. 如果使用 NodeMCU 或 Wemos D1 mini，可以 USB 连接后点击 `Upload` 烧录。
9. 如果使用 ESP-01/ESP-01S 专用下载器，也可以只用 PlatformIO 编译，再用 Flash Download Tools 烧录，见下面“ESP-01/ESP-01S 实际推荐烧录流程”。

也可以在 VS Code 的 PlatformIO Terminal 中执行：

```bash
pio run -e nodemcuv2
pio run -e nodemcuv2 -t upload
```

如果是 Wemos D1 mini：

```bash
pio run -e d1_mini
pio run -e d1_mini -t upload
```

如果是 ESP-01/ESP-01S：

```bash
pio run -e esp01_1m
```

编译成功后，固件文件在：

```text
F:\STM32CubeMX\project\balance_car\balance_car\esp8266_remote\.pio\build\esp01_1m\firmware.bin
```

ESP-01/ESP-01S 专用下载器如果用 PlatformIO 的 `Upload` 出现下面这种错误：

```text
Failed to connect to ESP8266: Timed out waiting for packet header
```

可以改用乐鑫 `Flash Download Tools` 烧录。这个方式已经验证可行。

ESP-01/ESP-01S 实际推荐烧录流程：

1. 在 PlatformIO 中执行 `esp01_1m -> General -> Build`，只编译，不点 Upload。
2. 新建纯英文目录：

```text
C:\ESP8266_FLASH
```

3. 把烧录工具复制到英文目录，例如：

```text
C:\ESP8266_FLASH\flash_download_tools
```

4. 把 PlatformIO 生成的固件复制到英文目录：

```text
源文件:
F:\STM32CubeMX\project\balance_car\balance_car\esp8266_remote\.pio\build\esp01_1m\firmware.bin

复制到:
C:\ESP8266_FLASH\firmware.bin
```

5. 右键以管理员身份运行：

```text
C:\ESP8266_FLASH\flash_download_tools\flash_download_tools_v3.6.8.exe
```

6. 选择 `ESP8266 DownloadTool`。
7. 添加 bin 文件：

```text
C:\ESP8266_FLASH\firmware.bin
```

8. 地址填写：

```text
0x00000
```

9. 参数建议：

```text
SPI SPEED: 40MHz
SPI MODE: DOUT
FLASH SIZE: 8Mbit / 1MByte
BAUD: 115200
COM: 选择设备管理器里 ESP-01S 下载器对应的 COM 口
```

10. 点击 `START` 开始烧录。若一直等待连接，可以在点击 `START` 后按一下下载器上的 `RST`。

注意：

- 烧录工具路径不要有中文。
- `firmware.bin` 路径不要有中文。
- 如果出现 `UnicodeDecodeError: 'gb2312' codec can't decode...`，就是工具或 bin 所在路径包含中文、特殊字符，按上面的方式放到 `C:\ESP8266_FLASH`。
- 这里不要选择教程里的 AT 固件；要选择本项目生成的 `firmware.bin`。

Arduino IDE 也可以使用，配置步骤：

1. 打开 Arduino IDE。
2. 进入 `文件 -> 首选项`。
3. 在“附加开发板管理器网址”填入：

```text
https://arduino.esp8266.com/stable/package_esp8266com_index.json
```

4. 进入 `工具 -> 开发板 -> 开发板管理器`。
5. 搜索 `esp8266`，安装 `ESP8266 by ESP8266 Community`。
6. 打开 `esp8266_remote/esp8266_remote.ino`。

开发板选择：

- NodeMCU：选择 `NodeMCU 1.0 (ESP-12E Module)`。
- Wemos D1 mini：选择 `LOLIN(WEMOS) D1 R2 & mini`。
- ESP-01/ESP-01S：选择 `Generic ESP8266 Module`。

NodeMCU / Wemos D1 mini 烧录：

1. 用 USB 连接电脑。
2. Arduino IDE 选择正确串口。
3. 点击上传。
4. 上传完成后，手机搜索 Wi-Fi：`BalanceCar`。

ESP-01 / ESP-01S 烧录接线：

| USB-TTL / 电源 | ESP8266 |
| --- | --- |
| TX | RX |
| RX | TX |
| GND | GND |
| 3.3V 稳压 | VCC |
| 3.3V | EN / CH_PD |
| GND | GPIO0 |
| 3.3V | GPIO2 |
| 3.3V | RST，烧录前可短接 GND 复位 |

烧录完成后断电，把 `GPIO0` 改回 3.3V 或上拉，再重新上电运行。

#### 6.9.2 只测试 ESP8266

先不要接 STM32。

1. ESP8266 单独供电。
2. 手机连接 Wi-Fi：`BalanceCar`，密码：`12345678`。
3. 浏览器打开：

```text
http://192.168.4.1
```

4. 页面应出现 `启动`、`前进`、`后退`、`左转`、`右转`、`停止`、`急停`按钮。
5. 如果 ESP8266 同时接着 USB 串口，点按钮时串口会输出：

```text
RUN 1
SPD 0.5
SPD 0
TURN 0.4
TURN 0
STOP
RUN 0
```

方向键按住时，ESP8266 会每约 200ms 重复发送当前方向命令；松开方向键后发送 `SPD 0` 或 `TURN 0`。这样手机断连、网页卡住或松手后，STM32 都能通过 500ms 超时保护把速度和转向目标清零。

调好标准：

- 手机能搜到 `BalanceCar` 热点。
- 能打开 `http://192.168.4.1`。
- 点按钮后页面状态文字会变化。
- 串口能看到对应 ASCII 命令。

#### 6.9.3 只测试 STM32 USART3 遥控

先不要接电机电源，可以用 USB-TTL 临时代替 ESP8266。

| USB-TTL | STM32 |
| --- | --- |
| TX | PB11 / USART3_RX |
| RX | PB10 / USART3_TX |
| GND | GND |

串口助手设置：

```text
115200
8 data bits
1 stop bit
No parity
发送换行符 \n
```

手动发送：

```text
PING
SPD 0.5
TURN 0.3
STOP
RUN 1
RUN 0
```

Ozone 中观察：

```c
g_remote_state.rx_count
g_remote_state.valid_cmd_count
g_remote_state.invalid_cmd_count
g_remote_state.last_command
g_remote_state.speed_cmd
g_remote_state.turn_cmd
g_remote_state.link_active
g_balance_debug.speed_target
g_balance_debug.turn_target
g_balance_debug.run_enable
```

调好标准：

- 每发送一个字符，`rx_count` 增加。
- 发送正确命令后，`valid_cmd_count` 增加。
- `last_command` 显示最近一条命令。
- `SPD 0.5` 后，`speed_target` 变为 `0.5`。
- `TURN 0.3` 后，`turn_target` 变为 `0.3`。
- `STOP` 后，`speed_target` 和 `turn_target` 都回到 0。
- `RUN 1/RUN 0` 能切换 `run_enable`。

如果 `rx_count` 不变，优先检查 PB10/PB11 是否接反、GND 是否共地、串口波特率是否为 115200。

#### 6.9.4 测 OLED 和 MPU6050 共用 I2C1

OLED 新接线：

| OLED | STM32 |
| --- | --- |
| SCL | PB8 |
| SDA | PB9 |
| VCC | 3.3V |
| GND | GND |

MPU6050 也接 PB8/PB9。先不要接电机电源，在 Ozone 中观察：

```c
g_balance_state.imu_ready
g_balance_state.mpu_id
g_balance_state.angle
g_sensor_state.oled_ready
g_sensor_state.oled_probe_mask
g_sensor_state.oled_fail_step
```

调好标准：

- `mpu_id == 0x68`。
- `imu_ready == 1`。
- 手动倾斜车体时，`angle` 连续变化。
- `oled_ready == 1`。
- OLED 正常显示温度、湿度、重量和 ADC。

如果 OLED 不亮，看：

```c
g_sensor_state.oled_probe_mask
g_sensor_state.oled_fail_step
```

`oled_fail_step == 2` 且 `oled_probe_mask == 0`，说明 PB8/PB9 上没有探测到 `0x3C` 或 `0x3D` OLED。

#### 6.9.5 ESP8266 和 STM32 联调

正式接线：

| STM32 | ESP8266 |
| --- | --- |
| PB10 / USART3_TX | RX |
| PB11 / USART3_RX | TX |
| GND | GND |
| 3.3V 稳压电源 | VCC / 3V3 |

步骤：

1. 先不接电机电源。
2. ESP8266 和 STM32 共地。
3. 手机连接 `BalanceCar` 热点。
4. 浏览器打开 `http://192.168.4.1`。
5. Ozone 观察 `g_remote_state` 和 `g_balance_debug`。
6. 点击网页按钮。

调好标准：

- 点 `启动`：`g_balance_debug.run_enable` 变 1。
- 点 `急停`：`g_balance_debug.run_enable` 变 0。
- 按住 `前进`：`speed_target` 变为正值；松开后回 0。
- 按住 `后退`：`speed_target` 变为负值；松开后回 0。
- 按住 `左转/右转`：`turn_target` 正负变化；松开后回 0。
- 按住方向键时，`last_command` 会持续刷新；停止发送超过 500ms 后，`link_active` 变 0，`speed_target/turn_target` 自动回 0。

#### 6.9.6 架空和落地测试

车轮离地后再接电机电源。

架空观察：

```c
g_balance_state.angle
g_balance_state.left_pwm
g_balance_state.right_pwm
g_balance_state.left_speed
g_balance_state.right_speed
g_balance_debug.speed_target
g_balance_debug.turn_target
```

调好标准：

- 前进时两个轮子方向一致。
- 后退时两个轮子方向一致，并与前进相反。
- 左转/右转时左右轮速度有差值。
- 松开网页按钮后目标归零。
- `left_pwm/right_pwm` 不长期打满。

落地测试：

- 先用手扶车。
- 先点 `启动`，确认车能直立。
- 前进/后退先用网页默认小目标，不要一开始改大。
- 如果前进后退反了，只改 ESP8266 网页里 `SPD 0.5` 和 `SPD -0.5` 的符号。
- 如果左右转反了，只改 ESP8266 网页里 `TURN 0.4` 和 `TURN -0.4` 的符号。
- 不要因为遥控方向反了去改已经调好的角度环、电机方向或编码器方向。

## 7. 方向反了怎么办

方向问题不要同时改多个地方。一次只改一处。

### 7.1 角度方向反了

如果 `angle` 的正负方向和你的车体安装方向相反，可以改：

```c
float angle_acc = -atan2f((float)raw.ax, (float)raw.az) / PI_F * 180.0f;
```

改成：

```c
float angle_acc = atan2f((float)raw.ax, (float)raw.az) / PI_F * 180.0f;
```

如果动态角度又不对，还需要把陀螺仪积分方向一起反：

```c
float angle_gyro = s_angle + gy_calibrated / 32768.0f * 2000.0f * ANGLE_LOOP_PERIOD_S;
```

改成：

```c
float angle_gyro = s_angle - gy_calibrated / 32768.0f * 2000.0f * ANGLE_LOOP_PERIOD_S;
```

### 7.2 两个电机整体反了

如果 `angle` 看起来对，但车往前倒时两个轮子都往后跑，改平均 PWM 符号。

当前代码是：

```c
float ave_pwm = g_angle_pid.Out;
```

可以改成：

```c
float ave_pwm = -g_angle_pid.Out;
```

或者反过来，根据你的车实际方向决定。

### 7.3 只有一个电机反了

如果一个轮子往前、一个轮子往后，改 `motor_tb6612.c` 中对应电机的方向脚逻辑，或者直接交换该侧电机线。

左电机方向由 `AIN1/AIN2` 决定，右电机方向由 `BIN1/BIN2` 决定。

### 7.4 编码器方向反了

如果手动往前转轮子，但速度变量是负的，可以在 `encoder_hal.c` 中把对应返回值取负。

例如左编码器反了：

```c
return delta;
```

改成：

```c
return -delta;
```

## 8. 故障标志

`g_balance_state.fault_flags` 是按位组合的。

| 值 | 含义 | 排查方向 |
| --- | --- | --- |
| `0x00000000` | 无故障 | 正常 |
| `0x00000001` | MPU6050 初始化失败 | 检查 PB8/PB9、供电、地址 |
| `0x00000002` | MPU6050 读取失败 | 检查 I2C 接触、供电干扰 |
| `0x00000004` | 倾角超过保护阈值 | 车倒了，程序自动停机 |
| `0x00000008` | 控制节拍堆积 | 主循环太慢或卡住 |
| `0x00000010` | 电机初始化失败 | 检查 TIM3 PWM/GPIO 初始化 |
| `0x00000020` | 编码器初始化失败 | 检查 TIM1/TIM2 初始化 |
| `0x00000040` | TIM4 初始化失败 | 检查控制节拍定时器 |

清除故障：

```c
g_balance_debug.clear_fault_request = 1;
```

如果硬件问题还在，故障会再次出现。

`g_sensor_state.sensor_fault_flags` 是 OLED、温湿度和压力/重量估算相关的故障位。

| 值 | 含义 | 排查方向 |
| --- | --- | --- |
| `0x00000000` | 无故障 | 正常 |
| `0x00000001` | DHT11 初始化失败 | 检查 PC14、供电和上拉 |
| `0x00000002` | DHT11 读取失败 | 检查数据线、上拉、电源稳定性 |
| `0x00000004` | FSR ADC 初始化失败 | 检查 ADC1/PA2 配置 |
| `0x00000008` | FSR ADC 读取失败 | 检查 PA2 输入和 ADC |
| `0x00000010` | OLED 初始化失败 | 检查 PB8/PB9、地址 0x3C、供电 |
| `0x00000020` | OLED 刷新失败 | 检查 I2C 总线和 OLED 接触 |

OLED 不亮时，优先看：

```c
g_sensor_state.oled_probe_mask
g_sensor_state.oled_fail_step
```

如果 `oled_fail_step == 2` 且 `oled_probe_mask == 0`，说明 PB8/PB9 上没有探测到 `0x3C` 或 `0x3D` OLED，应优先检查 SCL/SDA 是否接反、供电/GND、模块是否真的是 I2C 版本。

`g_remote_state.fault_flags` 是 ESP8266/USART3 遥控相关故障位。

| 值 | 含义 | 排查方向 |
| --- | --- | --- |
| `0x00000000` | 无故障 | 正常 |
| `0x00000001` | USART3 初始化失败 | 检查 UART HAL、PB10/PB11 是否被其他外设占用 |
| `0x00000002` | 串口接收重启失败 | 检查中断是否正常、USART3 是否初始化 |
| `0x00000004` | 命令行过长溢出 | ESP8266 发出的单条命令不要超过 31 字符 |
| `0x00000008` | 命令格式错误 | 检查是否为 `RUN/SPD/TURN/STOP/PING` 格式 |

## 9. 脱离 Ozone 后自启动

调试完成前，不建议自启动。默认代码保持停机：

```c
.run_enable = 0U
```

如果已经确认：

- MPU6050 方向正确
- 电机方向正确
- 编码器方向正确
- 角度环能稳定直立
- 速度环不会把车越推越快

可以改成：

```c
.run_enable = 1U
```

位置在：

```text
Core/Src/balance_car/balance_control.c
```

这样上电后程序会自动进入平衡控制。

当前工程也支持 PB6 实体按键启停。保持 `.run_enable = 0U` 时，单独上电后先停机，按下 PB6 后进入运行允许状态，再按一次停机；PC13 指示灯会跟随运行允许状态亮灭。这个方式更适合脱离 Ozone 后日常使用。

建议自启动时仍保持：

```c
.speed_target = 0.0f
.turn_target = 0.0f
```

## 10. 常见问题

### AX、AY、AZ 哪个是俯仰角？

都不是。

```c
g_balance_state.ax
g_balance_state.ay
g_balance_state.az
```

它们是加速度计三个轴的原始值。

真正的俯仰角是：

```c
g_balance_state.angle
```

其中：

```c
g_balance_state.angle_acc   // 加速度计算出的角度
g_balance_state.angle_gyro  // 陀螺仪积分角度
g_balance_state.angle       // 互补滤波后的最终角度
```

### 为什么找不到 ax/ay/az 的单独定义？

因为它们不是单独的全局变量，而是结构体成员。

结构体定义在：

```text
Core/Inc/balance_car/balance_control.h
```

变量定义在：

```text
Core/Src/balance_car/balance_control.c
```

完整变量名是：

```c
g_balance_state.ax
g_balance_state.ay
g_balance_state.az
```

### 一上电电机不动是不是坏了？

不是。默认：

```c
g_balance_debug.run_enable = 0
```

这是安全设计。需要在 Ozone 中改成：

```c
g_balance_debug.run_enable = 1
```

才允许输出 PWM。

### 为什么 Ozone 要用 ELF？

因为 ELF 里有调试符号，Ozone 可以直接识别：

```c
g_balance_debug
g_balance_state
g_angle_pid
g_speed_pid
g_turn_pid
```

HEX/BIN 只适合烧录，不适合看变量。

### CubeMX 重新生成后要注意什么？

本项目的 I2C、PWM、编码器、TIM4 控制节拍由 `Core/Src/balance_car/` 中的 HAL 初始化代码配置。如果重新用 CubeMX 生成代码，重点检查：

- `main.c` 中是否还调用 `BalanceCar_Init()` 和 `BalanceCar_Background()`
- `stm32f1xx_hal_msp.c` 中是否仍保留 SWD，不要禁用 SWD
- `Makefile` 是否仍包含 `Core/Src/balance_car/*.c`
- `stm32f1xx_hal_conf.h` 是否启用了 `HAL_I2C_MODULE_ENABLED`、`HAL_TIM_MODULE_ENABLED`、`HAL_ADC_MODULE_ENABLED` 和 `HAL_UART_MODULE_ENABLED`
- OLED 是否仍接在 PB8/PB9，ESP8266 是否仍接在 PB10/PB11

## 11. 安全建议

- 第一次调试不要接电机电源。
- 接电机电源时必须把车架空。
- 第一次运行时先把 PID 调小。
- 任何异常先把 `g_balance_debug.run_enable` 改为 `0`。
- 不要在方向没确认前加大 `Kp/Kd`。
- 不要同时修改角度方向、电机方向和编码器方向。
- 车能短时间直立后，再逐步调速度环和转向环。
