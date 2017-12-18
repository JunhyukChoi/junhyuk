#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>												// �Լ� atoi, atof ����� ���� ����
#include <sys/time.h> 
#include <termios.h> 
#include <poll.h> 												// �Լ� poll ����� ���� ����


#define	INPUT 0
#define	OUTPUT 1

#define	HIGH 1
#define	LOW 0

typedef enum gpio_edge {
    GPIO_EDGE_NONE,     /* No interrupt edge */
    GPIO_EDGE_RISING,   /* Rising edge 0 -> 1 */
    GPIO_EDGE_FALLING,  /* Falling edge 1 -> 0 */
    GPIO_EDGE_BOTH      /* Both edges X -> !X */
} gpio_edge_t;


#define GPIO0	128												// AP_GPE0 : General Pin E0
#define GPIO1	129												// AP_GPE1
#define GPIO2	130												// AP_GPE2
#define GPIO3	46												// AP_GPB14
#define GPIO4	14												// AP_GPA14
#define GPIO5	41												// AP_GPB9_I2SDIN1
#define GPIO6	25												// AP_GPA25_BACKKEY
#define GPIO7	0												// AP_GPA0_MENUKEY
#define GPIO8	26												// AP_GPA26_VOLUP
#define GPIO9	27												// AP_GPA27_VOLDOWN
#define AGPIO0	161												// AP_AGP1_HOMEKEY

void pinfree(int gpio){

	int fd;
	char buf[128];
	
	fd = open("/sys/class/gpio/unexport",O_WRONLY);				// �ش� gpio�� unexport�ϴ� �κ�.
	sprintf(buf,"%d",gpio);
	write(fd,buf,strlen(buf));
	close(fd);
	
}

void pinsetup(int gpio,int mode ){								// gpio ������ ���� �Լ�.
																	// wiringPI�� pinMODE() �Լ��� ���� ����.

	int fd,fdv;
	char buf[128],buf2[128];

	pinfree(gpio);
	fd = open("/sys/class/gpio/export",O_WRONLY);					// �ش� gpio�� export�ϴ� �κ�.
	sprintf(buf,"%d",gpio);
	write(fd,buf,strlen(buf));
	close(fd);

	sprintf(buf,"/sys/class/gpio/gpio%d/direction",gpio);			// gpio�� direction�� ����.
	fd = open(buf,O_WRONLY);
	if(mode == INPUT){	
		fd = write(fd,"in",3);										// input�� ��� (fd,"in",3) �Է�.
	}else{
		fd = write(fd,"out",4);										// output�� ��� (fd,"out",4) �Է�.
	}
	close(fd);

}


void pinwrite(int gpio,int val){

	int fd;
	char buf[128];

	sprintf(buf,"/sys/class/gpio/gpio%d/value",gpio);
	fd = open(buf,O_WRONLY);


	if(val==LOW) write(fd,"0",2);
	else write(fd,"1",2);
	
	close(fd);
	
}

int pinpoll(int fd,int timeout_ms){

	struct pollfd fds[1];
	char buf[255];
	int ret;
	
	fds[0].fd=fd;
	fds[0].events = POLLPRI ;
	
	if (lseek(fd, 0, SEEK_END) < 0) 								// Seek to end
		return printf("Seeking to end of GPIO 'value'"); 

	if ((ret = poll(fds, 1, timeout_ms)) < 0)						// Poll
		return printf("polling error!\n");

	if (ret) { 														// GPIO edge interrupt occurred
		if (lseek( fd, 0, SEEK_SET) < 0)							// Rewind
			return printf("Rewinding GPIO 'value'"); 

 		return 1; 
 	} 

	return 0;														// Timed out	
	
}

