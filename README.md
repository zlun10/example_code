# README

## 文件目录：

### Config  配置层

### app      具体实现

### middleware 中间组件

与lib的区别是，middleware比较复杂


### lib       三方库

### hal       抽象层

### drv       驱动层

理论上，drv应该并入到hal里面
/还是应该有所区别，hal是暂时为lib（也就是第三方的库相关）服务；drv与具体的硬件有联系（外设或者串口）

### dev       设备接口层

与硬件有相关 - 暂时

### bsp      板级应用

### chip     芯片相关

#### cw32l011

- 内核文件
- 启动文件
- 系统初始化文件



### project

> 暂时使用这个，之后使用gcc直接编译文件



### tools    测试与脚本
- tests
- scripts


config - app - bsp/componnet - hal/lib - dev/drv - chip
target:
demo:LCD-DEMO,
sfm: LCD-SFM,

细分领域：
键盘
lcd
led
key
module交互io

