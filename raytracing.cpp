#include <stdio.h>
#ifdef WIN32
#include <windows.h>
#endif
#include <GL/glut.h>
#include "raytracing.h"
#include <math.h> 
#include <thread>


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
	//MyMesh.loadMesh("../dodgeColorTest.obj", true);
	MyMesh.loadMesh("../cube_floor_reflect.obj", true);

	MyMesh.computeVertexNormals();

	//one first move: initialize the first light source
	//at least ONE light source has to be in the scene!!!
	//here, we set it to the current location of the camera
	//MyLightPositions.push_back(MyCameraPosition);
}


Vec3Df random_unit_vector() {
	double z = ((double)rand() / (double)RAND_MAX);
	double theta = z * (2.0 * 3.14159265358979323846);
	double x = z * (2.0) - 1.0;
	double s = sqrt(1.0 - x * x);

	return Vec3Df(x, s * cos(theta), s * sin(theta));
}



class hitResult
{
public:
	hitResult(Triangle tr, int index, Vec3Df p, float dist);
	hitResult();

	Triangle triangle;
	Vec3Df point;
	float distance;
	bool hit;
	int triangleIndex;
private:

};
hitResult::hitResult()
{
	hit = false;
}

hitResult::hitResult(Triangle tr, int index, Vec3Df p, float dist)
{
	triangle = tr;
	point = p;
	distance = dist;
	hit = true;
	triangleIndex = index;
}

hitResult rayIntersect(const Vec3Df & origin, const Vec3Df & dest, Triangle tr, int index){

	Vec3Df dir = dest - origin;
	dir.normalize();
	std::vector<Vertex> Vertices = MyMesh.vertices;

	Vec3Df vector0 = Vertices[tr.v[0]].p;
	Vec3Df vector1 = Vertices[tr.v[1]].p;
	Vec3Df vector2 = Vertices[tr.v[2]].p;

	Vec3Df v0v1 = vector1 - vector0;
	Vec3Df v0v2 = vector2 - vector0;

	Vec3Df N = Vec3Df::crossProduct(v0v1, v0v2);
	N.normalize();

	float NdotRayDir = Vec3Df::dotProduct(N, dir);
	if (NdotRayDir >= 0){
		return hitResult();
	}

	float d = Vec3Df::dotProduct(N, vector0);

	// compute distance t (equation 3)
	float t = -(Vec3Df::dotProduct(N, origin) - d) / NdotRayDir;
	// check if the triangle is in behind the ray
	if (t < 0) return hitResult(); // the triangle is behind

	// compute the intersection point using equation 1
	Vec3Df P = origin + t * dir;
	// Step 2: inside-outside test
	Vec3Df C; // vector perpendicular to triangle's plane 

	// edge 0
	Vec3Df edge0 = v0v1;
	Vec3Df vp0 = P - vector0;
	C = Vec3Df::crossProduct(edge0, vp0);
	if (Vec3Df::dotProduct(N, C) < 0) return hitResult(); // P is on the right side 

	// edge 1
	Vec3Df edge1 = vector2 - vector1;
	Vec3Df vp1 = P - vector1;
	C = Vec3Df::crossProduct(edge1, vp1);
	if (Vec3Df::dotProduct(N, C) < 0)  return hitResult(); // P is on the right side 

	// edge 2
	Vec3Df edge2 = vector0 - vector2;
	Vec3Df vp2 = P - vector2;
	C = Vec3Df::crossProduct(edge2, vp2);
	if (Vec3Df::dotProduct(N, C) < 0) return hitResult(); // P is on the right side; 

	return hitResult(tr, index, P, t); // this ray hits the triangle 
}

