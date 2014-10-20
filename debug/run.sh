./run-qemu.sh &
pidQemu=$!
./run-gdb.sh
kill $pidQemu
killall qemu-system-arm
