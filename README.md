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
left_speed = left_delta / 44.0 / 0.05 / 9.27666;
right_speed = right_delta / 44.0 / 0.05 / 9.27666;
ave_speed = (left_speed + right_speed) / 2.0;
dif_speed = left_speed - right_speed;
```

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
| `speed_target` | 目标前后速度 | 初期保持 0 |
| `turn_target` | 目标转向速度差 | 初期保持 0 |
| `gyro_y_offset` | 陀螺仪 Y 轴零漂 | 静止时观察 `g_balance_state.gy`，把静止平均值填进去 |
| `angle_offset` | 机械竖直角度偏移 | 扶正车后观察 `angle_acc`，按公式修正到接近 0 |
| `fall_angle_limit` | 倒车保护角度 | 默认 50 度 |

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

### 5.3 三个 PID

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

### 6.7 后续循迹调试

当前代码已撤掉循迹模块，只保留平衡车本体控制。后续如果重新加入循迹，先确认模块类型：

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

## 9. 脱离 Ozone 后自启动

调试完成前，不建议自启动。默认代码：

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
- `stm32f1xx_hal_conf.h` 是否启用了 `HAL_I2C_MODULE_ENABLED` 和 `HAL_TIM_MODULE_ENABLED`

## 11. 安全建议

- 第一次调试不要接电机电源。
- 接电机电源时必须把车架空。
- 第一次运行时先把 PID 调小。
- 任何异常先把 `g_balance_debug.run_enable` 改为 `0`。
- 不要在方向没确认前加大 `Kp/Kd`。
- 不要同时修改角度方向、电机方向和编码器方向。
- 车能短时间直立后，再逐步调速度环和转向环。
