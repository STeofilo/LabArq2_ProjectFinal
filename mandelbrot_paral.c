/* 
Modified from: http://rosettacode.org/wiki/Mandelbrot_set#PPM_non_interactive
c program:
--------------------------------
1. draws Mandelbrot set for Fc(z)=z*z +c
using Mandelbrot algorithm ( boolean escape time )
-------------------------------         
2. technique of creating ppm file is  based on the code of Claudio Rocchini
http://en.wikipedia.org/wiki/Image:Color_complex_plot.jpg
create 24 bit color graphic file ,  portable pixmap file = PPM 
see http://en.wikipedia.org/wiki/Portable_pixmap
to see the file use external application ( graphic viewer)

3. Instruções SIMD SSE :
http://softpixel.com/~cwright/programming/simd/sse.php e 
http://docs.oracle.com/cd/E18752_01/html/817-5477/eojde.html
*/
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#define TAM 4

int main(void)
{
	clock_t start, end;
	double cpu_time_used;

	/* screen ( integer) coordinate */
    int iX,iY;
    const int iXmax = 16384; 
    const int iYmax = 16384;

	/* world ( double) coordinate = parameter plane*/
	float Cy;
    float _Cx[TAM],_Cy[TAM];
    const float CxMin=-2.5;
    const float CxMax=1.5;
    const float CyMin=-2.0;
    const float CyMax=2.0;

	float PixelWidth=(CxMax-CxMin)/iXmax;
    float PixelHeight=(CyMax-CyMin)/iYmax;

	/* color component ( R or G or B) is coded from 0 to 255 */
    /* it is 24 bit color RGB file */
    const int MaxColorComponentValue=255; 
    FILE * fp;
    const char *filename="_simd_SSE.ppm";
    static unsigned char color[3];

	float Zx[TAM], Zy[TAM];	
    float Zx2[TAM], Zy2[TAM];

	int Iteration;
	const int IterationMax=256;
    /* bail-out value , radius of circle ;  */
    const float EscapeRadius=2;
    float ER2=EscapeRadius*EscapeRadius;

	//VARIAVEIS ADICIONAIS
	int Zxy2 = 0;							
	float zerar[TAM] = {0.0, 0.0, 0.0, 0.0};
	float addDois[TAM] = {2.0, 2.0, 2.0, 2.0};
	int nIteracao[TAM] = {1, 1, 1, 1};
	int flagIteracao[TAM];
	int *nIter = nIteracao;
	int *fIter = flagIteracao;
	int i;

    /*create new file,give it a name and open it in binary mode  */
    fp= fopen(filename, "wb"); /* b -  binary mode */
    /*write ASCII header to the file*/
    fprintf(fp,"P6\n %d\n %d\n %d\n",iXmax,iYmax,MaxColorComponentValue);
    /* compute and write image data bytes to the file*/

	start = clock();
    for(iY=0 ; iY<iYmax ; iY=iY++)
    {
		Cy=CyMin + (iY)*PixelHeight;
		for (i = 0; i < TAM; i++)
		{
			_Cy[i] = Cy;
			if (fabs(_Cy[i])< PixelHeight/2) 
				_Cy[i] = 0.0; 
		}
         
        for(iX=0 ; iX<iXmax ; iX=iX+TAM)
        {    
			for (i = 0; i < TAM; i++)
				_Cx[i]=CxMin + (iX+i)*PixelWidth;
			/* initial value of orbit = critical point Z= 0 */
			__asm{

				movups xmm1, [zerar]
									
				movups [Zx], xmm1		// Zx = 0			
				movups [Zy], xmm1		// Zy = 0
			
				movups [Zx2], xmm1		// Zx2 = Zx*Zx
				movups [Zy2], xmm1		// Zy2 = Zy*Zy
			}
			//-------------------------------------------------------------------------------------
			for(Iteration = 0, Zxy2 = 0; (Iteration < (IterationMax)) && (Zxy2 != 1); Iteration++){
				__asm{
					movss xmm7, [ER2]
					unpcklps xmm7, xmm7
					unpcklps xmm7, xmm7
					mov esi, nIter
					mov edi, fIter
					//Zy = 2*Zx*Zy + Cy;
					movups xmm0, [addDois]
					movups xmm1, [Zx]
					movups xmm2, [Zy]
					movups xmm3, [_Cy]
					mulps xmm0, xmm1
					mulps xmm0, xmm2
					addps xmm0, xmm3
					movups [Zy], xmm0

					//Zx = Zx2-Zy2 + Cx;
					movups xmm0, [Zx2]
					movups xmm1, [Zy2]
					movups xmm2, [_Cx]
					subps xmm0, xmm1
					addps xmm0, xmm2
					movups [Zx], xmm0

					//Zx2 = Zx*Zx;
					movups xmm0, [Zx]
					mulps xmm0, xmm0
					movups [Zx2], xmm0

					//Zy2 = Zy*Zy;
					movups xmm1, [Zy]
					mulps xmm1, xmm1
					movups [Zy2], xmm1

					//if((Zx2+Zy2) < ER2)
					addps xmm0, xmm1
					cmpltps xmm0, xmm7
					movmskps edx, xmm0
					cmp edx, 0
					je SET
					mov ecx, TAM
				L4:
					mov ebx, [edi]
					sar edx, 1
					jc NEXT1
					cmp ebx, 1
					je NEXT2
					mov [edi], 1
					jmp NEXT2 
				NEXT1:
					cmp ebx, 1
					je NEXT2
					inc [esi]
				NEXT2:
					add esi, 4
					add edi, 4
					loop L4
					jmp FIM
				SET:
					inc [Zxy2]
				FIM:
				//-------------------------------------------
				}
				
			}
			for (i = 0; i < TAM ; i++)
			{
				
				if (Iteration==(IterationMax))
				{ /*  interior of Mandelbrot set = black */
					color[0] = 0;
					color[1] = 0;
					color[2] = 0;         
				}
				else 
				{ 
					Iteration = nIteracao[i];
					/* exterior of Mandelbrot set = white */
					color[0] =((IterationMax-Iteration) % 8) *  63;  /* Red */
					color[1] =((IterationMax-Iteration) % 4) * 127;  /* Green */ 
					color[2] =((IterationMax-Iteration) % 2) * 255;  /* Blue */					
				}
				fwrite(color,1,3,fp);
			};
			//Zera as Iterações
			for(i = 0; i < TAM ; i++){
				nIteracao[i] = 1;
				flagIteracao[i] = 0;
			}
		}
	}
	end = clock();

	cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
	printf("Tempo = %f segundos\n", cpu_time_used);
    fclose(fp);
	system("PAUSE");
    return 0;
}
