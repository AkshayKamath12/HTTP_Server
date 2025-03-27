# HTTP_Server

Use the provided makefile. 

Run server through ./httpserver -t (insert number of threads) portNumber

ex: ./httpserver -t 5 5432 will run the server using 5 worker threads on port 5432

Note: Leaving out the threads paramete will cause the server to default to 4 worker threads. 
