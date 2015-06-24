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


class Sphere
{
public:
	Sphere(Vec3Df pos, float rad);
	Vec3Df position;
	float radius;
	Vec3Df color;
private:

};

Sphere::Sphere(Vec3Df pos, float rad)
{
	position = pos;
	radius = rad;
}
std::vector<Sphere> Spheres;

std::vector<Material> sphereMaterials;

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
	//MyMesh.loadMesh("../showcase.obj", true);
	MyMesh.loadMesh("../objects/mountain_final.obj", true);
	//MyMesh.loadMesh("../objects/reflect_floor.obj", true);

	MyMesh.computeVertexNormals();

	// Red reflective sphere
	Spheres.push_back(Sphere(Vec3Df(2.5, 0.5, 1), 0.5));
	Material SphereMat = Material();
	SphereMat.set_Kd(0.7, 0.2, 0.2);
	SphereMat.set_Ks(0.25, 0.25, 0.25);
	SphereMat.set_illum(3);
	SphereMat.set_Ns(250);
	sphereMaterials.push_back(SphereMat);

	// Blue non reflective sphere
	Spheres.push_back(Sphere(Vec3Df(1.5, 1, 0), 1));
	Material SphereMat1 = Material();
	SphereMat1.set_Kd(0.2, 0.2, 0.7);
	SphereMat1.set_Ks(0, 0, 0);
	SphereMat1.set_Ns(400);
	SphereMat1.set_illum(2);
	sphereMaterials.push_back(SphereMat1);

	// green reflective sphere
	Spheres.push_back(Sphere(Vec3Df(4.6, -0.13,- 4.5), 0.5));
	Material SphereMat2 = Material();
	SphereMat2.set_Kd(0.2, 0.7, 0.2);
	SphereMat2.set_Ks(0.25, 0.25, 0.25);
	SphereMat2.set_illum(3);
	SphereMat2.set_Ns(750);
	sphereMaterials.push_back(SphereMat2);

	//one first move: initialize the first light source
	//at least ONE light source has to be in the scene!!!
	//here, we set it to the current location of the camera
	//MyLightPositions.push_back(MyCameraPosition);
}



class hitResult
{
public:
	hitResult(Triangle & tr, int & index, Vec3Df & p, float & dist, Vec3Df & normal, float & u, float & v);
	void set(Triangle & tr, int & index, Vec3Df & p, float & dist, Vec3Df & normal, float & u, float & v);
	void set( int & index, Vec3Df & p, float & dist, Vec3Df & normal);
	hitResult();
	Triangle triangle;
	Vec3Df point;
	Vec3Df normal;
	float distance;
	float u;
	float v;
	bool hit;
	int triangleIndex;
	int sphereIndex;
private:

};
hitResult::hitResult()
{
	hit = false;
}

hitResult::hitResult(Triangle & tr, int & index, Vec3Df & p, float &  dist, Vec3Df & norm, float & U, float & V)
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
void hitResult::set(Triangle & tr, int & index, Vec3Df & p, float &  dist, Vec3Df & norm, float & U, float & V)
{
	triangle = tr;
	point = p;
	distance = dist;
	hit = true;
	triangleIndex = index;
	sphereIndex = -1;
	normal = norm;
	u = U;
	v = V;
}
void hitResult::set(int & index, Vec3Df & p, float &  dist, Vec3Df & norm)
{
	point = p;
	distance = dist;
	hit = true;
	sphereIndex = index;
	triangleIndex = -1;
	normal = norm;
}

