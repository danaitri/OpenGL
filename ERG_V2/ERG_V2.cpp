// ERG_V2.cpp : Defines the entry point for the console application.
// Danai Triantafyllidou


#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include <glut.h>
#include <math.h>
#include <stdio.h>     // - Just for some ASCII messages
#include "glut.h"  
#include "windows.h"
#include <limits>
#include <iostream>
#include <vector>
#include <assert.h>


float SunStep = 0.7;
float SunAngle = 0;
float SceneAngle = 0;
float SceneStep = 0.19;

// συντελεστής τραχύτητας
GLfloat D = 2.5;
// μέγεθος πλέγματος
int MAP_SIZE = 129;

using namespace std;

// Create Gaussian Noise
double generateGaussianNoise(double mu, double sigma)
{
	const double epsilon = DBL_MIN;
	const double two_pi = 2.0*3.14159265358979323846;

	static double z0, z1;
	static bool generate = true;
	generate = !generate;

	if (!generate)
		return z1 * sigma + mu;

	double u1, u2;
	do
	{
		u1 = rand() * (1.0 / RAND_MAX);
		u2 = rand() * (1.0 / RAND_MAX);
	} while (u1 <= epsilon);

	z0 = sqrt(-2.0 * log(u1)) * cos(two_pi * u2);
	z1 = sqrt(-2.0 * log(u1)) * sin(two_pi * u2);
	return z0 * sigma + mu;
}

// Convert Vector to unit vector

void normalizeVec(float* vec)
{
	double s = vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2];
	s = sqrt(s);
	if (s > 0)
	{
		vec[0] /= s;
		vec[1] /= s;
		vec[2] /= s;
	}
}


// From 3 points p1,p2,p3 that define a 3D plane compute Α = p2-p1 and B = p3-1 and then
// C = AxB that defines the perpendicular vector to this plane

void computeNormal(float* p1, float* p2, float* p3, float* result)
{
	double A[3];
	double B[3];
	for (int i = 0; i < 3; i++)
	{
		A[i] = +p1[i] - p2[i];
		B[i] = +p1[i] - p3[i];
	}

	//vector.x = (Ay*Bz) - (By*Az);
	result[0] = A[1] * B[2] - B[1] * A[2];
	//vector.y = -(Ax*Bz) + (Bx*Az);
	result[1] = -A[0] * B[2] + B[0] * A[2];
	//vector.z = (Ax*By) - (Ay*Bx);
	result[2] = A[0] * B[1] - A[1] * B[0];
	//return vector;

	normalizeVec(result);
}


class HeightMap
{

public:
	int _size;			// grid size
	GLfloat* _data;		// contains data for every x,y, of grid
	GLfloat _D;			// coefficient 
	GLfloat _scale;		// scale height
	GLfloat _alpha;		// transform x,y of grid to
	GLfloat _beta;		// x,y world coords

	GLfloat _txtR;

	HeightMap(int Size, GLfloat D, GLfloat scale, GLfloat minXY, GLfloat maxXY, GLfloat txtR) : _size(Size), _D(D), _scale(scale)
	{

		// solve a*x+b=minXY or a*x+b=maxXY for the grid's edges
		_beta = minXY;
		_alpha = (maxXY - minXY) / GLfloat(_size - 1);


		_txtR = txtR / GLfloat(_size);

		//  create square matrix
		_data = new GLfloat[_size*_size];
		int cnt = _size*_size;
		for (int i = 0; i < cnt; i++)
			_data[i] = 0.0;
	}

	~HeightMap()
	{
		delete[] _data;
	}

	GLfloat GetItem(int row, int col)
	{
		// check limits
		assert((row >= 0) && (col >= 0) && (row < _size) && (col < _size));
		return _data[row*_size + col];
	}

	void SetItem(int row, int col, GLfloat val)
	{
		assert((row >= 0) && (col >= 0) && (row < _size) && (col < _size));
		_data[row*_size + col] = val;
	}

	void MakeMountain(GLfloat height, GLfloat slope, int center_r, int center_c)
	{
		for (int r = 0; r < _size; r++)
		{
			for (int c = 0; c < _size; c++)
			{
				int dr = center_r - r;
				int dc = center_c - c;
				GLfloat dist = sqrtf(dr*dr + dc*dc);
				GLfloat h = height - dist*slope;
				SetItem(r, c, h);
			}
		}
	}

