if [ ! -p /tmp/oapw_stream ]; then
    mkfifo /tmp/oapw_stream
    chmod 666 /tmp/oapw_stream
fi
