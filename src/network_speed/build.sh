
g++ -Wall -O2 -std=c++11 -I../../tools ./server.cpp ../../tools/NetworkUtils.cpp -o ./server -pthread
g++ -Wall -O2 -std=c++11 -I../../tools ./client.cpp ../../tools/NetworkUtils.cpp -o ./client -pthread

g++ -Wall -O2 -std=c++11 -I../../tools ./server_read.cpp ../../tools/NetworkUtils.cpp -o ./server_read -pthread
g++ -Wall -O2 -std=c++11 -I../../tools ./client_write.cpp ../../tools/NetworkUtils.cpp -o ./client_write -pthread

g++ -Wall -O2 -std=c++11 -I../../tools ./server_poll.cpp ../../tools/NetworkUtils.cpp -o ./server_poll -pthread
