> 常用命令：
>
> ​    播放NV12:                ffplay -f rawvideo -pixel_format nv12 -video_size 650x850 650x850.nv12
>
> ​    其他文件转NV12：  ffmpeg -i xiyang.jpeg -f rawvideo -pix_fmt nv12 640x850.nv12

#### 001 非ffmpeg: 去除YUV彩色

去除色彩，只需要将UV的值均改为0x80.

#### 002 非ffmpeg: YUV画中画

#### 003 非ffmpeg: 合并两张YUV

#### 004 非ffmpeg: RGB转YUV

#### 005 非ffmpeg: RGB转YUV, NEON优化

#### 006 非ffmpeg: RGB转YUV, CUDA优化

#### 007 非ffmpeg: RGB转灰度图像

#### 008 非ffmpeg: YUV转灰度图像

#### 009 非ffmpeg: 灰度图像二值化

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
# ffplay -i /dev/video0
# ffplay -f v4l2 -input_format mjpeg -i /dev/video0       //指定摄像头输出格式为mjpeg

6. 查看图像(Mac平台)
# ffmpeg -f avfoundation -list_devices true -i ""         //列出所有设备
# ffplay -f avfoundation -framerate 30 -pixel_format yuyv422 -video_size 640x480 -i "0"   //播放设备0
```

#### 100 读取USB Camera 

#### 101 JPG解码YUV

#### PNG解码YUV

#### BMP解码YUV

#### MP4解码YUV

#### FLV解码yuv

#### MP4转FLV

#### MP4转TS

#### MP4提取H264


