//
//  This program check and prints missing timetag.
//  It is also a example for decoding the HRRT 64-bit listmode.
//  Author : Merence.Sibomana@cpspet.com - Dec2004

#include <stdio.h>
#include <sys/malloc.h>

static unsigned int ewtypes[16] = {3,3,1,0,3,3,2,2,3,3,3,3,3,3,3,3};
int nmpairs = 20;
static int mpairs[][2]={{-1,-1},{0,2},{0,3},{0,4},{0,5},{0,6},
                               {1,3},{1,4},{1,5},{1,6},{1,7},
                                     {2,4},{2,5},{2,6},{2,7},
                                           {3,5},{3,6},{3,7},
                                                 {4,6},{4,7},
                                                       {5,7}};


static int ev_count=0;
static unsigned *buf=NULL;
static int bufsize = 512*1024;  // acquisition buffers are 4MB size

// http://goo.gl/KzIrWK
#ifdef __APPLE__
#  define off64_t off_t
#  define fopen64 fopen
#endif

//static void print32_binary(int number) /* input in 32 bits */
//{
// int i=1;
// while (i<=4)
//   {
//     printf(



static void l64_timetag_check(FILE *fp)
{
	int count=0;  // acquisition buffers are 4MB size
	unsigned int ew1, ew2, type, tag;
	int current_time=0, total_singles=0, num_events=0;
	int prev_time = -1;
	unsigned *buf=NULL;
	int head,x,y;
	int block=0, bsingles=0;
	int ev_count=0;
	double pos_in_buffer;
	int i=0;
	int mp=0; // module pair [1,20]
	int doia,ahead,ax,ay,doib,bhead,bx,by;		// LOR identification
	int line_count = 0;
	int prompts=0;

	buf=(unsigned*)calloc(bufsize, 2*sizeof(unsigned));
	while ((count=fread(buf, sizeof(unsigned),2*bufsize, fp)) > 0)
	{
		i=0;
		while (i+1<count)
		{
			ew1 = buf[i];
			ew2 = buf[i+1];

			//			printf("\new1:%x ", ew1);
			//printf(" ew2:%x ", ew2);
			
			/* Note that ewtypes is defined above */
			type = ewtypes[(((ew2&0xc0000000)>>30)|((ew1&0xc0000000)>>28))];
			
			//printf("type:%d ",type);
			//fflush(stdout);
			
			if (type == 3)
			{ // sync
			  //if (current_time<10)
			  printf("\n Error: type[type_index:%x]=3", (((ew2&0xc0000000)>>30)|((ew1&0xc0000000)>>28)));
			  printf("\new1:%x ", ew1);
			  printf(" ew2:%x ", ew2);
			  fflush(stdout);

			  //printf("\new1:%x ", ew1);
			  //printf(" ew2:%x ", ew2);
			  //fflush(stdout);

				i += 1;
			}
			else if (type == 2)
			{	// tag word
			   tag = (ew1&0xffff)|((ew2&0xffff)<<16);
			   if ((tag & 0xE0000000) == 0xc0000000) 
			   {// Gantry Motions & positions
			   		head = (tag&0x000f0000) >> 16;
					y = (tag&0x0000ff00) >> 8;
					x = tag&0x000000ff;
					//fprintf(stderr,"%d: %d,%d,%d %d %d\n", current_time, head, y, x, num_events, total_singles);
					num_events = 0;
					total_singles = 0;
			   }
			   else if ((tag & 0xE0000000) == 0x80000000)
			   { // timetag
				   current_time = tag & 0x3fffffff; // msec
				   if (prev_time >= 0)
				   {
				     //				     if (current_time != prev_time+1){ 
				     if (abs(current_time-prev_time)>1){
				       printf("---BREAK---------\n"); line_count=0; 
				       printf("===== Missing data (in msec) : %d - %d = %d\n", current_time, prev_time, current_time-prev_time);
				       pos_in_buffer = (double)ev_count/bufsize;
				       fflush(stdout);
				       //exit(0);
				       //					   printf("===== EV number = %I64d, Position vs 4MB buffer = %g\n", ev_count, pos_in_buffer);
				     }
				   }
				   prev_time = current_time;

				   //printf("\n%x ", ew2);
				   //printf(" %x ", ew1);
				   //if (current_time % 1000 == 0) {
				   
				   if (current_time % 1000 == 0){
				   //if (current_time < 10 ){
				   printf("\ntime:%d ", current_time);
				   fflush(stdout);				   
				   }
				   //}
				   //if (line_count++ == 10) { printf("\n"); line_count=0; }
			   }
			   else if ((tag & 0xE0000000) == 0xA0000000)// block singles
			   {
					block = (tag & 0x1ff80000) >> 19;
					bsingles = tag & 0x0007ffff;
				    total_singles += bsingles;
			   }
			   i += 2;
			}
			else
			{ // event
				// type==0 for Prompt, type==1 for Delayed
			   mp = ((ew1&0x00070000)>>16) | ((ew2&0x00070000)>>13); // module pair [1,20]
			   //if (current_time<10){
			   //  printf("mp:%d", mp);
			   //}
			   
			   if (mp >=1 && mp<= nmpairs)
			   {
				   ahead = mpairs[mp][0];	//[0,7]
				   bhead = mpairs[mp][1];	//[0,7]
				   ax = (ew1&0xff);			//[0,71]
				   ay = ((ew1&0xff00)>>8);	//[0,103]
				   bx = (ew2&0xff);			//[0,71]
				   by = ((ew2&0xff00)>>8);	//[0,103]
				   doia = (ew1&0x01C00000)>>22; // 0 for Back layer, 1 for front layer
				   doib = (ew2&0x01C00000)>>22; // 0 for Back layer, 1 for front layer
				   // Transmission coincidences are prompts and the LOR is made with
				   // the crystal were the TX source is located (doi=7) and the crystal 
				   // where the single is detected (doi=0 or 1)

				   prompts++;
				   //printf("\n prompts:%d", prompts);
				   //fflush(stdout);

			   }


			   if (current_time%1000 == 0){
			     //if (ahead==2 && bhead==6 && abs(ay-by)<4 && ax==20 ){//&& current_time<50000) {
			     //if (ax==57 && bx==1 && current_time<10000000){//&& current_time<50000) {
			     //printf("time:%d ", current_time);
			     //fflush(stdout);

			     //    printf("\na/b--- head:%d/%d, x:%d/%d, y:%d/%d, doi:%d/%d, type:%d",ahead, bhead, ax, bx, ay, by, doia, doib,type);
			     //			     printf("\n\nahead:%d, ax:%d, ay:%d",ahead, ax, ay);
			     //printf("\nbhead:%d, bx:%d, by:%d",bhead, bx, by);
			     //fflush(stdout);
			     }
			   
			


				num_events++;
				i += 2;
			}
			ev_count++;
		}
	}


	printf("\n final time:%d, total prompts:%d", current_time, prompts);
	fflush(stdout);

	free(buf);
	printf("\n"); 
}

