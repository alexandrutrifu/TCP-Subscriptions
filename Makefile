build: server subscriber

server: server.cpp
	g++ server.cpp -o server

subscriber: subscriber.cpp
	g++ subscriber.cpp -o subscriber

zip:
	zip -r tema2.zip subscriber.cpp server.cpp server_backend.h subscriber_backend.h subscription_protocol.h helpers.h readme.txt Makefile

clean:
	rm subscriber server