#include "system.h"
#include "sys/alt_stdio.h"

#define TRUE 1

/* Variables */
unsigned char ENABLE = 0;
unsigned char hmin = 0, hmax = 0;

/* Image Processing Functions */

void control(int a){
	if(a < 128)
		ENABLE = 1;
	else
		ENABLE = 0;
}

void grayscale(int row, int col, unsigned char *image, unsigned char *grayImage){
	int i = 0, j = 0, x = row*col*3;	
	
	while(i < x){
		grayImage[j] = (image[i]*5)/16 + (image[i+1]*9)/16 + image[i+2]/8;
		i = i+3;
		j++;
	}
}

void resize(int x1, int y1, unsigned char* grayImage, unsigned char* resizedImage){
	if((x1%2) == 0 && (y1%2) == 0){ //col = y1
		int i = 0, j = 0, f = 1, colSum = 2, size1 = x1*y1, col = y1; //i is index for grayImage, j is index for resizedImage
		unsigned char resizedPixel = 0;
		int newLine = 0, size2 = (x1/2)*(y1/2);
		while(j < size2){
			resizedPixel = (grayImage[i] + grayImage[i+1] + grayImage[i+col] + grayImage[i+1+col])/4;
			resizedImage[j] = resizedPixel;
			if(f%(col/2) == 0){
				i = col * colSum;
				colSum = colSum + 2;
				newLine = 1;
			}
			else{
				i = i + 2;
				newLine = 0;
			}
			f++;
			j++;
		}
	}
}

void sobel(int x2, int y2, unsigned char* image, unsigned char* edgeImage){
	int kx[9] = {-1,0,1,-2,0,2,-1,0,1};
	int ky[9] = {1,2,1,0,0,0,-1,-2,-1};
	int i = 0, j = 0, z = 1, f = 1, limit = (x2-2)*(y2-2), col = y2;
	unsigned char gx = 0, gy = 0, g = 0;
	int newLine = 1;
	while(j < limit){
		gx = image[i]*kx[0] + image[i+2]*kx[2]
		   + image[i+col]*kx[3] + image[i+col+2]*kx[5]
		   + image[i+(2*col)]*kx[6] + image[i+(2*col)+2]*kx[8];
		
		gy = image[i]*ky[0] + image[i+1]*ky[1]
		   + image[i+2]*ky[2] + image[i+(2*col)]*ky[6]
		   + image[i+(2*col)+1]*ky[7] + image[i+(2*col)+2]*ky[8];
		
		//g = sqrt(gx*gx + gy*gy); //change with square root algorithm
		g = gx + gy;
		edgeImage[j] = g;
		
		if(f/(col-2) == z && i > 0 && !newLine){
			i = col*z;
			newLine = 1;
			z++; //current line in a 2D type of view
		}
		else{
			newLine = 0;
			i++;
		}
		
		f++;
		j++;
	}
}

void toAsciiArt(int row, int col, unsigned char *image, unsigned char *asciiImage){
	unsigned char asciiLevels[16] = {' ','.',':','-','=','+','/','t','z','U','w','*','0','#','%','@'};
	
	int i = 0, size = row*col;
	
	while(i < size){
		asciiImage[i] = asciiLevels[((int)image[i])/16];
		i++;
	}
}

unsigned char brightness(int size, unsigned char *min, unsigned char *max, unsigned char* array){
	*min = 10000;
	*max = -10000;
	int i = 0;
	unsigned char avg = 0; //holds the sums for the average brightness
	while(i < size){
		if(array[i] < *min)
			*min = array[i];
		if(array[i] > *max)
			*max = array[i];
		avg = avg + array[i];
		i++;
	}
	avg = avg/(i+1); //actual brightness
	return avg;
}

void correction(int size, unsigned char *array){
	control(brightness(size, &hmin, &hmax, array));
	if(ENABLE){
		unsigned char sub = hmax - hmin;
		int i = 0, mul = 1;
		if(sub <= 127){
			if(sub > 63)
				mul = 2;
			else{
				if(sub > 31)
					mul = 4;
				else{
					if(sub > 15)
						mul = 8;
					else
						mul = 16;
				}
			}
			while(i < size){
				array[i] = (array[i] - hmin) * mul;
				i++;
			}
		}
	}
}

extern void delay (int millisec);


int main()
{
	alt_printf("Hello from cpu_1!\n");
	
	delay(50);
	
	unsigned char* address = (unsigned char*) SHARED_ONCHIP_BASE, *sem_1 = address;
	unsigned char grayImage[480], resizedImage[120], edgeImage[72];
	
	while (TRUE) {
		while(!(*sem_1)){} //wait for the semaphore to be released
		*sem_1 = 0; //take the semaphore as soon as it gets released
		unsigned char j = *(address+5), i = *(address+5000)+2;
		unsigned char* asciiStart = address+6000;
		
		//the RGB pixels start at address+7
		grayscale(i, j, address+7, grayImage);
		resize(i, j, grayImage, resizedImage);
		correction(i*j/4, resizedImage);
		sobel(i/2,j/2, resizedImage, edgeImage);
		toAsciiArt((i/2)-2, (j/2)-2, edgeImage, asciiStart);
		
		*sem_1 = 1; //release the semaphore right after execution
		delay(20);
	}
	return 0;
}