hitResult closestHit(const Vec3Df & origin, const Vec3Df & dest){

	std::vector<Triangle> Triangles = MyMesh.triangles;
	std::vector<Vertex> Vertices = MyMesh.vertices;

	//Find the direction of the ray
	Vec3Df dir = dest - origin;
	dir.normalize();


	int index = 0;
	float closest = -1;
	hitResult res;
	std::vector<Triangle>::const_iterator iterator;
	for (iterator = Triangles.begin(); iterator != Triangles.end(); ++iterator) {
		Triangle tr = *iterator;

		//Get the normal of the triangle
		Vec3Df vector0 = Vertices[tr.v[0]].p;
		Vec3Df vector1 = Vertices[tr.v[1]].p;
		Vec3Df vector2 = Vertices[tr.v[2]].p;

		Vec3Df v0v1 = vector1 - vector0;
		Vec3Df v0v2 = vector2 - vector0;

		Vec3Df N = Vec3Df::crossProduct(v0v1, v0v2);
		N.normalize();

		// find if the ray would intersect the plane
		hitResult hit = rayIntersect(origin, dest, tr, index);
		if ((closest == -1 || hit.distance > closest) & hit.hit){

			res = hit;

		}
		index++;
	}

	return res;
}


//Test if point1 can see point2
bool visible(Vec3Df point1, Vec3Df point2){

	std::vector<Triangle> Triangles = MyMesh.triangles;

	Vec3Df l = point2 - point1;
	float distance = l.getLength();
	std::vector<Triangle>::const_iterator iterator;

	for (iterator = Triangles.begin(); iterator != Triangles.end(); ++iterator) {
		Triangle tr = *iterator;

		hitResult result = rayIntersect(point1, point2, tr, 0);
		if (result.hit && result.distance < distance) {
			return false;
		}
	}
	return true;
}

// Soft light class
class SoftLight
{
public:
	SoftLight();
	SoftLight(Vec3Df pos, float radius);
	Vec3Df position;
	float visibility(Vec3Df point){

		index = 0;
		totalHits = 0;

		std::thread t[samples];
		while (index < samples){
			Vec3Df RP = position + radius * random_unit_vector();
			t[index] = std::thread(&SoftLight::addLightHit,this, point,RP);
			index++;
		}
		index = 0;
		while (index < samples){
			t[index].join();
			index++;
		}
		float visibility = (float)totalHits / (float)samples;
		return visibility;
	}

private:

	//Sample size
	static const int samples = 4;
	int totalHits;
	int index;
	float radius;
	void addLightHit(Vec3Df point, Vec3Df RP){
		if (visible(point, RP)){
			totalHits++;
		}
	}
};

SoftLight::SoftLight()
{
}

SoftLight::SoftLight(Vec3Df pos, float rad)
{
	position = pos;
	radius = rad;

}

std::vector<SoftLight> SoftLights;