	// create fractal function
	void FillMap()
	{
		// in every iteration step=step/2
		int step = _size / 2;
		double H = 3 - _D;


		// while step > 0 divide the grid
		while (step > 0)
		{
			double segmentation = double(step) / (_size);
			double pow_seg = pow(segmentation, 2 * H);

			bool odd_r = false;  // ariable that checks if we are in an odd or even position in the grid (row)
			for (int r = 0; r < _size; r += step, odd_r = !odd_r)
			{
				bool odd_c = false;  // variable that checks if we are in an odd or even position in the grid (column)
				for (int c = 0; c < _size; c += step, odd_c = !odd_c)
				{
					if (odd_r || odd_c) // only when one of the two is οdd
					{

						GLfloat temp;
						// when both are odd we are in the center of the triangle
						if (odd_r && odd_c)
						{
							// compute average for center (r,c)
							temp = 0.25 *  (GetItem(r - step, c - step) + GetItem(r - step, c + step) + GetItem(r + step, c + step) + GetItem(r + step, c - step));
						}
						else
						{
							// horizontal average
							if (odd_c)
							{
								temp = 0.5 * (GetItem(r, c - step) + GetItem(r, c + step));
							}
							// vertical average
							if (odd_r)
							{
								temp = 0.5 * (GetItem(r - step, c) + GetItem(r + step, c));
							}
						}
						// add gaussian noise
						temp += _scale * generateGaussianNoise(0, pow_seg);
						//temp += GetItem(r, c);
						SetItem(r, c, temp);
					}
				}
			}
			step /= 2;
		}
	}

	void Draw()
	{


		// Variables r and c go through the grid and every time 4 points are selected. Once r,c are translated to world coordinates we draw 2 triangles from 
		// these points

		for (int r = 0; r < _size - 1; r++)
		{
			for (int c = 0; c < _size - 1; c++)
			{


				// Every time we take 4 points from data matrix and transform them into world coordinates

				GLfloat x1 = _alpha *  c + _beta;
				GLfloat x2 = _alpha * (c + 1) + _beta;
				GLfloat y1 = _alpha *  r + _beta;
				GLfloat y2 = _alpha * (r + 1) + _beta;

				// Set heights for these points

				GLfloat za = GetItem(r, c);
				GLfloat zb = GetItem(r, c + 1);
				GLfloat zc = GetItem(r + 1, c + 1);
				GLfloat zd = GetItem(r + 1, c);

				// Define these points

				GLfloat _a[3] = { x1, za, y1 };
				GLfloat _b[3] = { x2, zb, y1 };
				GLfloat _c[3] = { x2, zc, y2 };
				GLfloat _d[3] = { x1, zd, y2 };

				// _txtR μας controls how many times textures is repeated
				// Texture coords

				GLfloat tx1 = _txtR * c;
				GLfloat tx2 = _txtR * (c + 1);
				GLfloat ty1 = _txtR * r;
				GLfloat ty2 = _txtR * (r + 1);

				// Computer vectors perpendicular to triangles

				GLfloat n1[3];
				computeNormal(_c, _b, _a, n1);

				GLfloat n2[3];
				computeNormal(_a, _d, _c, n2);

				glNormal3fv(n1);



				// Draw Rectangles

				glBegin(GL_TRIANGLES);

				glTexCoord2f(tx1, ty1);
				glVertex3fv(_a);

				glTexCoord2f(tx2, ty1);
				glVertex3fv(_b);

				glTexCoord2f(tx2, ty2);
				glVertex3fv(_c);

				glEnd();

				glNormal3fv(n2);
				glBegin(GL_TRIANGLES);

				glTexCoord2f(tx2, ty2);
				glVertex3fv(_c);

				glTexCoord2f(tx1, ty2);
				glVertex3fv(_d);

				glTexCoord2f(tx1, ty1);
				glVertex3fv(_a);

				glEnd();

			}
		}
	}
};

// global μεταβλητή pointer to class HeightMap

HeightMap* global_map;

// Use image as texture

void LoadTexture(const char * filename, GLuint textureID)
{
	BITMAPFILEHEADER fh;
	BITMAPINFOHEADER bh;

	unsigned int dataPos;     // Position in the file where the actual data begins
	unsigned int width, height;
	unsigned int imageSize;   // = width*height*3
							  // Actual RGB data
	unsigned char * data;
	WORD   bits;

	FILE * file;
	fopen_s(&file, filename, "rb");
	if (!file)
	{
		printf("Image could not be opened\n");
		return;
	}

	int fh_size = sizeof(BITMAPFILEHEADER);
	if (fread(&fh, 1, fh_size, file) != fh_size)
	{
		printf("Not a correct BMP file\n");
		return;
	}

	int bh_size = sizeof(BITMAPINFOHEADER);
	if (fread(&bh, 1, bh_size, file) != bh_size)
	{
		printf("Not a correct BMP file\n");
		return;
	}
	// check if type = 'BM'
	if (fh.bfType != 0x4D42)
	{
		printf("Not a correct BMP file\n");
		return;
	}

	dataPos = fh.bfOffBits;
	imageSize = bh.biSizeImage;
	width = bh.biWidth;
	height = bh.biHeight;
	bits = bh.biBitCount;

	GLenum pixelFormat;

	switch (bits)
	{
	case 24:
		pixelFormat = GL_BGR_EXT;
		if (imageSize == 0)    imageSize = width*height * 3;
		break;
	case 32:
		pixelFormat = GL_RGBA;
		if (imageSize == 0)    imageSize = width*height * 3;
		break;
	default:
		printf("File format nor supported\n");
		return;
		break;
	}
	fseek(file, dataPos, SEEK_SET);

	// Create a buffer
	data = new unsigned char[imageSize];

	// Read the actual data from the file into the buffer
	fread(data, 1, imageSize, file);

	//Everything is in memory now, the file can be closed
	fclose(file);

	glBindTexture(GL_TEXTURE_2D, textureID);
	gluBuild2DMipmaps(GL_TEXTURE_2D,     // texture to specify
		GL_RGB,           // internal texture storage format
		width,             // texture width
		height,            // texture height
		pixelFormat,       // pixel format
		GL_UNSIGNED_BYTE,	// color component format
		data);    // pointer to texture image
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	delete[] data;
}