void rayIntersect(const Vec3Df & origin, const Vec3Df & dest, Triangle & tr, hitResult & res, int index){

	Vec3Df dir = dest - origin;
	dir.normalize();

	Vec3Df& vector0 = vertices[tr.v[0]].p;
	Vec3Df& vector1 = vertices[tr.v[1]].p;
	Vec3Df& vector2 = vertices[tr.v[2]].p;

	Vec3Df v0v1 = vector1 - vector0;
	Vec3Df v0v2 = vector2 - vector0;

	Vec3Df N = Vec3Df::crossProduct(v0v1, v0v2);
	float denom = Vec3Df::dotProduct(N, N);

	float NdotRayDir = Vec3Df::dotProduct(N, dir);
	if (NdotRayDir >= 0){
		return;
	}

	float d = Vec3Df::dotProduct(N, vector0);

	// compute distance t (equation 3)
	float t = -(Vec3Df::dotProduct(N, origin) - d) / NdotRayDir;

	if (res.hit && (0 > t || t > res.distance)){
		return;
	}
	// compute the intersection point
	Vec3Df P = origin + t * dir;
	// Step 2: inside-outside test
	Vec3Df C; // vector perpendicular to triangle's plane 
	float u;
	float v;
	// edge 0
	Vec3Df edge0 = v0v1;
	Vec3Df vp0 = P - vector0;
	C = Vec3Df::crossProduct(edge0, vp0);
	if (Vec3Df::dotProduct(N, C) < 0) return; // P is on the right side 

	// edge 1
	Vec3Df edge1 = vector2 - vector1;
	Vec3Df vp1 = P - vector1;
	C = Vec3Df::crossProduct(edge1, vp1);
	if ((u = Vec3Df::dotProduct(N, C)) < 0)  return; // P is on the right side 

	// edge 2
	Vec3Df edge2 = vector0 - vector2;
	Vec3Df vp2 = P - vector2;
	C = Vec3Df::crossProduct(edge2, vp2);
	if ((v = Vec3Df::dotProduct(N, C)) < 0) return; // P is on the right side; 

	u /= denom;
	v /= denom;
	Vec3Df normal = u * vertices[tr.v[0]].n + v *  vertices[tr.v[1]].n + (1 - u - v) *  vertices[tr.v[2]].n;
	normal.normalize();
	return res.set(tr, index, P, t, normal, u, v); // this ray hits the triangle 
}

bool solveQuadratic(const float &a, const float &b, const float &c, float &x0, float &x1)
{
	float discr = b * b - 4 * a * c;
	if (discr < 0) return false;
	else if (discr == 0) x0 = x1 = -0.5 * b / a;
	else {
		float q = (b > 0) ?
			0.5 * (b + sqrt(discr)) :
			0.5 * (b - sqrt(discr));
		x0 = q / a;
		x1 = c / q;
	}
	if (x0 > x1) std::swap(x0, x1);

	return true;
}

void rayIntersectSphere(const Vec3Df & origin, const Vec3Df & dest, Sphere & sp, hitResult & res, int index){

	float t0, t1; // solutions for t if the ray intersects 

	Vec3Df dir = dest - origin;
	dir.normalize();

	// analytic solution
	Vec3Df L = sp.position - origin;

	float radius2 = sp.radius*sp.radius;
	float tca = Vec3Df::dotProduct(L, dir);
	float d2 = Vec3Df::dotProduct(L, L) - tca * tca;

	float thc = sqrt(radius2 - d2);
	float a = Vec3Df::dotProduct(dir,dir);
	float b = 2 * Vec3Df::dotProduct(dir,L);
	float c = Vec3Df::dotProduct(L,L) - radius2;
	if (!solveQuadratic(a, b, c, t0, t1)) return;
	if (t0 > t1) std::swap(t0, t1);

	if (res.hit && t0 >= res.distance) return;

	if (t0 < 0) {
		t0 = t1; // if t0 is negative, let's use t1 instead 
		if (t0 <= 1) return; // both t0 and t1 are negative 
	}

	float t = t0;
	Vec3Df P = origin + dir * t;
	Vec3Df normal = P - sp.position;
	normal.normalize();
	res.set(index, P, t, normal);
	return;
}

hitResult closestHit(const Vec3Df & origin, const Vec3Df & dest){

	//Find the direction of the ray
	Vec3Df dir = dest - origin;
	dir.normalize();


	int index = 0;
	float closest = -1;
	hitResult res = hitResult();
	std::vector<Sphere>::const_iterator it;
	for (it = Spheres.begin(); it != Spheres.end(); ++it) {
		Sphere sp = *it;
		rayIntersectSphere(origin, dest, sp, res, index);
		index++;
	}
	index = 0;
	std::vector<Triangle>::const_iterator iterator;
	for (iterator = Triangles.begin(); iterator != Triangles.end(); ++iterator) {
		Triangle tr = *iterator;

		// find if the ray would intersect the plane
		rayIntersect(origin, dest, tr, res, index);
		index++;
	}

	return res;
}


