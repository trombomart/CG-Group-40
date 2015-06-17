#include <stdio.h>
#ifdef WIN32
#include <windows.h>
#endif
#include <GL/glut.h>
#include "raytracing.h"


//temporary variables
//these are only used to illustrate 
//a simple debug drawing. A ray 
Vec3Df testRayOrigin;
Vec3Df testRayDestination;


//use this function for any preprocessing of the mesh.
void init()
{
	//load the mesh file
	//please realize that not all OBJ files will successfully load.
	//Nonetheless, if they come from Blender, they should, if they 
	//are exported as WavefrontOBJ.
	//PLEASE ADAPT THE LINE BELOW TO THE FULL PATH OF THE dodgeColorTest.obj
	//model, e.g., "C:/temp/myData/GraphicsIsFun/dodgeColorTest.obj", 
	//otherwise the application will not load properly
	MyMesh.loadMesh("../dodgeColorTest.obj", true);
	MyMesh.computeVertexNormals();

	//one first move: initialize the first light source
	//at least ONE light source has to be in the scene!!!
	//here, we set it to the current location of the camera
	MyLightPositions.push_back(MyCameraPosition);
}
//#include "matrix.h"
void inverse2(const double *m, double *p)
{
	double determinant = +m[0] * (m[4] * m[8] - m[5] * m[7])
		- m[3] * (m[1] * m[8] - m[7] * m[2])
		+ m[6] *(m[1]*m[5] - m[4]*m[2]);
	if (determinant == 0) return;
	double invdet = 1 / determinant;
	p[0] = (m[4]*m[8] - m[5]*m[7])*invdet;
	p[1] = -(m[3]*m[8] - m[6]*m[5])*invdet;
	p[2] = (m[3]*m[7] - m[6]*m[4])*invdet;
	p[3] = -(m[1]*m[8] - m[7]*m[2])*invdet;
	p[4] = (m[0]*m[8] - m[6]*m[2])*invdet;
	p[5] = -(m[0]*m[7] - m[1]*m[6])*invdet;
	p[6] = (m[1]*m[5] - m[2]*m[4])*invdet;
	p[7] = -(m[1]*m[5] - m[2]*m[3])*invdet;
	p[8] = (m[0]*m[4] - m[1]*m[3])*invdet;
}
//return the color of your pixel.
Vec3Df performRayTracing(const Vec3Df & origin, const Vec3Df & dest)
{
	std::vector<Triangle> triangles = MyMesh.triangles;
	std::vector<Vertex> vertices = MyMesh.vertices;
	std::vector<unsigned int> triangleMaterials = MyMesh.triangleMaterials;
	std::vector<Material> materials = MyMesh.materials;
	int t = MAXINT;
	Triangle res;
	int triangleIndex;
	int index = 0;
	for (std::vector<Triangle>::const_iterator it = triangles.begin(); it != triangles.end(); it++){
		Triangle triangle = *it;
		int v0 = triangle.v[0];
		int v1 = triangle.v[1];
		int v2 = triangle.v[2];
		Vec3Df V0 = vertices[v0].p;
		Vec3Df V1 = vertices[v1].p;
		Vec3Df V2 = vertices[v2].p;
		Vec3Df right = Vec3Df(V0.p[0] - origin.p[0], V0.p[1] - origin.p[1], V0.p[2] - origin.p[2]);
		const GLdouble leftmatrix[] = { V0.p[0] - V1.p[0], V0.p[0] - V2.p[0], dest.p[0], V0.p[1] - V1.p[1], V0.p[1] - V2.p[1], dest.p[1], V0.p[2] - V1.p[2], V0.p[2] - V2.p[2], dest.p[2] };
		GLdouble subresult[9];
		inverse2(leftmatrix, subresult);
		if (subresult){
			Vec3Df result = Vec3Df(subresult[0] * right.p[0] + subresult[1] * right.p[1] + subresult[2] * right.p[2], subresult[3] * right.p[0] + subresult[4] * right.p[1] + subresult[5] * right.p[2], subresult[6] * right.p[0] + subresult[7] * right.p[1] + subresult[8] * right.p[2]);
			if (t > result.p[2] && (result.p[0] + result.p[1] <= 1)){
				t = result.p[2];
				res = *it;
				triangleIndex = index;
			}
		}
		index++;
	}
	if (!t){
		return Vec3Df(0, 0, 0);
	}
	index = 0;
	int materialIndex;
	for (std::vector<unsigned int>::const_iterator it = triangleMaterials.begin(); it != triangleMaterials.end(); it++){
		if (index == triangleIndex){
			materialIndex = *it;
		}
		index++;
	}
	index = 0;
	Material material;
	for (std::vector<Material>::const_iterator it = materials.begin(); it != materials.end(); it++){
		if (index == materialIndex){
			material = *it;
		}
		index++;
	}

	Vec3Df vector0 = vertices[res.v[0]].p;
	Vec3Df vector1 = vertices[res.v[1]].p;
	Vec3Df vector2 = vertices[res.v[2]].p;
	Vec3Df v0v1 = vector1 - vector0;
	Vec3Df v0v2 = vector2 - vector0;
	Vec3Df N = Vec3Df::crossProduct(v0v1, v0v2);
	
	Vec3Df kD = material.Kd();
	Vec3Df kA = material.Ka();
	Vec3Df kS = material.Ks();
	float shine = material.Ns();

	for (std::vector<Vec3Df>::const_iterator it = MyLightPositions.begin(); it != MyLightPositions.end(); it++){
		Vec3Df surfaceP = origin + t*dest;
		Vec3Df l = *it - surfaceP;
		l.normalize();
		//float dot = dotProduct(l, N);
	}

	return kD;
}



