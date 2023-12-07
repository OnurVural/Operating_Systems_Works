all: histserver histclient histserver_th histclient_th

histserver: histserver.c
	gcc -Wall -o histserver histserver.c -lrt

histclient: histclient.c
	gcc -Wall -o histclient histclient.c -lrt

histserver_th: histserver_th.c
	gcc -Wall -o histserver_th histserver_th.c -lrt -lpthread
	
histclient_th: histclient_th.c
	gcc -Wall -o histclient_th histclient_th.c -lrt -lpthread

clean: 
	rm -fr *~ histserver histclient histserver_th histclient_th
