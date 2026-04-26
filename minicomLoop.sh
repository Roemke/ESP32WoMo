while true; do
  for PORT in /dev/ttyACM*; do
    if [ -e "$PORT" ]; then
      echo "Verbinde mit $PORT..."
      minicom -D $PORT -b 115200
    fi
  done
  sleep 1
done
