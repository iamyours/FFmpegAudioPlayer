在Android使用FFmpeg的一些例子
- [x] **mp3解码成pcm**
- [x] **多个文件解码合成**
    (使用amix过滤器合成多个音频文件)
- [x] **实时解码播放**
     (c++生产消费模型，实时解码，OpenSL ES播放pcm)
- [x] **多音轨播放**
     (多种AVFilter综合组合使用,实现音频变速，调音，多音轨合成实时播放)
- [x] **音频波形显示**
    (Visualizer替代方案，避免需要录音权限)
- [ ] **录音编码** (libmp3lame+FFmpeg交叉编译，使用FFmpeg实现实时录音编码成mp3)