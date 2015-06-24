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
	MyMesh.loadMesh("../cube_refract.obj", true);
	//MyMesh.loadMesh("../sphere_floor.obj", true);

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
	hitResult(Triangle & tr, int & index, Vec3Df & p, float & dist, Vec3Df & normal, float & u, float & v);
	hitResult();
	Triangle triangle;
	Vec3Df point;
	Vec3Df normal;
	float distance;
	float u;
	float v;
	bool hit;
	int triangleIndex;
private:

};
hitResult::hitResult()
{
	hit = false;
}

hitResult::hitResult(Triangle & tr, int & index, Vec3Df & p, float &  dist, Vec3Df & norm,float & U,float & V)
{
	triangle = tr;
	point = p;
	distance = dist;
	hit = true;
	triangleIndex = index;
	normal = norm;
	u = U;
	v = V;
}

hitResult rayIntersect(const Vec3Df & origin, const Vec3Df & dest, Triangle & tr, int index){

	Vec3Df dir = dest - origin;
	dir.normalize();

	Vec3Df& vector0 = vertices[tr.v[0]].p;
	Vec3Df& vector1 = vertices[tr.v[1]].p;
	Vec3Df& vector2 = vertices[tr.v[2]].p;

	Vec3Df v0v1 = vector1 - vector0;
	Vec3Df v0v2 = vector2 - vector0;

	Vec3Df N = Vec3Df::crossProduct(v0v1, v0v2);
	float denom = Vec3Df::dotProduct(N,N);

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
	float u;
	float v;
	// edge 0
	Vec3Df edge0 = v0v1;
	Vec3Df vp0 = P - vector0;
	C = Vec3Df::crossProduct(edge0,vp0);
	if (Vec3Df::dotProduct(N,C) < 0) return hitResult(); // P is on the right side 

	// edge 1
	Vec3Df edge1 = vector2 - vector1;
	Vec3Df vp1 = P - vector1;
	C = Vec3Df::crossProduct(edge1,vp1);
	if ((u = Vec3Df::dotProduct(N,C)) < 0)  return hitResult(); // P is on the right side 

	// edge 2
	Vec3Df edge2 = vector0 - vector2;
	Vec3Df vp2 = P - vector2;
	C = Vec3Df::crossProduct(edge2,vp2);
	if ((v = Vec3Df::dotProduct(N,C)) < 0) return hitResult(); // P is on the right side; 

	u /= denom;
	v /= denom;
	Vec3Df normal = u * vertices[tr.v[0]].n + v *  vertices[tr.v[1]].n + (1 - u - v) *  vertices[tr.v[2]].n;
	normal.normalize();
	return hitResult(tr, index, P, t, normal, u, v); // this ray hits the triangle 
}

hitResult closestHit(const Vec3Df & origin, const Vec3Df & dest){
	
	//Find the direction of the ray
	Vec3Df dir = dest - origin;
	dir.normalize();


	int index = 0;
	float closest = -1;
	hitResult res;
	std::vector<Triangle>::const_iterator iterator; 
	for (iterator = Triangles.begin(); iterator != Triangles.end(); ++iterator) {
		Triangle tr = *iterator;
		
		// find if the ray would intersect the plane
		hitResult& hit = rayIntersect(origin, dest, tr, index);
		if ((closest == -1 || hit.distance <= closest) & hit.hit){

			res = hit;
			closest = res.distance;

		}
		index++;
	}

	return res;
}


//Test if point1 can see point2
bool visible(Vec3Df & point1, Vec3Df & point2){

	std::vector<Triangle> Triangles = MyMesh.triangles;

	Vec3Df l = point2 - point1;
	float distance = l.getLength();
	std::vector<Triangle>::const_iterator iterator;
	int index = 0;

	for (iterator = Triangles.begin(); iterator != Triangles.end(); ++iterator) {
		Triangle tr = *iterator;
		hitResult& result = rayIntersect(point1, point2, tr, index);
		if (result.hit && result.distance < distance) {
			return false;
		}
	}
	return true;
}

//Normal light
class Light
{
public:
	Light(Vec3Df pos, Vec3Df col);
	Vec3Df position;
	Vec3Df color;
private:

};

Light::Light(Vec3Df pos, Vec3Df col)
{
	position = pos;
	color = col;
}

// Soft light class
class SoftLight
{
public:
	SoftLight();
	SoftLight(Vec3Df pos, double radius);
	Vec3Df position;
	float visibility(Vec3Df point){

		index = 0;
		totalHits = 0;
		while (index < sampleSize){
			Vec3Df RP = position + radius * random_unit_vector();
			if (visible(point, RP)){
				totalHits++;
			}
			index++;
		}
		float visibility = (float)totalHits / (float)sampleSize;
		return visibility;
	}

private:

	//Sample size
	int totalHits;
	int index;
	float radius;
};

SoftLight::SoftLight()
{
}

SoftLight::SoftLight(Vec3Df pos, double rad)
{
	position = pos;
	radius = rad;

}

std::vector<SoftLight> SoftLights;
std::vector<Light> Lights;



