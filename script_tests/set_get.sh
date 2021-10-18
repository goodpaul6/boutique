./../build/Debug/server/boutique 8080 &

SERVER_PID=$!

./../build/Debug/test_client/test_client localhost 8080 < set_get_input.txt

kill -9 $SERVER_PID
wait
