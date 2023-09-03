установка gstreaming

```

apt-get install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-tools gstreamer1.0-x gstreamer1.0-alsa gstreamer1.0-gl gstreamer1.0-gtk3 gstreamer1.0-qt5 gstreamer1.0-pulseaudio
```

компиляция gcc

```
gcc basic-tutorial-1.c -o basic-tutorial-1 `pkg-config --cflags --libs gstreamer-1.0`
```

компиляция meson

```

```

Компиляция cmake

```
mkdir build && cd build
cmake .. && make
```

Запуск

```
export GST_PLUGIN_PATH=.:$GST_PLUGIN_PATH
export GST_DEBUG=*:4;
gst-launch-1.0 videotestsrc ! video/x-raw,width=800,height=600,framerate=30/1 ! cvfilter ! autovideosink –v

```