//return the color of your pixel.
Vec3Df performRayTracing(const Vec3Df & origin, const Vec3Df & dest, int & depth, int & depthrefr)
{
	bool invert = false;
	if (depthrefr % 2 == 1) invert = true;
	hitResult hit = closestHit(origin, dest);
	Triangle& triangle = hit.triangle;
	
	if (hit.hit){
		Material & material = materials[triangleMaterials[hit.triangleIndex]];

		Vec3Df& N = hit.normal;
		Vec3Df kD = material.Kd();
		Vec3Df kA = material.Ka();

		//Specular/reflection0
		Vec3Df kS = material.Ks();
		float shine = material.Ns();

		//Transparancy
		float Tr = material.Tr();
		float Ni = material.Ni();
		float d = material.Tr();

		int illum = material.illum();

		Vec3Df dir = dest - origin;
		dir.normalize();
		Vec3Df res;
		//Normal lights
		for (std::vector<Light>::const_iterator it = Lights.begin(); it != Lights.end(); it++){

			Light light = *it;
			Vec3Df l = light.position - hit.point;
			l.normalize();

			float dot = Vec3Df::dotProduct(l, N);


			if (dot > 0.0) {
				bool shadow = !visible(hit.point, light.position);
			
				if (shadow == false){

					res += dot*kD;

					Vec3Df viewDir = origin - dest;
					Vec3Df reflectDir = viewDir - 2 * (Vec3Df::dotProduct(N, dir)*N);
					viewDir.normalize();
					reflectDir.normalize();
					float dotSpec = Vec3Df::dotProduct(viewDir, reflectDir);
					//res += kS * light.color*pow(dotSpec, shine);
				}

			}
		}
		res = res + kD*0.2;

		// SoftLights
		for (std::vector<SoftLight>::const_iterator it = SoftLights.begin(); it != SoftLights.end(); it++){

			SoftLight light = *it;
			Vec3Df l = light.position - hit.point;
			l.normalize();

			float dot = Vec3Df::dotProduct(l, N);


			if (dot > 0.0) {
				float shadow = light.visibility(hit.point);

				if (shadow > 0){

					res += dot*kD*shadow;

					Vec3Df viewDir = origin - dest;
					Vec3Df reflectDir = viewDir - 2 * (Vec3Df::dotProduct(N, dir)*N);
					viewDir.normalize();
					reflectDir.normalize();
					float dotSpec = Vec3Df::dotProduct(viewDir, reflectDir);
					//res += kS * light.color*pow(dotSpec, shine);
				}

			}

		}

		//Reflection
		if (illum == 3 & depth < 1 & shine > 0.0){
			Vec3Df r = dir - 2 * (Vec3Df::dotProduct(N, dir)*N);
			depth++;
			res += shine / 1000 * performRayTracing(hit.point, r + hit.point, depth, depthrefr);
			//std::cout << " Reflection: " << shine * kS* performRayTracing(hit.point, r + hit.point, depth);
		}

		//Refraction
		if (illum == 4 && Tr < 1.0 & depthrefr < 4){
			float cos = Vec3Df::dotProduct(dir, N);
			Vec3Df W;
			float n,n1,n2;
			if (cos < 0){
				W = Vec3Df(0, 0, 0) - N;
				n = 1.2 / 1.0f;
				n1 = 1.2;
				n2 = 1.0f;
			}
			else{
				cos = -cos;
				W = N;
				n = 1.0f / 1.2;
				n1 = 1.0f;
				n2 = 1.2;
			}
			W.normalize();
			Vec3Df t = n*(dir - (cos*W)) - W*(sqrt(1 - (n1*n1*(1 - (cos*cos)) / (n2*n2))));
			depthrefr++;
			res += performRayTracing(hit.point, t + hit.point + dir*Vec3Df(0.01,0.01,0.01), depth, depthrefr);
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
	for (int i = 0; i<Lights.size(); ++i)
		glVertex3fv(Lights[i].position.pointer());
	for (int i = 0; i<SoftLights.size(); ++i)
		glVertex3fv(SoftLights[i].position.pointer());
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

	Triangles = MyMesh.triangles;
	triangleMaterials = MyMesh.triangleMaterials;
	materials = MyMesh.materials;
	vertices = MyMesh.vertices;

	testRayOrigin = rayOrigin;
	hitResult hit = closestHit(rayOrigin, rayDestination);
	Vec3Df dir = rayDestination - rayOrigin;
	int depth = 0;
	int depthrefr = 0;
	dir.normalize();
	testRayDestination = rayOrigin + dir * hit.distance;
	Vec3Df color = performRayTracing(rayOrigin, rayDestination, depth, depthrefr);
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
		//add/update a light based on the camera position.
	case 'L':
		Lights.push_back(Light(rayOrigin, Vec3Df(1, 1, 1)));
		break;
	case 'l':
		if (Lights.size() != 0){
			Lights[Lights.size() - 1] = Light(rayOrigin, Vec3Df(1, 1, 1));
		}
		else{
			Lights.push_back(Light(rayOrigin, Vec3Df(1, 1, 1)));
		}
		break;
	}
}
