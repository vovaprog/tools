
g++ -Wall -O2 -std=c++11 -I../cpp ./server.cpp ../cpp/NetworkUtils.cpp -o ./server -pthread
g++ -Wall -O2 -std=c++11 -I../cpp ./client.cpp ../cpp/NetworkUtils.cpp -o ./client -pthread

g++ -Wall -O2 -std=c++11 -I../cpp ./server_read.cpp ../cpp/NetworkUtils.cpp -o ./server_read -pthread
g++ -Wall -O2 -std=c++11 -I../cpp ./client_write.cpp ../cpp/NetworkUtils.cpp -o ./client_write -pthread

g++ -Wall -O2 -std=c++11 -I../cpp ./server_poll.cpp ../cpp/NetworkUtils.cpp -o ./server_poll -pthread
g++ -Wall -O2 -std=c++11 -I../cpp ./server_epoll.cpp ../cpp/NetworkUtils.cpp -o ./server_epoll
