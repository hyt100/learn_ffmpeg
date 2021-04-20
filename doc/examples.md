> 采用FFmpeg版本：4.3.2

#### 001 非ffmpeg: 去除YUV彩色

去除色彩，只需要将UV的值均改为0x80.

#### 002 非ffmpeg: YUV画中画

#### 003 非ffmpeg: 合并两张YUV

#### 004 非ffmpeg: RGB转YUV

#### 005 非ffmpeg: RGB转YUV, NEON优化 (TODO)

#### 006 非ffmpeg: RGB转YUV, CUDA优化 (TODO)

#### 007 非ffmpeg: RGB转灰度图像 (TODO)

#### 008 非ffmpeg: YUV转灰度图像 (TODO)

#### 009 非ffmpeg: 灰度图像二值化 (TODO)

#### 010 非ffmpeg: V4L2读取USB Camera并渲染

操作USB camera

```shell
1. 安装v4l2
# sudo apt install v4l-utils

2. 查看设备
# v4l2-ctl --list-devices

3. 查看当前摄像头支持的视频压缩故事
# v4l2-ctl -d /dev/video0 --list-formats

4. 查看当前摄像头所有参数
# v4l2-ctl -d /dev/video0 --all

5. 查看图像
Linux:
# ffplay -i /dev/video0
# ffplay -f v4l2 -input_format mjpeg -i /dev/video0       //指定摄像头输出格式为mjpeg
MAC:
# ffmpeg -f avfoundation -list_devices true -i ""         //列出所有设备
# ffplay -f avfoundation -framerate 30 -pixel_format yuyv422 -video_size 640x480 -i "0"   //播放设备0
```

#### 011 非ffmpeg: 给YUV增加黑色边框 (TODO)

#### 012 非ffmpeg: OPENGL渲染视频 (TODO)

#### 013 非ffmpeg: YUV转RGB，OPENGL实现 (TODO)

#### 非ffmpeg: 搜索H264文件NALU (TODO)

---

#### 100 读取USB Camera - MJPEG

#### 101 读取USB Camera - YUY2

#### 102 JPG解码YUV 

#### 103 YUV编码JPG (TODO)

#### 104 H264解码YUV

#### 105 YUV编码H264

x264源码安装默认不会安装库，需要增加配置： 

```shell
# ./configure --enable-shared --enable-static
```

然后重新编译ffmpeg:

```shell
# ./configure --enable-libx264 --enable-gpl --enable-shared --enable-static
```

验证x264是否生效：

```shell
# ffmpeg -h encoder=libx264      //可以看到x264支持格式信息
# ldd libavcodec.so              //可以看到库依赖libx264.so
```

编码器使用x264，代码中的名字是`libx264`:

```c++
avcodec_find_encoder_by_name("libx264");
```

#### 106 mp4通过滤镜倍速 (TODO)

#### 107 MP4抽帧 (TODO)

#### 108 YUV转MP4 (TODO)

#### 109 MP4提取H264 (TODO)

#### 110 H264封装MP4 (TODO)

#### PNG解码YUV (TODO)

#### BMP解码YUV (TODO)

#### MP4解码YUV (TODO)

#### FLV解码yuv (TODO)

#### MP4转FLV (TODO)

#### MP4转TS (TODO)





