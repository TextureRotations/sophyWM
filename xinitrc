# 1. Start some programs to play with.
# xclock &
# xeyes &
#sleep 2 && xterm &
xterm &

# 2. autostart config.
/home/oy/code/sophy/autostart.sh &

# 3. Start our window manager.
export GLOG_logtostderr=1
exec ./sophy


