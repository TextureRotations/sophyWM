# 1. Start some programs to play with.
# xclock &
# xeyes &
#sleep 2 && xterm &
xeyes &

# 2. autostart config.
/home/oy/code/sophyWM/autostart.sh &

# exec xrdb -merge /home/oy/.Xresources

# 3. Start our window manager.
export GLOG_logtostderr=1
exec ./sophy