//return the color of your pixel.
Vec3Df performRayTracing(const Vec3Df & origin, const Vec3Df & dest, int & depth)
{

	std::vector<Triangle> Triangles = MyMesh.triangles;
	std::vector<unsigned int> triangleMaterials = MyMesh.triangleMaterials;
	std::vector<Material> materials = MyMesh.materials;
	std::vector<Vertex> vertices = MyMesh.vertices;

	std::vector<Triangle>::const_iterator iterator;
	float closest = -1;

	hitResult hit = closestHit(origin, dest);
	Triangle triangle = hit.triangle;
	
	if (hit.hit){
		Material material = materials[triangleMaterials[hit.triangleIndex]];
		Vec3Df vector0 = vertices[triangle.v[0]].p;
		Vec3Df vector1 = vertices[triangle.v[1]].p;
		Vec3Df vector2 = vertices[triangle.v[2]].p;
		Vec3Df v0v1 = vector1 - vector0;
		Vec3Df v0v2 = vector2 - vector0;
		Vec3Df N = Vec3Df::crossProduct(v0v1, v0v2);

		Vec3Df kD = material.Kd();
		Vec3Df kA = material.Ka();
		Vec3Df kS = material.Ks();
		float shine = material.Ns();
		float Tr = material.Tr();
		int illum = material.illum();

		Vec3Df dir = dest - origin;
		dir.normalize();
		Vec3Df res;
		//Normal lights
		for (std::vector<Vec3Df>::const_iterator it = MyLightPositions.begin(); it != MyLightPositions.end(); it++){
			Vec3Df pos = *it;
			bool shadow = !visible(hit.point, pos);
			
			Vec3Df l = pos - hit.point;
			float lightDist = l.getLength();
			l.normalize();

			N.normalize();
			float dot = Vec3Df::dotProduct(l, N);

			if (dot <= 0.0) {
				res = res + kD*0.2;
			}
			
			if (shadow == false){

				if (dot > 0.0){
					res = res + dot*kD;
				}

				Vec3Df viewDir = MyCameraPosition - hit.point;
				Vec3Df reflectDir = -l - 2.0*Vec3Df::dotProduct(N, -l)*N;
				viewDir.normalize();
				//reflectDir.normalize();
				float dotSpec = Vec3Df::dotProduct(viewDir, reflectDir);
				//res = res + kS*pow(cos(dotSpec), shine);
				if (dotSpec > 0.0){
					//	res = res + kS*pow(dotSpec, shine);
				}
				else {
					//	res = res + kS*pow(0.0,32);
				}

			}
		}
		res = res + kA;

		// SoftLights
		for (std::vector<SoftLight>::const_iterator it = SoftLights.begin(); it != SoftLights.end(); it++){
			SoftLight light = *it;
			float shadow = light.visibility(hit.point);
			Vec3Df l = light.position - hit.point;
			float lightDist = l.getLength();
			l.normalize();

			N.normalize();
			float dot = Vec3Df::dotProduct(l, N);

			if (dot <= 0.0) {
				res = res + kD*0.2;
			}

			if (dot > 0.0){
				res = res + dot*kD*shadow;
			}


		}

		//Reflection
		if (illum == 3 & depth < 1 & shine > 0.0){
			N.normalize();
			Vec3Df r = dir - 2 * (Vec3Df::dotProduct(N, dir)*N);
			depth++;
			res += shine/1000 * performRayTracing(hit.point, r + hit.point, depth);
			//std::cout << " Reflection: " << shine * kS* performRayTracing(hit.point, r + hit.point, depth);
		}

		return res;
	}
	else{
		return Vec3Df(0, 0, 0);
	}
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
	glColor3f(1,1,1);
	glPointSize(10);
	glBegin(GL_POINTS);
	for (int i=0;i<MyLightPositions.size();++i)
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
	glColor3f(0,1,1);
	glVertex3f(testRayOrigin[0], testRayOrigin[1], testRayOrigin[2]);
	glColor3f(0,0,1);
	glVertex3f(testRayDestination[0], testRayDestination[1], testRayDestination[2]);
	glEnd();
	glPointSize(10);
	glBegin(GL_POINTS);
	//glVertex3fv(MyLightPositions[0].pointer());
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
void yourKeyboardFunc(char key, int x, int y, const Vec3Df & rayOrigin, const Vec3Df & rayDestination)
{

	//here, as an example, I use the ray to fill in the values for my upper global ray variable
	//I use these variables in the debugDraw function to draw the corresponding ray.
	//try it: Press a key, move the camera, see the ray that was launched as a line.

	std::vector<unsigned int> triangleMaterials = MyMesh.triangleMaterials;
	std::vector<Material> materials = MyMesh.materials;

	testRayOrigin = rayOrigin;
	hitResult hit = closestHit(rayOrigin, rayDestination);
	Vec3Df dir = rayDestination - rayOrigin;
	int depth = 0;
	dir.normalize();
	testRayDestination = rayOrigin + dir * hit.distance;
	performRayTracing(rayOrigin, rayDestination, depth);
	//std::cout << material.illum() << std::endl;
	//std::cout << material.Ns() << std::endl;
	//std::cout << hit.distance << std::endl;
	switch (key)
	{
	case 'o':
		if (SoftLights.size() != 0){
			SoftLights[SoftLights.size() - 1] = SoftLight(rayOrigin, 0.5);
		}
		else{
			SoftLights.push_back(SoftLight(rayOrigin, 0.5));
		}
		break;

	case 'O':
		SoftLights.push_back(SoftLight(rayOrigin, 0.5));
		break;
	}
}
