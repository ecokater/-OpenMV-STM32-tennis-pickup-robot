################################################################################
# OpenMV 网球检测 + 串口输出（单文件版）
################################################################################
# 这是一个“单文件可运行”的脚本版本：不依赖 uart_service/network_service/vision_service 等外部模块，
# 直接把串口、图像处理、异步调度全部放在一个 main.py 里，方便拷贝到 /flash 运行。
#
# 功能概览：
# 1) 图像：每帧采集 -> 颜色阈值找球 -> 绘制框/十字/圆 -> 显示 FPS（OpenMV IDE 可直接看到）
# 2) 串口：可选通过 UART3 输出 JSON（目标坐标/半径）
#
# 重要开关：
# - SEND_JSON_ON_DATA_UART = True/False：是否把 JSON 打到数据串口（UART3）
#
# 串口分工（按你之前约定）：
# - UART3：数据串口（发 JSON 文本，便于调试/外部 MCU 解析）
################################################################################

import sensor
import time
import uasyncio as asyncio
from machine import UART, SPI, Pin
import struct

################################################################################
# 参数配置区
################################################################################

# 数据串口（UART3）
SERVO_UART_ID = 3
SERVO_UART_BAUD = 115200

# 颜色阈值（LAB）：格式为 (Lmin, Lmax, Amin, Amax, Bmin, Bmax)
thresholds = [(0, 100, -67, -15, 22, 99)]


WIN_W = 80
WIN_H = 80
MAX_W = 640
MAX_H = 480


################################################################################
# 视觉结果共享变量（图像/追踪/遥测任务共享）
################################################################################
# latest_target：最近一次检测到的目标信息 {"cx","cy","r","blob"}
# latest_target_seq：目标序号（检测到新目标就 +1），用于区分新旧（任务只处理 seq 变化的事件）
latest_target = None
latest_target_seq = 0

################################################################################
# 串口初始化（只初始化一次，避免循环里重复 UART(...)）
################################################################################
servo_uart = UART(SERVO_UART_ID, SERVO_UART_BAUD)
#SPI初始化
cs = Pin("P3", Pin.OUT,value=1)
spi = SPI(2, baudrate=10000000, polarity=1, phase=1, bits=8, firstbit=SPI.MSB)
################################################################################
# 图像采集计时器（FPS）与当前帧
################################################################################
clock = time.clock()
img = None


# 当前帧尺寸（追踪时用来计算画面中心）
frame_w = 0
frame_h = 0


def clamp(n, minn, maxn):
    return max(min(maxn, n), minn)

def send_data(ball_x, ball_y):
    # 打包格式：0xAA + 0xBB + X(2字节) + Y(2字节) + 0x0D + 0x0A
    ball_data = struct.pack("<BBHHBB", 0xAA, 0xBB, int(ball_x), int(ball_y), 0x0D, 0x0A)
    servo_uart.write(ball_data)


##spi发送数据与图像给stm32##
def send_img(img, roi_x, roi_y):
    header = struct.pack("<IHH", 0xDEADBEEF, int(roi_x), int(roi_y))
    cs.value(0)
    spi.write(bytearray([0x00, 0x00, 0x00, 0x00])) # Dummy
    spi.write(header)
    spi.write(img.bytearray()) # Image (Zero-Copy)
    cs.value(1)

def crop_roi(src_img, roi_x, roi_y, roi_w, roi_h):
    try:
        return src_img.copy(roi=(roi_x, roi_y, roi_w, roi_h))
    except:
        pass
    try:
        return src_img.crop((roi_x, roi_y, roi_w, roi_h))
    except:
        return src_img.crop(roi_x, roi_y, roi_w, roi_h)


def sensor_init():
    # 相机初始化：一般放在主循环外，设置一次即可持续生效
    sensor.reset()
    sensor.set_pixformat(sensor.RGB565)
    sensor.set_framesize(sensor.VGA)
    sensor.skip_frames(time=2000)
    sensor.set_brightness(0)
    sensor.set_contrast(0)
    sensor.set_auto_gain(True)
    sensor.set_auto_whitebal(True)
    sensor.set_auto_exposure(True, exposure_us=2000)

def largest_blob(blobs):
    # 从多个色块中选最大一个（像素最多的 blob）
    if not blobs:
        return None
    m = blobs[0]
    a = blobs[0].pixels()
    for b in blobs:
        if b.pixels() > a:
            a = b.pixels()
            m = b
    return m

def draw_target_overlay(target):
    # 在图像上画出目标的外接框、中心十字、近似圆，方便在 IDE 观察
    b = target["blob"]
    cx = target["cx"]
    cy = target["cy"]
    r = target["r"]
    img.draw_rectangle(b.rect(), color=(255, 0, 0))
    img.draw_cross(cx, cy, color=(0, 255, 0))
    img.draw_circle(cx, cy, r, color=(0, 0, 255))



async def vision_task():
    # 图像任务：
    # 1) 采集一帧（IDE 自动显示）
    # 2) 找球并绘制可视化
    # 3) 发布 latest_target（通过 seq 让其他任务只处理“新事件”）
    global img, frame_w, frame_h
    global latest_target, latest_target_seq
    while True:
        clock.tick()
        img = sensor.snapshot()
        frame_w = img.width()
        frame_h = img.height()

        img.draw_string(0, 0, "FPS:%d" % clock.fps(), color=(255, 255, 255))

        blobs = img.find_blobs(thresholds, pixels_threshold=50, area_threshold=50, merge=True)
        if blobs:
            b = largest_blob(blobs)
            if b:
                cx = b.cx()
                cy = b.cy()
                r = max(b.w(), b.h()) // 2
                target = {"cx": cx, "cy": cy, "r": r, "blob": b}
                draw_target_overlay(target)

                latest_target = target
                latest_target_seq += 1

        await asyncio.sleep_ms(0)


async def telemetry_task():
    global latest_target_seq
    last_send = 0
    while True:
        if img is None:
            await asyncio.sleep_ms(0)
            continue
        now = time.ticks_ms()
        if time.ticks_diff(now, last_send) < 50:
            await asyncio.sleep_ms(0)
            continue
        last_send = now
        fw = frame_w if frame_w > 0 else MAX_W
        fh = frame_h if frame_h > 0 else MAX_H
        roi_x = (fw - WIN_W) // 2
        roi_y = (fh - WIN_H) // 2
        if latest_target:
            ball_x = latest_target["cx"]
            ball_y = latest_target["cy"]
            target_x = ball_x - (WIN_W // 2)
            target_y = ball_y - (WIN_H // 2)
            roi_x = clamp(target_x, 0, fw - WIN_W)
            roi_y = clamp(target_y, 0, fh - WIN_H)
        else:
            ball_x = fw // 2
            ball_y = fh // 2
        cut_img = crop_roi(img, roi_x, roi_y, WIN_W, WIN_H)

        # 优化：优先发送 UART 控制数据（数据量小，重要性高）
        send_data(ball_x, ball_y)#uart

        # 其次发送 SPI 图像数据（数据量大，耗时较长）
        send_img(cut_img, roi_x, roi_y)#spi

        # 非阻塞延时，让出 CPU 给其他任务，同时控制发送频率
        await asyncio.sleep_ms(0)

async def main():
    # 主入口：启动 4 个协作式任务
    asyncio.create_task(telemetry_task())
    await vision_task()

sensor_init()

try:
    asyncio.run(main())
finally:
    pass