static void l32_timetag_check(FILE *fp)
{
	int i, count=0, line_count;  // acquisition buffers are 4MB size
	unsigned int tag;
	int current_time=0, total_singles=0, num_events=0;
	int prev_time = -1;
	int block=0, bsingles=0;
	buf=(unsigned*)calloc(bufsize, sizeof(unsigned));
	while ((count=fread(buf, sizeof(unsigned),bufsize, fp)) > 0)
	{
		for (i=0; i<count; i++)
		{
			tag = buf[i];
			if ((tag & 0xE0000000) == 0x80000000)
			{ // timetag
				current_time = tag & 0x3fffffff; // msec
				if (prev_time >= 0)
				{
					if (current_time != prev_time+1) 
					{
						printf("---BREAK---------\n"); line_count=0; 
						printf("===== Missing data (in msec) : %d - %d = %d\n",
							prev_time+1, current_time-1, current_time-prev_time-1);
						exit(0);
					}
				}
				prev_time = current_time;
				printf("%d ", current_time);
				if (line_count++ == 10) { printf("\n"); line_count=0; }
			}
			else if ((tag & 0xE0000000) == 0xA0000000)// block singles
			{
				block = (tag & 0x1ff80000) >> 19;
				bsingles = tag & 0x0007ffff;
				total_singles += bsingles;
			}
			else
			{ // event
				num_events++;
			}
			ev_count++;
		}
	}
	free(buf);
	printf("\n"); 
}

int main(int argc, char **argv)
{
	FILE *fp;
	long long int skip=10;
	if (argc < 2)  {
		printf("usage: %s file.l64 \n",argv[0]);
		return 1;
	}
	if ((fp = fopen64(argv[1],"rb")) == NULL)
	{
		perror(argv[1]);
		return 1;
	}
	
	//fseek(fp, (long long int)(0), SEEK_END); 
        //skip=ftell(fp);/* Gives size of file in bytes */
	//skip=100000000000;
        //printf("\n skip:%d", skip); fflush(stdout);
	//skip=fseek(fp, (skip-300000), SEEK_SET); 
  
	l64_timetag_check(fp);

	fclose(fp);
	return 0;
}