void yourDebugDraw()
{
	//draw open gl debug stuff
	//this function is called every frame

	//let's draw the mesh
	MyMesh.draw();

	//let's draw the lights in the scene as points
	glPushAttrib(GL_ALL_ATTRIB_BITS); //store all GL attributes
	glDisable(GL_LIGHTING);
	glColor3f(1, 1, 1);
	glPointSize(10);
	glBegin(GL_POINTS);
	for (int i = 0; i<MyLightPositions.size(); ++i)
		glVertex3fv(MyLightPositions[i].pointer());
	glEnd();
	glPopAttrib();//restore all GL attributes
	//The Attrib commands maintain the state. 
	//e.g., even though inside the two calls, we set
	//the color to white, it will be reset to the previous 
	//state after the pop.


	//as an example: we draw the test ray, which is set by the keyboard function
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glDisable(GL_LIGHTING);
	glBegin(GL_LINES);
	glColor3f(0, 1, 1);
	glVertex3f(testRayOrigin[0], testRayOrigin[1], testRayOrigin[2]);
	glColor3f(0, 0, 1);
	glVertex3f(testRayDestination[0], testRayDestination[1], testRayDestination[2]);
	glEnd();
	glPointSize(10);
	glBegin(GL_POINTS);
	glVertex3fv(MyLightPositions[0].pointer());
	glEnd();
	glPopAttrib();

	//draw whatever else you want...
	////glutSolidSphere(1,10,10);
	////allows you to draw a sphere at the origin.
	////using a glTranslate, it can be shifted to whereever you want
	////if you produce a sphere renderer, this 
	////triangulated sphere is nice for the preview
}


//yourKeyboardFunc is used to deal with keyboard input.
//t is the character that was pressed
//x,y is the mouse position in pixels
//rayOrigin, rayDestination is the ray that is going in the view direction UNDERNEATH your mouse position.
//
//A few keys are already reserved: 
//'L' adds a light positioned at the camera location to the MyLightPositions vector
//'l' modifies the last added light to the current 
//    camera position (by default, there is only one light, so move it with l)
//    ATTENTION These lights do NOT affect the real-time rendering. 
//    You should use them for the raytracing.
//'r' calls the function performRaytracing on EVERY pixel, using the correct associated ray. 
//    It then stores the result in an image "result.ppm".
//    Initially, this function is fast (performRaytracing simply returns 
//    the target of the ray - see the code above), but once you replaced 
//    this function and raytracing is in place, it might take a 
//    while to complete...
void yourKeyboardFunc(char t, int x, int y, const Vec3Df & rayOrigin, const Vec3Df & rayDestination)
{

	//here, as an example, I use the ray to fill in the values for my upper global ray variable
	//I use these variables in the debugDraw function to draw the corresponding ray.
	//try it: Press a key, move the camera, see the ray that was launched as a line.
	testRayOrigin = rayOrigin;
	testRayDestination = rayDestination;

	// do here, whatever you want with the keyboard input t.

	//...


	std::cout << t << " pressed! The mouse was in location " << x << "," << y << "!" << std::endl;
}
