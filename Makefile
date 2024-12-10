all: jobServer jobCom queue

queue: src/queue.c
	gcc -Wall -c src/queue.c

jobCom: src/jobCommander.c 
	gcc -Wall -o jobCommander src/jobCommander.c src/queue.c

jobServer: src/jobExecutorServer.c
	gcc -Wall -o jobExecutorServer src/jobExecutorServer.c src/queue.c

clean:
	rm -f *.txt *.txtls 