//Test if point1 can see point2
bool visible(Vec3Df & point1, Vec3Df & point2){

	std::vector<Triangle> Triangles = MyMesh.triangles;

	Vec3Df l = point2 - point1;
	float distance = l.getLength();
	int index = 0;
	hitResult res;
	res.hit = true;
	res.distance = distance;

	std::vector<Sphere>::const_iterator it;
	for (it = Spheres.begin(); it != Spheres.end(); ++it) {
		Sphere sp = *it;
		rayIntersectSphere(point1, point2, sp, res, index);
		if (res.distance != distance){
			return false;
		}
	}

	std::vector<Triangle>::const_iterator iterator;
	for (iterator = Triangles.begin(); iterator != Triangles.end(); ++iterator) {
		Triangle tr = *iterator;
		rayIntersect(point1, point2, tr, res, index);
		if (res.distance != distance){
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
std::vector<Light> Lights;

// Soft light class
class SoftLight
{
public:
	SoftLight();
	SoftLight(Vec3Df pos, Vec3Df dest, Vec3Df color, double radius);
	Vec3Df position;
	Vec3Df color;
	float size;

	float visibility(Vec3Df point){

		totalHits = 0;


		for (int i = 0; i < sampleSize; i++){
			int xCor = i % 4;
			int yCor = i / 4;
			double random = ((double)rand() / (double)RAND_MAX) - 0.5;
			Vec3Df middle = position + x * xCor + y * yCor - 1.5*x - 1.5*y;
			Vec3Df RP = middle + random * x + random * y;

			if (visible(point, RP)){
				totalHits++;
			}
		}

		float visibility = (float)totalHits / (float)sampleSize;
		return visibility;
	}

private:

	//Sample size
	Vec3Df x;
	Vec3Df y;
	int totalHits;
};

SoftLight::SoftLight()
{
}

SoftLight::SoftLight(Vec3Df pos, Vec3Df dest,Vec3Df col, double s)
{
	position = pos;
	size = s;
	color = col;
	Vec3Df dir = dest - pos;
	dir.normalize();
	x = Vec3Df(-dir[1],dir[0], 0);
	y = Vec3Df::crossProduct(dir,x);
	x *= size;
	y *= size;
}

std::vector<SoftLight> SoftLights;



//return the color of your pixel.
Vec3Df performRayTracing(const Vec3Df origin, const Vec3Df dest, int depth, int depthrefr)
{
	bool invert = false;
	if (depthrefr % 2 == 1) invert = true;
	hitResult hit = closestHit(origin, dest);
	Triangle& triangle = hit.triangle;

	if (hit.hit){
		Material material;
		if (hit.triangleIndex != -1){
			material = materials[triangleMaterials[hit.triangleIndex]];
		}
		else{
			material = sphereMaterials[hit.sphereIndex];
		}

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

			bool shadow = !visible(hit.point, light.position);
			if (dot > 0.0) {

				if (shadow == false){
					res += dot*kD*light.color;

					//Vec3Df viewDir = origin - dest;
					Vec3Df reflectDir = l - 2 * (Vec3Df::dotProduct(N, l)*N);
					//viewDir.normalize();
					reflectDir.normalize();
					float dotSpec = Vec3Df::dotProduct(dir, reflectDir);
					if (dotSpec > 0.0)
						res += light.color*kS*pow(dotSpec, shine);
					else
						res += light.color*kS*pow(0.0, shine);
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

			float shadow = light.visibility(hit.point);
			if (dot > 0.0) {

				if (shadow > 0){

					res += dot*kD*shadow*light.color;

					Vec3Df viewDir = origin - dest;
					Vec3Df reflectDir = l - 2 * (Vec3Df::dotProduct(N, l)*N);
					viewDir.normalize();
					reflectDir.normalize();
					float dotSpec = Vec3Df::dotProduct(dir, reflectDir);
					if (dotSpec > 0.0)
						res += light.color*kS*pow(dotSpec, shine);
					else
						res += light.color*kS*pow(0.0, shine);
				}
			}
		}

		//Reflection
		if (illum == 3 & depth > 0 & shine > 0.0){
			Vec3Df r = dir - 2 * (Vec3Df::dotProduct(N, dir)*N);
			depth--;
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
		// Sky (Arjans nice blue)
		return Vec3Df(0, 0.66, 1);
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
	glColor3f(1, 1, 1);
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
	glColor3f(0, 1, 1);
	glVertex3f(testRayOrigin[0], testRayOrigin[1], testRayOrigin[2]);
	glColor3f(0, 0, 1);
	glVertex3f(testRayDestination[0], testRayDestination[1], testRayDestination[2]);
	glEnd();
	glPointSize(10);
	glBegin(GL_POINTS);
	//glVertex3fv(MyLightPositions[0].pointer());
	glEnd();
	glPopAttrib();
	//draw whatever else you want...
	for (int i = 0; i < Spheres.size(); ++i){
		glPushMatrix();
		glTranslatef(Spheres[i].position[0], Spheres[i].position[1], Spheres[i].position[2]);
		glutSolidSphere(Spheres[i].radius, 20, 10);
		glPopMatrix();
	}
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
	int depth = 2;
	int depthrefr = 0;
	dir.normalize();
	testRayDestination = rayOrigin + dir * hit.distance;
	Vec3Df color = performRayTracing(rayOrigin, rayDestination, depth, depthrefr);
	//std::cout << material.illum() << std::endl;
	//std::cout << material.Ns() << std::endl;
	//std::cout << hit.distance << std::endl;
	float size;
	float red;
	float green;
	float blue;
	int index;
	int vectorSize;
	switch (key)
	{
	case 'o':

		std::cout << "size: ";
		std::cin >> size;
		std::cout << "red: ";
		std::cin >> red;
		std::cout << "green: ";
		std::cin >> green;
		std::cout << "blue: ";
		std::cin >> blue;

		if (SoftLights.size() != 0){
			SoftLights[SoftLights.size() - 1] = SoftLight(rayOrigin, rayDestination,Vec3Df(red,green,blue),size );
		}
		else{
			SoftLights.push_back(SoftLight(rayOrigin, rayDestination, Vec3Df(red, green, blue), size));
		}
		break;

	case 'O':

		std::cout << "size: ";
		std::cin >> size;
		std::cout << "red: ";
		std::cin >> red;
		std::cout << "green: ";
		std::cin >> green;
		std::cout << "blue: ";
		std::cin >> blue;
		SoftLights.push_back(SoftLight(rayOrigin, rayDestination, Vec3Df(red, green, blue), size));
		break;
		//add/update a light based on the camera position.
	case 'L':

		std::cout << "red: ";
		std::cin >> red;
		std::cout << "green: ";
		std::cin >> green;
		std::cout << "blue: ";
		std::cin >> blue;
		Lights.push_back(Light(rayOrigin, Vec3Df(red, green, blue)));
		break;
	case 'l':

		std::cout << "red: ";
		std::cin >> red;
		std::cout << "green: ";
		std::cin >> green;
		std::cout << "blue: ";
		std::cin >> blue;
		if (Lights.size() != 0){
			Lights[Lights.size() - 1] = Light(rayOrigin, Vec3Df(red, green, blue));
		}
		else{
			Lights.push_back(Light(rayOrigin, Vec3Df(red, green, blue)));
		}
		break;

	case 12:
		vectorSize = Lights.size();
		if (vectorSize == 0) break;
		std::cout << "Number of Lights: " << vectorSize << std::endl;
		std::cout << "Delete index: ";

		std::cin >> index;

		if (index < vectorSize && index >= 0){
			Lights.erase(Lights.begin() + index);
		}
		else{
			std::cout << "Please insert a valid index ";
		}
		break;

	case 15:
		vectorSize = SoftLights.size();
		if (vectorSize == 0) break;
		std::cout << "Number of SoftLights: " << vectorSize << std::endl;
		std::cout << "Delete index: ";

		std::cin >> index;

		if (index < vectorSize && index >= 0){
			SoftLights.erase(SoftLights.begin() + index);
		}
		else{
			std::cout << "Please insert a valid index ";
		}
		break;
	}
}