void setedge(int gpio, gpio_edge_t edge){

	int fd;
	char buf[128];

	sprintf(buf,"/sys/class/gpio/gpio%d/edge", gpio);				
	fd = open(buf,O_WRONLY);	
	if(edge==0) write(fd,"none",sizeof("none")+1);
	else if(edge==1) write(fd,"rising",sizeof("rising")+1);
	else if(edge==2) write(fd,"falling",sizeof("falling")+1);
	else if(edge==3) write(fd,"both",sizeof("both")+1);
	close(fd);

}


int pinread(int fd){

	char ch;
	
	read(fd, &ch, 1);	
	lseek(fd, 0, SEEK_SET);
	if (ch == '0') {
		return 0;	
	}else{
		return 1;
	}
}



float gettime(struct timeval Tstart, struct timeval Tend){		// ����ð��� �����ϴ� �Լ� 

	Tend.tv_usec = Tend.tv_usec - Tstart.tv_usec;					// �ʴ����� ����ũ�� �ʴ����� ���� �Ǿ��ֱ� ������
	Tend.tv_sec  = Tend.tv_sec - Tstart.tv_sec;						// ������ ���缭 ���̸� ���
    Tend.tv_usec += (Tend.tv_sec*1000000);

	return Tend.tv_usec ;
}


void Waitusec(int usec){

	struct timeval start,end;
	int microsec;
	
	gettimeofday(&start,NULL);
	do{
		gettimeofday(&end,NULL);
		microsec = gettime(start,end);
		if(microsec >= usec) break;
	}while(microsec < usec);

}



int main(){

	char buf[128];

	struct timeval start,end;
	float microsec;
	int distance;
	int val,fd=0,fd2=0,chk,fdv;

	/* Setting
	 	export&direction, value, edge	*/
	pinsetup(128,OUTPUT);
	pinsetup(130,INPUT);

	sprintf(buf,"/sys/class/gpio/gpio%d/value", 130);
	fd = open(buf,O_RDONLY);

	sprintf(buf,"/sys/class/gpio/gpio%d/value", 128);
	fd2 = open(buf,O_WRONLY);	
	setedge(130, GPIO_EDGE_BOTH);
	/* �Լ����� ���ȭ ����.*/


	printf("Distance measure start\n");


	while(1){
		pinwrite(128,1);											// make trig
		Waitusec(100);
		pinwrite(128,0);
	while(1) {
		val = pinread(fd);
		if(val == 1){
			pinpoll(fd, 250);										// echo 1 -> 0 �϶� �ð� ���� �Ϸ�.
			gettimeofday(&end,NULL);
			microsec = gettime(start,end);							// �Ÿ� ���.
			distance = (microsec)/58;								// �ð����̸� �̿��Ͽ� �Ÿ� ���
			if(distance > 4000){									// 40m�̻��� ���������� ��� ������ ó��.
				printf("RANGE, ANGLE error! \n");
				distance = 9999;

				close(fd);											// echo���� 1���� �������� ���ϹǷ�
				pinfree(130);										// echo pin�� output pin���� �ٲٰ�
				pinsetup(130,OUTPUT);								// ����Ͽ� �ð��� �ΰ� 0���� �������� ����
				Waitusec(100);
				pinfree(130);
				pinsetup(130,INPUT);								// echo pin�� �ٽ� setting export&direction
				sprintf(buf,"/sys/class/gpio/gpio%d/value", 130);	// value
				fd= open(buf,O_RDONLY);				
				setedge(130, GPIO_EDGE_BOTH);						// edge
				
			}
			printf("%d : %d cm\n",val,distance);					// ������ �Ÿ� ���
			break;													// echo ������ �������Ƿ� �ٽ� trig�Ϸ� break;
		}
		else{
//			pinpoll(fd, 1500);
			if(pinpoll(fd,1500)==0) break;							// echo 0 -> 1 �϶� �ð� ���� ����.
			gettimeofday(&start,NULL);
		}

		    	
		}
		Waitusec(500*1000);
	}	

	printf("FINISHED\n");
	
    return 0;
}




