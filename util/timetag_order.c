//
//  This program check and prints missing timetag.
//  It is also a example for decoding the HRRT 64-bit listmode.
//  Author : Merence.Sibomana@cpspet.com - Dec2004

#include <stdio.h>
#include <sys/malloc.h>

static int ev_count=0;
static unsigned *buf=NULL;
static int bufsize = 512*1024;  // acquisition buffers are 4MB size
int allow_write=1;

// http://goo.gl/KzIrWK
#ifdef __APPLE__
#  define off64_t off_t
#  define fopen64 fopen
#endif

static void l64_timetag_order(FILE *fp, FILE *outputfp)
{
	int count=0;  // acquisition buffers are 4MB size
	unsigned int ew1, ew2, outputew1, outputew2, type, tag;
	int current_time=0, total_singles=0, num_events=0, ordered_time=0, diff_time=1;
	int prev_time = -1;
	unsigned *buf=NULL;
	int head,x,y;
	int block=0, bsingles=0;
	int ev_count=0;
	double pos_in_buffer;
	int mp=0; // module pair [1,20]
	int doia,ahead,ax,ay,doib,bhead,bx,by;		// LOR identification
	int line_count = 0;
	int prompts=0;

	buf=(unsigned*)calloc(bufsize, 2*sizeof(unsigned));
	while ((count=fread(buf, sizeof(unsigned),2*bufsize, fp)) > 0) {
	  int i=0; 
	  int number_of_errors=0;
		while (i + 1 < count) {
			ew1 = buf[i];
			ew2 = buf[i+1];
			outputew1 = buf[i];
			outputew2 = buf[i+1];

			type = GeometryInfo::EWTYPES[(((ew2&0xc0000000)>>30)|((ew1&0xc0000000)>>28))];
			if (type == 3)
			{ // sync
			  //if (current_time<10)
			  number_of_errors ++;
			  if ((number_of_errors % 1)==0){
			    printf("(Error #:%d- type:%x i:%d)", number_of_errors, (((ew2&0xc0000000)>>30)|((ew1&0xc0000000)>>28)),i);
			    fflush(stdout);
			  }			  

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
				   outputew1 &= 0xffff0000; // Sets front 16 bits to zero
				   outputew1 |= (ordered_time&0xffff);
				   outputew2 &= 0xffff8000; // Sets front 15 bits to zero
				   outputew2 |= ((ordered_time&0x3fff0000)>>16);


				   if (current_time % 1000 == 0){//ordered_time % 1000 == 0){// && current_time % 1000 == 0){
				     printf("\ntime prev_time:%d, current_time:%d, ordered_time:%d ", prev_time, current_time, ordered_time);
				     fflush(stdout);
				     //  exit(0); 
				     }

				   if (prev_time > 0)
				     {

				       // high activity approach				       
				       //diff_time=current_time-prev_time;
				       //ordered_time += diff_time;

				       // flipping timetags approach
				       ordered_time+=1;


				       /*				   
					   if (current_time != prev_time+1) 
					   {
						   printf("---BREAK---------\n"); line_count=0; 
						   printf("===== Missing data (in msec) : %d - %d = %d\n", current_time, prev_time, current_time-prev_time);
										   
						   pos_in_buffer = (double)ev_count/bufsize;
						   fflush(stdout);
						   printf("===== EV number = %I64d, Position vs 4MB buffer = %g\n", ev_count, pos_in_buffer);
					   }
				       */

				     }
				   prev_time = current_time;

				   //printf("\n%x ", ew2);
				   //printf(" %x ", ew1);
				   //if (current_time % 1000 == 0) {
				   
				   //if (current_time % 1000 == 0){
				   //if (current_time < 10 ){
				  
				   //}
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
			   
			   if (mp >=1 && mp<= GeometryInfo::NMPAIRS)
			   {
				   ahead = GeometryInfo::MPAIRS[mp][0];	//[0,7]
				   bhead = GeometryInfo::MPAIRS[mp][1];	//[0,7]
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
				   if (prompts % 10000000 == 0){
				     printf("\n prompts:%d", prompts);
				     fflush(stdout);
				   }
			   }


			   if (current_time<200000){
			     //if (ahead==2 && bhead==6 && abs(ay-by)<4 && ax==20 ){//&& current_time<50000) {
			     //if (ax==57 && bx==1 && current_time<10000000){//&& current_time<50000) {
			     //printf("time:%d ", current_time);
			     //fflush(stdout);
			     
			     //printf("\na/b--- head:%d/%d, x:%d/%d, y:%d/%d, doi:%d/%d, type:%d",ahead, bhead, ax, bx, ay, by, doia, doib,type);
			     //			     printf("\n\nahead:%d, ax:%d, ay:%d",ahead, ax, ay);
			     //printf("\nbhead:%d, bx:%d, by:%d",bhead, bx, by);
			     //fflush(stdout);
			   }
			   
			   
			   
			   
			   num_events++;
			   i += 2;
			   
			   
			}
			ev_count++;
			
			
			//printf("\n type=%d, allow_write=%d", type, allow_write); fflush(stdout);		
			if (type!=3) {
 			  if (allow_write) {
			    fwrite(&outputew1, sizeof(unsigned), 1, outputfp);
			    fwrite(&outputew2, sizeof(unsigned), 1, outputfp);
			  }
			}
		}

	}
	

	printf("\n final time:%d, total prompts:%d, number_of_errors:%d", current_time, prompts, number_of_errors);
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
	FILE *outputfp;

	if (argc < 3)  {
		printf("usage: %s in_file.l64 out_file.l64 \n",argv[0]);
		return 1;
	}
	if ((fp = fopen64(argv[1],"rb")) == NULL)
	{
		perror(argv[1]);
		return 1;
	}
	if ((outputfp = fopen64(argv[2],"wb")) == NULL)
	{
		perror(argv[2]);
		return 1;
	}

	l64_timetag_order(fp, outputfp);

	fclose(fp);
	fclose(outputfp);
	return 0;
}