// Change rotation angle every 0.25 msec

void Animate(int value)
{
	SunAngle += SunStep;
	if (SunAngle >= 180)
		SunAngle = 0;

	SceneAngle += SceneStep;
	if (SceneAngle > 360)
		SceneAngle -= 360;

	glutPostRedisplay();
	glutTimerFunc(25, Animate, 0);
}


void reshape(int width, int height)
{
	GLdouble aspect = double(width) / double(height);

	/* define the viewport transformation */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(65, aspect, 1, 200);

	/* define the viewing transformation */
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glViewport(0, 0, width, height);
}

void drawSphere();

void drawscene(void)
{
	glClearColor(0.10, 0.27, 0.40, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	/* viewing transformation  */
	gluLookAt(0, 30, 100,
		0.0, 0.0, 0.0,
		0.0, 1.0, 0.0);

	glRotatef(SceneAngle, 0.0, 1.0, 0.0);

	glPushMatrix();

	// Place sun in its initial position

	glRotatef(-SunAngle, 0.0, 0.0, 1.0);
	glTranslatef(-50, 0, 0);

	GLfloat spec_1[] = { 1.0, 1.0, 0.0, 1.0 };
	GLfloat diff_1[] = { 1.0, 1.0, 0.0, 1.0 };
	GLfloat ambi_1[] = { 0.0, 0.0, 0.0, 1.0 };
	GLfloat emmi_1[] = { 1.0, 1.0, 0.0, 1.0 };
	int shininess_1 = 1;

	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec_1);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, diff_1);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, ambi_1);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emmi_1);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess_1);

	drawSphere();

	GLfloat lightpos[] = { 0, 0, 0.0 };
	glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

	glPopMatrix();


	// Light out

	GLfloat emmi_2[] = { 0.0, 0.0, 0.0, 1.0 };
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emmi_2);


	GLfloat spec_2[] = { 0.6, 0.6, 0.6, 0.6 };
	GLfloat diff_2[] = { 0.8, 0.8, 0.8, 1.0 };
	GLfloat ambi_2[] = { 0.7, 0.7, 0.7, 1.0 };


	int shininess_2 = 30;

	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec_2);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, diff_2);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, ambi_2);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess_2);

	glEnable(GL_TEXTURE_2D);



	glBindTexture(GL_TEXTURE_2D, 1);
	global_map->Draw();

	glDisable(GL_TEXTURE_2D);

	glutSwapBuffers();
}

void divideAndDraw(float* v1, float* v2, float*v3, int depth);

void drawSphere()
{
	float tetrahedron[4][3] = {
		{ 0.0f, 0.0f, 1.0f },
		{ 0.0f, 0.942809f, -0.33333f },
		{ -0.816497f, -0.471405f, -0.333333f },
		{ 0.816497f, -0.471405f, -0.333333f } };

	int depth = 3;

	divideAndDraw(tetrahedron[0], tetrahedron[1], tetrahedron[2], depth);
	divideAndDraw(tetrahedron[0], tetrahedron[1], tetrahedron[3], depth);
	divideAndDraw(tetrahedron[0], tetrahedron[2], tetrahedron[3], depth);
	divideAndDraw(tetrahedron[1], tetrahedron[2], tetrahedron[3], depth);
}

void normalize(float* v)
{
	float a = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
	a = sqrt(a);
	v[0] /= a;
	v[1] /= a;
	v[2] /= a;
}

