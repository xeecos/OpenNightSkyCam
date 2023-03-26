# Open Night Sky Cam
# 蓝牙连接
### 查询名称和ip
### 配置WiFi网络
### 修改名称
# WiFi连接
### 查询状态
 * 版本号
 * 任务状态
 * 时间
### 查询SD文件列表
### 获取最新照片
### 设置参数
 * 拍摄参数
 * 时间
### 设置任务
### 开始任务
### 暂停任务
### 继续任务
# OTA升级

### Upload Firmware
esptool.exe --chip esp32s3 --port "COM5" --baud 921600  --before default_reset --after hard_reset write_flash  -z --flash_mode dio --flash_freq 80m --flash_size 8MB 0x0 ".pio//build//esp32s3//bootloader.bin" 0x8000 ".pio//build//esp32s3//partitions.bin" 0x10000 ".pio//build//esp32s3//firmware.bin"