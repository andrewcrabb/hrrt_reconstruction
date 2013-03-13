#define PP_SIZE_X 256
#define PP_SIZE_Y 256
#define PP_SIZE 65536		// PP_SIZE_X * PP_SIZE_Y

void SetMasterFlag(long MasterFound);
void Standard_CRM_Pattern(long NumBlocks, long XCrystals, long YCrystals, long *Position);
void Fill_CRM_Verticies(long Edge, long XCrystals, long YCrystals, long *Vert);
void Smooth_2d(long CorrectDNL, long range, long passes, long in[PP_SIZE], double out[PP_SIZE]);
long Find_Region(long *line_x, long *line_y, long *Region);
long region_high(double data[PP_SIZE], long NumPixels, long *Region);
double region_sum(double data[PP_SIZE], long NumPixels, long *Region);
double Shifted_Peaks(double *pp, long *Verts, long MiddleX, long MiddleY, long x, long y, long XCrystals, long YCrystals, long *NewVerts);
void Peak_Shift(double *pp, long vx, long vy, long Shift_X, long Shift_Y, long XCrystals, long YCrystals, long *Verts);
long Next_Neighbor(long *Flag, long XCrystals, long YCrystals, long *Verts, long *NewVerts, long *shift_x, long *shift_y);
long Number_Neighbors(long *Flag, long x, long y, long XCrystals, long YCrystals);
void Build_Line(long vx, long vy, long XCrystals, long YCrystals, long *Verts, long *line_x, long *line_y);
void Build_CRM(long *Verts, long XCrystals, long YCrystals, unsigned char *CRM);
long Side(long x, long y, long x1, long y1, long x2, long y2);
void hist_equal(long DirectScale, long NumElements, long *In, long *Out);
