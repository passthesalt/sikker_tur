all : b1 b2

b1 : bounded_buffer_1.c
		gcc -o b1 bounded_buffer_1.c -pthread

b2 : bounded_buffer_2.c
		gcc -o b2 bounded_buffer_2.c -pthread

clean :
		rm -f b1 b2