void divideAndDraw(float* v1, float* v2, float*v3, int depth)
{
	if (depth == 0)
	{
		glBegin(GL_TRIANGLES);
		//glNormal3fv(v1); 
		glVertex3fv(v1);
		//glNormal3fv(v2);
		glVertex3fv(v2);
		//glNormal3fv(v3); 
		glVertex3fv(v3);
		glEnd();
	}
	else
	{
		float v12[3], v23[3], v13[3];
		for (int i = 0; i < 3; i++)
		{
			v12[i] = (v1[i] + v2[i]) / 2.0;
			v23[i] = (v2[i] + v3[i]) / 2.0;
			v13[i] = (v3[i] + v1[i]) / 2.0;
		}
		normalize(v12);
		normalize(v23);
		normalize(v13);
		int new_depth = depth - 1;
		divideAndDraw(v1, v12, v13, new_depth);
		divideAndDraw(v12, v13, v23, new_depth);
		divideAndDraw(v12, v2, v23, new_depth);
		divideAndDraw(v13, v23, v3, new_depth);
	}
}



void InitTerrain()
{
	srand(25);

	// 65 // 129 // 257 

	GLfloat scale = 20;



	// Create object of class heightma with grid size MAP_SIZE which is going to be translated to world coordinates where minXY=-80 and maxXY=80

	global_map = new HeightMap(MAP_SIZE, D, scale, -80, 80, 5);

	global_map->FillMap();
}

void SetLevel(int d)
{
	switch (d)
	{
	case 0:
		D = 2.0;
		break;
	case 1:
		D = 2.1;
		break;
	case 2:
		D = 2.2;
		break;
	case 3:
		D = 2.3;
		break;
	case 4:
		D = 2.0;
		break;
	case 5:
		D = 2.5;
		break;
	case 6:
		D = 2.6;
		break;
	case 7:
		D = 2.7;
		break;
	case 8:
		D = 2.8;
		break;
	case 9:
		D = 2.9;
		break;
	case 10:
		D = 3;
		break;
	default:
		D = 2.5;
	}
	InitTerrain();
}

void SetGridSize(int option)
{
	switch (option)
	{
	case 0:
		MAP_SIZE = 9;
		break;
	case 1:
		MAP_SIZE = 17;
		break;
	case 2:
		MAP_SIZE = 33;
		break;
	case 3:
		MAP_SIZE = 65;
		break;
	case 4:
		MAP_SIZE = 129;
		break;
	case 5:
		MAP_SIZE = 257;
		break;
	default:
		MAP_SIZE = 257;
		break;
	}
	InitTerrain();
}


void handlemenu(int a)
{

}

// Create Menu

void MenuInit(void)
{
	int submenu3, submenu2, submenu1;

	submenu1 = glutCreateMenu(SetLevel);
	glutAddMenuEntry("2.0", 0);  glutAddMenuEntry("2.1", 1);
	glutAddMenuEntry("2.2", 2);  glutAddMenuEntry("2.3", 3);
	glutAddMenuEntry("2.4", 4);  glutAddMenuEntry("2.5", 5);
	glutAddMenuEntry("2.6", 6);  glutAddMenuEntry("2.7", 7);
	glutAddMenuEntry("2.8", 8);  glutAddMenuEntry("2.9", 9);
	glutAddMenuEntry("3.0", 10);

	submenu2 = glutCreateMenu(SetGridSize);
	glutAddMenuEntry("9", 0); glutAddMenuEntry("17", 1);
	glutAddMenuEntry("33", 2); glutAddMenuEntry("65", 3);
	glutAddMenuEntry("129", 4); glutAddMenuEntry("257", 5);

	glutCreateMenu(handlemenu);
	glutAddSubMenu("Level", submenu1);
	//glutAddSubMenu("Fractal", submenu2);
	//glutAddSubMenu("Movement", submenu3);
	glutAddSubMenu("Grid Size", submenu2);
	//glutAddMenuEntry("Toggle Axes", MENU_AXES);
	glutAddMenuEntry("Quit", 2);
	glutAttachMenu(GLUT_RIGHT_BUTTON);
}

int main(int argc, char** argv)
{
	InitTerrain();

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(1024, 800);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("Mountains");
	glutDisplayFunc(drawscene);


	glutReshapeFunc(reshape);


	glutTimerFunc(25.0, Animate, 0);


	glClearDepth(10.0);
	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_TEST);

	glEnable(GL_NORMALIZE);

	GLfloat lightpos[] = { 0.0, 10, 10.0, 0 };
	GLfloat specolor[] = { 0.8, 0.8, 0.8, 1.0 };
	GLfloat difcolor[] = { 0.8, 0.8, 0.8, 1.0 };
	GLfloat ambcolor[] = { 0.1, 0.1, 0.1, 0.0 };

	glEnable(GL_LIGHTING);
	//glLightModelf(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
	glShadeModel(GL_SMOOTH);

	glEnable(GL_LIGHT0);


	LoadTexture("mount6.bmp", 1);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);


	MenuInit();
	glutMainLoop();
	return 0;
}